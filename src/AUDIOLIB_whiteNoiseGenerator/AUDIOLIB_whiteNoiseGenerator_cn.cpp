// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_whiteNoiseGenerator_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{
   AUDIOLIB_STATUS                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_whiteNoiseGenerator_exec_cn");

   dataType *restrict pOutLocal = (dataType *) pOut;

   int32_t samples     = pKerPrivArgs->samples;
   int32_t numChannels = pKerPrivArgs->numChannels;
   int32_t strideOut   = pKerPrivArgs->strideOut;
   float   range       = pKerPrivArgs->range;

   // Standard LCG parameters
   const uint32_t a = 1664525u;
   const uint32_t c = 1013904223u;
   // m = 2**32 is implicit in uint32_t arithmetic wrap-around

   // range / 2^31 maps the zero-centred LCG output to [-range, +range]
   const dataType scale = (dataType) AUDIOLIB_LCG_SIGNED_NORM_FACTOR_F32 * range;

   // Channel 0's next output; every other channel's is CHANNEL_SPACING steps after the previous channel's
   uint32_t channelNext = pKerPrivArgs->states[0];

   for (int32_t ch = 0; ch < numChannels; ch++) {
      uint32_t x  = channelNext;
      channelNext = pKerPrivArgs->channelMultiplier * channelNext + pKerPrivArgs->channelIncrement;

      for (int32_t i = 0; i < samples; i++) {
         // 1. Centre the pseudo-random integer on zero: (int32_t) (x ^ 2^31) = x - 2^31
         int32_t centred = (int32_t) (x ^ AUDIOLIB_LCG_SIGN_FLIP);

         // 2. Scale to the desired range [-range, range]: (x / 2^31 - 1) * range
         pOutLocal[ch * strideOut + i] = (dataType) centred * scale;

         // 3. Generate the next pseudo-random integer: x = (a * x + c) % m
         // The modulo is implicit for uint32_t.
         x = (a * x + c);
      }
   }

   // Channel 0 continues `samples` steps on, so the next exec call continues every channel's sequence
   pKerPrivArgs->states[0] =
       pKerPrivArgs->jumpHeadMultiplierNextFrame * pKerPrivArgs->states[0] + pKerPrivArgs->jumpHeadIncNextFrame;

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pOut);
