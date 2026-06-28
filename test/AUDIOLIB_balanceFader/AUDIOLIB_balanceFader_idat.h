// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticIn;
   void    *staticOut;
   uint32_t sampleDataType;
   uint32_t numSamples;
   uint32_t numChannels;
   float    balance;
   float    fade;
   float    sideGainFactor;
   float    lfeBalanceGainFactor;
   float    lfeFaderGainFactor;
   int32_t  lfeBalanceMode;
   int32_t  lfeFaderMode;
   int32_t *channelConfig;
   uint8_t  isInterleave;
   uint32_t strideIn;
   uint32_t strideOut;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
} AUDIOLIB_balanceFader_testParams_t;

void AUDIOLIB_balanceFader_getTestParams(AUDIOLIB_balanceFader_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_IDAT_H */
