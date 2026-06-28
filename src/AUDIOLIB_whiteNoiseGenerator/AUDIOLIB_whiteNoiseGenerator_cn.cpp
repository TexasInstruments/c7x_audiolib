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

   uint32_t samples = pKerPrivArgs->samples;
   float    range   = pKerPrivArgs->range;

   // Standard LCG parameters
   const uint32_t a = 1664525u;
   const uint32_t c = 1013904223u;
   // m = 2**32 is implicit in uint32_t arithmetic wrap-around

   // Pre-calculate 1.0 / (2**32) for normalization
   const dataType m_inv = (dataType) 2.3283064365386963e-10;

   // Initialize LCG state from the handle
   uint32_t x = pKerPrivArgs->seed;

   for (uint32_t i = 0; i < samples; i++) {
      // 1. Generate the next pseudo-random integer: x = (a * x + c) % m
      // The modulo is implicit for uint32_t.
      x = (a * x + c);

      // 2. Normalize the integer to [0.0, 1.0)
      // normalized_val = (dataType)x / (double)(1ULL << 32)
      dataType normalized_val = (dataType) x * m_inv;

      // 3. Scale to the desired range [-range, range]
      // out[i] = (2 * range_val * normalized_val) - range_val
      pOutLocal[i] = (2.0f * range * normalized_val) - range;
   }

   // Update the seed in the handle for the next exec call
   // This ensures the noise sequence continues from where it left off.
   pKerPrivArgs->seed = x;

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pOut);
