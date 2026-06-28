// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_MUTE_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_MUTE_IXX_IXX_OXX_IDAT_H

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
   uint32_t isMute;
   int32_t  samplingRate;
   float    fadeTime;
   uint8_t  isInterleaved;
   uint32_t fadeType; // 0=LINEAR, 1=SMOOTH, 2=HARD
   uint32_t strideIn;
   uint32_t strideOut;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
   int32_t  frames; // number of frames to process
} AUDIOLIB_mute_testParams_t;

void AUDIOLIB_mute_getTestParams(AUDIOLIB_mute_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_MUTE_IXX_IXX_OXX_IDAT_H */
