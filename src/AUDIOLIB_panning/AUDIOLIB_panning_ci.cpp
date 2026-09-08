// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/*******************************************************************************
 *
 * INCLUDES
 *
 ******************************************************************************/

#include "AUDIOLIB_panning_priv.h"

/*******************************************************************************
 *
 * PERFORMANCE ESTIMATE
 *
 ******************************************************************************/

void AUDIOLIB_panning_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   const AUDIOLIB_panning_PrivArgs *pKerPrivArgs = (const AUDIOLIB_panning_PrivArgs *) handle;

   const uint64_t numRowVec = (uint64_t) pKerPrivArgs->numRowVec;
   const uint64_t numColVec = (uint64_t) pKerPrivArgs->numColVec;

   const uint64_t startupCycles  = 27U;
   const uint64_t teardownCycles = 3U;
   uint64_t       operationCycles;

   if (pKerPrivArgs->bypassSmoothing == 1U) {
      /* One vector per iteration at ii = 1, over the unpadded vector count. */
      operationCycles = (4U + (uint64_t) pKerPrivArgs->numColVecBypass) * numRowVec;
   }
   else {
      /* Both smoothing variants retire four vectors per iteration at ii = 4. */
      operationCycles = (AUDIOLIB_PANNING_SMOOTH_ROW_OVERHEAD + numColVec) * numRowVec;
   }

   *estCycles  = operationCycles + startupCycles + teardownCycles;
   *archCycles = operationCycles;
}

