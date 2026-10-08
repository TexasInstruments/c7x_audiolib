// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_IDAT_H

#define AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by MMALIB
#include "../common/AUDIOLIB_test.h"

/* Field order must match what AUDIOLIB_idat_gen/AUDIOLIB_gainNChTrimSmoothing/gen_data.py
   writes into AUDIOLIB_gainNChTrimSmoothing_idat.c. */
typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static
                           array, 4: file, etc */
   void    *staticIn;
   void    *currentGain; // numChannels entries: initialGain, the gain each channel starts at
   void    *targetGain;  // numChannels entries: trimGain, the target is masterGain * trimGain
   float    masterGain;
   void    *staticOut;
   uint32_t numSamples;
   uint32_t numChannels;
   uint8_t  isInterleaved;
   uint32_t strideIn;
   uint32_t strideOut;
   float    fadeTime;
   int32_t  samplingRate;
   uint32_t numExecReps;
   uint8_t  fadeType; // 0: SYMMETRIC, 1: SMOOTH
   uint32_t sampleDataType;
   uint32_t outputDataLocation; // 0 -> HEAP (probably L2SRAM), 1 -> MSMC
   uint32_t numReps;
   uint32_t testID;
} AUDIOLIB_gainNChTrimSmoothing_testParams_t;

void AUDIOLIB_gainNChTrimSmoothing_getTestParams(AUDIOLIB_gainNChTrimSmoothing_testParams_t **params,
                                                 int32_t                                     *numTests);

#endif /* define AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_IDAT_H */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainNChTrimSmoothing_idat.h                                     */
/* ======================================================================== */
