// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/********************************************************************************
 *  @file     AUDIOLIB_upSample_ci.cpp
 *  @brief    Optimized C7x implementation of AUDIOLIB_upSample. Provides the
 *            interleaved variant, per-upFactor planar specializations, and a
 *            generic planar fallback selected by AUDIOLIB_upSample_init.
 *  @version  0.1 - 2025 : Initial Version
 ********************************************************************************/

/*******************************************************************************
 *
 * INCLUDES
 *
 ******************************************************************************/

#include "AUDIOLIB_upSample_masks.h"
#include "AUDIOLIB_upSample_priv.h"

void AUDIOLIB_upSample_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_upSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;

   uint64_t upSampleStartupCycles     = 0;
   uint64_t upSampleTeardownCycles    = 0;
   uint64_t upSampleZeroFillingCycles = 0;
   uint64_t phaseFillingCycles        = 0;
   uint64_t upSampleOperationCycles   = 0;
   uint64_t upSampleOverheadCycles    = 0;
   uint32_t nVecs                     = 0;
   uint32_t phaseZeroFillVecs         = 0;
   uint32_t upSampleZeroFillVecs      = 0;
   nVecs                              = pKerPrivArgs->nVecs;
   phaseZeroFillVecs                  = pKerPrivArgs->phaseZeroFillVecs;
   upSampleZeroFillVecs               = pKerPrivArgs->numZeroFillVecs;

   if ((pKerPrivArgs->phase != 0U) && (pKerPrivArgs->genericNonInterleaveCase == 0U)) {
      phaseFillingCycles = 11 + 2 + (1 + phaseZeroFillVecs * 1) + 2;
   }
   if ((pKerPrivArgs->isInterleave == 1U) && (pKerPrivArgs->useNonInterleave == 0U)) {
      upSampleStartupCycles   = 26 + 2;
      upSampleOperationCycles = 1 + nVecs * 1;
      upSampleTeardownCycles  = 1 + 1 + 1;
   }
   else {
      if (pKerPrivArgs->upFactor == 2U || ((pKerPrivArgs->upFactor == 4U) && (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32))) {
         upSampleStartupCycles   = 25 + 3;
         upSampleOperationCycles = 4 + nVecs * 2;
         upSampleTeardownCycles  = 2 + 1 + 2;
      }
      else if (pKerPrivArgs->upFactor == 3U ||
               ((pKerPrivArgs->upFactor == 6U) && (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32))) {
         upSampleStartupCycles   = 25 + 1;
         upSampleOperationCycles = 8 + nVecs * 3;
         upSampleTeardownCycles  = 4 + 3 + 3;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {

         if (pKerPrivArgs->upFactor == 5U || pKerPrivArgs->upFactor == 10U) {
            upSampleStartupCycles   = 26 + 3;
            upSampleOperationCycles = 4 + nVecs * 5;
            upSampleTeardownCycles  = 2 + 5 + 2;
         }
         else if (pKerPrivArgs->upFactor == 7U || pKerPrivArgs->upFactor == 14U) {
            upSampleStartupCycles   = 26 + 1;
            upSampleOperationCycles = 5 + nVecs * 7;
            upSampleTeardownCycles  = 4 + 7 + 2;
         }
         else if (pKerPrivArgs->upFactor == 8U || pKerPrivArgs->upFactor == 16U || pKerPrivArgs->upFactor == 24U) {
            upSampleStartupCycles   = 26 + 2;
            upSampleOperationCycles = 1 + nVecs * 1;
            upSampleTeardownCycles  = 3;
         }
         else if (pKerPrivArgs->upFactor == 12U) {
            upSampleStartupCycles   = 25 + 2;
            upSampleOperationCycles = 4 + nVecs * 6;
            upSampleTeardownCycles  = 8 + 1 + 2;
         }
         else {
            upSampleStartupCycles     = 26 + 3;
            upSampleZeroFillingCycles = 1 + upSampleZeroFillVecs * 1;
            upSampleOperationCycles   = 1 + nVecs * 1;
            upSampleTeardownCycles    = 3;
         }
      }
      else {
         if (pKerPrivArgs->upFactor == 4U) {
            upSampleStartupCycles   = 25 + 4;
            upSampleOperationCycles = 4 + nVecs * 4;
            upSampleTeardownCycles  = 4 + 3;
         }
         else if (pKerPrivArgs->upFactor == 5U) {
            upSampleStartupCycles   = 26 + 1;
            upSampleOperationCycles = 4 + nVecs * 5;
            upSampleTeardownCycles  = 3 + 5 + 2;
         }
         else if (pKerPrivArgs->upFactor == 6U) {
            upSampleStartupCycles   = 26 + 3;
            upSampleOperationCycles = 5 + nVecs * 6;
            upSampleTeardownCycles  = 3 + 6 + 2;
         }
         else if (pKerPrivArgs->upFactor == 7U) {
            upSampleStartupCycles   = 26 + 3;
            upSampleOperationCycles = 8 + nVecs * 8;
            upSampleTeardownCycles  = 8 + 1 + 2;
         }
         else if (pKerPrivArgs->upFactor == 8U) {
            upSampleStartupCycles   = 26 + 3;
            upSampleOperationCycles = 5 + nVecs * 8;
            upSampleTeardownCycles  = 3 + 8 + 2;
         }
         else {
            upSampleStartupCycles     = 26 + 3;
            upSampleZeroFillingCycles = 1 + upSampleZeroFillVecs * 1;
            upSampleOperationCycles   = 1 + nVecs * 1;
            upSampleTeardownCycles    = 3;
         }
      }
   }

   upSampleOverheadCycles += upSampleStartupCycles + upSampleTeardownCycles;
   *estCycles  = phaseFillingCycles + upSampleZeroFillingCycles + upSampleOperationCycles + upSampleOverheadCycles;
   *archCycles = upSampleOperationCycles;
}
/*-------------------------------------AUDIOLIB_phase (Filling Zero) --------------------------*/
template <bool hasPhase, typename dataType> struct AUDIOLIB_phaseZeroExecutor {
   static inline void execute(void *, uint32_t, const uint8_t *) {} /* Empty: Generates zero instructions*/
};

