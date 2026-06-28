// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_inputAggregator_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_inputAggregator_exec_cn(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_inputAggregator_PrivArgs *pKerPrivArgs = (AUDIOLIB_inputAggregator_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_inputAggregator_exec_cn");

   dataType *restrict pOutLocal = (dataType *) pOut;
   uint32_t outChannelOffset    = 0;

   for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {

      dataType *restrict pInLocal = (dataType *) pIn[i];
      uint32_t numSamples         = pKerPrivArgs->numInputSamples[i];
      uint32_t numChannels        = pKerPrivArgs->numInputChannels[i];
      uint32_t strideIn           = pKerPrivArgs->strideIn[i];

      if (pKerPrivArgs->isInputInterleave) {
         // Input: samples in dim_y, channels in dim_x
         if (pKerPrivArgs->isOutputInterleave) {
            // Output: samples in dim_y, channels in dim_x
            for (uint32_t s = 0; s < numSamples; s++) {
               for (uint32_t c = 0; c < numChannels; c++) {
                  pOutLocal[s * pKerPrivArgs->strideOut + outChannelOffset + c] = pInLocal[s * strideIn + c];
               }
            }
         }
         else {
            // Output: samples in dim_x, channels in dim_y
            for (uint32_t c = 0; c < numChannels; c++) {
               for (uint32_t s = 0; s < numSamples; s++) {
                  pOutLocal[(outChannelOffset + c) * pKerPrivArgs->strideOut + s] = pInLocal[s * strideIn + c];
               }
            }
         }
      }
      else {
         // Input: samples in dim_x, channels in dim_y
         if (pKerPrivArgs->isOutputInterleave) {
            // Output: samples in dim_y, channels in dim_x
            for (uint32_t s = 0; s < numSamples; s++) {
               for (uint32_t c = 0; c < numChannels; c++) {
                  pOutLocal[s * pKerPrivArgs->strideOut + outChannelOffset + c] = pInLocal[c * strideIn + s];
               }
            }
         }
         else {
            // Output: samples in dim_x, channels in dim_y
            for (uint32_t c = 0; c < numChannels; c++) {
               for (uint32_t s = 0; s < numSamples; s++) {
                  pOutLocal[(outChannelOffset + c) * pKerPrivArgs->strideOut + s] = pInLocal[c * strideIn + s];
               }
            }
         }
      }
      outChannelOffset += numChannels;
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_inputAggregator_exec_cn<float>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_inputAggregator_exec_cn<double>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);
