// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_gainNChTrimSmoothing_ci.cpp
*
*  @brief    C7x optimized implementation of AUDIOLIB_gainNChTrimSmoothing.
*
*  @details  All kernels are vector-length agnostic: the vector type, the lane
*            count and the SE/SA element/vector-length enums are derived from
*            c7x::make_full_vector<dataType>, so the same source runs on 256-bit
*            and 512-bit devices.
*
*            Stream layouts (chosen once in init_ci)
*            - Interleaved, block: one channel per lane. SE0/SA0 walk 3D
*              (lanes = one channel block, rows = samples, outer = channel blocks).
*              DECDIM1 masks lanes past numChannels, DECDIM2 masks the rows that
*              pad the frame to a multiple of four.
*            - Planar: channel pairs. SE0/SA0 stream channels [0, H), SE1/SA1
*              channels [H, 2H), H = ceil(C/2). Each row is padded to a multiple
*              of 2L samples; DECDIM on a single-iteration DIM2 masks the padding.
*              For odd C >= 3, SE1/SA1's last row (channel C) is masked by
*              DECDIM2 on the row dimension, so no channel is streamed twice.
*              Mono runs both halves on channel 0 in lockstep.
*
*            Fade kernels keep four independent ramp/decay chains so the 4-cycle
*            loop-carried multiply overlaps, giving one vector per cycle in steady
*            state (store-bound and .M/.N-bound at the same time). No kernel has a
*            remainder path: padding is zero-filled by the SE and store-masked by
*            the SA predicate.
*
*  @version  0.2 - Oct 2026 : planar channel pairs, DECDIM row padding,
*                             SE1 gain stream for bypass.
*
*******************************************************************************
*/

#include "AUDIOLIB_gainNChTrimSmoothing_priv.h"

/* ========================================================================== */
/*  Helpers                                                                   */
/* ========================================================================== */

/* Predicated stores of one output vector. One side effect per statement: the SA
   predicate must be read before the SA address advances. */
template <typename vec, typename dataType>
static inline void AUDIOLIB_gainNChTrimSmoothing_storeSA0(dataType *restrict pOut, vec vOut)
{
   __vpred pred = c7x::strm_agen<0, vec>::get_vpred();
   vec    *addr = c7x::strm_agen<0, vec>::get_adv(pOut);
   __vstore_pred(pred, addr, vOut);
}

template <typename vec, typename dataType>
static inline void AUDIOLIB_gainNChTrimSmoothing_storeSA1(dataType *restrict pOut, vec vOut)
{
   __vpred pred = c7x::strm_agen<1, vec>::get_vpred();
   vec    *addr = c7x::strm_agen<1, vec>::get_adv(pOut);
   __vstore_pred(pred, addr, vOut);
}

static inline int32_t AUDIOLIB_gainNChTrimSmoothing_planarOffB(int32_t numChannels, int32_t numPairs)
{
   return AUDIOLIB_min(numPairs, numChannels - 1);
}

/* ========================================================================== */
/*  perfEst                                                                   */
/* ========================================================================== */

void AUDIOLIB_gainNChTrimSmoothing_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;

   const uint64_t numOuter  = (uint64_t) pKerPrivArgs->numOuter;
   const uint64_t numInner  = (uint64_t) pKerPrivArgs->numInner;
   const uint32_t converged = (uint32_t) (pKerPrivArgs->maxRemainingFadeSamples == 0U);
   const uint8_t  isBypass  = (uint8_t) (pKerPrivArgs->execute[converged] == pKerPrivArgs->bypassFxn);
   const uint8_t  isPlanar  = (uint8_t) (pKerPrivArgs->layout == (uint8_t) AUDIOLIB_GAINNCHTRIMSMOOTHING_LAYOUT_PLANAR);
   const uint8_t  isSmooth =
       (uint8_t) (pKerPrivArgs->setArgs.fadeType == AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH);
   /* Vectors of channels in the planar frame-end state pass. */
   const uint64_t numChBlocks =
       (uint64_t) AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, AUDIOLIB_GAINNCHTRIMSMOOTHING_LANES_F32);

   /* Both fade kernels stream four vectors per inner iteration at II = 4, i.e. one vector
      per cycle; the constant-gain kernel streams one vector per cycle at II = 1. */
   const uint64_t fadeVecs   = numOuter * numInner * 4U;
   const uint64_t bypassVecs = (uint64_t) pKerPrivArgs->bypassIters;

   /* Overheads per exec() call, fitted to Loki (C7504) measurements of the whole exec()
      call (wrapper, stream opens/closes, per-block/per-pair setup, loop prolog/epilog and
      the frame-end state update), within a few cycles for C = 1..24, N = 256..1024. */
   uint64_t overheadCycles;

   if (isBypass != 0U) {
      /* SE0 + SE1 + SA0 opens, one flat loop. */
      overheadCycles = 66U;
      *archCycles    = bypassVecs;
   }
   else if (isPlanar != 0U) {
      /* SE0 + SE1 + SA0 + SA1 opens; per channel pair: scalar state, chain seeds,
         prolog/epilog; then the vector frame-end pass (II 4 SYMMETRIC, II 2 SMOOTH). */
      overheadCycles = (isSmooth != 0U) ? (90U + (numOuter * 29U) + (numChBlocks * 2U))
                                        : (84U + (numOuter * 27U) + (numChBlocks * 4U));
      *archCycles    = fadeVecs;
   }
   else {
      /* SE0 + SA0 opens; per channel block: state load, chain seeds, prolog/epilog and the
         fused frame-end state update. */
      overheadCycles = (isSmooth != 0U) ? (55U + (numOuter * 35U)) : (56U + (numOuter * 37U));
      *archCycles    = fadeVecs;
   }

   *estCycles = *archCycles + overheadCycles;
}