/* Specialized executor template when phase operations are active */
template <typename dataType> struct AUDIOLIB_phaseZeroExecutor<true, dataType> {

   static inline void execute(void *pOut, uint32_t phaseFillVecs, const uint8_t *pBlock)
   {
      typedef typename c7x::make_full_vector<dataType>::type vec;
      dataType *restrict pOutPtr = (dataType *) pOut;
      vec zeroVec                = vec(0);

      __SA_TEMPLATE_v1 phaseFillParams = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
      __SA0_OPEN(phaseFillParams);

      for (uint32_t p = 0; p < phaseFillVecs; p++) {
         __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutPtr);
         __vstore_pred(vpred, addr, zeroVec);
      }
      __SA0_CLOSE();
   }
};

/*----------------------------------AUDIOLIB_upSample Init --------------------------------------*/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample_init_ci(AUDIOLIB_kernelHandle             handle,
                                          const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                          const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                          const AUDIOLIB_upSample_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   /* bufParams and InitArgs are not needed here — the output geometry is taken
    * from the PrivArgs fields populated by AUDIOLIB_upSample_init. */
   (void) bufParamsIn;
   (void) bufParamsOut;
   (void) pKerInitArgs;
   pKerPrivArgs->phaseZeroFillVecs = 0U;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_init_ci \n");

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   uint32_t upFactor  = pKerPrivArgs->upFactor;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = 0;

   pKerPrivArgs->numZeroFillVecs          = 0;
   __SE_TEMPLATE_v1 seIn                  = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saOut                 = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saPhase               = __gen_SA_TEMPLATE_v1();
   pKerPrivArgs->genericNonInterleaveCase = 0;
   if ((pKerPrivArgs->isInterleave == 1U) && (pKerPrivArgs->useNonInterleave == 0U)) {

      uint32_t numChannels = pKerPrivArgs->dimXIn;
      uint32_t numSamples  = pKerPrivArgs->dimYIn;
      uint32_t chnPerVec   = (numChannels < eleCount) ? numChannels : eleCount;
      uint32_t vecsPerRow  = AUDIOLIB_ceilingDiv(numChannels, eleCount);
      strideOut            = pKerPrivArgs->strideOut;
      uint32_t outNumRows  = upFactor * numSamples;

      seIn.ELETYPE = SE_ELETYPE;
      seIn.VECLEN  = SE_VECLEN;
      saOut.VECLEN = SA_VECLEN;

      seIn.ICNT0         = chnPerVec;
      seIn.ICNT1         = numSamples;
      seIn.DIM1          = strideIn;
      seIn.ICNT2         = vecsPerRow;
      seIn.DIM2          = eleCount;
      seIn.DIMFMT        = __SE_DIMFMT_3D;
      seIn.LEZR          = __SE_LEZR_ICNT0;
      seIn.LEZR_CNT      = upFactor - 1;
      seIn.DECDIM1_WIDTH = numChannels;
      seIn.DECDIM1       = __SE_DECDIM_DIM3;

      saOut.ICNT0         = chnPerVec;
      saOut.DIM1          = pKerPrivArgs->strideOut;
      saOut.ICNT1         = outNumRows;
      saOut.ICNT2         = vecsPerRow;
      saOut.DIM2          = eleCount;
      saOut.DIMFMT        = __SA_DIMFMT_3D;
      saOut.DECDIM1_WIDTH = (outNumRows - pKerPrivArgs->phase) * pKerPrivArgs->strideOut;
      saOut.DECDIM1       = __SA_DECDIM_DIM1;
      saOut.DECDIM2_WIDTH = numChannels;
      saOut.DECDIM2       = __SA_DECDIM_DIM2;

      if (pKerPrivArgs->phase > 0U) {
         saPhase.VECLEN = SA_VECLEN;
         saPhase.ICNT0  = numChannels;
         saPhase.ICNT1  = pKerPrivArgs->phase;
         saPhase.DIM1   = strideOut;
         saPhase.DIMFMT = __SA_DIMFMT_2D;

         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = saPhase;

         pKerPrivArgs->phaseZeroFillVecs = AUDIOLIB_ceilingDiv(numChannels, eleCount) * pKerPrivArgs->phase;
      }

      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = seIn;
      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = saOut;
      pKerPrivArgs->nVecs       = (outNumRows * AUDIOLIB_ceilingDiv(numChannels, eleCount)) - pKerPrivArgs->phase;
      pKerPrivArgs->phaseOffset = (int32_t) (pKerPrivArgs->phase * strideOut);
   }
   else {

      uint32_t numChannels = pKerPrivArgs->dimYIn;
      uint32_t numSamples  = pKerPrivArgs->dimXIn;

      if (pKerPrivArgs->useNonInterleave == 1U) {
         numChannels = 1;
         numSamples  = pKerPrivArgs->dimYIn;
         strideOut   = numSamples * upFactor; /*To handle the phase case of interleave && 1 CH && NotAlign*/
         strideIn    = pKerPrivArgs->dimYIn;
      }
      else {
         strideOut = pKerPrivArgs->strideOut;
      }
      uint32_t       outSamples    = upFactor * numSamples;
      const bool     isFloat32     = (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32);
      const bool     isFloat64     = (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64);
      const bool     isLowUpFactor = (upFactor == 2U) || (upFactor == 3U) || (upFactor == 5U) || (upFactor == 7U);
      /* The promoted path specifically handles 6, 10, 14 for Float32*/
      const bool isPromotedPath = ((upFactor == 4U) || (upFactor == 6U) || (upFactor == 10U) ||
                                   (upFactor == 12U) || (upFactor == 14U)) && isFloat32;
      const bool isSpecialF64 = ((upFactor == 4U) || (upFactor == 6U) || (upFactor == 8U)) && isFloat64;

      /* Check if current setup meets execution criteria */
      if (isLowUpFactor || isSpecialF64 || isPromotedPath) {
         uint32_t mul2;
         uint32_t factor;
         if (isPromotedPath) {
            mul2                       = 2;
            const uint32_t upFactorBy2 = upFactor >> 1;
            factor                     = upFactorBy2;
            seIn.VECLEN                = c7x::se_veclen<float4>::value;
            seIn.PROMOTE               = __SE_PROMOTE_2X_ZEROEXT;
         }
         else {
            mul2        = 1;
            factor      = upFactor;
            seIn.VECLEN = SE_VECLEN;
         }

         seIn.ELETYPE       = SE_ELETYPE;
         seIn.ICNT0         = eleCount;
         seIn.DIM1          = eleCount;
         seIn.ICNT1         = AUDIOLIB_ceilingDiv(numSamples, eleCount);
         seIn.DIM2          = strideIn;
         seIn.ICNT2         = numChannels;
         seIn.DIMFMT        = __SE_DIMFMT_3D;
         seIn.DECDIM1_WIDTH = numSamples;
         seIn.DECDIM1       = __SE_DECDIM_DIM1;

         saOut.VECLEN        = SA_VECLEN;
         saOut.ICNT0         = eleCount * factor;
         saOut.DIM1          = eleCount * factor;
         saOut.ICNT1         = ((AUDIOLIB_ceilingDiv(numSamples, eleCount)) * mul2);
         saOut.DIM2          = strideOut;
         saOut.ICNT2         = numChannels;
         saOut.DIMFMT        = __SA_DIMFMT_3D;
         saOut.DECDIM1       = __SA_DECDIM_DIM1;
         saOut.DECDIM1_WIDTH = outSamples - pKerPrivArgs->phase;

         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = seIn;
         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = saOut;
         pKerPrivArgs->nVecs = numChannels * (AUDIOLIB_ceilingDiv(numSamples, eleCount)) * mul2;
      }
      else if (((upFactor == 8U) || (upFactor == 16U) || (upFactor == 24U)) && isFloat32) {

         seIn.ICNT0         = 1;
         seIn.ICNT1         = numSamples;
         seIn.DIM1          = 1;
         seIn.ICNT2         = numChannels;
         seIn.DIM2          = strideIn;
         seIn.DIMFMT        = __SE_DIMFMT_3D;
         seIn.ELETYPE       = SE_ELETYPE;
         seIn.VECLEN        = SE_VECLEN;
         seIn.DECDIM1_WIDTH = numSamples;
         seIn.DECDIM1       = __SE_DECDIM_DIM1;
         seIn.LEZR          = (upFactor == 8U) ? __SE_LEZR_OFF : __SE_LEZR_ICNT0;
         seIn.LEZR_CNT      = (upFactor >> 3U) - 1U;

         saOut.VECLEN        = SA_VECLEN;
         saOut.ICNT0         = eleCount;
         saOut.DIM1          = eleCount;
         saOut.ICNT1         = (upFactor >> 3U) * (numSamples);
         saOut.DIM2          = strideOut;
         saOut.ICNT2         = numChannels;
         saOut.DIMFMT        = __SA_DIMFMT_3D;
         saOut.DECDIM1       = __SA_DECDIM_DIM1;
         saOut.DECDIM1_WIDTH = outSamples - pKerPrivArgs->phase;

         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = seIn;
         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = saOut;
         pKerPrivArgs->nVecs = (numSamples) * (upFactor >> 3) * numChannels;
      }
      else {
         pKerPrivArgs->genericNonInterleaveCase = 1;
         __SA_TEMPLATE_v1 saZero                = __gen_SA_TEMPLATE_v1();
         saZero.ICNT0                           = outSamples;
         saZero.ICNT1                           = numChannels;
         saZero.DIM1                            = strideOut;
         saZero.VECLEN                          = SA_VECLEN;
         saZero.DIMFMT                          = __SA_DIMFMT_2D;

         seIn.ICNT0   = 1;
         seIn.ICNT1   = numSamples;
         seIn.DIM1    = 1;
         seIn.ICNT2   = numChannels;
         seIn.DIM2    = strideIn;
         seIn.DIMFMT  = __SE_DIMFMT_3D;
         seIn.ELETYPE = SE_ELETYPE;
         seIn.VECLEN  = SE_VECLEN;

         saOut.ICNT0  = 1;
         saOut.ICNT1  = numSamples;
         saOut.DIM1   = upFactor;
         saOut.ICNT2  = numChannels;
         saOut.DIM2   = strideOut;
         saOut.VECLEN = SA_VECLEN;
         saOut.DIMFMT = __SA_DIMFMT_3D;

         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = saZero;
         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = seIn;
         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = saOut;
         pKerPrivArgs->nVecs                                              = numSamples * numChannels;
         pKerPrivArgs->numZeroFillVecs = AUDIOLIB_ceilingDiv(outSamples, eleCount) * numChannels;
      }
      /* Phase Included case for Non Interleave Data Format*/
      if (pKerPrivArgs->genericNonInterleaveCase == 0U) {
         pKerPrivArgs->phaseZeroFillVecs = AUDIOLIB_ceilingDiv(pKerPrivArgs->phase, eleCount) * numChannels;
      }
      pKerPrivArgs->phaseOffset       = (int32_t) pKerPrivArgs->phase;
      if ((pKerPrivArgs->phase > 0U) && (pKerPrivArgs->genericNonInterleaveCase == 0U)) {
         saPhase.VECLEN = SA_VECLEN;
         saPhase.ICNT0  = pKerPrivArgs->phase;
         saPhase.ICNT1  = numChannels;
         saPhase.DIM1   = strideOut;
         saPhase.DIMFMT = __SA_DIMFMT_2D;

         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = saPhase;
      }
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_upSample_init_ci<float>(AUDIOLIB_kernelHandle             handle,
                                                          const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                                          const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                                          const AUDIOLIB_upSample_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_upSample_init_ci<double>(AUDIOLIB_kernelHandle             handle,
                                                           const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                                           const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                                           const AUDIOLIB_upSample_InitArgs *pKerInitArgs);

/*-----------------------------------INTERLEAVE DATA FORMAT UPFACTOR ------------------------*/
template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_upSample_execInterleave_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock        = pKerPrivArgs->bufPblock;
   uint32_t                                               nVecs         = pKerPrivArgs->nVecs;
   uint32_t                                               phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_exec_ci numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   AUDIOLIB_phaseZeroExecutor<hasPhase, dataType>::execute(pOut, phaseFillVecs, pBlock);

   dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;
   __SE_TEMPLATE_v1 se0Params  = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params  = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < nVecs; i++) {
      vec     inVec  = c7x::strm_eng<0, vec>::get_adv();
      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred0, addr, inVec);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_upSample_execInterleave_ci<true, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_upSample_execInterleave_ci<true, double>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_upSample_execInterleave_ci<false, float>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_upSample_execInterleave_ci<false, double>(AUDIOLIB_kernelHandle handle,
                                                                            void *restrict pIn,
                                                                            void *restrict pOut);

/* ------------------------ NONINTERLEAVE DATA FORMAT UPFACTOR -------------------------*/
/* ============================================================================
            UPFACTOR 2 FLOAT AND DOUBLE SPECIALIZATION
   ============================================================================*/
template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample2_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   uint8_t *pBlock        = pKerPrivArgs->bufPblock;
   uint32_t nVecs         = pKerPrivArgs->nVecs;
   uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample2_exec_ci numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

   AUDIOLIB_phaseZeroExecutor<hasPhase, dataType>::execute(pOut, phaseFillVecs, pBlock);
   dataType *restrict pOutData = (dataType *) (pOutLocal + (int32_t) pKerPrivArgs->phaseOffset);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < nVecs; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      vec permuteVec0 = c7x::reinterpret<vec>(__shuffle_stride2_low_low(zeroVec, inVec));
      vec permuteVec1 = c7x::reinterpret<vec>(__shuffle_stride2_high_high(zeroVec, inVec));

      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred0, addr0, permuteVec0);
      __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred1, addr1, permuteVec1);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_upSample2_exec_ci<true, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample2_exec_ci<true, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample2_exec_ci<false, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample2_exec_ci<false, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*============================================================================
