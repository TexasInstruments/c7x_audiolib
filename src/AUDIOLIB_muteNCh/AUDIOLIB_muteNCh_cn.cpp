// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_muteNCh_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChHard_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_muteNChHard_exec_cn");
   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples           = pKerPrivArgs->samples;
   uint32_t channels          = pKerPrivArgs->channels;
   float   *targetGain        = pKerPrivArgs->targetGain;
   uint8_t  isInterleaved     = pKerPrivArgs->initArgs.isInterleaved;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;

   if (isInterleaved) {
      // Interleaved: [ch0_s0, ch1_s0, ..., ch0_s1, ...]
      for (uint32_t j = 0; j < samples; j++) {
         for (uint32_t c = 0; c < channels; c++) {
            pOutLocal[j * strideOutElements + c] = pInLocal[j * strideInElements + c] * targetGain[c];
         }
      }
   }
   else {
      // Non-interleaved: [ch0_s0, ch0_s1, ..., ch1_s0, ...]
      for (uint32_t c = 0; c < samples; c++) {
         for (uint32_t j = 0; j < channels; j++) {
            pOutLocal[j * strideOutElements + c] = pInLocal[j * strideInElements + c] * targetGain[j];
         }
      }
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChLinearFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_muteNChLinearFade_exec_cn");
   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples           = pKerPrivArgs->samples;
   uint32_t channels          = pKerPrivArgs->channels;
   float   *gainStep          = pKerPrivArgs->gainStep;
   float   *targetGain        = pKerPrivArgs->targetGain;
   float   *currentGain       = pKerPrivArgs->currentGain;
   uint8_t  isInterleaved     = pKerPrivArgs->initArgs.isInterleaved;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;

   float localCurrentGain[channels]; // Local copy per channel for this frame
   for (uint32_t ch = 0; ch < channels; ch++) {
      localCurrentGain[ch] = currentGain[ch];
   }

   if (isInterleaved) {
      // Interleaved: [ch0_s0, ch1_s0, ..., ch0_s1, ...]
      for (uint32_t j = 0; j < samples; j++) {
         for (uint32_t c = 0; c < channels; c++) {
            localCurrentGain[c] += gainStep[c];
            if ((gainStep[c] > 0 && localCurrentGain[c] > targetGain[c]) ||
                (gainStep[c] < 0 && localCurrentGain[c] < targetGain[c])) {
               localCurrentGain[c] = targetGain[c];
            }
         }

         for (uint32_t c = 0; c < channels; c++) {
            pOutLocal[j * strideOutElements + c] = pInLocal[j * strideInElements + c] * localCurrentGain[c];
         }
      }
   }
   else {
      // Non-interleaved: [ch0_s0, ch0_s1, ..., ch1_s0, ...]
      for (uint32_t j = 0; j < samples; j++) {

         for (uint32_t c = 0; c < channels; c++) {
            localCurrentGain[c] += gainStep[c];
            if ((gainStep[c] > 0 && localCurrentGain[c] > targetGain[c]) ||
                (gainStep[c] < 0 && localCurrentGain[c] < targetGain[c])) {
               localCurrentGain[c] = targetGain[c];
            }
            pOutLocal[c * strideOutElements + j] = pInLocal[c * strideInElements + j] * localCurrentGain[c];
         }
      }
   }

   // Save updated currentGains back
   for (uint32_t ch = 0; ch < channels; ch++) {
      currentGain[ch] = localCurrentGain[ch];
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChSmoothFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_muteNChSmoothFade_exec_cn");
   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples              = pKerPrivArgs->samples;
   uint32_t channels             = pKerPrivArgs->channels;
   float   *targetGain           = pKerPrivArgs->targetGain;
   float   *currentGain          = pKerPrivArgs->currentGain;
   uint32_t strideInElements     = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements    = pKerPrivArgs->strideOutElements;
   uint8_t  isInterleaved        = pKerPrivArgs->initArgs.isInterleaved;
   float    smoothingCoefficient = pKerPrivArgs->smoothingCoefficient;

   float localCurrentGain[channels]; // Local copy per channel for this frame
   for (uint32_t ch = 0; ch < channels; ch++) {
      localCurrentGain[ch] = currentGain[ch];
   }

   if (isInterleaved) {
      // Interleaved: [ch0_s0, ch1_s0, ..., ch0_s1, ...]
      for (uint32_t j = 0; j < samples; j++) {
         for (uint32_t c = 0; c < channels; c++) {
            localCurrentGain[c] =
                smoothingCoefficient * localCurrentGain[c] + (1.0f - smoothingCoefficient) * targetGain[c];
         }
         for (uint32_t c = 0; c < channels; c++) {
            pOutLocal[j * strideOutElements + c] = pInLocal[j * strideInElements + c] * localCurrentGain[c];
         }
      }
   }
   else {
      // Non-interleaved: [ch0_s0, ch0_s1, ..., ch1_s0, ...]
      for (uint32_t j = 0; j < samples; j++) {

         for (uint32_t c = 0; c < channels; c++) {
            localCurrentGain[c] =
                smoothingCoefficient * localCurrentGain[c] + (1.0f - smoothingCoefficient) * targetGain[c];
            pOutLocal[c * strideOutElements + j] = pInLocal[c * strideInElements + j] * localCurrentGain[c];
         }
      }
   }

   // Save updated currentGains back
   for (uint32_t ch = 0; ch < channels; ch++) {
      currentGain[ch] = localCurrentGain[ch];
   }
   return status;
}

// Explicit instantiation for float data type
template AUDIOLIB_STATUS
AUDIOLIB_muteNChHard_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_muteNChLinearFade_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_muteNChSmoothFade_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