/* ========================================================================== */
/*  init                                                                      */
/* ========================================================================== */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_init_ci(AUDIOLIB_kernelHandle                         handle,
                                                      const AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;
   uint8_t                                *pBlock       = pKerPrivArgs->bufPblock;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   const int32_t                                          eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrimSmoothing_init_ci");

   const int32_t numChannels       = (int32_t) pKerPrivArgs->numChannels;
   const int32_t numSamples        = (int32_t) pKerPrivArgs->numSamples;
   const int32_t strideInElements  = (int32_t) pKerPrivArgs->strideInElements;
   const int32_t strideOutElements = (int32_t) pKerPrivArgs->strideOutElements;

   __SE_TEMPLATE_v1 seFade    = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saFade    = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seBypData = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saBypData = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seBypGain = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seFadeB   = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saFadeB   = __gen_SA_TEMPLATE_v1();

   seFade.ELETYPE    = c7x::se_eletype<vec>::value;
   seFade.VECLEN     = c7x::se_veclen<vec>::value;
   saFade.VECLEN     = c7x::sa_veclen<vec>::value;
   seBypData.ELETYPE = c7x::se_eletype<vec>::value;
   seBypData.VECLEN  = c7x::se_veclen<vec>::value;
   saBypData.VECLEN  = c7x::sa_veclen<vec>::value;
   seBypGain.ELETYPE = c7x::se_eletype<vec>::value;
   seBypGain.VECLEN  = c7x::se_veclen<vec>::value;

   /* Unity per-lane target until set() fills it; the constant-gain kernel streams it. */
   for (int32_t i = 0; i < (int32_t) (AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS * eleCount); i++) {
      pKerPrivArgs->laneTarget[i] = 1.0f;
   }

   if (pKerInitArgs->isInterleaved != 0U) {
      /* One channel per lane, channel blocks of one vector. */
      const int32_t numOuter = AUDIOLIB_ceilingDiv(numChannels, eleCount);

      pKerPrivArgs->layout = (uint8_t) AUDIOLIB_GAINNCHTRIMSMOOTHING_LAYOUT_BLOCK;

      /* Pad the rows to whole groups of four; DECDIM2 zero-fills / store-masks the padding. */
      const int32_t rowsPad = AUDIOLIB_ceilingDiv(numSamples, AUDIOLIB_GAINNCHTRIMSMOOTHING_ROW_CHAINS) *
                              AUDIOLIB_GAINNCHTRIMSMOOTHING_ROW_CHAINS;

      pKerPrivArgs->numOuter    = (uint32_t) numOuter;
      pKerPrivArgs->numInner    = (uint32_t) (rowsPad / AUDIOLIB_GAINNCHTRIMSMOOTHING_ROW_CHAINS);
      pKerPrivArgs->bypassIters = (uint32_t) (numOuter * rowsPad);

      /*   dim0: L lanes of one row (contiguous)
           dim1: rowsPad rows, stride = row stride        -> DECDIM2: rows past the frame
           dim2: channel blocks, stride = L channels      -> DECDIM1: lanes past numChannels */
      seFade.DIMFMT        = __SE_DIMFMT_3D;
      seFade.ICNT0         = (uint32_t) eleCount;
      seFade.ICNT1         = (uint32_t) rowsPad;
      seFade.DIM1          = strideInElements;
      seFade.ICNT2         = (uint32_t) numOuter;
      seFade.DIM2          = eleCount;
      seFade.DECDIM1       = __SE_DECDIM_DIM2;
      seFade.DECDIM1_WIDTH = (uint32_t) numChannels;
      seFade.DECDIM2       = __SE_DECDIM_DIM1;
      seFade.DECDIM2_WIDTH = (uint32_t) (numSamples * strideInElements);

      saFade.DIMFMT        = __SA_DIMFMT_3D;
      saFade.ICNT0         = (uint32_t) eleCount;
      saFade.ICNT1         = (uint32_t) rowsPad;
      saFade.DIM1          = strideOutElements;
      saFade.ICNT2         = (uint32_t) numOuter;
      saFade.DIM2          = eleCount;
      saFade.DECDIM1       = __SA_DECDIM_DIM2;
      saFade.DECDIM1_WIDTH = (uint32_t) numChannels;
      saFade.DECDIM2       = __SA_DECDIM_DIM1;
      saFade.DECDIM2_WIDTH = (uint32_t) (numSamples * strideOutElements);

      /* Constant gain: same data walk; SE1 re-reads the block's target vector for every row. */
      seBypData = seFade;
      saBypData = saFade;

      seBypGain.DIMFMT = __SE_DIMFMT_3D;
      seBypGain.ICNT0  = (uint32_t) eleCount;
      seBypGain.ICNT1  = (uint32_t) rowsPad;
      seBypGain.DIM1   = 0;
      seBypGain.ICNT2  = (uint32_t) numOuter;
      seBypGain.DIM2   = eleCount;
   }
   else {
      /* Planar channel pairs: H rows per stream, rows padded to whole pairs of vectors. */
      const int32_t numPairs   = AUDIOLIB_ceilingDiv(numChannels, 2);
      const int32_t pairLen    = 2 * eleCount;
      const int32_t rowPad     = AUDIOLIB_ceilingDiv(numSamples, pairLen) * pairLen;
      const int32_t vecsPerRow = AUDIOLIB_ceilingDiv(numSamples, eleCount);

      pKerPrivArgs->layout      = (uint8_t) AUDIOLIB_GAINNCHTRIMSMOOTHING_LAYOUT_PLANAR;
      pKerPrivArgs->numOuter    = (uint32_t) numPairs;
      pKerPrivArgs->numInner    = (uint32_t) (rowPad / pairLen);
      pKerPrivArgs->bypassIters = (uint32_t) (numChannels * vecsPerRow);
      for (int32_t l = 0; l < eleCount; l++) {
         pKerPrivArgs->laneOffset[l] = l;
      }

      /*   dim0: rowPad samples of one channel row
           dim1: H channel rows, stride = row stride
           dim2: one iteration; DECDIM1 on it never deducts nor reloads, so its budget stays
                 numSamples and masks samples [numSamples, rowPad) of every row. */
      seFade.DIMFMT        = __SE_DIMFMT_2D;
      seFade.ICNT0         = (uint32_t) rowPad;
      seFade.ICNT1         = (uint32_t) numPairs;
      seFade.DIM1          = strideInElements;
      seFade.DECDIM1       = __SE_DECDIM_DIM2;
      seFade.DECDIM1_WIDTH = (uint32_t) numSamples;

      saFade.DIMFMT        = __SA_DIMFMT_2D;
      saFade.ICNT0         = (uint32_t) rowPad;
      saFade.ICNT1         = (uint32_t) numPairs;
      saFade.DIM1          = strideOutElements;
      saFade.DECDIM1       = __SA_DECDIM_DIM2;
      saFade.DECDIM1_WIDTH = (uint32_t) numSamples;

      /* Second half, from channel chOffB: same walk, plus DECDIM2 on the rows. Its budget,
         C - chOffB rows, drops by one row per row, so for odd C >= 3 the last row (channel
         C, past the buffer) is zero-filled without a fetch and its stores are masked. For
         even C it never masks, and for mono both halves walk channel 0 (the budget is
         never 0, which a stream may not be opened with). */
      const int32_t rowsB   = numChannels - AUDIOLIB_gainNChTrimSmoothing_planarOffB(numChannels, numPairs);
      seFadeB               = seFade;
      seFadeB.DECDIM2       = __SE_DECDIM_DIM1;
      seFadeB.DECDIM2_WIDTH = (uint32_t) (rowsB * strideInElements);

      saFadeB               = saFade;
      saFadeB.DECDIM2       = __SA_DECDIM_DIM1;
      saFadeB.DECDIM2_WIDTH = (uint32_t) (rowsB * strideOutElements);

      /* Constant gain: all channel rows in one 2D walk; the partial last vector of each row
         is zero-filled by the SE and masked by the SA predicate. */
      seBypData.DIMFMT = __SE_DIMFMT_2D;
      seBypData.ICNT0  = (uint32_t) numSamples;
      seBypData.ICNT1  = (uint32_t) numChannels;
      seBypData.DIM1   = strideInElements;

      saBypData.DIMFMT = __SA_DIMFMT_2D;
      saBypData.ICNT0  = (uint32_t) numSamples;
      saBypData.ICNT1  = (uint32_t) numChannels;
      saBypData.DIM1   = strideOutElements;

      /* SE1 re-reads channel ch's broadcast target row for each of its vectors. */
      seBypGain.DIMFMT = __SE_DIMFMT_3D;
      seBypGain.ICNT0  = (uint32_t) eleCount;
      seBypGain.ICNT1  = (uint32_t) vecsPerRow;
      seBypGain.DIM1   = 0;
      seBypGain.ICNT2  = (uint32_t) numChannels;
      seBypGain.DIM2   = eleCount;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_OFFSET)     = seFade;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_OFFSET)     = saFade;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_BYP_DATA_OFFSET) = seBypData;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_BYP_DATA_OFFSET) = saBypData;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_BYP_GAIN_OFFSET) = seBypGain;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_B_OFFSET)   = seFadeB;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_B_OFFSET)   = saFadeB;

   return status;
}

