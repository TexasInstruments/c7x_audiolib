// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticIn0;
   void    *staticOut;
   uint32_t inDataType;
   uint32_t outDataType;
   uint32_t inSamples;
   uint32_t inChannels;
   uint32_t strideIn;
   uint32_t strideOut;
   uint8_t  testQ31;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
} AUDIOLIB_typeConversion_testParams_t;

void AUDIOLIB_typeConversion_getTestParams(AUDIOLIB_typeConversion_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_IDAT_H */
