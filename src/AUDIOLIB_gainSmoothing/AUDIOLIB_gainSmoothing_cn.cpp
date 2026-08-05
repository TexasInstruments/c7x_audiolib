// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_gainSmoothing_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSymmetricFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                  status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainSmoothingSymmetricFade_exec_cn");
   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t numCols           = pKerPrivArgs->numCols;
   uint32_t numRows           = pKerPrivArgs->numRows;
   float    gainStep          = pKerPrivArgs->gainStep;
   float    targetGain        = pKerPrivArgs->targetGain;
   float    currentGain       = pKerPrivArgs->currentGain;
   uint8_t  isInterleaved     = pKerPrivArgs->initArgs.isInterleaved;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;

   float         localCurrentGain = currentGain;
   const uint8_t isFadingUp       = (gainStep > 1.0f);

   // The gain advances once per sample. Interleaved buffers hold one sample frame per
   // row, planar buffers hold one sample per column, so the two layouts differ only in
   // which axis carries the gain.
   const uint32_t numGainUpdates     = isInterleaved ? numRows : numCols;
   const uint32_t elemsPerGainUpdate = isInterleaved ? numCols : numRows;

   if (pKerPrivArgs->bypassSmoothing) {
      // The gain is constant, so the channel/sample layout does not matter here:
      // walk numRows rows of numCols elements for either layout.
      for (uint32_t row = 0; row < numRows; row++) {
         for (uint32_t col = 0; col < numCols; col++) {
            pOutLocal[row * strideOutElements + col] = pInLocal[row * strideInElements + col] * targetGain;
         }
      }
   }
   else {
      for (uint32_t upd = 0; upd < numGainUpdates; upd++) {
         localCurrentGain *= gainStep;
         // The ramp overshoots by design; clamp it at the target.
         if (isFadingUp) {
            if (localCurrentGain > targetGain)
               localCurrentGain = targetGain;
         }
         else {
            if (localCurrentGain < targetGain)
               localCurrentGain = targetGain;
         }
         for (uint32_t e = 0; e < elemsPerGainUpdate; e++) {
            const uint32_t inIdx  = isInterleaved ? (upd * strideInElements + e) : (e * strideInElements + upd);
            const uint32_t outIdx = isInterleaved ? (upd * strideOutElements + e) : (e * strideOutElements + upd);
            pOutLocal[outIdx]     = pInLocal[inIdx] * localCurrentGain;
         }
      }
   }
   if (pKerPrivArgs->bypassSmoothing) {
      pKerPrivArgs->currentGain = targetGain;
   }
   else {
      pKerPrivArgs->currentGain = localCurrentGain;
   }
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                  status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainSmoothingSmoothFade_exec_cn");
   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t numCols              = pKerPrivArgs->numCols;
   uint32_t numRows              = pKerPrivArgs->numRows;
   float    targetGain           = pKerPrivArgs->targetGain;
   float    currentGain          = pKerPrivArgs->currentGain;
   uint32_t strideInElements     = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements    = pKerPrivArgs->strideOutElements;
   uint8_t  isInterleaved        = pKerPrivArgs->initArgs.isInterleaved;
   float    smoothingCoefficient = pKerPrivArgs->smoothingCoefficient;
   float    localCurrentGain     = currentGain;

   // As in the symmetric variant: the gain advances once per sample, which is once per
   // row when interleaved and once per column when planar.
   const uint32_t numGainUpdates     = isInterleaved ? numRows : numCols;
   const uint32_t elemsPerGainUpdate = isInterleaved ? numCols : numRows;

   for (uint32_t upd = 0; upd < numGainUpdates; upd++) {
      localCurrentGain = (smoothingCoefficient * localCurrentGain) + ((1.0f - smoothingCoefficient) * targetGain);
      for (uint32_t e = 0; e < elemsPerGainUpdate; e++) {
         const uint32_t inIdx  = isInterleaved ? (upd * strideInElements + e) : (e * strideInElements + upd);
         const uint32_t outIdx = isInterleaved ? (upd * strideOutElements + e) : (e * strideOutElements + upd);
         pOutLocal[outIdx]     = pInLocal[inIdx] * localCurrentGain;
      }
   }
   pKerPrivArgs->currentGain = localCurrentGain;

   return status;
}

// Explicit instantiation for float data type
template AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSymmetricFade_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                            void *restrict pIn,
                                                                            void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
