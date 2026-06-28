// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ASRC_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_ASRC_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticIn;
   void    *staticOut;
   uint32_t sampleDataType;
   uint32_t maxSampleCountPerBlock;
   uint8_t  inputSampleRate;
   uint8_t  outputSampleRate;
   uint8_t  numChannels;
   uint32_t blockCount;
   float    signalFrequency;
   uint32_t genOutSamplesPerChannel;
   uint32_t moduloFactor;
   uint32_t dataFormat;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
} asrc_testParams_t;

void asrc_getTestParams(asrc_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_ASRC_IXX_IXX_OXX_IDAT_H */
