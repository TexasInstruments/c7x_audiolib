// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticIn0;
   void    *staticOut;
   uint32_t sampleDataType;
   uint32_t blockSize;
   uint32_t numChannels;
   uint32_t strideIn; // stride between channels, in elements
   int32_t  samplingRate;
   float    smoothingTime;
   uint32_t frames;             // number of frames (exec calls) to process
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
} AUDIOLIB_rmsSmoothed_testParams_t;

void AUDIOLIB_rmsSmoothed_getTestParams(AUDIOLIB_rmsSmoothed_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_IDAT_H */
