// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_mute_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteHard_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_muteHard_exec_cn");
   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples           = pKerPrivArgs->samples;
   uint32_t channels          = pKerPrivArgs->channels;
   float    targetGain        = pKerPrivArgs->targetGain;
   uint8_t  isInterleaved     = pKerPrivArgs->initArgs.isInterleaved;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;

   if (isInterleaved) {
      // Interleaved: [ch0_s0, ch1_s0, ..., ch0_s1, ...]
      for (uint32_t j = 0; j < channels; j++) {
         for (uint32_t c = 0; c < samples; c++) {
            pOutLocal[j * strideOutElements + c] = pInLocal[j * strideInElements + c] * targetGain;
         }
      }
   }
   else {
      // Non-interleaved: [ch0_s0, ch0_s1, ..., ch1_s0, ...]
      for (uint32_t c = 0; c < samples; c++) {
         for (uint32_t j = 0; j < channels; j++) {
            pOutLocal[j * strideOutElements + c] = pInLocal[j * strideInElements + c] * targetGain;
         }
      }
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteLinearFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_muteLinearFade_exec_cn");
   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples           = pKerPrivArgs->samples;
   uint32_t channels          = pKerPrivArgs->channels;
   float    gainStep          = pKerPrivArgs->gainStep;
   float    targetGain        = pKerPrivArgs->targetGain;
   float    currentGain       = pKerPrivArgs->currentGain;
   uint8_t  isInterleaved     = pKerPrivArgs->initArgs.isInterleaved;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;

   float localCurrentGain = currentGain;

   if (isInterleaved) {
      // Interleaved: [ch0_s0, ch1_s0, ..., ch0_s1, ...]
      for (uint32_t j = 0; j < channels; j++) {
         localCurrentGain += gainStep;
         if ((gainStep > 0 && localCurrentGain > targetGain) || (gainStep < 0 && localCurrentGain < targetGain)) {
            localCurrentGain = targetGain;
         }

         for (uint32_t c = 0; c < samples; c++) {
            pOutLocal[j * strideOutElements + c] = pInLocal[j * strideInElements + c] * localCurrentGain;
         }
      }
   }
   else {
      // Non-interleaved: [ch0_s0, ch0_s1, ..., ch1_s0, ...]
      for (uint32_t j = 0; j < samples; j++) {
         localCurrentGain += gainStep;
         if ((gainStep > 0 && localCurrentGain > targetGain) || (gainStep < 0 && localCurrentGain < targetGain)) {
            localCurrentGain = targetGain;
         }

         for (uint32_t c = 0; c < channels; c++) {
            pOutLocal[c * strideOutElements + j] = pInLocal[c * strideInElements + j] * localCurrentGain;
         }
      }
   }

   pKerPrivArgs->currentGain = localCurrentGain;
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteSmoothFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_muteSmoothFade_exec_cn");
   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples              = pKerPrivArgs->samples;
   uint32_t channels             = pKerPrivArgs->channels;
   float    targetGain           = pKerPrivArgs->targetGain;
   float    currentGain          = pKerPrivArgs->currentGain;
   uint32_t strideInElements     = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements    = pKerPrivArgs->strideOutElements;
   uint8_t  isInterleaved        = pKerPrivArgs->initArgs.isInterleaved;
   float    smoothingCoefficient = pKerPrivArgs->smoothingCoefficient;

   float localCurrentGain = currentGain;

   if (isInterleaved) {
      // Interleaved: [ch0_s0, ch1_s0, ..., ch0_s1, ...]
      for (uint32_t j = 0; j < channels; j++) {
         localCurrentGain = smoothingCoefficient * localCurrentGain + (1.0f - smoothingCoefficient) * targetGain;

         for (uint32_t c = 0; c < samples; c++) {
            pOutLocal[j * strideOutElements + c] = pInLocal[j * strideInElements + c] * localCurrentGain;
         }
      }
   }
   else {
      // Non-interleaved: [ch0_s0, ch0_s1, ..., ch1_s0, ...]
      for (uint32_t j = 0; j < samples; j++) {
         localCurrentGain = smoothingCoefficient * localCurrentGain + (1.0f - smoothingCoefficient) * targetGain;

         for (uint32_t c = 0; c < channels; c++) {
            pOutLocal[c * strideOutElements + j] = pInLocal[c * strideInElements + j] * localCurrentGain;
         }
      }
   }

   pKerPrivArgs->currentGain = localCurrentGain;
   return status;
}

// Explicit instantiation for float data type
template AUDIOLIB_STATUS
AUDIOLIB_muteHard_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_muteLinearFade_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_muteSmoothFade_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
