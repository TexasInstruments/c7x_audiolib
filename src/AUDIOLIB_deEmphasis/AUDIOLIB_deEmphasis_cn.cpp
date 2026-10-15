// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_deEmphasis_priv.h"
#include <cstdint>

/**
 * De-emphasis, the exact inverse of pre-emphasis:
 *
 *    y[n] = x[n] + alpha * y[n-1]
 *
 * Note the two differences from AUDIOLIB_deEmphasis's FIR counterpart: the sign of
 * the coefficient, and the fact that the delayed term is the filter's own output.
 * The latter is what makes this recursive, so the state saved for the next call is
 * the last output sample, not the last input sample.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_deEmphasis_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_deEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_deEmphasis_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_deEmphasis_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t numChannels = pKerPrivArgs->numChannels;
   uint32_t samples     = pKerPrivArgs->samples;
   int32_t  strideIn    = pKerPrivArgs->strideIn;
   int32_t  strideOut   = pKerPrivArgs->strideOut;
   float   *yPrev      = &(pKerPrivArgs->yPrev[0]);
   float    alpha       = pKerPrivArgs->alpha;

   if (pKerPrivArgs->isInterleave == 1) {
      // Interleaved layout: samples span rows, channels span columns
      for (uint32_t n = 0; n < numChannels; n++) {
         // First sample resumes from the previous frame's final output
         float acc                    = pInLocal[0 * strideIn + n] + alpha * yPrev[n];
         pOutLocal[0 * strideOut + n] = (dataType) acc;

         for (uint32_t m = 1; m < samples; m++) {
            acc                          = pInLocal[m * strideIn + n] + alpha * acc;
            pOutLocal[m * strideOut + n] = (dataType) acc;
         }

         // Carry the last output into the next frame
         yPrev[n] = acc;
      }
   }
   else {
      // Non-interleaved layout: channels span rows, samples span columns
      for (uint32_t n = 0; n < numChannels; n++) {
         float acc                    = pInLocal[n * strideIn + 0] + alpha * yPrev[n];
         pOutLocal[n * strideOut + 0] = (dataType) acc;

         for (uint32_t m = 1; m < samples; m++) {
            acc                          = pInLocal[n * strideIn + m] + alpha * acc;
            pOutLocal[n * strideOut + m] = (dataType) acc;
         }

         yPrev[n] = acc;
      }
   }

   return status;
}

// Explicit instantiation for float data type
template AUDIOLIB_STATUS AUDIOLIB_deEmphasis_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                            void *restrict pIn,
                                                            void *restrict pOut);
