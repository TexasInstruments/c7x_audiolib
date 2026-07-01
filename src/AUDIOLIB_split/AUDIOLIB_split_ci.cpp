// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_split_priv.h"
#include <cstdio>

void AUDIOLIB_split_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_split_PrivArgs *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;
   uint8_t                 *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t *restrict pIterCountLocal    = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   uint64_t splitStartupCycles   = 0;
   uint64_t splitTeardownCycles  = 0;
   uint64_t splitOperationCycles = 0;
   uint64_t splitOverheadCycles  = 0;

   // Planar, or interleaved with uniform channels, load the whole input with a single SE
   // opened once (per-output SA). Only interleaved with differing channels opens an SE per output.
   bool singleLoad = (!pKerPrivArgs->isInputInterleave) || (pKerPrivArgs->outChannelsUniform != 0);
   if (singleLoad) {
      splitStartupCycles = 27; // single SE open
      for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
         uint32_t iterationCount = pIterCountLocal[i];
         splitOperationCycles += 14 + 1 + iterationCount * 1;
         splitTeardownCycles += 9; // SA close per output
      }
      splitTeardownCycles += 1; // SE close
   }
   else {
      // Interleaved with differing channels: both SE and SA are opened per output.
      splitStartupCycles = 7;
      for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
         uint32_t iterationCount = pIterCountLocal[i];
         splitOperationCycles += 27 + 1 + iterationCount * 1;
         splitTeardownCycles += 9;
      }
      splitTeardownCycles += 1;
   }

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

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_split_init_ci\n");

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   uint32_t eleCount = c7x::element_count_of<vec>::value;

   __SA_TEMPLATE_v1 sa0Params[pKerPrivArgs->numOutputs];
   __SA_TEMPLATE_v1 *restrict pSa0Params = sa0Params;

   uint32_t *restrict pInOffsetLocal  = (uint32_t *) ((uint8_t *) pBlock + SE_INOFFSET_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   if (!pKerPrivArgs->isInputInterleave) {
      // Deinterleaved (planar) input: one SE streams the whole contiguous input; the SA is
      // opened per output. The SE advances continuously across outputs.
      __SE_TEMPLATE_v1 seSingle = __gen_SE_TEMPLATE_v1();
      seSingle.ELETYPE          = SE_ELETYPE;
      seSingle.VECLEN           = SE_VECLEN;
      seSingle.DIMFMT           = __SE_DIMFMT_2D;
      seSingle.ICNT0            = pKerPrivArgs->numInputSamples;
      seSingle.ICNT1            = pKerPrivArgs->numInputChannels;
      seSingle.DIM1             = pKerPrivArgs->strideIn;

      for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
         uint32_t outCh = pKerPrivArgs->outChannels[i];

         pSa0Params[i]        = __gen_SA_TEMPLATE_v1();
         pSa0Params[i].VECLEN = SA_VECLEN;
         pSa0Params[i].DIMFMT = __SA_DIMFMT_2D;
         pSa0Params[i].ICNT0  = pKerPrivArgs->numInputSamples;
         pSa0Params[i].ICNT1  = outCh;
         pSa0Params[i].DIM1   = pKerPrivArgs->strideOut[i];

         pIterCountLocal[i] = (AUDIOLIB_ceilingDiv(pKerPrivArgs->numInputSamples, eleCount)) * outCh;
      }

      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_SINGLE_PARAM_OFFSET) = seSingle;
      memcpy((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET, sa0Params, sizeof(sa0Params));
   }
   else if (pKerPrivArgs->outChannelsUniform) {
      // Interleaved input, all outputs share one channel count C: a single 3D SE folds the
      // output dimension (DIM2 = C, ICNT2 = numOutputs), so it is opened once and advanced
      // across outputs. One 2D SA per output drains it.
      uint32_t channelsPerOutput = pKerPrivArgs->outChannels[0];
      __SE_TEMPLATE_v1 seSingle = __gen_SE_TEMPLATE_v1();
      seSingle.ELETYPE          = SE_ELETYPE;
      seSingle.VECLEN           = SE_VECLEN;
      seSingle.DIMFMT           = __SE_DIMFMT_3D;
      seSingle.ICNT0            = channelsPerOutput;
      seSingle.ICNT1            = pKerPrivArgs->numInputSamples;
      seSingle.ICNT2            = pKerPrivArgs->numOutputs;
      seSingle.DIM1             = pKerPrivArgs->strideIn;
      seSingle.DIM2             = channelsPerOutput;

      for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
         pSa0Params[i]        = __gen_SA_TEMPLATE_v1();
         pSa0Params[i].VECLEN = SA_VECLEN;
         pSa0Params[i].DIMFMT = __SA_DIMFMT_2D;
         pSa0Params[i].DIM1   = pKerPrivArgs->strideOut[i];
         pSa0Params[i].ICNT0  = channelsPerOutput;
         pSa0Params[i].ICNT1  = pKerPrivArgs->numInputSamples;

         pIterCountLocal[i] = (AUDIOLIB_ceilingDiv(channelsPerOutput, eleCount)) * pKerPrivArgs->numInputSamples;
      }

      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_SINGLE_PARAM_OFFSET) = seSingle;
      memcpy((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET, sa0Params, sizeof(sa0Params));
   }
   else {
      // Interleaved input with differing channel counts: each output reads a non-contiguous
      // channel window, so one SE and one SA template are built per output, with a per-output
      // input offset.
      __SE_TEMPLATE_v1 se0Params[pKerPrivArgs->numOutputs];
      __SE_TEMPLATE_v1 *restrict pSe0Params = se0Params;

      uint32_t cumulativeOffset = 0; // cumulative input channel offset (in channels)

      for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
         uint32_t outCh = pKerPrivArgs->outChannels[i];

         pSe0Params[i]         = __gen_SE_TEMPLATE_v1();
         pSe0Params[i].ELETYPE = SE_ELETYPE;
         pSe0Params[i].VECLEN  = SE_VECLEN;
         pSe0Params[i].DIMFMT  = __SE_DIMFMT_2D;
         pSe0Params[i].DIM1    = pKerPrivArgs->strideIn;
         pSe0Params[i].ICNT0   = outCh;
         pSe0Params[i].ICNT1   = pKerPrivArgs->numInputSamples;

         pSa0Params[i]        = __gen_SA_TEMPLATE_v1();
         pSa0Params[i].VECLEN = SA_VECLEN;
         pSa0Params[i].DIMFMT = __SA_DIMFMT_2D;
         pSa0Params[i].DIM1   = pKerPrivArgs->strideOut[i];
         pSa0Params[i].ICNT0  = outCh;
         pSa0Params[i].ICNT1  = pKerPrivArgs->numInputSamples;

         pInOffsetLocal[i]  = cumulativeOffset; // channel offset within each input sample row
         pIterCountLocal[i] = (AUDIOLIB_ceilingDiv(outCh, eleCount)) * pKerPrivArgs->numInputSamples;

         cumulativeOffset += outCh;
      }

      memcpy((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET, se0Params, sizeof(se0Params));
      memcpy((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET, sa0Params, sizeof(sa0Params));
   }

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
AUDIOLIB_STATUS
AUDIOLIB_split_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_split_PrivArgs                               *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_split_exec_ci\n");

   __SE_TEMPLATE_v1 se0Params           = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_SINGLE_PARAM_OFFSET);
   __SA_TEMPLATE_v1 *restrict sa0Params = (__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal   = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   // Open the SE once on the whole contiguous input; it advances across all outputs.
   __SE0_OPEN(pInLocal, se0Params);
   for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
      dataType *restrict pOutLocalI = (dataType *) pOut[i];
      uint32_t iterationCount       = pIterCountLocal[i];

      __SA0_OPEN(sa0Params[i]);
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

template AUDIOLIB_STATUS AUDIOLIB_split_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                 void *restrict pIn,
                                                                                 void **restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_split_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                                  void *restrict pIn,
                                                                                  void **restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_splitPerOutput_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_split_PrivArgs                               *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_splitPerOutput_exec_ci\n");

   __SE_TEMPLATE_v1 *restrict se0Params = (__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 *restrict sa0Params = (__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   uint32_t *restrict pInOffsetLocal    = (uint32_t *) ((uint8_t *) pBlock + SE_INOFFSET_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal   = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
      dataType *restrict pInLocalI  = pInLocal + pInOffsetLocal[i];
      dataType *restrict pOutLocalI = (dataType *) pOut[i];
      uint32_t iterationCount       = pIterCountLocal[i];

      __SE0_OPEN(pInLocalI, se0Params[i]);
      __SA0_OPEN(sa0Params[i]);
      for (uint32_t j = 0; j < iterationCount; j++) {
         vec     inputSe0  = c7x::strm_eng<0, vec>::get_adv();
         __vpred predSa0   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocalI);

         __vstore_pred(predSa0, outPtrSa0, inputSe0);
      }
      __SA0_CLOSE();
      __SE0_CLOSE();
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_splitPerOutput_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_splitPerOutput_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);
