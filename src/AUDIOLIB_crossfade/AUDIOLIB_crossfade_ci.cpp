// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_crossfade_priv.h"
// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/

void AUDIOLIB_crossfade_perfEst(AUDIOLIB_kernelHandle handle,
                                uint64_t             *archCycles,
                                uint64_t             *estCycles,
                                uint32_t              data_type)
{
   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;
   uint32_t                     samples      = pKerPrivArgs->samples;
   uint32_t                     channels     = pKerPrivArgs->channels;
   uint32_t                     nVecs        = pKerPrivArgs->nVecs;

   uint32_t crossfadeStartupCycles   = 8;
   uint32_t crossfadeOperationCycles = 0;
   uint32_t crossfadeTearDownCycles  = 0;
   uint32_t crossfadeOverheadCycles  = 0;

   crossfadeStartupCycles = 21;
   if (pKerPrivArgs->isInterleave == 1) {
      if (data_type == AUDIOLIB_FLOAT32) {
         crossfadeOperationCycles = (7 + (7 + nVecs * 1)) * samples;
      }
      else {
         crossfadeOperationCycles = (7 + (7 + nVecs * 2)) * samples;
      }
   }
   else {
      if (data_type == AUDIOLIB_FLOAT32) {
         crossfadeOperationCycles = (4 + (7 + channels * 1)) * nVecs;
      }
      else {
         crossfadeOperationCycles = (4 + (7 + channels * 2)) * nVecs;
      }
   }
   crossfadeTearDownCycles = 6;

   *archCycles = crossfadeOperationCycles;
   *estCycles  = crossfadeStartupCycles + crossfadeOperationCycles + crossfadeOverheadCycles + crossfadeTearDownCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_crossfade_init_ci(AUDIOLIB_kernelHandle              handle,
                                           const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                           const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                           const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                           const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                           const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                           const AUDIOLIB_crossfade_InitArgs *pKerInitArgs)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS              status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;
   uint8_t                     *pBlock       = pKerPrivArgs->bufPblock;

   int32_t eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

   se0Params.ICNT0         = eleCount;
   se0Params.ICNT1         = pKerPrivArgs->channels;
   se0Params.DIM1          = pKerPrivArgs->strideInElements;
   se0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   se0Params.DIM2          = eleCount;
   se0Params.ELETYPE       = SE_ELETYPE;
   se0Params.VECLEN        = SE_VECLEN;
   se0Params.DIMFMT        = __SE_DIMFMT_3D;
   se0Params.DECDIM1_WIDTH = pKerPrivArgs->samples;
   se0Params.DECDIM1       = __SE_DECDIM_DIM2;

   sa0Params.ICNT0  = pKerPrivArgs->samples;
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_1D;

   sa1Params.ICNT0         = eleCount;
   sa1Params.ICNT1         = pKerPrivArgs->channels;
   sa1Params.DIM1          = pKerPrivArgs->strideOutElements;
   sa1Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   sa1Params.DIM2          = eleCount;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;
   sa1Params.DECDIM1_WIDTH = pKerPrivArgs->samples;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM2;

   pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_init_ci(AUDIOLIB_kernelHandle              handle,
                                                     const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                                     const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                                     const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                                     const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                                     const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                                     const AUDIOLIB_crossfade_InitArgs *pKerInitArgs)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS              status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;
   uint8_t                     *pBlock       = pKerPrivArgs->bufPblock;

   int32_t eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

   se0Params.ICNT0   = pKerPrivArgs->channels;
   se0Params.DIM1    = pKerPrivArgs->strideInElements;
   se0Params.ICNT1   = pKerPrivArgs->samples;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_2D;

   sa0Params.ICNT0  = 1;
   sa0Params.DIM1   = 1;
   sa0Params.ICNT1  = pKerPrivArgs->samples;
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_2D;

   sa1Params.ICNT0  = pKerPrivArgs->channels;
   sa1Params.DIM1   = pKerPrivArgs->strideOutElements;
   sa1Params.ICNT1  = pKerPrivArgs->samples;
   sa1Params.VECLEN = SA_VECLEN;
   sa1Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(pKerPrivArgs->channels, eleCount);

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_crossfade_init_ci<float>(AUDIOLIB_kernelHandle              handle,
                                                           const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                                           const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                                           const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                                           const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                                           const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                                           const AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_crossfade_init_ci<double>(AUDIOLIB_kernelHandle              handle,
                                                            const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                                            const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                                            const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                                            const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                                            const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                                            const AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_init_ci<float>(AUDIOLIB_kernelHandle              handle,
                                                                     const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                                                     const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                                                     const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                                                     const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                                                     const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                                                     const AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_init_ci<double>(AUDIOLIB_kernelHandle              handle,
                                                                      const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                                                      const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                                                      const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                                                      const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                                                      const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                                                      const AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_ci(AUDIOLIB_kernelHandle handle,
                                           void *restrict pIn0,
                                           void *restrict pIn1,
                                           void *restrict pIn2,
                                           void *restrict pIn3,
                                           void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS              status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;
   uint8_t                     *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                     nVecs        = pKerPrivArgs->nVecs;
   uint32_t                     channels     = pKerPrivArgs->channels;

   dataType *restrict pInLocalPrev = (dataType *) pIn0;
   dataType *restrict pInLocalNext = (dataType *) pIn1;
   dataType *restrict pGainCos     = (dataType *) pIn2;
   dataType *restrict pGainSine    = (dataType *) pIn3;
   dataType *restrict pOutLocalOut = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   __SE0_OPEN(pInLocalPrev, se0Params);
   __SE1_OPEN(pInLocalNext, se0Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa0Params);
   __SA2_OPEN(sa1Params);

   for (uint32_t i = 0; i < nVecs; i++) {

      __vpred sinePred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *pSineVec = c7x::strm_agen<0, vec>::get_adv(pGainSine);
      vec     sineGain = __vload_pred(sinePred, pSineVec);

      __vpred cosPred = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pCosVec = c7x::strm_agen<1, vec>::get_adv(pGainCos);
      vec     cosGain = __vload_pred(cosPred, pCosVec);

      for (uint32_t j = 0; j < channels; j++) {

         vec prevInput = c7x::strm_eng<0, vec>::get_adv();
         vec nextInput = c7x::strm_eng<1, vec>::get_adv();

         vec outVec = prevInput * cosGain + nextInput * sineGain;

         __vpred outPred = c7x::strm_agen<2, vec>::get_vpred();
         vec    *outPtr  = c7x::strm_agen<2, vec>::get_adv(pOutLocalOut);
         __vstore_pred(outPred, outPtr, outVec);
      }
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pIn0,
                                                     void *restrict pIn1,
                                                     void *restrict pIn2,
                                                     void *restrict pIn3,
                                                     void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS              status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;
   uint8_t                     *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                     nVecs        = pKerPrivArgs->nVecs;
   uint32_t                     samples      = pKerPrivArgs->samples;

   dataType *restrict pInLocalPrev = (dataType *) pIn0;
   dataType *restrict pInLocalNext = (dataType *) pIn1;
   dataType *restrict pGainCos     = (dataType *) pIn2;
   dataType *restrict pGainSine    = (dataType *) pIn3;
   dataType *restrict pOutLocalOut = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   __SE0_OPEN(pInLocalPrev, se0Params);
   __SE1_OPEN(pInLocalNext, se0Params);

   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa0Params);
   __SA2_OPEN(sa1Params);

   for (uint32_t i = 0; i < samples; i++) {

      __vpred   sinePred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *pSineVec = c7x::strm_agen<0, dataType>::get_adv(pGainSine);
      dataType  sineVal  = __vload_pred(sinePred, pSineVec);

      __vpred   cosPred = c7x::strm_agen<1, dataType>::get_vpred();
      dataType *pCosVec = c7x::strm_agen<1, dataType>::get_adv(pGainCos);
      dataType  coseVal = __vload_pred(cosPred, pCosVec);

      vec sineGainVec = (vec) sineVal;
      vec coseGainVec = (vec) coseVal;

      for (uint32_t j = 0; j < nVecs; j++) {

         vec prevInput = c7x::strm_eng<0, vec>::get_adv();
         vec nextInput = c7x::strm_eng<1, vec>::get_adv();

         vec outVec = prevInput * coseGainVec + nextInput * sineGainVec;

         __vpred outPred = c7x::strm_agen<2, vec>::get_vpred();
         vec    *outPtr  = c7x::strm_agen<2, vec>::get_adv(pOutLocalOut);
         __vstore_pred(outPred, outPtr, outVec);
      }
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                           void *restrict pIn0,
                                                           void *restrict pIn1,
                                                           void *restrict pIn2,
                                                           void *restrict pIn3,
                                                           void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                            void *restrict pIn0,
                                                            void *restrict pIn1,
                                                            void *restrict pIn2,
                                                            void *restrict pIn3,
                                                            void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                     void *restrict pIn0,
                                                                     void *restrict pIn1,
                                                                     void *restrict pIn2,
                                                                     void *restrict pIn3,
                                                                     void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                      void *restrict pIn0,
                                                                      void *restrict pIn1,
                                                                      void *restrict pIn2,
                                                                      void *restrict pIn3,
                                                                      void *restrict pOut);