/*******************************************************************************
 *
 * INITIALIZATION FUNCTION
 *
 ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_panning_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_panning_InitArgs *pKerInitArgs)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   /* The geometry is read from the handle, which AUDIOLIB_panning_init has
    * already filled in; these are kept only for the common init_ci signature. */
   (void) bufParamsIn;
   (void) bufParamsGain;
   (void) bufParamsOut;
   (void) pKerInitArgs;

   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_panning_PrivArgs *pKerPrivArgs = (AUDIOLIB_panning_PrivArgs *) handle;
   uint8_t                   *pBlock       = pKerPrivArgs->bufPblock;

   const uint32_t eleCount   = (uint32_t) c7x::element_count_of<vec>::value;
   const uint32_t samples    = (uint32_t) pKerPrivArgs->samples;
   const uint32_t channels   = pKerPrivArgs->channels;
   const uint32_t strideIn   = (uint32_t) pKerPrivArgs->strideInElements;
   const uint32_t strideOut  = (uint32_t) pKerPrivArgs->strideOutElements;
   const uint32_t numChGroup = AUDIOLIB_ceilingDiv(channels, eleCount);

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_init_ci channels %u samples %u strideIn %u eleCount %u\n", channels,
                         samples, strideIn, eleCount);

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   se0Params.ELETYPE = c7x::se_eletype<vec>::value;
   se0Params.VECLEN  = c7x::se_veclen<vec>::value;
   sa0Params.VECLEN  = c7x::sa_veclen<vec>::value;

   uint32_t numVec;
   uint32_t numVecPadded;

   if (pKerPrivArgs->isInterleave == 1U) {
      /* SA1 writes the running gains of each channel group back to currentGain. */
      __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();

      sa1Params.DIMFMT = __SA_DIMFMT_1D;
      sa1Params.VECLEN = c7x::sa_veclen<vec>::value;
      sa1Params.ICNT0  = channels;

      *(__SA_TEMPLATE_v1 *) (pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

      /* One output vector per sample and group of eleCount channels: SE0
       * broadcasts the sample across the lanes. */
      numVec       = samples;
      numVecPadded = AUDIOLIB_ceilingDiv(numVec, 4U) * 4U;

      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.ELEDUP        = c7x::se_eledup<dataType, vec>::value;
      se0Params.ICNT0         = 1U;
      se0Params.ICNT1         = numVecPadded;
      se0Params.DIM1          = (int32_t) strideIn;
      se0Params.ICNT2         = numChGroup;
      se0Params.DIM2          = 0;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.DECDIM1_WIDTH = samples * strideIn;

      /* Output rows are samples, columns channels: DECDIM1 masks the padding
       * rows and DECDIM2 the lanes of the last group past numChannels. */
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.ICNT0         = AUDIOLIB_min(channels, eleCount);
      sa0Params.ICNT1         = numVecPadded;
      sa0Params.DIM1          = (int32_t) strideOut;
      sa0Params.ICNT2         = numChGroup;
      sa0Params.DIM2          = (int32_t) eleCount;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = strideOut * samples;
      sa0Params.DECDIM2       = __SA_DECDIM_DIM2;
      sa0Params.DECDIM2_WIDTH = channels;

      pKerPrivArgs->numRowVec = numChGroup;
   }
   else {
      /* Planar output: one lane per sample. A vector is eleCount consecutive input
       * samples. The channel dimension re-reads the input (DIM2 = 0). */
      numVec       = AUDIOLIB_ceilingDiv(samples, eleCount);
      numVecPadded = AUDIOLIB_ceilingDiv(numVec, 4U) * 4U;

      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.ICNT0         = eleCount;
      se0Params.ICNT1         = numVecPadded;
      se0Params.DIM1          = (int32_t) eleCount;
      se0Params.ICNT2         = channels;
      se0Params.DIM2          = 0;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.DECDIM1_WIDTH = samples;

      /* Output rows are channels, columns samples: DECDIM1 masks the padding. */
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.ICNT0         = AUDIOLIB_min(samples, eleCount);
      sa0Params.ICNT1         = numVecPadded;
      sa0Params.DIM1          = (int32_t) eleCount;
      sa0Params.ICNT2         = channels;
      sa0Params.DIM2          = (int32_t) strideOut;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = samples;

      const uint32_t numPairs = channels >> 1U;
      const uint32_t numPass  = channels - numPairs;

      __SE_TEMPLATE_v1 se0PairParams = se0Params;
      __SA_TEMPLATE_v1 sa0PairParams = sa0Params;

      se0PairParams.ICNT2 = numPass;
      sa0PairParams.ICNT2 = numPass;
      sa0PairParams.DIM2  = (int32_t) (2U * strideOut);

      __SA_TEMPLATE_v1 sa1PairParams = sa0PairParams;
      sa1PairParams.ICNT2            = AUDIOLIB_max(numPairs, 1U);

      *(__SE_TEMPLATE_v1 *) (pBlock + SE_SE0_PAIR_PARAM_OFFSET) = se0PairParams;
      *(__SA_TEMPLATE_v1 *) (pBlock + SE_SA0_PAIR_PARAM_OFFSET) = sa0PairParams;
      *(__SA_TEMPLATE_v1 *) (pBlock + SE_SA1_PARAM_OFFSET)      = sa1PairParams;

      pKerPrivArgs->numRowVec = channels;
   }

   __SE_TEMPLATE_v1 se0BypassParams = se0Params;
   __SA_TEMPLATE_v1 sa0BypassParams = sa0Params;

   se0BypassParams.ICNT1 = numVec;
   sa0BypassParams.ICNT1 = numVec;

   pKerPrivArgs->numColVec       = numVecPadded;
   pKerPrivArgs->numColVecBypass = numVec;

   *(__SE_TEMPLATE_v1 *) (pBlock + SE_SE0_PARAM_OFFSET)        = se0Params;
   *(__SA_TEMPLATE_v1 *) (pBlock + SE_SA0_PARAM_OFFSET)        = sa0Params;
   *(__SE_TEMPLATE_v1 *) (pBlock + SE_SE0_BYPASS_PARAM_OFFSET) = se0BypassParams;
   *(__SA_TEMPLATE_v1 *) (pBlock + SE_SA0_BYPASS_PARAM_OFFSET) = sa0BypassParams;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_panning_init_ci<float>(AUDIOLIB_kernelHandle            handle,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                                         const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                                         const AUDIOLIB_panning_InitArgs *pKerInitArgs);

/*******************************************************************************
 *
 * EXECUTION FUNCTIONS
 *
 ******************************************************************************/

/* Smoothing bypassed: every output is the input scaled by its channel's target
 * gain. One vector per iteration. */
template <typename dataType, bool isInterleave>
AUDIOLIB_STATUS AUDIOLIB_panning_byPassSmoothing_exec_ci(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pGain,
                                                         void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   const AUDIOLIB_panning_PrivArgs *pKerPrivArgs = (const AUDIOLIB_panning_PrivArgs *) handle;
   const uint8_t                   *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_byPassSmoothing_exec_ci nColVec %u nRowVec %u\n",
                         pKerPrivArgs->numColVecBypass, pKerPrivArgs->numRowVec);

   __SE_TEMPLATE_v1 se0Params = *(const __SE_TEMPLATE_v1 *) (pBlock + SE_SE0_BYPASS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(const __SA_TEMPLATE_v1 *) (pBlock + SE_SA0_BYPASS_PARAM_OFFSET);

   const uint32_t numRowVec = pKerPrivArgs->numRowVec;
   const uint32_t numColVec = pKerPrivArgs->numColVecBypass;

   dataType *restrict pInLocal   = (dataType *) pIn;
   dataType *restrict pGainLocal = (dataType *) pGain;
   dataType *restrict pOutLocal  = (dataType *) pOut;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0U; i < numRowVec; i++) {
      vec targetGainVec = TargetGainFetcher<isInterleave, vec, dataType>::get(pGainLocal, i);

      /* numColVec is the exact, non-zero vector count, see init_ci */
#pragma MUST_ITERATE(1, , )
      for (uint32_t j = 0U; j < numColVec; j++) {
         vec inVec  = c7x::strm_eng<0, vec>::get_adv();
         vec outVec = inVec * targetGainVec;

         __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred, addr, outVec);
      }
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_panning_byPassSmoothing_exec_ci<float, true>(AUDIOLIB_kernelHandle handle,
                                                                               void *restrict pIn,
                                                                               void *restrict pGain,
                                                                               void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_panning_byPassSmoothing_exec_ci<float, false>(AUDIOLIB_kernelHandle handle,
                                                                                void *restrict pIn,
                                                                                void *restrict pGain,
                                                                                void *restrict pOut);


template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_panning_smooth_nonInterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                              void *restrict pIn,
                                                              void *restrict pGain,
                                                              void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   const AUDIOLIB_panning_PrivArgs *pKerPrivArgs = (const AUDIOLIB_panning_PrivArgs *) handle;
   const uint8_t                   *pBlock       = pKerPrivArgs->bufPblock;

   const uint32_t numChannels = pKerPrivArgs->numRowVec;
   const uint32_t numPairs    = numChannels >> 1U;
   const uint32_t hasTailChannel   = numChannels & 1U;
   const uint32_t pairIter    = pKerPrivArgs->numColVec >> 1U; /* two vectors per channel per iteration */
   const uint32_t quadIter    = pKerPrivArgs->numColVec >> 2U; /* four vectors per iteration */
   const dataType alphaN      = pKerPrivArgs->alphaN;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_smooth_nonInterleave_exec_ci nColVec %u nRowVec %u\n",
                         pKerPrivArgs->numColVec, numChannels);

   __SE_TEMPLATE_v1 se0Params = *(const __SE_TEMPLATE_v1 *) (pBlock + SE_SE0_PAIR_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(const __SA_TEMPLATE_v1 *) (pBlock + SE_SA0_PAIR_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(const __SA_TEMPLATE_v1 *) (pBlock + SE_SA1_PARAM_OFFSET);

   dataType *restrict pInLocal     = (dataType *) pIn;
   dataType *restrict pGainLocal   = (dataType *) pGain;
   dataType *restrict pOutEven     = (dataType *) pOut;
   dataType *restrict pOutOdd      = pOutEven + pKerPrivArgs->strideOutElements;
   dataType *restrict pCurrentGain = (dataType *) pKerPrivArgs->currentGain;

   /* Lane powers of the first vector of each slot: alpha^1..alpha^eleCount times
    * alphaPerBlock^k for slot k. */
   const vec alphaPerBlock = vec(pKerPrivArgs->alphaMultiplier);
   const vec alphaPerPair  = vec(pKerPrivArgs->alphaMultiplierPair); /* alphaPerBlock^2 */
   const vec alphaPerQuad  = vec(pKerPrivArgs->alphaMultiplierQuad); /* alphaPerBlock^4 */
   const vec slotAlpha0    = *stov_ptr(vec, &pKerPrivArgs->alphaCoeff[0]);
   const vec slotAlpha1    = slotAlpha0 * alphaPerBlock;
   const vec slotAlpha2    = slotAlpha0 * alphaPerPair;
   const vec slotAlpha3    = slotAlpha1 * alphaPerPair;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   /* The next pair's gains are loaded before the current pair's inner loop, so
    * their load latency is hidden behind it. The last pair reloads itself
    * instead of reading past the arrays; with a single channel B reads A. */
   const uint32_t firstB      = (uint32_t) (numChannels > 1U);
   dataType       nextTargetA = pGainLocal[0];
   dataType       nextCurA    = pCurrentGain[0];
   dataType       nextTargetB = pGainLocal[firstB];
   dataType       nextCurB    = pCurrentGain[firstB];

   for (uint32_t pair = 0U; pair < numPairs; pair++) {
      const dataType targetA  = nextTargetA;
      const dataType targetB  = nextTargetB;
      const dataType deltaA   = nextCurA - targetA;
      const dataType deltaB   = nextCurB - targetB;
      const uint32_t nextA    = 2U * (pair + (uint32_t) ((pair + 1U) < numPairs));

      nextTargetA = pGainLocal[nextA];
      nextCurA    = pCurrentGain[nextA];
      nextTargetB = pGainLocal[nextA + 1U];
      nextCurB    = pCurrentGain[nextA + 1U];

      /* Jump both running gains to their end-of-frame values. The pair works
       * from the deltas, so these stores overlap the setup. */
      pCurrentGain[2U * pair]        = (alphaN * deltaA) + targetA;
      pCurrentGain[(2U * pair) + 1U] = (alphaN * deltaB) + targetB;

      const vec targetVecA = vec(targetA);
      const vec targetVecB = vec(targetB);
      const vec deltaVecA  = vec(deltaA);
      const vec deltaVecB  = vec(deltaB);

      vec decayA0 = slotAlpha0 * deltaVecA;
      vec decayA1 = slotAlpha1 * deltaVecA;
      vec decayB0 = slotAlpha0 * deltaVecB;
      vec decayB1 = slotAlpha1 * deltaVecB;

      /* numColVec is a non-zero multiple of four, so pairIter is a non-zero
       * multiple of two. */
#pragma MUST_ITERATE(2, , 2)
      for (uint32_t j = 0U; j < pairIter; j++) {

         vec inVec0 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec1 = c7x::strm_eng<0, vec>::get_adv();

         vec outA0 = inVec0 * (decayA0 + targetVecA);
         vec outA1 = inVec1 * (decayA1 + targetVecA);
         vec outB0 = inVec0 * (decayB0 + targetVecB);
         vec outB1 = inVec1 * (decayB1 + targetVecB);

         decayA0 = decayA0 * alphaPerPair;
         decayA1 = decayA1 * alphaPerPair;
         decayB0 = decayB0 * alphaPerPair;
         decayB1 = decayB1 * alphaPerPair;

         __vpred vpredA0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addrA0  = c7x::strm_agen<0, vec>::get_adv(pOutEven);
         __vpred vpredA1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addrA1  = c7x::strm_agen<0, vec>::get_adv(pOutEven);
         __vpred vpredB0 = c7x::strm_agen<1, vec>::get_vpred();
         vec    *addrB0  = c7x::strm_agen<1, vec>::get_adv(pOutOdd);
         __vpred vpredB1 = c7x::strm_agen<1, vec>::get_vpred();
         vec    *addrB1  = c7x::strm_agen<1, vec>::get_adv(pOutOdd);
         __vstore_pred(vpredA0, addrA0, outA0);
         __vstore_pred(vpredA1, addrA1, outA1);
         __vstore_pred(vpredB0, addrB0, outB0);
         __vstore_pred(vpredB1, addrB1, outB1);
      }
   }

   /* Odd last channel: SE0's last input pass and SA0's last row. */
   for (uint32_t r = 0U; r < hasTailChannel; r++) {
      const uint32_t lastCh     = numChannels - 1U;
      const dataType targetGain = pGainLocal[lastCh];
      const dataType delta      = pCurrentGain[lastCh] - targetGain;

      pCurrentGain[lastCh] = (alphaN * delta) + targetGain;

      const vec targetGainVec = vec(targetGain);
      const vec gainDelta     = vec(delta);

      vec decay0 = slotAlpha0 * gainDelta;
      vec decay1 = slotAlpha1 * gainDelta;
      vec decay2 = slotAlpha2 * gainDelta;
      vec decay3 = slotAlpha3 * gainDelta;

#pragma MUST_ITERATE(1, , 1)
      for (uint32_t j = 0U; j < quadIter; j++) {

         vec inVec0 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec1 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec2 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec3 = c7x::strm_eng<0, vec>::get_adv();

         vec outVec0 = inVec0 * (decay0 + targetGainVec);
         vec outVec1 = inVec1 * (decay1 + targetGainVec);
         vec outVec2 = inVec2 * (decay2 + targetGainVec);
         vec outVec3 = inVec3 * (decay3 + targetGainVec);

         decay0 = decay0 * alphaPerQuad;
         decay1 = decay1 * alphaPerQuad;
         decay2 = decay2 * alphaPerQuad;
         decay3 = decay3 * alphaPerQuad;

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutEven);
         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutEven);
         __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutEven);
         __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutEven);
         __vstore_pred(vpred0, addr0, outVec0);
         __vstore_pred(vpred1, addr1, outVec1);
         __vstore_pred(vpred2, addr2, outVec2);
         __vstore_pred(vpred3, addr3, outVec3);
      }
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_panning_smooth_nonInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                              void *restrict pIn,
                                                                              void *restrict pGain,
                                                                              void *restrict pOut);

