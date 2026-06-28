// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_balance_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_balance_exec_cn(AUDIOLIB_kernelHandle handle,
                                         void *restrict pInL,
                                         void *restrict pInR,
                                         void *restrict pOutL,
                                         void *restrict pOutR)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_balance_PrivArgs *pKerPrivArgs = (AUDIOLIB_balance_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_balance_exec_cn");
   dataType *restrict pInLocalLeft   = (dataType *) pInL;
   dataType *restrict pInLocalRight  = (dataType *) pInR;
   dataType *restrict pOutLocalLeft  = (dataType *) pOutL;
   dataType *restrict pOutLocalRight = (dataType *) pOutR;

   uint32_t samples           = pKerPrivArgs->samples;
   uint32_t channels          = pKerPrivArgs->channels;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;
   float    targetGainL       = pKerPrivArgs->targetGainL;
   float    targetGainR       = pKerPrivArgs->targetGainR;
   float    currentGainL      = pKerPrivArgs->currentGainL;
   float    currentGainR      = pKerPrivArgs->currentGainR;
   float    smoothingCoeff    = pKerPrivArgs->smoothingCoefficient;

   int32_t bypassSmoothing =
       pKerPrivArgs->bypassSmoothing || (fabsf(currentGainL - targetGainL) < (AUDIOLIB_BALANCE__GAIN_THRESHOLD) &&
                                         fabsf(currentGainR - targetGainR) < (AUDIOLIB_BALANCE__GAIN_THRESHOLD));

   if (!bypassSmoothing) {
      // Initialize with current gains
      currentGainL = pKerPrivArgs->currentGainL;
      currentGainR = pKerPrivArgs->currentGainR;

      // Apply smoothing and compute output on-the-fly
      for (uint32_t j = 0; j < samples; j++) {
         // Apply smoothing to the gain for left and right channels
         currentGainL = smoothingCoeff * currentGainL + (1.0f - smoothingCoeff) * targetGainL;
         currentGainR = smoothingCoeff * currentGainR + (1.0f - smoothingCoeff) * targetGainR;

         // Apply gains to all channels for this sample
         for (uint32_t i = 0; i < channels; i++) {
            pOutLocalLeft[j + i * strideOutElements]  = pInLocalLeft[j + i * strideInElements] * currentGainL;
            pOutLocalRight[j + i * strideOutElements] = pInLocalRight[j + i * strideInElements] * currentGainR;
         }
      }

      // Update current gains for the next frame
      pKerPrivArgs->currentGainL = currentGainL;
      pKerPrivArgs->currentGainR = currentGainR;
   }
   else {
      // Apply the target gain to the input samples and write to output
      for (uint32_t j = 0; j < samples; j++) {
         for (uint32_t i = 0; i < channels; i++) {
            pOutLocalLeft[j + i * strideOutElements]  = pInLocalLeft[j + i * strideInElements] * targetGainL;
            pOutLocalRight[j + i * strideOutElements] = pInLocalRight[j + i * strideInElements] * targetGainR;
         }
      }
   }
   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_balance_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pInL,
                                                         void *restrict pInR,
                                                         void *restrict pOutL,
                                                         void *restrict pOutR);
