// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_preEmphasis_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_preEmphasis_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_preEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_preEmphasis_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_preEmphasis_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t numChannels    = pKerPrivArgs->numChannels;
   uint32_t samples     = pKerPrivArgs->samples;
   int32_t  strideIn    = pKerPrivArgs->strideIn;
   int32_t  strideOut   = pKerPrivArgs->strideOut;
   float   *xPrev      = &(pKerPrivArgs->xPrev[0]);
   float alpha = pKerPrivArgs->alpha;

   if (pKerPrivArgs->isInterleave == 1) {
      // Interleaved layout: numChannels span rows, samples span columns
      for (uint32_t n = 0; n < numChannels; n++) {
         // First sample uses xPrev state
         pOutLocal[0 * strideOut + n] = pInLocal[0 * strideIn + n] - alpha * xPrev[n];

         // Remaining samples use previous input sample
         for (uint32_t m = 1; m < samples; m++) {
            pOutLocal[m * strideOut + n] = pInLocal[m * strideIn + n] - alpha * pInLocal[(m - 1) * strideIn + (n)];
         }
         
         // Update state with the last sample of this frame
         xPrev[n] = pInLocal[(samples - 1) * strideIn + n];
      }
   }
   else {
      // Non-interleaved layout: samples span rows, numChannels span columns
      for (uint32_t n = 0; n < numChannels; n++) {
         // First sample uses xPrev state
         pOutLocal[n * strideOut + 0] = pInLocal[n * strideIn + 0] - alpha * xPrev[n];
         // Remaining samples use previous input sample
         for (uint32_t m = 1; m < samples; m++) {
            pOutLocal[n * strideOut + m] = pInLocal[n * strideIn + m] - alpha * pInLocal[(n) *strideIn + (m - 1)];

         }
         xPrev[n] = pInLocal[n * strideIn + (samples - 1)];
      }
   }

   return status;
}

// explicit inspreEmphasistiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_preEmphasis_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn,
                                                             void *restrict pOut);