/ UPFACTOR 3 FLOAT AND DOUBLE SPECIALIZATION
/ ============================================================================*/
template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample3_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   uint8_t *pBlock        = pKerPrivArgs->bufPblock;
   uint32_t nVecs         = pKerPrivArgs->nVecs;
   uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample3_exec_ci numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

   AUDIOLIB_phaseZeroExecutor<hasPhase, dataType>::execute(pOut, phaseFillVecs, pBlock);
   dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

   typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;
   typedef upSampleRatio3_Mask<dataType>                                            mask;
   v_bool_tp vsMask0 = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel0());
   v_bool_tp vsMask1 = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel1());
   v_bool_tp vsMask2 = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel2());
   uchar32   vpMask0 = *stov_ptr(uchar32, (uint8_t *) mask::permute0());
   uchar32   vpMask1 = *stov_ptr(uchar32, (uint8_t *) mask::permute1());
   uchar32   vpMask2 = *stov_ptr(uchar32, (uint8_t *) mask::permute2());

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < nVecs; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      vec selectVec01 = __select(vsMask0, zeroVec, inVec);
      vec selectVec02 = __select(vsMask1, zeroVec, inVec);
      vec selectVec03 = __select(vsMask2, zeroVec, inVec);
      vec permuteVec0 = c7x::reinterpret<vec>(__permute(vpMask0, __as_uchar32(selectVec01)));
      vec permuteVec1 = c7x::reinterpret<vec>(__permute(vpMask1, __as_uchar32(selectVec02)));
      vec permuteVec2 = c7x::reinterpret<vec>(__permute(vpMask2, __as_uchar32(selectVec03)));

      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred0, addr, permuteVec0);
      __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred1, addr1, permuteVec1);
      __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred2, addr2, permuteVec2);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_upSample3_exec_ci<true, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample3_exec_ci<true, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample3_exec_ci<false, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample3_exec_ci<false, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

