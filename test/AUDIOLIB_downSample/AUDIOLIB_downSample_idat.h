// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0


#ifndef AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by MMALIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticIn;
   void    *staticOut;
   uint32_t inSamples;
   uint32_t inChannels;
   uint8_t  isInterleave;
   uint32_t downSampleRatio;
   uint32_t strideIn;
   uint32_t strideOut;
   uint32_t dataType;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
   uint32_t phase; /* offset of first kept sample, 0 <= phase < downSampleRatio.
                      Appended last so pre-phase idat.c initializers zero-fill it
                      (phase = 0); regenerate idat.c to exercise phase > 0.      */
} AUDIOLIB_downSample_testParams_t;

void AUDIOLIB_downSample_getTestParams(AUDIOLIB_downSample_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_IDAT_H */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_downSample_idat.h                                     */
/* ======================================================================== */
