// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_concat_priv.h"

void AUDIOLIB_concat_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_concat_PrivArgs *pKerPrivArgs    = (AUDIOLIB_concat_PrivArgs *) handle;
   uint8_t                  *pBlock          = pKerPrivArgs->bufPblock;
   uint32_t *restrict pIterCountLocal        = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   uint64_t concatStartupCycles   = 0;
   uint64_t concatTeardownCycles  = 0;
   uint64_t concatOperationCycles = 0;
   uint64_t concatOverheadCycles  = 0;

   // Planar, or interleaved with uniform channels, store the whole output with a single SA
   // opened once (per-input SE). Only interleaved with differing channels opens an SA per input.
   bool singleStore = (!pKerPrivArgs->isInterleave) || (pKerPrivArgs->inChannelsUniform != 0);
   if (singleStore) {
      concatStartupCycles = 13;
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         uint32_t iterationCount = pIterCountLocal[i];
         concatOperationCycles += 29 + 1 + iterationCount * 1;
         concatTeardownCycles += 2 + 1; // SE close per input
      }
      concatTeardownCycles += 1; // SA close
   }
   else {
      concatStartupCycles = 9;
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         uint32_t iterationCount = pIterCountLocal[i];
         concatOperationCycles += 29 + 1 + iterationCount * 1;
         concatTeardownCycles += 2;
      }
      concatTeardownCycles += 1;
   }

   concatOverheadCycles += concatStartupCycles + concatTeardownCycles;
   *estCycles  = concatOperationCycles + concatOverheadCycles;
   *archCycles = concatOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_concat_init_ci(AUDIOLIB_kernelHandle           handle,
                                        const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                        const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                        const AUDIOLIB_concat_InitArgs *pKerInitArgs)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_concat_PrivArgs                              *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount     = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concat_init_ci \n");

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se0Params[pKerPrivArgs->numInputs];
   __SA_TEMPLATE_v1 sa1Params[pKerPrivArgs->numInputs];

   __SE_TEMPLATE_v1 *restrict pSe0Params = se0Params;
   __SA_TEMPLATE_v1 *restrict pSa1Params = sa1Params;

   uint32_t *restrict pOutOffsetLocal = (uint32_t *) ((uint8_t *) pBlock + SE_OUTOFFSET_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   if (!pKerInitArgs->isInterleave) {
      // Non-interleaved (planar) input and output: one 2D SE per input, single 2D SA.
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         pSe0Params[i]         = __gen_SE_TEMPLATE_v1();
         pSe0Params[i].ICNT0   = pKerPrivArgs->inSamples;
         pSe0Params[i].ICNT1   = pKerPrivArgs->inChannels[i];
         pSe0Params[i].DIM1    = pKerPrivArgs->strideIn[i];
         pSe0Params[i].DIMFMT  = __SE_DIMFMT_2D;
         pSe0Params[i].ELETYPE = SE_ELETYPE;
         pSe0Params[i].VECLEN  = SE_VECLEN;

         pIterCountLocal[i] = (AUDIOLIB_ceilingDiv(pKerPrivArgs->inSamples, eleCount)) * pKerPrivArgs->inChannels[i];
      }

      sa0Params.ICNT0  = pKerPrivArgs->inSamples;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->totalInChannels;
      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;
   }
   else if (pKerPrivArgs->inChannelsUniform) {
      // Interleaved input and output, all inputs share one channel count C: a single 3D SA
      // folds the input dimension (DIM2 = C, ICNT2 = numInputs), so it is opened once and
      // advanced across inputs. One 2D SE per input feeds it.
      uint32_t channelsPerInput = pKerPrivArgs->inChannels[0];
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         pSe0Params[i]         = __gen_SE_TEMPLATE_v1();
         pSe0Params[i].ICNT0   = channelsPerInput;
         pSe0Params[i].ICNT1   = pKerPrivArgs->inSamples;
         pSe0Params[i].DIM1    = pKerPrivArgs->strideIn[i];
         pSe0Params[i].ELETYPE = SE_ELETYPE;
         pSe0Params[i].VECLEN  = SE_VECLEN;
         pSe0Params[i].DIMFMT  = __SE_DIMFMT_2D;

         pIterCountLocal[i] = (AUDIOLIB_ceilingDiv(channelsPerInput, eleCount)) * pKerPrivArgs->inSamples;
      }

      sa0Params.ICNT0  = channelsPerInput;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->inSamples;
      sa0Params.DIM2   = channelsPerInput;
      sa0Params.ICNT2  = pKerPrivArgs->numInputs;
      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.DIMFMT = __SA_DIMFMT_3D;
   }
   else {
      // Interleaved input and output with differing channel counts: one SE and one SA template
      // per input, with each input written at its cumulative channel offset in the output.
      uint32_t cumulativeOffset = 0;
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         pSe0Params[i]         = __gen_SE_TEMPLATE_v1();
         pSe0Params[i].ICNT0   = pKerPrivArgs->inChannels[i];
         pSe0Params[i].ICNT1   = pKerPrivArgs->inSamples;
         pSe0Params[i].DIM1    = pKerPrivArgs->strideIn[i];
         pSe0Params[i].ELETYPE = SE_ELETYPE;
         pSe0Params[i].VECLEN  = SE_VECLEN;
         pSe0Params[i].DIMFMT  = __SE_DIMFMT_2D;

         pSa1Params[i]        = __gen_SA_TEMPLATE_v1();
         pSa1Params[i].ICNT0  = pKerPrivArgs->inChannels[i];
         pSa1Params[i].DIM1   = pKerPrivArgs->strideOut;
         pSa1Params[i].ICNT1  = pKerPrivArgs->inSamples;
         pSa1Params[i].DIM2   = eleCount;
         pSa1Params[i].ICNT2  = AUDIOLIB_ceilingDiv(pKerPrivArgs->totalInChannels, eleCount);
         pSa1Params[i].VECLEN = SA_VECLEN;
         pSa1Params[i].DIMFMT = __SA_DIMFMT_3D;

         pOutOffsetLocal[i] = cumulativeOffset;
         cumulativeOffset += pKerPrivArgs->inChannels[i];
         pIterCountLocal[i] = (AUDIOLIB_ceilingDiv(pKerPrivArgs->inChannels[i], eleCount)) * pKerPrivArgs->inSamples;
      }
   }

   memcpy((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET, se0Params, sizeof(se0Params));
   memcpy((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET, sa1Params, sizeof(sa1Params));

   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_concat_init_ci<float>(AUDIOLIB_kernelHandle           handle,
                                                        const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                                        const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                                        const AUDIOLIB_concat_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_concat_init_ci<double>(AUDIOLIB_kernelHandle           handle,
                                                         const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                                         const AUDIOLIB_concat_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_concat_exec_ci(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_concat_PrivArgs                              *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;

   dataType *restrict pOutLocal = (dataType *) pOut;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concat_exec_ci");

   __SE_TEMPLATE_v1 *restrict se0Params = (__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params           = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal   = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   __SA0_OPEN(sa0Params);
   for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {

      dataType *restrict pInLocal = (dataType *) pIn[i];
      uint32_t iterationCount     = pIterCountLocal[i];

      __SE0_OPEN(pInLocal, se0Params[i]);
      for (uint32_t j = 0; j < iterationCount; j++) {
         vec inputSe0 = c7x::strm_eng<0, vec>::get_adv();

         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, inputSe0);
      }
      __SE0_CLOSE();
   }
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_concat_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                  void **restrict pIn,
                                                                                  void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_concat_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                                   void **restrict pIn,
                                                                                   void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_concatPerInputStore_exec_ci(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_concat_PrivArgs                              *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;

   dataType *restrict pOutLocal  = (dataType *) pOut;
   dataType *restrict pOutLocal1 = (dataType *) pOut;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concatPerInputStore_exec_ci");

   __SE_TEMPLATE_v1 *restrict se0Params = (__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 *restrict sa1Params = (__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   uint32_t *restrict pOutOffsetLocal   = (uint32_t *) ((uint8_t *) pBlock + SE_OUTOFFSET_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal   = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
      dataType *restrict pInLocal = (dataType *) pIn[i];
      uint32_t offset             = pOutOffsetLocal[i];
      pOutLocal1                  = pOutLocal + offset;
      uint32_t iterationCount     = pIterCountLocal[i];

      __SA0_OPEN(sa1Params[i]);
      __SE0_OPEN(pInLocal, se0Params[i]);
      for (uint32_t j = 0; j < iterationCount; j++) {
         vec inputSe0 = c7x::strm_eng<0, vec>::get_adv();

         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal1);
         __vstore_pred(tmpSa0, outPtrSa0, inputSe0);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_concatPerInputStore_exec_ci<float>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_concatPerInputStore_exec_ci<double>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);