/* ========================================================================== */
/*  Interleaved fade (channel block layout)                                   */
/* ========================================================================== */

/* SYMMETRIC: gain0 * step^s per lane; lanes whose fade budget is used up
   (remaining - offset <= k) snap exactly to the target. */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothingLaneSymmetric_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   typedef typename c7x::make_full_vector<int32_t>::type  ivec;
   const int32_t                                          eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;
   uint8_t                                *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrimSmoothingLaneSymmetric_exec_ci");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   const int32_t numChannels = (int32_t) pKerPrivArgs->numChannels;
   const int32_t numOuter    = (int32_t) pKerPrivArgs->numOuter;
   const int32_t numInner    = (int32_t) pKerPrivArgs->numInner;

   __SE_TEMPLATE_v1 seParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_OFFSET);
   __SA_TEMPLATE_v1 saParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_OFFSET);

   /* Fade budget offsets of the four vectors of a group, and of one whole group, in rows. */
   const ivec vK0     = (ivec) ((int32_t) 0);
   const ivec vK1     = (ivec) ((int32_t) 1);
   const ivec vK2     = (ivec) ((int32_t) 2);
   const ivec vK3     = (ivec) ((int32_t) 3);
   const ivec vKGroup = (ivec) ((int32_t) 4);
   /* Fade samples consumed by one frame. */
   const ivec vFrameLen = (ivec) ((int32_t) pKerPrivArgs->numSamples);

   __SE0_OPEN(pInLocal, seParams);
   __SA0_OPEN(saParams);

   for (int32_t blk = 0; blk < numOuter; blk++) {
      const int32_t chOff = blk * eleCount;

      /* Per-lane running state: lane l is channel chOff + l. */
      const vec  vGain0  = *(const vec *) &pKerPrivArgs->currentGain[chOff];
      const ivec vRemain = *(const ivec *) &pKerPrivArgs->remainingFadeSamples[chOff];
      const vec  vTarget = *(const vec *) &pKerPrivArgs->laneTarget[chOff];
      const vec  vAdv    = *(const vec *) &pKerPrivArgs->rowAdv[chOff];

      /* Four independent chains gain0 * step^s, one per vector of a group, seeded from the
         set() tables. */
      vec vRamp0 = vGain0 * (*(const vec *) &pKerPrivArgs->rowPow[0][chOff]);
      vec vRamp1 = vGain0 * (*(const vec *) &pKerPrivArgs->rowPow[1][chOff]);
      vec vRamp2 = vGain0 * (*(const vec *) &pKerPrivArgs->rowPow[2][chOff]);
      vec vRamp3 = vGain0 * (*(const vec *) &pKerPrivArgs->rowPow[3][chOff]);
      /* Per-lane fade budget relative to the first vector of the current group. */
      ivec vRem = vRemain;

#pragma MUST_ITERATE(1, , 1)
      for (int32_t grp = 0; grp < numInner; grp++) {
         vec vIn0 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn1 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn2 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn3 = c7x::strm_eng<0, vec>::get_adv();

         vec vOut0 = vIn0 * __select(__cmp_gt_pred(vRem, vK0), vRamp0, vTarget);
         vec vOut1 = vIn1 * __select(__cmp_gt_pred(vRem, vK1), vRamp1, vTarget);
         vec vOut2 = vIn2 * __select(__cmp_gt_pred(vRem, vK2), vRamp2, vTarget);
         vec vOut3 = vIn3 * __select(__cmp_gt_pred(vRem, vK3), vRamp3, vTarget);

         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut0);
         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut1);
         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut2);
         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut3);

         vRamp0 = vRamp0 * vAdv;
         vRamp1 = vRamp1 * vAdv;
         vRamp2 = vRamp2 * vAdv;
         vRamp3 = vRamp3 * vAdv;
         vRem   = vRem - vKGroup;
      }

      /* Frame-end state, one frame forward from the double-precision frame power, for the
         channel lanes 0..C-1 of this block: still fading -> gain0 * step^N, otherwise it
         ended on the target. */
      const vec vFramePow    = *(const vec *) &pKerPrivArgs->framePow[chOff];
      __vpred   pCh          = __mask_int((uint32_t) AUDIOLIB_min(numChannels - chOff, eleCount));
      __vpred   pStillFading = __cmp_gt_pred(vRemain, vFrameLen);
      vec       vGainEnd     = __select(pStillFading, vGain0 * vFramePow, vTarget);
      ivec      vRemainEnd   = __max(vRemain - vFrameLen, vK0);
      __vstore_pred(pCh, (vec *) &pKerPrivArgs->currentGain[chOff], vGainEnd);
      __vstore_pred(pCh, (ivec *) &pKerPrivArgs->remainingFadeSamples[chOff], vRemainEnd);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingLaneSymmetric_exec_ci<float>(AUDIOLIB_kernelHandle, void *restrict, void *restrict);

