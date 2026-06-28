// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_CONCAT_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_CONCAT_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t  testPattern;        /* 0: constant, 1: sequential, 2: random, 3: static array, 4: file, etc */
   void   **staticIn;           // Pointer to an array of input buffer pointers
   void    *staticOut;          // Output buffer pointer
   uint32_t inSamples;          // Number of samples
   int32_t  inChannels;         // Number of channels for each input (same for all inputs)
   uint8_t  isInterleave;       // Flag indicating if data is interleaved (1) or non-interleaved (0)
   uint32_t numInputs;          // Number of inputs
   uint32_t strideIn0;          // Stride for input (aligned to 64 bytes)
   uint32_t strideOut;          // Stride for output (aligned to 64 bytes)
   uint32_t dataType;           // Precision (e.g., AUDIOLIB_FLOAT32)
   uint32_t outputDataLocation; // 0 -> HEAP (L2SRAM), 1 -> MSMC
   uint32_t numReps;            // Number of repetitions
   uint32_t testID;             // Test ID
} AUDIOLIB_concat_testParams_t;

void AUDIOLIB_concat_getTestParams(AUDIOLIB_concat_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_CONCAT_IXX_IXX_OXX_IDAT_H */
