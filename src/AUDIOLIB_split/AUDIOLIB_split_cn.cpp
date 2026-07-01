// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_split_priv.h"
#include <cstdint>
#include <cstdio>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_split_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut)
{
   AUDIOLIB_split_PrivArgs *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;
   dataType *restrict pInLocal           = (dataType *) pIn;

   uint32_t inChannelOffset = 0;
   uint32_t numSamples      = pKerPrivArgs->numInputSamples;
   uint32_t strideIn        = pKerPrivArgs->strideIn;

   for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
      dataType *restrict pOutLocal = (dataType *) pOut[i];
      uint32_t numChannels         = pKerPrivArgs->outChannels[i];
      uint32_t strideOut           = pKerPrivArgs->strideOut[i];
      if (pKerPrivArgs->isInputInterleave) {
         for (uint32_t s = 0; s < numSamples; s++) {
            for (uint32_t c = 0; c < numChannels; c++) {
               pOutLocal[s * strideOut + c] = pInLocal[s * strideIn + inChannelOffset + c];
            }
         }
      }
      else {
         for (uint32_t c = 0; c < numChannels; c++) {
            for (uint32_t s = 0; s < numSamples; s++) {
               pOutLocal[c * strideOut + s] = pInLocal[(inChannelOffset + c) * strideIn + s];
            }
         }
      }
      inChannelOffset += numChannels;
   }
   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS
AUDIOLIB_split_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_split_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);