/* SMOOTH: gain = target + (gain0 - target) * coeff^(s + 1) per lane. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingLaneSmooth_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   const int32_t                                          eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;
   uint8_t                                *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrimSmoothingLaneSmooth_exec_ci");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   const int32_t numChannels = (int32_t) pKerPrivArgs->numChannels;
   const int32_t numOuter    = (int32_t) pKerPrivArgs->numOuter;
   const int32_t numInner    = (int32_t) pKerPrivArgs->numInner;

   __SE_TEMPLATE_v1 seParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_OFFSET);
   __SA_TEMPLATE_v1 saParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_OFFSET);

   /* Every channel decays by the same coeff, so the chain seeds, the advance and the frame
      power (coeff^N) are the same for every lane of every channel block. The first block's
      tables serve them all. */
   const vec vFramePow = *(const vec *) &pKerPrivArgs->framePow[0];
   const vec vAdv      = *(const vec *) &pKerPrivArgs->rowAdv[0];
   const vec vPow0     = *(const vec *) &pKerPrivArgs->rowPow[0][0];
   const vec vPow1     = *(const vec *) &pKerPrivArgs->rowPow[1][0];
   const vec vPow2     = *(const vec *) &pKerPrivArgs->rowPow[2][0];
   const vec vPow3     = *(const vec *) &pKerPrivArgs->rowPow[3][0];

   __SE0_OPEN(pInLocal, seParams);
   __SA0_OPEN(saParams);

   for (int32_t blk = 0; blk < numOuter; blk++) {
      const int32_t chOff = blk * eleCount;

      /* Per-lane running state: lane l is channel chOff + l. */
      const vec vGain0  = *(const vec *) &pKerPrivArgs->currentGain[chOff];
      const vec vTarget = *(const vec *) &pKerPrivArgs->laneTarget[chOff];
      const vec vScale  = vGain0 - vTarget;

      /* Four independent decay chains (gain0 - target) * coeff^(s + 1), one per vector of a
         group. */
      vec vDecay0 = vScale * vPow0;
      vec vDecay1 = vScale * vPow1;
      vec vDecay2 = vScale * vPow2;
      vec vDecay3 = vScale * vPow3;

#pragma MUST_ITERATE(1, , 1)
      for (int32_t grp = 0; grp < numInner; grp++) {
         vec vIn0 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn1 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn2 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn3 = c7x::strm_eng<0, vec>::get_adv();

         vec vOut0 = vIn0 * (vDecay0 + vTarget);
         vec vOut1 = vIn1 * (vDecay1 + vTarget);
         vec vOut2 = vIn2 * (vDecay2 + vTarget);
         vec vOut3 = vIn3 * (vDecay3 + vTarget);

         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut0);
         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut1);
         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut2);
         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut3);

         vDecay0 = vDecay0 * vAdv;
         vDecay1 = vDecay1 * vAdv;
         vDecay2 = vDecay2 * vAdv;
         vDecay3 = vDecay3 * vAdv;
      }

      /* Frame-end gain, one frame forward from the double-precision frame power, for the
         channel lanes 0..C-1 of this block: target + (gain0 - target) * coeff^N. */
      __vpred pCh      = __mask_int((uint32_t) AUDIOLIB_min(numChannels - chOff, eleCount));
      vec     vGainEnd = vTarget + (vScale * vFramePow);
      __vstore_pred(pCh, (vec *) &pKerPrivArgs->currentGain[chOff], vGainEnd);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingLaneSmooth_exec_ci<float>(AUDIOLIB_kernelHandle, void *restrict, void *restrict);

