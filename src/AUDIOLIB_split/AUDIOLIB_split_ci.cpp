// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#include "AUDIOLIB_split_priv.h"
#include <cstdio>

void AUDIOLIB_split_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_split_PrivArgs *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;

   uint64_t splitStartupCycles   = 26;
   uint64_t splitTeardownCycles  = 0;
   uint64_t splitOperationCycles = 0;
   uint64_t splitOverheadCycles  = 0;
   uint32_t iterationCount       = pKerPrivArgs->iterCount;

   for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
      splitOperationCycles += 10 + 1 + iterationCount * 1;
      splitTeardownCycles += 9;
   }
   splitTeardownCycles += 1;

   splitOverheadCycles += splitStartupCycles + splitTeardownCycles;
   *estCycles  = splitOperationCycles + splitOverheadCycles;
   *archCycles = splitOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_split_init_ci(AUDIOLIB_kernelHandle          handle,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                       const AUDIOLIB_split_InitArgs *pKerInitArgs)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_split_PrivArgs                               *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_split_init_ci - 3D SE Config\n");

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   __SE_TEMPLATE_v1 se0Params;
   __SA_TEMPLATE_v1 sa0Params;

   uint32_t eleCount = c7x::element_count_of<vec>::value;
   se0Params         = __gen_SE_TEMPLATE_v1();
   sa0Params         = __gen_SA_TEMPLATE_v1();
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;

   // INTERLEAVED: input layout is [sample][channel], use 3D SE to skip across outputs
   if (pKerPrivArgs->isInputInterleave) {

      pKerPrivArgs->iterCount =
          AUDIOLIB_ceilingDiv(pKerPrivArgs->numOutputChannels, eleCount) * pKerPrivArgs->numInputSamples;

      // SE0: read numOutputChannels per sample, advance by strideIn, skip via DIM2
      se0Params.ICNT0  = pKerPrivArgs->numOutputChannels;
      se0Params.ICNT1  = pKerPrivArgs->numInputSamples;
      se0Params.ICNT2  = pKerPrivArgs->numOutputs;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.DIM2   = pKerPrivArgs->numOutputChannels;
      se0Params.DIMFMT = __SE_DIMFMT_3D;

      // SA0: write numOutputChannels per sample into each output buffer
      sa0Params.ICNT0  = pKerPrivArgs->numOutputChannels;
      sa0Params.ICNT1  = pKerPrivArgs->numInputSamples;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;
   }

   else {
      // DEINTERLEAVED: input layout is [channel][sample], 2D SE reads channel-by-channel
      pKerPrivArgs->iterCount =
          AUDIOLIB_ceilingDiv(pKerPrivArgs->numInputSamples, eleCount) * pKerPrivArgs->numOutputChannels;

      // SE0: read numInputSamples per channel row
      se0Params.ICNT0  = pKerPrivArgs->numInputSamples;
      se0Params.ICNT1  = pKerPrivArgs->numInputChannels;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      // SA0: write numInputSamples per channel into each output
      sa0Params.ICNT0  = pKerPrivArgs->numInputSamples;
      sa0Params.ICNT1  = pKerPrivArgs->numOutputChannels;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;
   }

   // Store SE/SA templates into pBlock for use by exec_ci
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_split_init_ci<float>(AUDIOLIB_kernelHandle          handle,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                       const AUDIOLIB_split_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_split_init_ci<double>(AUDIOLIB_kernelHandle          handle,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                        const AUDIOLIB_split_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_split_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_split_PrivArgs                               *pKerPrivArgs   = (AUDIOLIB_split_PrivArgs *) handle;
   AUDIOLIB_STATUS                                        status         = AUDIOLIB_SUCCESS;
   uint8_t                                               *pBlock         = pKerPrivArgs->bufPblock;
   uint32_t                                               iterationCount = pKerPrivArgs->iterCount;
   dataType *restrict pInLocal                                           = (dataType *) pIn;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_splitGeneric_exec_ci\n");

   // Retrieve pre-computed SE/SA templates from pBlock
   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   // Open SE0 on the full input buffer; it walks across output groups automatically
   __SE0_OPEN(pInLocal, se0Params);
   for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
      dataType *pOutLocalI = (dataType *) pOut[i];
      __SA0_OPEN(sa0Params);
      // 1 + iterationCount * 1
      for (uint32_t j = 0; j < iterationCount; j++) {
         vec     inputSe0  = c7x::strm_eng<0, vec>::get_adv();
         __vpred predSa0   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocalI);

         __vstore_pred(predSa0, outPtrSa0, inputSe0);
      }
      __SA0_CLOSE();
   }

   __SE0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_split_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_split_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);
