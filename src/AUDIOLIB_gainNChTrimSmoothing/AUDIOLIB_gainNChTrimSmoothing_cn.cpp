// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_gainNChTrimSmoothing_priv.h"

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingSymmetric_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrimSmoothingSymmetric_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t numChannels       = pKerPrivArgs->numChannels;
   uint32_t numSamples        = pKerPrivArgs->numSamples;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;
   uint8_t  isInterleaved     = pKerPrivArgs->isInterleaved;

   // Interleaved buffers hold one sample frame per row, planar buffers one channel per
   // row, so the two layouts differ only in which index the row stride multiplies.
   const uint32_t inSampleStride  = isInterleaved ? strideInElements : 1U;
   const uint32_t inChannelStride = isInterleaved ? 1U : strideInElements;
   const uint32_t outSampleStride = isInterleaved ? strideOutElements : 1U;
   const uint32_t outChanStride   = isInterleaved ? 1U : strideOutElements;

   for (uint32_t ch = 0; ch < numChannels; ch++) {
      dataType gain       = (dataType) pKerPrivArgs->currentGain[ch];
      dataType gainStep   = (dataType) pKerPrivArgs->gainStep[ch];
      dataType targetGain = (dataType) pKerPrivArgs->targetGain[ch];
      int32_t  remaining  = pKerPrivArgs->remainingFadeSamples[ch];

      dataType *restrict pInCh  = pInLocal + ch * inChannelStride;
      dataType *restrict pOutCh = pOutLocal + ch * outChanStride;

      for (uint32_t s = 0; s < numSamples; s++) {
         // The sample is scaled by the gain first, then the gain takes its step.
         if (remaining > 0) {
            pOutCh[s * outSampleStride] = pInCh[s * inSampleStride] * gain;
            gain *= gainStep;
            remaining--;
            // Land exactly on the target rather than on the accumulated product.
            if (remaining == 0) {
               gain = targetGain;
            }
         }
         else {
            pOutCh[s * outSampleStride] = pInCh[s * inSampleStride] * targetGain;
            gain                        = targetGain;
         }
      }

      pKerPrivArgs->currentGain[ch]          = (float) gain;
      pKerPrivArgs->remainingFadeSamples[ch] = remaining;
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingSmooth_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrimSmoothingSmooth_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t numChannels       = pKerPrivArgs->numChannels;
   uint32_t numSamples        = pKerPrivArgs->numSamples;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;
   uint8_t  isInterleaved     = pKerPrivArgs->isInterleaved;
   /* Weight of the previous gain, 1 - k (exp(-1 / (fs * tau))). */
   dataType smoothingCoeff    = (dataType) pKerPrivArgs->smoothingCoeff;

   const uint32_t inSampleStride  = isInterleaved ? strideInElements : 1U;
   const uint32_t inChannelStride = isInterleaved ? 1U : strideInElements;
   const uint32_t outSampleStride = isInterleaved ? strideOutElements : 1U;
   const uint32_t outChanStride   = isInterleaved ? 1U : strideOutElements;

   for (uint32_t ch = 0; ch < numChannels; ch++) {
      dataType gain       = (dataType) pKerPrivArgs->currentGain[ch];
      dataType targetGain = (dataType) pKerPrivArgs->targetGain[ch];
      /* gain[n] = target + (gain0 - target) * coeff^(n + 1): the distance to the target
         decays by coeff per sample. The gain is updated first, then applied, so the first
         sample has already decayed once. */
      dataType decay = (gain - targetGain) * smoothingCoeff;

      dataType *restrict pInCh  = pInLocal + ch * inChannelStride;
      dataType *restrict pOutCh = pOutLocal + ch * outChanStride;

      for (uint32_t s = 0; s < numSamples; s++) {
         gain                        = decay + targetGain;
         pOutCh[s * outSampleStride] = pInCh[s * inSampleStride] * gain;
         decay                       = decay * smoothingCoeff;
      }

      /* The gain of the last sample; the next frame continues from here. */
      pKerPrivArgs->currentGain[ch] = (float) gain;
   }

   return status;
}

// Explicit instantiation for float data type
template AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothingSymmetric_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                               void *restrict pIn,
                                                                               void *restrict pOut);
template AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothingSmooth_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                            void *restrict pIn,
                                                                            void *restrict pOut);
