// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
 ******************************************************************************
 * @file     AUDIOLIB_panning_cn.cpp
 * @brief    C Natural implementation of the panning kernel.
 *
 * @details  This file contains the reference C implementation for the
 * AUDIOLIB_panning kernel.
 *******************************************************************************
 */

#include "AUDIOLIB_panning_priv.h"

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_panning_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut)
{
   AUDIOLIB_panning_PrivArgs *pKerPrivArgs = (AUDIOLIB_panning_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_panning_exec_cn");

   const dataType *restrict pInLocal   = (const dataType *) pIn;
   dataType *restrict       pOutLocal  = (dataType *) pOut;
   const float *restrict    targetGain = (const float *) pGain;

   const uint32_t numSamples  = (uint32_t) pKerPrivArgs->samples;
   const uint32_t numChannels = pKerPrivArgs->channels;
   const float    alpha       = pKerPrivArgs->smoothingCoefficient;

   /* Element steps between consecutive input samples, consecutive output samples
    * of one channel, and the first samples of consecutive channels. Interleaved
    * output rows are samples; planar output rows are channels and the mono input
    * is a single contiguous row. */
   const bool     isInterleave   = (pKerPrivArgs->isInterleave == 1U);
   const uint32_t inSampleStep   = isInterleave ? (uint32_t) pKerPrivArgs->strideInElements : 1U;
   const uint32_t outSampleStep  = isInterleave ? (uint32_t) pKerPrivArgs->strideOutElements : 1U;
   const uint32_t outChannelStep = isInterleave ? 1U : (uint32_t) pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0U; ch < numChannels; ch++) {
      const float        target = targetGain[ch];
      dataType *restrict pOutCh = &pOutLocal[ch * outChannelStep];

      if (pKerPrivArgs->bypassSmoothing == 1U) {
         for (uint32_t n = 0U; n < numSamples; n++) {
            pOutCh[n * outSampleStep] = pInLocal[n * inSampleStep] * target;
         }
      }
      else {
         /* First-order smoothing: g[n] = alpha * g[n-1] + (1 - alpha) * target,
          * with g carried across frames in currentGain. */
         float gain = pKerPrivArgs->currentGain[ch];
         for (uint32_t n = 0U; n < numSamples; n++) {
            gain                      = (alpha * gain) + ((1.0f - alpha) * target);
            pOutCh[n * outSampleStep] = pInLocal[n * inSampleStep] * gain;
         }
         pKerPrivArgs->currentGain[ch] = gain;
      }
   }

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_panning_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pGain,
                                                         void *restrict pOut);

/* ======================================================================== */
/*  End of file:  AUDIOLIB_panning_cn.cpp                                   */
/* ======================================================================== */
