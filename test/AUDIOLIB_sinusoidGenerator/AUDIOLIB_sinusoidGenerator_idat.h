// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t  testPattern; // 1.Static
   void    *staticOut;
   uint32_t numSamples;
   float    startFrequency;
   float    targetFrequency;
   float    startPhase;
   float    smoothingTime;
   uint32_t samplingRate;
   uint32_t executionMode; // 1 for VECTOR, 0 for SCALAR
   uint32_t dataType;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
   int32_t  numExecReps;
} AUDIOLIB_sinusoidGenerator_testParams_t;

void AUDIOLIB_sinusoidGenerator_getTestParams(AUDIOLIB_sinusoidGenerator_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_IDAT_H */
