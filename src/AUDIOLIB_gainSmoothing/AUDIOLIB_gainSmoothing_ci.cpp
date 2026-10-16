// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

// /*******************************************************************************
//  *
//  * INCLUDES
//  *
//  ******************************************************************************/

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_gainSmoothing_priv.h"

/* Predicated stores of one output vector. One side effect per statement: the SA
   predicate must be read before the SA address advances. */
template <typename vec, typename dataType>
static inline void AUDIOLIB_gainSmoothing_storeSA0(dataType *restrict pOut, vec vOut)
{
   __vpred pred = c7x::strm_agen<0, vec>::get_vpred();
   vec    *addr = c7x::strm_agen<0, vec>::get_adv(pOut);
   __vstore_pred(pred, addr, vOut);
}

template <typename vec, typename dataType>
static inline void AUDIOLIB_gainSmoothing_storeSA1(dataType *restrict pOut, vec vOut)
{
   __vpred pred = c7x::strm_agen<1, vec>::get_vpred();
   vec    *addr = c7x::strm_agen<1, vec>::get_adv(pOut);
   __vstore_pred(pred, addr, vOut);
}

/* Planar: first channel streamed by SE1/SA1. Mono runs both halves on channel 0. */
static inline int32_t AUDIOLIB_gainSmoothing_planarOffB(int32_t numChannels, int32_t numPairs)
{
   return AUDIOLIB_min(numPairs, numChannels - 1);
}

void AUDIOLIB_gainSmoothing_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;

   const uint64_t numOuter = (uint64_t) pKerPrivArgs->numOuter;
   const uint64_t numInner = (uint64_t) pKerPrivArgs->numInner;
   const uint8_t  isSmooth = (uint8_t) (pKerPrivArgs->setArgs.fadeType == AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH);
   const uint8_t  isPlanar = (uint8_t) (pKerPrivArgs->isInterleaved == 0U);

   const uint8_t converged =
       (uint8_t) (fabsf(pKerPrivArgs->currentGain - pKerPrivArgs->targetGain) < AUDIOLIB_GAINSMOOTHING_GAIN_EPSILON);
   const uint8_t isBypass = (uint8_t) ((pKerPrivArgs->bypassSmoothing != 0U) || (converged != 0U));

   const uint64_t fadeVecs   = numOuter * numInner * 4U;
   const uint64_t bypassVecs = (uint64_t) pKerPrivArgs->bypassIters;

   uint64_t overheadCycles;

   if (isBypass != 0U) {
      overheadCycles = 74U;
      *archCycles    = bypassVecs;
   }
   else if (isPlanar != 0U) {
      overheadCycles = (isSmooth != 0U) ? (87U + (numOuter * 11U)) : (87U + (numOuter * 10U));
      *archCycles    = fadeVecs;
   }
   else {
      overheadCycles = (isSmooth != 0U) ? (85U + (numOuter * 11U)) : (78U + (numOuter * 10U));
      *archCycles    = fadeVecs;
   }

   *estCycles = *archCycles + overheadCycles;
}

// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_init_ci(AUDIOLIB_kernelHandle                  handle,
                                               const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothing_init_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                  status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs      = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock            = pKerPrivArgs->bufPblock;
   const int32_t                    eleCount          = c7x::element_count_of<vec>::value;
   const int32_t                    numCols           = (int32_t) pKerPrivArgs->numCols;
   const int32_t                    numRows           = (int32_t) pKerPrivArgs->numRows;
   const int32_t                    strideInElements  = (int32_t) pKerPrivArgs->strideInElements;
   const int32_t                    strideOutElements = (int32_t) pKerPrivArgs->strideOutElements;
   const uint8_t                    isInterleaved     = pKerInitArgs->isInterleaved;

   const __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   const __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   const __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif
   // /*********************************************************************************/
   // /* ByPass SE/SA Configuration                                                                   */
   // /*********************************************************************************/

   __SE_TEMPLATE_v1 seByp = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saByp = __gen_SA_TEMPLATE_v1();

   seByp.ELETYPE = SE_ELETYPE;
   seByp.VECLEN  = SE_VECLEN;
   seByp.DIMFMT  = __SE_DIMFMT_2D;
   seByp.ICNT0   = (uint32_t) numCols;
   seByp.ICNT1   = (uint32_t) numRows;
   seByp.DIM1    = strideInElements;

   saByp.VECLEN = SA_VECLEN;
   saByp.DIMFMT = __SA_DIMFMT_2D;
   saByp.ICNT0  = (uint32_t) numCols;
   saByp.ICNT1  = (uint32_t) numRows;
   saByp.DIM1   = strideOutElements;

   pKerPrivArgs->bypassIters = (uint32_t) (numRows * AUDIOLIB_ceilingDiv(numCols, eleCount));

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE_BYPASS_PARAM_OFFSET) = seByp;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA_BYPASS_PARAM_OFFSET) = saByp;

   // /*********************************************************************************/
   // /* Fade SE/SA Configuration                                                                     */
   // /*********************************************************************************/
   const int32_t numChannels = (isInterleaved != 0U) ? numCols : numRows;
   const int32_t numSamples  = (isInterleaved != 0U) ? numRows : numCols;

   pKerPrivArgs->numChannels = (uint32_t) numChannels;
   pKerPrivArgs->numSamples  = (uint32_t) numSamples;

   /* 1-based sample offset of each lane, for the planar SYMMETRIC ramp-vs-target select. */
   for (int32_t lane = 0; lane < eleCount; lane++) {
      pKerPrivArgs->laneOffset[lane] = (float) (lane + 1);
   }

   __SE_TEMPLATE_v1 seFade = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saFade = __gen_SA_TEMPLATE_v1();

   seFade.ELETYPE = SE_ELETYPE;
   seFade.VECLEN  = SE_VECLEN;
   saFade.VECLEN  = SA_VECLEN;

   if (isInterleaved != 0U) {
    
      const int32_t numBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
      const int32_t rowsPad =
          AUDIOLIB_ceilingDiv(numSamples, AUDIOLIB_GAINSMOOTHING_ROW_CHAINS) * AUDIOLIB_GAINSMOOTHING_ROW_CHAINS;

      pKerPrivArgs->numOuter = (uint32_t) numBlocks;
      pKerPrivArgs->numInner = (uint32_t) (rowsPad / AUDIOLIB_GAINSMOOTHING_ROW_CHAINS);

      seFade.DIMFMT        = __SE_DIMFMT_3D;
      seFade.ICNT0         = (uint32_t) eleCount;
      seFade.ICNT1         = (uint32_t) rowsPad;
      seFade.DIM1          = strideInElements;
      seFade.ICNT2         = (uint32_t) numBlocks;
      seFade.DIM2          = eleCount;
      seFade.DECDIM1       = __SE_DECDIM_DIM2;
      seFade.DECDIM1_WIDTH = (uint32_t) numChannels;
      seFade.DECDIM2       = __SE_DECDIM_DIM1;
      seFade.DECDIM2_WIDTH = (uint32_t) (numSamples * strideInElements);

      saFade.DIMFMT        = __SA_DIMFMT_3D;
      saFade.ICNT0         = (uint32_t) eleCount;
      saFade.ICNT1         = (uint32_t) rowsPad;
      saFade.DIM1          = strideOutElements;
      saFade.ICNT2         = (uint32_t) numBlocks;
      saFade.DIM2          = eleCount;
      saFade.DECDIM1       = __SA_DECDIM_DIM2;
      saFade.DECDIM1_WIDTH = (uint32_t) numChannels;
      saFade.DECDIM2       = __SA_DECDIM_DIM1;
      saFade.DECDIM2_WIDTH = (uint32_t) (numSamples * strideOutElements);
   }
   else {
   
      const int32_t numPairs = AUDIOLIB_ceilingDiv(numChannels, 2);
      const int32_t pairLen  = 2 * eleCount;
      const int32_t rowPad   = AUDIOLIB_ceilingDiv(numSamples, pairLen) * pairLen;

      pKerPrivArgs->numOuter = (uint32_t) numPairs;
      pKerPrivArgs->numInner = (uint32_t) (rowPad / pairLen);

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

      /* Second half: rows past numChannels (odd channel count) are zero-filled / store-masked. */
      const int32_t    rowsB   = numChannels - AUDIOLIB_gainSmoothing_planarOffB(numChannels, numPairs);
      __SE_TEMPLATE_v1 seFadeB = seFade;
      __SA_TEMPLATE_v1 saFadeB = saFade;

      seFadeB.DECDIM2       = __SE_DECDIM_DIM1;
      seFadeB.DECDIM2_WIDTH = (uint32_t) (rowsB * strideInElements);
      saFadeB.DECDIM2       = __SA_DECDIM_DIM1;
      saFadeB.DECDIM2_WIDTH = (uint32_t) (rowsB * strideOutElements);

      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SE_FADE_B_OFFSET) = seFadeB;
      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SA_FADE_B_OFFSET) = saFadeB;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SE_FADE_OFFSET) = seFade;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SA_FADE_OFFSET) = saFade;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_init_ci<float>(AUDIOLIB_kernelHandle                  handle,
                                                               const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

/* Constant gain, both layouts: one flat 2D stream (see init_ci), one vector per iteration. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingBypass_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothingBypass_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                   = (dataType *) pIn;
   dataType *restrict pOutLocal                  = (dataType *) pOut;
   const int32_t bypassIters                     = (int32_t) pKerPrivArgs->bypassIters;

   __SE_TEMPLATE_v1 seParams = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE_BYPASS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 saParams = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA_BYPASS_PARAM_OFFSET);

   const vec targetGainVec = (vec) (pKerPrivArgs->targetGain);

   __SE0_OPEN(pInLocal, seParams);
   __SA0_OPEN(saParams);

#pragma MUST_ITERATE(1, , 1)
   for (int32_t i = 0; i < bypassIters; i++) {
      vec vIn = c7x::strm_eng<0, vec>::get_adv();
      AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn * targetGainVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   // Keep the running gain in step with the natural C variant, which also pins it here.
   pKerPrivArgs->currentGain = pKerPrivArgs->targetGain;

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingBypass_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

// /*******************************************************************************
//  *
//  * NON-INTERLEAVED (PLANAR) EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/


template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSymmetricFade_exec_planar_ci(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothingSymmetricFade_exec_planar_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                   = (dataType *) pIn;
   dataType *restrict pOutLocal                  = (dataType *) pOut;

   const int32_t numChannels       = (int32_t) pKerPrivArgs->numChannels;
   const int32_t numPairs          = (int32_t) pKerPrivArgs->numOuter;
   const int32_t numInner          = (int32_t) pKerPrivArgs->numInner;
   const int32_t strideInElements  = (int32_t) pKerPrivArgs->strideInElements;
   const int32_t strideOutElements = (int32_t) pKerPrivArgs->strideOutElements;

   const int32_t chOffB = AUDIOLIB_gainSmoothing_planarOffB(numChannels, numPairs);
   dataType     *pInB   = pInLocal + (chOffB * strideInElements);
   dataType     *pOutB  = pOutLocal + (chOffB * strideOutElements);

   __SE_TEMPLATE_v1 seParams  = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SE_FADE_OFFSET);
   __SA_TEMPLATE_v1 saParams  = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SA_FADE_OFFSET);
   __SE_TEMPLATE_v1 seParamsB = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SE_FADE_B_OFFSET);
   __SA_TEMPLATE_v1 saParamsB = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SA_FADE_B_OFFSET);

   const vec targetGainVec  = (vec) (pKerPrivArgs->targetGain);
   const vec initialGainVec = (vec) (pKerPrivArgs->currentGain);

   // Initial gains of the two vector chains (samples [0, L) and [L, 2L)), and the factor that
   // advances each chain by one pair of vectors (2L samples).
   const vec gainStepVec     = *stov_ptr(vec, &pKerPrivArgs->alphaCoeff[0]);
   const vec initialGainVec0 = initialGainVec * gainStepVec;
   const vec initialGainVec1 = initialGainVec0 * vec(pKerPrivArgs->gainStepMultiplier);
   const vec vAdv            = vec(pKerPrivArgs->gainStepMultiplierBi);

   // A lane takes its ramp gain while its sample offset is short of the samples left in the
   // fade, and targetGain from there on. Chain 0 holds offsets [1, L], chain 1 [L + 1, 2L].
   const vec laneOffsetVec0 = *stov_ptr(vec, &pKerPrivArgs->laneOffset[0]);
   const vec laneOffsetVec1 = laneOffsetVec0 + (vec) ((dataType) c7x::element_count_of<vec>::value);
   const vec pairLenVec     = (vec) ((dataType) (2 * c7x::element_count_of<vec>::value));
   const vec fadeLeftVec    = (vec) (pKerPrivArgs->fadeSamplesLeft);

   __SE0_OPEN(pInLocal, seParams);
   __SE1_OPEN(pInB, seParamsB);
   __SA0_OPEN(saParams);
   __SA1_OPEN(saParamsB);

   for (int32_t pairIdx = 0; pairIdx < numPairs; pairIdx++) {
      // Every channel ramps from the same start gain.
      vec ramp0          = initialGainVec0;
      vec ramp1          = initialGainVec1;
      vec samplesLeftVec = fadeLeftVec;

#pragma MUST_ITERATE(1, , 1)
      for (int32_t grp = 0; grp < numInner; grp++) {
         vec vInA0 = c7x::strm_eng<0, vec>::get_adv();
         vec vInA1 = c7x::strm_eng<0, vec>::get_adv();
         vec vInB0 = c7x::strm_eng<1, vec>::get_adv();
         vec vInB1 = c7x::strm_eng<1, vec>::get_adv();

         vec currentGainVec0 = __select(__cmp_gt_pred(samplesLeftVec, laneOffsetVec0), ramp0, targetGainVec);
         vec currentGainVec1 = __select(__cmp_gt_pred(samplesLeftVec, laneOffsetVec1), ramp1, targetGainVec);

         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vInA0 * currentGainVec0);
         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vInA1 * currentGainVec1);
         AUDIOLIB_gainSmoothing_storeSA1<vec, dataType>(pOutB, vInB0 * currentGainVec0);
         AUDIOLIB_gainSmoothing_storeSA1<vec, dataType>(pOutB, vInB1 * currentGainVec1);

         ramp0          = ramp0 * vAdv;
         ramp1          = ramp1 * vAdv;
         samplesLeftVec = samplesLeftVec - pairLenVec;
      }
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   // Save the gain of the last sample so the next frame resumes the ramp from here, or lands
   // on the target once the fade has run its full length.
   pKerPrivArgs->fadeSamplesLeft -= (float) pKerPrivArgs->numSamples;
   pKerPrivArgs->currentGain = (pKerPrivArgs->fadeSamplesLeft > 0.0f)
                                   ? (pKerPrivArgs->factorPerBlock * pKerPrivArgs->currentGain)
                                   : pKerPrivArgs->targetGain;
   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSymmetricFade_exec_planar_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                   void *restrict pIn,
                                                                                   void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_planar_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothingSmoothFade_exec_planar_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                   = (dataType *) pIn;
   dataType *restrict pOutLocal                  = (dataType *) pOut;

   const int32_t numChannels       = (int32_t) pKerPrivArgs->numChannels;
   const int32_t numPairs          = (int32_t) pKerPrivArgs->numOuter;
   const int32_t numInner          = (int32_t) pKerPrivArgs->numInner;
   const int32_t strideInElements  = (int32_t) pKerPrivArgs->strideInElements;
   const int32_t strideOutElements = (int32_t) pKerPrivArgs->strideOutElements;

   const int32_t chOffB = AUDIOLIB_gainSmoothing_planarOffB(numChannels, numPairs);
   dataType     *pInB   = pInLocal + (chOffB * strideInElements);
   dataType     *pOutB  = pOutLocal + (chOffB * strideOutElements);

   __SE_TEMPLATE_v1 seParams  = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SE_FADE_OFFSET);
   __SA_TEMPLATE_v1 saParams  = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SA_FADE_OFFSET);
   __SE_TEMPLATE_v1 seParamsB = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SE_FADE_B_OFFSET);
   __SA_TEMPLATE_v1 saParamsB = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SA_FADE_B_OFFSET);

   const vec targetGainVec  = (vec) (pKerPrivArgs->targetGain);
   const vec initialGainVec = (vec) (pKerPrivArgs->currentGain);

   // Initial values of the two decay chains (samples [0, L) and [L, 2L)), and the factor that
   // advances each chain by one pair of vectors (2L samples).
   const vec alphaVec         = *stov_ptr(vec, &pKerPrivArgs->alphaCoeff[0]);
   const vec gainDeltaVec     = initialGainVec - targetGainVec;
   const vec initialDecayVec0 = gainDeltaVec * alphaVec;
   const vec initialDecayVec1 = initialDecayVec0 * vec(pKerPrivArgs->alphaMultiplier);
   const vec decayAdvVec      = vec(pKerPrivArgs->alphaMultiplierBi);

   __SE0_OPEN(pInLocal, seParams);
   __SE1_OPEN(pInB, seParamsB);
   __SA0_OPEN(saParams);
   __SA1_OPEN(saParamsB);

   for (int32_t pairIdx = 0; pairIdx < numPairs; pairIdx++) {
      // Every channel decays from the same start gain.
      vec decayVec0 = initialDecayVec0;
      vec decayVec1 = initialDecayVec1;

#pragma MUST_ITERATE(1, , 1)
      for (int32_t grp = 0; grp < numInner; grp++) {
         vec vInA0 = c7x::strm_eng<0, vec>::get_adv();
         vec vInA1 = c7x::strm_eng<0, vec>::get_adv();
         vec vInB0 = c7x::strm_eng<1, vec>::get_adv();
         vec vInB1 = c7x::strm_eng<1, vec>::get_adv();

         vec currentGainVec0 = decayVec0 + targetGainVec;
         vec currentGainVec1 = decayVec1 + targetGainVec;

         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vInA0 * currentGainVec0);
         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vInA1 * currentGainVec1);
         AUDIOLIB_gainSmoothing_storeSA1<vec, dataType>(pOutB, vInB0 * currentGainVec0);
         AUDIOLIB_gainSmoothing_storeSA1<vec, dataType>(pOutB, vInB1 * currentGainVec1);

         decayVec0 = decayVec0 * decayAdvVec;
         decayVec1 = decayVec1 * decayAdvVec;
      }
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   vec finalGain             = gainDeltaVec * pKerPrivArgs->factorPerBlock + targetGainVec;
   pKerPrivArgs->currentGain = __get_vector_element(finalGain, 0);

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSmoothFade_exec_planar_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                void *restrict pIn,
                                                                                void *restrict pOut);

// /*******************************************************************************
//  *
//  * INTERLEAVED (LANE) EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/


template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSymmetricFade_exec_lane_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothingSymmetricFade_exec_lane_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                   = (dataType *) pIn;
   dataType *restrict pOutLocal                  = (dataType *) pOut;

   const int32_t numBlocks = (int32_t) pKerPrivArgs->numOuter;
   const int32_t numInner  = (int32_t) pKerPrivArgs->numInner;

   __SE_TEMPLATE_v1 seParams = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SE_FADE_OFFSET);
   __SA_TEMPLATE_v1 saParams = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SA_FADE_OFFSET);

   const vec targetGainVec  = (vec) (pKerPrivArgs->targetGain);
   const vec initialGainVec = (vec) (pKerPrivArgs->currentGain);

   const dataType step            = (dataType) pKerPrivArgs->gainStep;
   const dataType stepSq          = step * step;
   const vec      initialGainVec0 = initialGainVec * (vec) step;
   const vec      initialGainVec1 = initialGainVec0 * (vec) step;
   const vec      initialGainVec2 = initialGainVec0 * (vec) stepSq;
   const vec      initialGainVec3 = initialGainVec1 * (vec) stepSq;
   const vec      vAdv            = (vec) (stepSq * stepSq);

   // Every lane of a row holds the same sample, so row r has offset r + 1 within its group of
   // four. A row takes its ramp gain while that offset is short of the samples left in the
   // fade, and targetGain from there on.
   const vec rowOffsetVec0 = (vec) ((dataType) 1);
   const vec rowOffsetVec1 = (vec) ((dataType) 2);
   const vec rowOffsetVec2 = (vec) ((dataType) 3);
   const vec rowOffsetVec3 = (vec) ((dataType) 4);
   const vec groupLenVec   = (vec) ((dataType) AUDIOLIB_GAINSMOOTHING_ROW_CHAINS);
   const vec fadeLeftVec   = (vec) (pKerPrivArgs->fadeSamplesLeft);

   __SE0_OPEN(pInLocal, seParams);
   __SA0_OPEN(saParams);

   for (int32_t blk = 0; blk < numBlocks; blk++) {
      // Every channel block ramps from the same start gain.
      vec ramp0          = initialGainVec0;
      vec ramp1          = initialGainVec1;
      vec ramp2          = initialGainVec2;
      vec ramp3          = initialGainVec3;
      vec samplesLeftVec = fadeLeftVec;

#pragma MUST_ITERATE(1, , 1)
      for (int32_t grp = 0; grp < numInner; grp++) {
         vec vIn0 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn1 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn2 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn3 = c7x::strm_eng<0, vec>::get_adv();

         vec currentGainVec0 = __select(__cmp_gt_pred(samplesLeftVec, rowOffsetVec0), ramp0, targetGainVec);
         vec currentGainVec1 = __select(__cmp_gt_pred(samplesLeftVec, rowOffsetVec1), ramp1, targetGainVec);
         vec currentGainVec2 = __select(__cmp_gt_pred(samplesLeftVec, rowOffsetVec2), ramp2, targetGainVec);
         vec currentGainVec3 = __select(__cmp_gt_pred(samplesLeftVec, rowOffsetVec3), ramp3, targetGainVec);

         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn0 * currentGainVec0);
         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn1 * currentGainVec1);
         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn2 * currentGainVec2);
         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn3 * currentGainVec3);

         ramp0          = ramp0 * vAdv;
         ramp1          = ramp1 * vAdv;
         ramp2          = ramp2 * vAdv;
         ramp3          = ramp3 * vAdv;
         samplesLeftVec = samplesLeftVec - groupLenVec;
      }
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   pKerPrivArgs->fadeSamplesLeft -= (float) pKerPrivArgs->numSamples;
   pKerPrivArgs->currentGain = (pKerPrivArgs->fadeSamplesLeft > 0.0f)
                                   ? (pKerPrivArgs->factorPerBlock * pKerPrivArgs->currentGain)
                                   : pKerPrivArgs->targetGain;
   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSymmetricFade_exec_lane_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                 void *restrict pIn,
                                                                                 void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_lane_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothingSmoothFade_exec_lane_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                   = (dataType *) pIn;
   dataType *restrict pOutLocal                  = (dataType *) pOut;

   const int32_t numBlocks = (int32_t) pKerPrivArgs->numOuter;
   const int32_t numInner  = (int32_t) pKerPrivArgs->numInner;

   __SE_TEMPLATE_v1 seParams = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SE_FADE_OFFSET);
   __SA_TEMPLATE_v1 saParams = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_GAINSMOOTHING_SA_FADE_OFFSET);

   const vec targetGainVec  = (vec) (pKerPrivArgs->targetGain);
   const vec initialGainVec = (vec) (pKerPrivArgs->currentGain);

   const dataType coeff            = (dataType) pKerPrivArgs->smoothingCoefficient;
   const dataType coeffSq          = coeff * coeff;
   const vec      gainDeltaVec     = initialGainVec - targetGainVec;
   const vec      initialDecayVec0 = gainDeltaVec * (vec) coeff;
   const vec      initialDecayVec1 = initialDecayVec0 * (vec) coeff;
   const vec      initialDecayVec2 = initialDecayVec0 * (vec) coeffSq;
   const vec      initialDecayVec3 = initialDecayVec1 * (vec) coeffSq;
   const vec      decayAdvVec      = (vec) (coeffSq * coeffSq);

   __SE0_OPEN(pInLocal, seParams);
   __SA0_OPEN(saParams);

   for (int32_t blk = 0; blk < numBlocks; blk++) {
      // Every channel block decays from the same start gain.
      vec decayVec0 = initialDecayVec0;
      vec decayVec1 = initialDecayVec1;
      vec decayVec2 = initialDecayVec2;
      vec decayVec3 = initialDecayVec3;

#pragma MUST_ITERATE(1, , 1)
      for (int32_t grp = 0; grp < numInner; grp++) {
         vec vIn0 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn1 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn2 = c7x::strm_eng<0, vec>::get_adv();
         vec vIn3 = c7x::strm_eng<0, vec>::get_adv();

         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn0 * (decayVec0 + targetGainVec));
         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn1 * (decayVec1 + targetGainVec));
         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn2 * (decayVec2 + targetGainVec));
         AUDIOLIB_gainSmoothing_storeSA0<vec, dataType>(pOutLocal, vIn3 * (decayVec3 + targetGainVec));

         decayVec0 = decayVec0 * decayAdvVec;
         decayVec1 = decayVec1 * decayAdvVec;
         decayVec2 = decayVec2 * decayAdvVec;
         decayVec3 = decayVec3 * decayAdvVec;
      }
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   vec finalGain             = gainDeltaVec * pKerPrivArgs->factorPerBlock + targetGainVec;
   pKerPrivArgs->currentGain = __get_vector_element(finalGain, 0);

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSmoothFade_exec_lane_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                              void *restrict pIn,
                                                                              void *restrict pOut);