/* Smoothing, interleaved output: one channel group per outer iteration, one
 * channel per lane. Every lane advances by alpha per sample, so alphaPerBlock is
 * alpha and the same four decay chains as the planar variant apply, one per
 * sample row of the iteration. The group's running gains are written back to
 * currentGain through SA1, which also masks the lanes past numChannels. */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_panning_smooth_interleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                           void *restrict pIn,
                                                           void *restrict pGain,
                                                           void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   const uint32_t eleCount = (uint32_t) c7x::element_count_of<vec>::value;

   const AUDIOLIB_panning_PrivArgs *pKerPrivArgs = (const AUDIOLIB_panning_PrivArgs *) handle;
   const uint8_t                   *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_smooth_interleave_exec_ci nColVec %u nRowVec %u\n",
                         pKerPrivArgs->numColVec, pKerPrivArgs->numRowVec);

   __SE_TEMPLATE_v1 se0Params = *(const __SE_TEMPLATE_v1 *) (pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(const __SA_TEMPLATE_v1 *) (pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(const __SA_TEMPLATE_v1 *) (pBlock + SE_SA1_PARAM_OFFSET);

   dataType *restrict pInLocal    = (dataType *) pIn;
   dataType *restrict pGainLocal  = (dataType *) pGain;
   dataType *restrict pOutLocal   = (dataType *) pOut;
   dataType *restrict currentGain = (dataType *) pKerPrivArgs->currentGain;

   const vec alphaCoefficient = *stov_ptr(vec, &pKerPrivArgs->alphaCoeff[0]);
   const vec alphaPerBlock    = vec(pKerPrivArgs->alphaMultiplier);
   const vec alphaStep1       = alphaPerBlock;
   const vec alphaStep2       = alphaStep1 * alphaPerBlock;
   const vec alphaStep3       = alphaStep2 * alphaPerBlock;
   const vec alphaPerIter     = alphaStep3 * alphaPerBlock;

   const uint32_t quadCount = pKerPrivArgs->numColVec >> 2U; /* four vectors per iteration */

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   for (uint32_t i = 0U; i < pKerPrivArgs->numRowVec; i++) {
      const vec targetGainVec = *stov_ptr(vec, pGainLocal + (eleCount * i));
      const vec gainDelta     = *stov_ptr(vec, currentGain + (eleCount * i)) - targetGainVec;

      vec decay0 = alphaCoefficient * gainDelta;
      vec decay1 = alphaStep1 * decay0;
      vec decay2 = alphaStep2 * decay0;
      vec decay3 = alphaStep3 * decay0;

#pragma MUST_ITERATE(1, , 1)
      for (uint32_t j = 0U; j < quadCount; j++) {

         vec inVec0 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec1 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec2 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec3 = c7x::strm_eng<0, vec>::get_adv();

         vec outVec0 = inVec0 * (decay0 + targetGainVec);
         vec outVec1 = inVec1 * (decay1 + targetGainVec);
         vec outVec2 = inVec2 * (decay2 + targetGainVec);
         vec outVec3 = inVec3 * (decay3 + targetGainVec);

         decay0 = decay0 * alphaPerIter;
         decay1 = decay1 * alphaPerIter;
         decay2 = decay2 * alphaPerIter;
         decay3 = decay3 * alphaPerIter;

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred0, addr0, outVec0);
         __vstore_pred(vpred1, addr1, outVec1);
         __vstore_pred(vpred2, addr2, outVec2);
         __vstore_pred(vpred3, addr3, outVec3);
      }

      const vec     finalGainVec = (pKerPrivArgs->alphaN * gainDelta) + targetGainVec;
      const __vpred gainPred     = c7x::strm_agen<1, vec>::get_vpred();
      vec          *gainAddr     = c7x::strm_agen<1, vec>::get_adv(currentGain);
      __vstore_pred(gainPred, gainAddr, finalGainVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_panning_smooth_interleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pGain,
                                                                           void *restrict pOut);