// ============================================================================
// UPFACTOR 4 DOUBLE SPECIALIZATION
// ============================================================================

template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample4_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   uint8_t *pBlock        = pKerPrivArgs->bufPblock;
   uint32_t nVecs         = pKerPrivArgs->nVecs;
   uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample4_exec_ci numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

   AUDIOLIB_phaseZeroExecutor<hasPhase, dataType>::execute(pOut, phaseFillVecs, pBlock);
   dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

   typedef upSampleRatio4_Mask<dataType> mask;
   uchar32                               vpMask0 = *stov_ptr(uchar32, (uint8_t *) mask::permute0());
   uchar32                               vpMask1 = *stov_ptr(uchar32, (uint8_t *) mask::permute1());

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < nVecs; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      vec permuteVec0 = c7x::reinterpret<vec>(__permute_low_low(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec1 = c7x::reinterpret<vec>(__permute_low_low(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec2 = c7x::reinterpret<vec>(__permute_high_high(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec3 = c7x::reinterpret<vec>(__permute_high_high(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));

      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred0, addr, permuteVec0);
      __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred1, addr1, permuteVec1);
      __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred2, addr2, permuteVec2);
      __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred3, addr3, permuteVec3);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_upSample4_exec_ci<true, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample4_exec_ci<false, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

// ============================================================================
// UPFACTOR 5 FLOAT SPECIALIZATION
// ============================================================================

template <bool hasPhase, typename dataType> struct AUDIOLIB_nonInterleaveUpSample5;
template <bool hasPhase> struct AUDIOLIB_nonInterleaveUpSample5<hasPhase, float> {
   static inline AUDIOLIB_STATUS execute(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
   {
      AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
      AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
      typedef float                                          dataType;
      typedef typename c7x::make_full_vector<dataType>::type vec;

      uint8_t *pBlock        = pKerPrivArgs->bufPblock;
      uint32_t nVecs         = pKerPrivArgs->nVecs;
      uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

      AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample5_exec_ci (Float) numZeroFillVecs %u nVecs %u\n",
                            pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

      dataType *restrict pInLocal  = (dataType *) pIn;
      dataType *restrict pOutLocal = (dataType *) pOut;

      vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

      AUDIOLIB_phaseZeroExecutor<hasPhase, float>::execute(pOut, phaseFillVecs, pBlock);
      dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

      typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;
      typedef upSampleRatio5_Mask<dataType>                                            mask;
      uchar32   vpMask0 = *stov_ptr(uchar32, (uint8_t *) mask::permute0());
      uchar32   vpMask1 = *stov_ptr(uchar32, (uint8_t *) mask::permute1());
      v_bool_tp vpMask2 = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel1());
      uchar32   vpMask3 = *stov_ptr(uchar32, (uint8_t *) mask::permute3());
      uchar32   vpMask4 = *stov_ptr(uchar32, (uint8_t *) mask::permute4());

      __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
      __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
      __SE0_OPEN(pInLocal, se0Params);
      __SA0_OPEN(sa0Params);

      for (uint32_t i = 0; i < nVecs; i++) {
         vec inVec = c7x::strm_eng<0, vec>::get_adv();

         vec permuteVec0 =
             c7x::reinterpret<vec>(__permute_low_low(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec permuteVec1 =
             c7x::reinterpret<vec>(__permute_low_low(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec permuteVec2 = c7x::reinterpret<vec>(__select(vpMask2, (zeroVec), (inVec)));
         vec permuteVec3 =
             c7x::reinterpret<vec>(__permute_high_high(vpMask3, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec permuteVec4 =
             c7x::reinterpret<vec>(__permute_high_high(vpMask4, __as_uchar32(zeroVec), __as_uchar32(inVec)));

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred0, addr, permuteVec0);
         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred1, addr1, permuteVec1);
         __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred2, addr2, permuteVec2);
         __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred3, addr3, permuteVec3);
         __vpred vpred4 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr4  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred4, addr4, permuteVec4);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
      return status;
   }
};

// ============================================================================
// UPFACTOR 5 DOUBLE SPECIALIZATION
// ============================================================================

template <bool hasPhase> struct AUDIOLIB_nonInterleaveUpSample5<hasPhase, double> {
   static inline AUDIOLIB_STATUS execute(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
   {

      AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
      AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
      typedef double                                         dataType;
      typedef typename c7x::make_full_vector<dataType>::type vec;

      uint8_t *pBlock        = pKerPrivArgs->bufPblock;
      uint32_t nVecs         = pKerPrivArgs->nVecs;
      uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

      AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample5_exec_ci (Double) numZeroFillVecs %u nVecs %u\n",
                            pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

      dataType *restrict pInLocal  = (dataType *) pIn;
      dataType *restrict pOutLocal = (dataType *) pOut;

      vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

      AUDIOLIB_phaseZeroExecutor<hasPhase, double>::execute(pOut, phaseFillVecs, pBlock);
      dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

      typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;
      typedef upSampleRatio5_Mask<dataType>                                            mask;
      v_bool_tp vpMask0 = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel0());
      v_bool_tp vpMask1 = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel1());
      v_bool_tp vpMask2 = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel2());
      v_bool_tp vpMask3 = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel3());

      __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
      __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
      __SE0_OPEN(pInLocal, se0Params);
      __SA0_OPEN(sa0Params);

      for (uint32_t i = 0; i < nVecs; i++) {
         vec inVec = c7x::strm_eng<0, vec>::get_adv();

         vec permuteVec0 = __select(vpMask0, zeroVec, inVec);
         vec permuteVec1 = __select(vpMask1, zeroVec, inVec);
         vec permuteVec2 = __select(vpMask2, (zeroVec), (inVec));
         vec permuteVec3 = __select(vpMask3, zeroVec, inVec);

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred0, addr, permuteVec0);
         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred1, addr1, permuteVec1);
         __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred2, addr2, permuteVec2);
         __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred3, addr3, permuteVec3);
         __vpred vpred4 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr4  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred4, addr4, zeroVec);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
      return status;
   }
};

