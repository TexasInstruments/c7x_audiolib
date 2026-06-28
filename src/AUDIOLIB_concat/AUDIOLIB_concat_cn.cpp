// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_concat_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_concat_exec_cn(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_concat_PrivArgs *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_concat_exec_cn");

   dataType *restrict pOutLocal = (dataType *) pOut;
   uint32_t outChannelOffset    = 0;
   uint32_t numSamples          = pKerPrivArgs->inSamples;
   uint32_t numChannels         = pKerPrivArgs->inChannels;
   uint32_t strideIn            = pKerPrivArgs->strideIn;

   for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {

      dataType *restrict pInLocal = (dataType *) pIn[i];

      if (pKerPrivArgs->isInterleave) {
         // Interleave: samples in dim_y, channels in dim_x
         for (uint32_t s = 0; s < numSamples; s++) {
            for (uint32_t c = 0; c < numChannels; c++) {
               pOutLocal[s * pKerPrivArgs->strideOut + outChannelOffset + c] = pInLocal[s * strideIn + c];
            }
         }
      }
      else {
         // Non-interleave: samples in dim_x, channels in dim_y
         for (uint32_t c = 0; c < numChannels; c++) {
            for (uint32_t s = 0; s < numSamples; s++) {
               pOutLocal[(outChannelOffset + c) * pKerPrivArgs->strideOut + s] = pInLocal[c * strideIn + s];
            }
         }
      }
      outChannelOffset += numChannels;
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_concat_exec_cn<float>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_concat_exec_cn<double>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);
