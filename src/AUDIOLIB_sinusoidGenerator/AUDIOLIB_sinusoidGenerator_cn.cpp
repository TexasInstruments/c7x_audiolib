// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_sinusoidGenerator_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{
   AUDIOLIB_STATUS                      status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_sinusoidGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_sinusoidGenerator_exec_cn");

   dataType *restrict pOutLocal = (dataType *) pOut;

   dataType TWO_PIF = 2 * 3.14159265358979323846;
   dataType MAX_VAL = 1048576.0f;

   uint32_t numSamples                   = pKerPrivArgs->numSamples;
   dataType phase                        = pKerPrivArgs->phase;
   dataType phaseInc                     = pKerPrivArgs->phaseInc;
   dataType phaseIncTarget               = pKerPrivArgs->phaseIncTarget;
   dataType smoothingCoefficient         = pKerPrivArgs->smoothingCoefficient;
   dataType oneMinusSmoothingCoefficient = pKerPrivArgs->oneMinusSmoothingCoefficient;

   dataType phases[numSamples];

   for (uint32_t i = 0; i < numSamples; i++) {
      phases[i] = phase;

      if (fabsf(phases[i]) > MAX_VAL) {
         pOutLocal[i] = 0.0f;
      }
      else {
         pOutLocal[i] = sinf(phases[i]);
      }

      phase += phaseInc;

      if (phase >= TWO_PIF) {
         phase -= TWO_PIF;
      }
      phaseInc = phaseInc * oneMinusSmoothingCoefficient + phaseIncTarget * smoothingCoefficient;
   }
   pKerPrivArgs->phase    = phase;
   pKerPrivArgs->phaseInc = phaseInc;
   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pOut);