template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample5_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   return AUDIOLIB_nonInterleaveUpSample5<hasPhase, dataType>::execute(handle, pIn, pOut);
}
template AUDIOLIB_STATUS
AUDIOLIB_upSample5_exec_ci<true, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample5_exec_ci<false, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample5_exec_ci<true, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample5_exec_ci<false, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample6_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   uint8_t *pBlock        = pKerPrivArgs->bufPblock;
   uint32_t nVecs         = pKerPrivArgs->nVecs;
   uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample6_exec_ci (double) numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

   AUDIOLIB_phaseZeroExecutor<hasPhase, dataType>::execute(pOut, phaseFillVecs, pBlock);
   dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

   typedef upSampleRatio6_Mask<dataType> mask;
   uchar32                               vpMask0 = *stov_ptr(uchar32, (uint8_t *) mask::permute0());
   uchar32                               vpMask1 = *stov_ptr(uchar32, (uint8_t *) mask::permute1());

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < nVecs; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      vec permuteVec0 = c7x::reinterpret<vec>(__permute_low_low(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec1 = c7x::reinterpret<vec>(__permute_low_low(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec3 = c7x::reinterpret<vec>(__permute_high_high(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec4 = c7x::reinterpret<vec>(__permute_high_high(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));

      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred0, addr, permuteVec0);
      __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred1, addr1, permuteVec1);
      __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred2, addr2, zeroVec);
      __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred3, addr3, permuteVec3);
      __vpred vpred4 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr4  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred4, addr4, permuteVec4);
      __vpred vpred5 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr5  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred5, addr5, zeroVec);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_upSample6_exec_ci<true, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample6_exec_ci<false, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool hasPhase, typename dataType> struct AUDIOLIB_nonInterleaveUpSample7;
