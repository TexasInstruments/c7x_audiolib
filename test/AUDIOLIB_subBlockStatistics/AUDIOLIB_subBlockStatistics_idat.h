// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: conssubBlockStatisticst, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticIn;
   void    *staticOut;
   uint32_t inputBlockSize;
   uint32_t subBlockSize;
   uint32_t numChannels;
   uint32_t statisticsType;
   bool     flagPerChanStats; // True if per channel statistics, False if overall statistics
   uint32_t strideIn;
   uint32_t strideOut;
   uint32_t dataType;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
} AUDIOLIB_subBlockStatistics_testParams_t;

void AUDIOLIB_subBlockStatistics_getTestParams(AUDIOLIB_subBlockStatistics_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_IDAT_H */
