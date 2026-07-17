/*******************************************************************************
**+--------------------------------------------------------------------------+**
**|                            ****                                          |**
**|                            ****                                          |**
**|                            ******o***                                    |**
**|                      ********_///_****                                   |**
**|                      ***** /_//_/ ****                                   |**
**|                       ** ** (__/ ****                                    |**
**|                           *********                                      |**
**|                            ****                                          |**
**|                            ***                                           |**
**|                                                                          |**
**|        Copyright (c) 2025 Texas Instruments Incorporated                 |**
**|                                                                          |**
**|              All rights reserved not granted herein.                     |**
**|                         Limited License.                                 |**
**+--------------------------------------------------------------------------+**
*******************************************************************************/

#ifndef AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_IDAT_H
#define AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_IDAT_H

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

/*
 * Field order MUST match the initializer emitted by
 * test/AUDIOLIB_idat_gen/AUDIOLIB_upSample/gen_data.py (and hence
 * AUDIOLIB_upSample_idat.c).  strideIn / strideOut are BYTE strides
 * (produced via AUDIOLIB_CALC_STRIDE) — not element counts.
 */
typedef struct {
   uint8_t testPattern; /* 0: constant, 1: sequential, 2: random, 3: static array, 4: file */
   void   *staticIn;    /* static reference input  (channels x numSamples)            */
   void   *staticOut;   /* static reference output (channels x upFactor*numSamples)    */
   uint32_t numSamples; /* input samples per channel                                  */
   uint32_t numChannels;
   uint32_t upFactor;          /* upsampling factor n (>= 1)                          */
   uint32_t phase;             /* sample offset, 0 <= phase < upFactor                */
   uint32_t isInterleave;      /* 0 = planar (ch x samples), 1 = interleaved (samples x ch) */
   uint32_t strideIn;          /* input  row stride in BYTES                          */
   uint32_t strideOut;         /* output row stride in BYTES                          */
   uint32_t dataType;          /* AUDIOLIB_FLOAT32 / AUDIOLIB_FLOAT64                  */
   uint32_t outputDataLocation; /* 0 -> HEAP, 1 -> MSMC                               */
   uint32_t numReps;
   uint32_t testID;
} AUDIOLIB_upSample_testParams_t;

void AUDIOLIB_upSample_getTestParams(AUDIOLIB_upSample_testParams_t **params, int32_t *numTests);

#endif /* AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_IDAT_H */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample_idat.h                                   */
/* ======================================================================== */