/* ========================================================================== */
/*  Planar fade (channel pairs on SE0/SA0 and SE1/SA1)                         */
/* ========================================================================== */

/* Second half of the channels: [H, 2H). For odd C >= 3 its last channel, C, does not exist:
   the SE1/SA1 templates zero-fill that row and mask its stores, and its state slot (inside
   the per-channel arrays, C < MAX_NUM_CHANNELS) is read but never written back. */

/* SYMMETRIC: gain0 * step^s per sample; samples past the fade budget snap to the target. */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothingPlanarSymmetric_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                     void *restrict pIn,
                                                                     void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   typedef typename c7x::make_full_vector<int32_t>::type  ivec;
   const int32_t                                          eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;
   uint8_t                                *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrimSmoothingPlanarSymmetric_exec_ci");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   const int32_t numChannels       = (int32_t) pKerPrivArgs->numChannels;
   const int32_t numPairs          = (int32_t) pKerPrivArgs->numOuter;
   const int32_t numInner          = (int32_t) pKerPrivArgs->numInner;
   const int32_t strideInElements  = (int32_t) pKerPrivArgs->strideInElements;
   const int32_t strideOutElements = (int32_t) pKerPrivArgs->strideOutElements;

   const int32_t chOffB = AUDIOLIB_gainNChTrimSmoothing_planarOffB(numChannels, numPairs);
   dataType     *pInB   = pInLocal + (chOffB * strideInElements);
   dataType     *pOutB  = pOutLocal + (chOffB * strideOutElements);

   __SE_TEMPLATE_v1 seParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_OFFSET);
   __SA_TEMPLATE_v1 saParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_OFFSET);
   __SE_TEMPLATE_v1 seParamsB =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_B_OFFSET);
   __SA_TEMPLATE_v1 saParamsB =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_B_OFFSET);

   /* Sample offsets of vectors 0 and 1 of a pair, and of a whole pair. */
   const ivec vK0      = (ivec) ((int32_t) 0);
   const ivec vK1      = (ivec) ((int32_t) eleCount);
   const ivec vKPair   = (ivec) ((int32_t) (2 * eleCount));
   const ivec vLaneOff = *(const ivec *) &pKerPrivArgs->laneOffset[0];

   __SE0_OPEN(pInLocal, seParams);
   __SE1_OPEN(pInB, seParamsB);
   __SA0_OPEN(saParams);
   __SA1_OPEN(saParamsB);

   const float *restrict targetGain = pKerPrivArgs->targetGain;
   float *restrict currentGain      = pKerPrivArgs->currentGain;

   for (int32_t pairIdx = 0; pairIdx < numPairs; pairIdx++) {
      const int32_t chA = pairIdx;
      const int32_t chB = chOffB + pairIdx;

      const dataType gainA   = (dataType) currentGain[chA];
      const dataType targetA = (dataType) targetGain[chA];
      const int32_t  remainA = pKerPrivArgs->remainingFadeSamples[chA];
      const dataType blkPowA = (dataType) pKerPrivArgs->blockPow[chA];
      const vec      vLanePA = *(const vec *) &pKerPrivArgs->lanePow[chA][0];

      const dataType gainB   = (dataType) currentGain[chB];
      const dataType targetB = (dataType) targetGain[chB];
      const int32_t  remainB = pKerPrivArgs->remainingFadeSamples[chB];
      const dataType blkPowB = (dataType) pKerPrivArgs->blockPow[chB];
      const vec      vLanePB = *(const vec *) &pKerPrivArgs->lanePow[chB][0];

      /* Lane l of vector v carries sample v*L + l: gain0 * lanePow[l] * blockPow^v. The
         second chain's scalar seed is formed while the lanePow vector loads. */
      vec       vRampA0  = ((vec) gainA) * vLanePA;
      vec       vRampA1  = ((vec) (gainA * blkPowA)) * vLanePA;
      vec       vRampB0  = ((vec) gainB) * vLanePB;
      vec       vRampB1  = ((vec) (gainB * blkPowB)) * vLanePB;
      const vec vAdvA    = (vec) (blkPowA * blkPowA);
      const vec vAdvB    = (vec) (blkPowB * blkPowB);
      const vec vTargetA = (vec) targetA;
      const vec vTargetB = (vec) targetB;
      /* Sample (v*L + l) still fading  <=>  remaining - l > v*L. */
      ivec vRemA = ((ivec) remainA) - vLaneOff;
      ivec vRemB = ((ivec) remainB) - vLaneOff;

#pragma MUST_ITERATE(1, , 1)
      for (int32_t grp = 0; grp < numInner; grp++) {
         vec vInA0 = c7x::strm_eng<0, vec>::get_adv();
         vec vInA1 = c7x::strm_eng<0, vec>::get_adv();
         vec vInB0 = c7x::strm_eng<1, vec>::get_adv();
         vec vInB1 = c7x::strm_eng<1, vec>::get_adv();

         vec vOutA0 = vInA0 * __select(__cmp_gt_pred(vRemA, vK0), vRampA0, vTargetA);
         vec vOutA1 = vInA1 * __select(__cmp_gt_pred(vRemA, vK1), vRampA1, vTargetA);
         vec vOutB0 = vInB0 * __select(__cmp_gt_pred(vRemB, vK0), vRampB0, vTargetB);
         vec vOutB1 = vInB1 * __select(__cmp_gt_pred(vRemB, vK1), vRampB1, vTargetB);

         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOutA0);
         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOutA1);
         AUDIOLIB_gainNChTrimSmoothing_storeSA1<vec, dataType>(pOutB, vOutB0);
         AUDIOLIB_gainNChTrimSmoothing_storeSA1<vec, dataType>(pOutB, vOutB1);

         vRampA0 = vRampA0 * vAdvA;
         vRampA1 = vRampA1 * vAdvA;
         vRampB0 = vRampB0 * vAdvB;
         vRampB1 = vRampB1 * vAdvB;
         vRemA   = vRemA - vKPair;
         vRemB   = vRemB - vKPair;
      }
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   const ivec    vFrameLen   = (ivec) ((int32_t) pKerPrivArgs->numSamples);
   const int32_t numChBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   for (int32_t blk = 0; blk < numChBlocks; blk++) {
      const int32_t chOff        = blk * eleCount;
      const vec     vGain0       = *(const vec *) &currentGain[chOff];
      const vec     vTarget      = *(const vec *) &targetGain[chOff];
      const ivec    vRemain      = *(const ivec *) &pKerPrivArgs->remainingFadeSamples[chOff];
      const vec     vFramePow    = *(const vec *) &pKerPrivArgs->framePow[chOff];
      __vpred       pCh          = __mask_int((uint32_t) AUDIOLIB_min(numChannels - chOff, eleCount));
      __vpred       pStillFading = __cmp_gt_pred(vRemain, vFrameLen);
      vec           vGainEnd     = __select(pStillFading, vGain0 * vFramePow, vTarget);
      ivec          vRemainEnd   = __max(vRemain - vFrameLen, vK0);
      __vstore_pred(pCh, (vec *) &pKerPrivArgs->currentGain[chOff], vGainEnd);
      __vstore_pred(pCh, (ivec *) &pKerPrivArgs->remainingFadeSamples[chOff], vRemainEnd);
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingPlanarSymmetric_exec_ci<float>(AUDIOLIB_kernelHandle, void *restrict, void *restrict);

/* SMOOTH: gain = target + (gain0 - target) * coeff^(s + 1) per sample. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingPlanarSmooth_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;
   uint8_t                                *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrimSmoothingPlanarSmooth_exec_ci");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   const int32_t numChannels       = (int32_t) pKerPrivArgs->numChannels;
   const int32_t numPairs          = (int32_t) pKerPrivArgs->numOuter;
   const int32_t numInner          = (int32_t) pKerPrivArgs->numInner;
   const int32_t strideInElements  = (int32_t) pKerPrivArgs->strideInElements;
   const int32_t strideOutElements = (int32_t) pKerPrivArgs->strideOutElements;

   const int32_t chOffB = AUDIOLIB_gainNChTrimSmoothing_planarOffB(numChannels, numPairs);
   dataType     *pInB   = pInLocal + (chOffB * strideInElements);
   dataType     *pOutB  = pOutLocal + (chOffB * strideOutElements);

   __SE_TEMPLATE_v1 seParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_OFFSET);
   __SA_TEMPLATE_v1 saParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_OFFSET);
   __SE_TEMPLATE_v1 seParamsB =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_B_OFFSET);
   __SA_TEMPLATE_v1 saParamsB =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_B_OFFSET);

   const vec      vLanePow = *(const vec *) &pKerPrivArgs->lanePow[0][0];
   const dataType blockPow = (dataType) pKerPrivArgs->blockPow[0];
   const vec      vAdv     = (vec) (blockPow * blockPow);

   __SE0_OPEN(pInLocal, seParams);
   __SE1_OPEN(pInB, seParamsB);
   __SA0_OPEN(saParams);
   __SA1_OPEN(saParamsB);

   const float *restrict targetGain = pKerPrivArgs->targetGain;
   float *restrict currentGain      = pKerPrivArgs->currentGain;

   for (int32_t pairIdx = 0; pairIdx < numPairs; pairIdx++) {
      const int32_t chA = pairIdx;
      const int32_t chB = chOffB + pairIdx;

      const dataType targetA = (dataType) targetGain[chA];
      const dataType scaleA  = ((dataType) currentGain[chA]) - targetA;
      const dataType targetB = (dataType) targetGain[chB];
      const dataType scaleB  = ((dataType) currentGain[chB]) - targetB;

      /* Lane l of vector v carries sample v*L + l: (gain0 - target) * lanePow[l] * blockPow^v. */
      vec       vDecayA0 = ((vec) scaleA) * vLanePow;
      vec       vDecayA1 = ((vec) (scaleA * blockPow)) * vLanePow;
      vec       vDecayB0 = ((vec) scaleB) * vLanePow;
      vec       vDecayB1 = ((vec) (scaleB * blockPow)) * vLanePow;
      const vec vTargetA = (vec) targetA;
      const vec vTargetB = (vec) targetB;

#pragma MUST_ITERATE(1, , 1)
      for (int32_t grp = 0; grp < numInner; grp++) {
         vec vInA0 = c7x::strm_eng<0, vec>::get_adv();
         vec vInA1 = c7x::strm_eng<0, vec>::get_adv();
         vec vInB0 = c7x::strm_eng<1, vec>::get_adv();
         vec vInB1 = c7x::strm_eng<1, vec>::get_adv();

         vec vOutA0 = vInA0 * (vDecayA0 + vTargetA);
         vec vOutA1 = vInA1 * (vDecayA1 + vTargetA);
         vec vOutB0 = vInB0 * (vDecayB0 + vTargetB);
         vec vOutB1 = vInB1 * (vDecayB1 + vTargetB);

         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOutA0);
         AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOutA1);
         AUDIOLIB_gainNChTrimSmoothing_storeSA1<vec, dataType>(pOutB, vOutB0);
         AUDIOLIB_gainNChTrimSmoothing_storeSA1<vec, dataType>(pOutB, vOutB1);

         vDecayA0 = vDecayA0 * vAdv;
         vDecayA1 = vDecayA1 * vAdv;
         vDecayB0 = vDecayB0 * vAdv;
         vDecayB1 = vDecayB1 * vAdv;
      }
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   const int32_t eleCount    = c7x::element_count_of<vec>::value;
   const int32_t numChBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   const vec     vFramePow   = *(const vec *) &pKerPrivArgs->framePow[0];
   for (int32_t blk = 0; blk < numChBlocks; blk++) {
      const int32_t chOff    = blk * eleCount;
      const vec     vGain0   = *(const vec *) &currentGain[chOff];
      const vec     vTarget  = *(const vec *) &targetGain[chOff];
      __vpred       pCh      = __mask_int((uint32_t) AUDIOLIB_min(numChannels - chOff, eleCount));
      vec           vGainEnd = vTarget + ((vGain0 - vTarget) * vFramePow);
      __vstore_pred(pCh, (vec *) &currentGain[chOff], vGainEnd);
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingPlanarSmooth_exec_ci<float>(AUDIOLIB_kernelHandle, void *restrict, void *restrict);

/* ========================================================================== */
/*  Constant gain (bypass / converged), both layouts                          */
/* ========================================================================== */

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingBypass_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;
   uint8_t                                *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrimSmoothingBypass_exec_ci");

   dataType *restrict pInLocal    = (dataType *) pIn;
   dataType *restrict pOutLocal   = (dataType *) pOut;
   const dataType *restrict pGain = (const dataType *) &pKerPrivArgs->laneTarget[0];

   const int32_t nVecs = (int32_t) pKerPrivArgs->bypassIters;

   __SE_TEMPLATE_v1 seData =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_BYP_DATA_OFFSET);
   __SA_TEMPLATE_v1 saData =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_BYP_DATA_OFFSET);
   __SE_TEMPLATE_v1 seGain =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_BYP_GAIN_OFFSET);

   __SE0_OPEN(pInLocal, seData);
   __SE1_OPEN(pGain, seGain);
   __SA0_OPEN(saData);

