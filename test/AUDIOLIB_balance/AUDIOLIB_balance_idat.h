// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_BALANCE_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_BALANCE_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticInLeft;
   void    *staticOutLeft;
   void    *staticInRight;
   void    *staticOutRight;
   uint32_t sampleDataType;
   uint32_t numSamples;
   uint32_t numChannels;
   float    balance;
   int32_t  samplingRate;
   float    smoothingTime;
   uint32_t strideIn;
   uint32_t strideOut;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
   int32_t  frames; // number of frames to process
} AUDIOLIB_balance_testParams_t;

void AUDIOLIB_balance_getTestParams(AUDIOLIB_balance_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_BALANCE_IXX_IXX_OXX_IDAT_H */
