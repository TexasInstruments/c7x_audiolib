// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_panning_idat.h
*******************************************************************************
*/

#ifndef AUDIOLIB_PANNING_IXX_IXX_OXX_IDAT_H

#define AUDIOLIB_PANNING_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by MMALIB
#include "../common/AUDIOLIB_test.h"

typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticIn;
   void    *staticOut;
   void    *channelConfig;
   uint32_t sampleDataType;
   uint32_t numSamples;
   uint32_t numChannels;
   uint32_t isInterleave;
   float    sourceAzimuth;
   float    nextAzimuth;
   uint32_t changeFrame;
   int32_t    samplingRate;
   float    smoothingTime;
   uint32_t strideIn;
   uint32_t strideOut;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   int32_t  frames; // number of frames to process

   uint32_t testID;
} AUDIOLIB_panning_testParams_t;

void AUDIOLIB_panning_getTestParams(AUDIOLIB_panning_testParams_t **params, int32_t *numTests);

#endif /* define AUDIOLIB_PANNING_IXX_IXX_OXX_IDAT_H */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_panning_idat.h                                     */
/* ======================================================================== */