template <bool hasPhase> struct AUDIOLIB_nonInterleaveUpSample7<hasPhase, float> {
   static inline AUDIOLIB_STATUS execute(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
   {
      AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

      AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
      typedef float                                          dataType;
      typedef typename c7x::make_full_vector<dataType>::type vec;

      uint8_t *pBlock        = pKerPrivArgs->bufPblock;
      uint32_t nVecs         = pKerPrivArgs->nVecs;
      uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

      AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample7_exec_ci (Float) numZeroFillVecs %u nVecs %u\n",
                            pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

      dataType *restrict pInLocal  = (dataType *) pIn;
      dataType *restrict pOutLocal = (dataType *) pOut;

      vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

      AUDIOLIB_phaseZeroExecutor<hasPhase, float>::execute(pOut, phaseFillVecs, pBlock);
      dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

      typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;
      typedef upSampleRatio7_Mask<dataType>                                            mask;

      uchar32 vpMask0 = *stov_ptr(uchar32, (uint8_t *) mask::permute0());
      uchar32 vpMask1 = *stov_ptr(uchar32, (uint8_t *) mask::permute1());

      v_bool_tp sel2    = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel2());
      v_bool_tp sel3    = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel3());
      v_bool_tp sel4    = *stov_ptr(v_bool_tp, (uint8_t *) mask::sel4());
      uchar32   vpMask5 = *stov_ptr(uchar32, (uint8_t *) mask::permute5());
      uchar32   vpMask6 = *stov_ptr(uchar32, (uint8_t *) mask::permute6());

      __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
      __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
      __SE0_OPEN(pInLocal, se0Params);
      __SA0_OPEN(sa0Params);

      for (uint32_t i = 0; i < nVecs; i++) {
         vec inVec = c7x::strm_eng<0, vec>::get_adv();

         vec permuteVec0 =
             c7x::reinterpret<vec>(__permute_low_low(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec permuteVec1 =
             c7x::reinterpret<vec>(__permute_low_low(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec vTemp       = __select(sel2, zeroVec, inVec);
         vec permuteVec2 = c7x::reinterpret<vec>(__shift_left_full(__as_ulong4(vTemp), (uchar) 64));
         vec permuteVec3 = c7x::reinterpret<vec>(__select(sel3, zeroVec, inVec));
         vec vTemp1      = c7x::reinterpret<vec>(__select(sel4, zeroVec, inVec));
         vec permuteVec4 = c7x::reinterpret<vec>(__shift_right_full(__as_ulong4(vTemp1), (uchar) 64));
         vec permuteVec5 =
             c7x::reinterpret<vec>(__permute_high_high(vpMask5, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec permuteVec6 =
             c7x::reinterpret<vec>(__permute_high_high(vpMask6, __as_uchar32(zeroVec), __as_uchar32(inVec)));

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred0, addr0, permuteVec0);
         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred1, addr1, permuteVec1);
         __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred2, addr2, permuteVec2);
         __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred3, addr3, permuteVec3);
         __vpred vpred4 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr4  = c7x::strm_agen<0, vec>::get_adv(pOutData);

         __vstore_pred(vpred4, addr4, permuteVec4);
         __vpred vpred5 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr5  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred5, addr5, permuteVec5);
         __vpred vpred6 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr6  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred6, addr6, permuteVec6);
      }

      __SE0_CLOSE();
      __SA0_CLOSE();

      return status;
   }
};

