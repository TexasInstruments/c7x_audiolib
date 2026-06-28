// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t  testPattern; // 1.Static, 2.Sine
   void    *staticIn0;
   void    *staticOut;
   uint32_t inSamples;
   uint32_t inChannels;
   uint32_t strideIn;
   uint32_t strideOut;
   float    threshold;
   float    endKnee;
   uint32_t dataType;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
} AUDIOLIB_softClip_testParams_t;

void AUDIOLIB_softClip_getTestParams(AUDIOLIB_softClip_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_IDAT_H */
