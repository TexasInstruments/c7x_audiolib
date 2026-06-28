// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticOut;
   uint32_t numSamples;
   uint32_t seed;
   float    range;
   uint32_t sampleDataType;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
   uint32_t numExecReps;
} AUDIOLIB_whiteNoiseGenerator_testParams_t;

void AUDIOLIB_whiteNoiseGenerator_getTestParams(AUDIOLIB_whiteNoiseGenerator_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_IDAT_H */