template <bool hasPhase> struct AUDIOLIB_nonInterleaveUpSample7<hasPhase, double> {
   static inline AUDIOLIB_STATUS execute(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
   {
      AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

      AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
      typedef double                                         dataType;
      typedef typename c7x::make_full_vector<dataType>::type vec;

      uint8_t *pBlock        = pKerPrivArgs->bufPblock;
      uint32_t nVecs         = pKerPrivArgs->nVecs;
      uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

      AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample7_exec_ci (double) numZeroFillVecs %u nVecs %u\n",
                            pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

      dataType *restrict pInLocal  = (dataType *) pIn;
      dataType *restrict pOutLocal = (dataType *) pOut;

      vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

      AUDIOLIB_phaseZeroExecutor<hasPhase, double>::execute(pOut, phaseFillVecs, pBlock);
      dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

      typedef upSampleRatio7_Mask<dataType> mask;

      uchar32 vpMask0 = *stov_ptr(uchar32, (uint8_t *) mask::permute0());
      uchar32 vpMask1 = *stov_ptr(uchar32, (uint8_t *) mask::permute1());
      uchar32 vpMask2 = *stov_ptr(uchar32, (uint8_t *) mask::permute2());
      uchar32 vpMask3 = *stov_ptr(uchar32, (uint8_t *) mask::permute3());

      __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
      __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
      __SE0_OPEN(pInLocal, se0Params);
      __SA0_OPEN(sa0Params);

      for (uint32_t i = 0; i < nVecs; i++) {
         vec inVec = c7x::strm_eng<0, vec>::get_adv();

         vec permuteVec0 =
             c7x::reinterpret<vec>(__permute_low_low(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec permuteVec1 =
             c7x::reinterpret<vec>(__permute_low_low(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec permuteVec3 =
             c7x::reinterpret<vec>(__permute_high_high(vpMask2, __as_uchar32(zeroVec), __as_uchar32(inVec)));
         vec permuteVec4 =
             c7x::reinterpret<vec>(__permute_high_high(vpMask3, __as_uchar32(zeroVec), __as_uchar32(inVec)));

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred0, addr0, permuteVec0);
         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred1, addr1, permuteVec1);
         __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred2, addr2, zeroVec);
         __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred3, addr3, permuteVec3);
         __vpred vpred4 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr4  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred4, addr4, zeroVec);
         __vpred vpred5 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr5  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred5, addr5, permuteVec4);
         __vpred vpred6 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr6  = c7x::strm_agen<0, vec>::get_adv(pOutData);
         __vstore_pred(vpred6, addr6, zeroVec);
      }

      __SE0_CLOSE();
      __SA0_CLOSE();

      return status;
   }
};