#pragma MUST_ITERATE(1, , 1)
   for (int32_t vecIdx = 0; vecIdx < nVecs; vecIdx++) {
      vec vIn   = c7x::strm_eng<0, vec>::get_adv();
      vec vGain = c7x::strm_eng<1, vec>::get_adv();
      vec vOut  = vIn * vGain;
      AUDIOLIB_gainNChTrimSmoothing_storeSA0<vec, dataType>(pOutLocal, vOut);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingBypass_exec_ci<float>(AUDIOLIB_kernelHandle, void *restrict, void *restrict);

/* ========================================================================== */
/*  set: per-lane tables and kernel selection                                 */
/* ========================================================================== */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_set_ci(AUDIOLIB_kernelHandle                   handle,
                                                     uint8_t                                 fadeType,
                                                     uint8_t                                 isInstant,
                                                     pFxnAUDIOLIB_gainNChTrimSmoothing_exec *pRampFxn,
                                                     pFxnAUDIOLIB_gainNChTrimSmoothing_exec *pConvergedFxn)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   const int32_t                                          eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;

   const uint8_t isSmooth    = (uint8_t) (fadeType == AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH);
   const int32_t numChannels = (int32_t) pKerPrivArgs->numChannels;
   const double  numSamples  = (double) pKerPrivArgs->numSamples;
   const double  coeff       = (double) pKerPrivArgs->smoothingCoeff;
   const uint8_t layout      = pKerPrivArgs->layout;

   const int32_t chPad = AUDIOLIB_ceilingDiv(numChannels, eleCount) * eleCount;

   /* One frame forward, per channel, for every layout. */
   for (int32_t ch = 0; ch < chPad; ch++) {
      const double base          = (isSmooth != 0U) ? coeff : (double) pKerPrivArgs->gainStep[ch];
      pKerPrivArgs->framePow[ch] = (float) pow(base, numSamples);
   }

   if (layout == (uint8_t) AUDIOLIB_GAINNCHTRIMSMOOTHING_LAYOUT_PLANAR) {
      const int32_t chPairPad = AUDIOLIB_ceilingDiv(numChannels, 2) * 2;
      for (int32_t ch = 0; ch < chPairPad; ch++) {
         const double base = (isSmooth != 0U) ? coeff : (double) pKerPrivArgs->gainStep[ch];
         /* SYMMETRIC lane l: step^l. SMOOTH lane l: coeff^(l + 1). */
         double p = (isSmooth != 0U) ? base : 1.0;
         for (int32_t l = 0; l < eleCount; l++) {
            pKerPrivArgs->lanePow[ch][l]                  = (float) p;
            pKerPrivArgs->laneTarget[(ch * eleCount) + l] = pKerPrivArgs->targetGain[ch];
            p *= base;
         }
         pKerPrivArgs->blockPow[ch] = (float) pow(base, (double) eleCount);
      }
   }
   else {
      for (int32_t ch = 0; ch < chPad; ch++) {
         const double base = (isSmooth != 0U) ? coeff : (double) pKerPrivArgs->gainStep[ch];
         /* Chain j seeds row j: SYMMETRIC step^j, SMOOTH coeff^(j + 1). */
         double p = (isSmooth != 0U) ? base : 1.0;
         for (int32_t j = 0; j < AUDIOLIB_GAINNCHTRIMSMOOTHING_ROW_CHAINS; j++) {
            pKerPrivArgs->rowPow[j][ch] = (float) p;
            p *= base;
         }
         pKerPrivArgs->rowAdv[ch]     = (float) pow(base, (double) AUDIOLIB_GAINNCHTRIMSMOOTHING_ROW_CHAINS);
         pKerPrivArgs->laneTarget[ch] = pKerPrivArgs->targetGain[ch];
      }
   }

   /* Fade kernel for the fade type and layout: [isSmooth][layout]. */
   static const pFxnAUDIOLIB_gainNChTrimSmoothing_exec fadeKernels[2][2] = {
       {AUDIOLIB_gainNChTrimSmoothingPlanarSymmetric_exec_ci<dataType>,
        AUDIOLIB_gainNChTrimSmoothingLaneSymmetric_exec_ci<dataType>},
       {AUDIOLIB_gainNChTrimSmoothingPlanarSmooth_exec_ci<dataType>,
        AUDIOLIB_gainNChTrimSmoothingLaneSmooth_exec_ci<dataType>}};
   const pFxnAUDIOLIB_gainNChTrimSmoothing_exec fadeFxn = fadeKernels[isSmooth][layout];

   if (isSmooth != 0U) {
      *pRampFxn      = (isInstant != 0U) ? pKerPrivArgs->bypassFxn : fadeFxn;
      *pConvergedFxn = *pRampFxn;

      if (isInstant != 0U) {
         for (int32_t ch = 0; ch < numChannels; ch++) {
            pKerPrivArgs->currentGain[ch] = pKerPrivArgs->targetGain[ch];
         }
      }
   }
   else {
      *pRampFxn      = fadeFxn;
      *pConvergedFxn = pKerPrivArgs->bypassFxn;
   }

   return status;
}

/* ========================================================================== */
/*  Explicit instantiations (float only; see init_checkParams)                */
/* ========================================================================== */

template AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothing_init_ci<float>(AUDIOLIB_kernelHandle                         handle,
                                             const AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothing_set_ci<float>(AUDIOLIB_kernelHandle                   handle,
                                            uint8_t                                 fadeType,
                                            uint8_t                                 isInstant,
                                            pFxnAUDIOLIB_gainNChTrimSmoothing_exec *pRampFxn,
                                            pFxnAUDIOLIB_gainNChTrimSmoothing_exec *pConvergedFxn);

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainNChTrimSmoothing_ci.cpp                      */
/* ======================================================================== */