template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample7_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   return AUDIOLIB_nonInterleaveUpSample7<hasPhase, dataType>::execute(handle, pIn, pOut);
}
template AUDIOLIB_STATUS
AUDIOLIB_upSample7_exec_ci<true, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample7_exec_ci<false, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample7_exec_ci<true, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample7_exec_ci<false, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   uint8_t *pBlock        = pKerPrivArgs->bufPblock;
   uint32_t nVecs         = pKerPrivArgs->nVecs;
   uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample8_exec_ci (Float) numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

   AUDIOLIB_phaseZeroExecutor<hasPhase, dataType>::execute(pOut, phaseFillVecs, pBlock);
   dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

   typedef upSampleRatio8_Mask<dataType> mask;
   uchar32                               vpMask0 = *stov_ptr(uchar32, (uint8_t *) mask::permute0());
   uchar32                               vpMask1 = *stov_ptr(uchar32, (uint8_t *) mask::permute1());

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < nVecs; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      vec permuteVec0 = c7x::reinterpret<vec>(__permute_low_low(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec1 = c7x::reinterpret<vec>(__permute_low_low(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec2 = c7x::reinterpret<vec>(__permute_high_high(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec3 = c7x::reinterpret<vec>(__permute_high_high(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));

      __vpred vp0   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp0, addr0, permuteVec0);
      __vpred vp1   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp1, addr1, zeroVec);
      __vpred vp2   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr2 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp2, addr2, permuteVec1);
      __vpred vp3   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr3 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp3, addr3, zeroVec);
      __vpred vp4   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr4 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp4, addr4, permuteVec2);
      __vpred vp5   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr5 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp5, addr5, zeroVec);
      __vpred vp6   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr6 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp6, addr6, permuteVec3);
      __vpred vp7   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr7 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp7, addr7, zeroVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_upSample8_exec_ci<true, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample8_exec_ci<false, double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample6x2_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   uint8_t *pBlock        = pKerPrivArgs->bufPblock;
   uint32_t nVecs         = pKerPrivArgs->nVecs;
   uint32_t phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample6x2_exec_ci (Float) numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   vec zeroVec = (vec) ((dataType) 0); /* broadcast 0 to every lane */

   AUDIOLIB_phaseZeroExecutor<hasPhase, dataType>::execute(pOut, phaseFillVecs, pBlock);
   dataType *restrict pOutData = pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

   typedef upSampleRatio6_Mask<dataType> mask;
   uchar32                               vpMask0 = *stov_ptr(uchar32, (uint8_t *) mask::permute0());
   uchar32                               vpMask1 = *stov_ptr(uchar32, (uint8_t *) mask::permute1());
   uchar32                               vpMask2 = *stov_ptr(uchar32, (uint8_t *) mask::permute2());

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < nVecs; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      vec permuteVec0 = c7x::reinterpret<vec>(__permute_low_low(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec1 = c7x::reinterpret<vec>(__permute_low_low(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec2 = c7x::reinterpret<vec>(__permute_low_low(vpMask2, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec3 = c7x::reinterpret<vec>(__permute_high_high(vpMask0, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec4 = c7x::reinterpret<vec>(__permute_high_high(vpMask1, __as_uchar32(zeroVec), __as_uchar32(inVec)));
      vec permuteVec5 = c7x::reinterpret<vec>(__permute_high_high(vpMask2, __as_uchar32(zeroVec), __as_uchar32(inVec)));

      __vpred vp0   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp0, addr0, permuteVec0);
      __vpred vp1   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp1, addr1, permuteVec1);
      __vpred vp2   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr2 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp2, addr2, permuteVec2);
      __vpred vp3   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr3 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp3, addr3, permuteVec3);
      __vpred vp4   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr4 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp4, addr4, permuteVec4);
      __vpred vp5   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr5 = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vp5, addr5, permuteVec5);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_upSample6x2_exec_ci<true, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample6x2_exec_ci<false, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample8X_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock        = pKerPrivArgs->bufPblock;
   uint32_t                                               nVecs         = pKerPrivArgs->nVecs;
   uint32_t                                               phaseFillVecs = pKerPrivArgs->phaseZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample8X_exec_ci (float) numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   AUDIOLIB_phaseZeroExecutor<hasPhase, dataType>::execute(pOut, phaseFillVecs, pBlock);
   dataType *restrict pOutData = (dataType *) pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

   __SE_TEMPLATE_v1 seParms = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 saOut   = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE0_OPEN(pInLocal, seParms);
   __SA0_OPEN(saOut);

   for (uint32_t i = 0; i < nVecs; i++) {
      vec     inVec  = c7x::strm_eng<0, vec>::get_adv();
      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred0, addr0, inVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_upSample8X_exec_ci<true, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_upSample8X_exec_ci<false, float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool hasPhase, typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_upSampleNonInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock          = pKerPrivArgs->bufPblock;
   uint32_t                                               nVecs           = pKerPrivArgs->nVecs;
   uint32_t                                               numZeroFillVecs = pKerPrivArgs->numZeroFillVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSampleNonInterleave_exec_ci numZeroFillVecs %u nVecs %u\n",
                         pKerPrivArgs->numZeroFillVecs, pKerPrivArgs->nVecs);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   vec zeroVec                 = (vec) ((dataType) 0); /* broadcast 0 to every lane */
   dataType *restrict pOutData = (dataType *) pOutLocal + (int32_t) pKerPrivArgs->phaseOffset;

   /**********************************************************************/
   /* Zero-fill (common): the target is the OUTPUT for interleaved and    */
   /* the SCRATCH for planar (init_ci sized the SA template and nVecs to   */
   /* match the chosen target).                                          */
   /**********************************************************************/
   __SA_TEMPLATE_v1 saZero     = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   dataType *restrict zeroBase = pOutLocal;

   __SA0_OPEN(saZero);
   for (uint32_t i = 0U; i < numZeroFillVecs; i++) {
      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr   = c7x::strm_agen<0, vec>::get_adv(zeroBase);
      __vstore_pred(vpred0, addr, zeroVec);
   }
   __SA0_CLOSE();

   __SE_TEMPLATE_v1 seParms = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 saOut   = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   __SE0_OPEN(pInLocal, seParms);
   __SA0_OPEN(saOut);

   for (uint32_t i = 0U; i < nVecs; i++) {
      vec     v      = c7x::strm_eng<0, vec>::get_adv();
      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutData);
      __vstore_pred(vpred0, addr, v);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_upSampleNonInterleave_exec_ci<true, float>(AUDIOLIB_kernelHandle handle,
                                                                             void *restrict pIn,
                                                                             void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_upSampleNonInterleave_exec_ci<true, double>(AUDIOLIB_kernelHandle handle,
                                                                              void *restrict pIn,
                                                                              void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_upSampleNonInterleave_exec_ci<false, float>(AUDIOLIB_kernelHandle handle,
                                                                              void *restrict pIn,
                                                                              void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_upSampleNonInterleave_exec_ci<false, double>(AUDIOLIB_kernelHandle handle,
                                                                               void *restrict pIn,
                                                                               void *restrict pOut);

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample_ci.cpp                                   */
/* ======================================================================== */
