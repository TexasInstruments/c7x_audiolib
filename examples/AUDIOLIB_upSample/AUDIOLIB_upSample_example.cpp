/******************************************************************************
 * Copyright (C) 2025 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/******************************************************************************/
/*                                                                            */
/* Includes                                                                   */
/*                                                                            */
/******************************************************************************/

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

/* upSample by zero insertion: input sample k is placed at output position
 * phase + k*upFactor; all other output positions are 0. */
#define NUM_CH    (2)                       /* channels                          */
#define NUM_SAMP  (4)                       /* input samples per channel         */
#define UP_FACTOR (3)                       /* upsampling factor (>= 1)          */
#define PHASE     (0)                       /* kept-sample offset, 0..upFactor-1 */
#define OUT_SAMP  (UP_FACTOR * NUM_SAMP)    /* output samples per channel        */
#define TOTAL_IN  (NUM_CH * NUM_SAMP)
#define TOTAL_OUT (NUM_CH * OUT_SAMP)


/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{
   // clang-format off
   /* Planar input: NUM_CH rows x NUM_SAMP cols (each row = one channel's samples). */
   float inPlanar[TOTAL_IN] = { 1, 2, 3, 4,        /* channel 0 */
                                5, 6, 7, 8 };       /* channel 1 */

   /* Interleaved input: NUM_SAMP rows x NUM_CH cols (each row = one sample's channels). */
   float inInter[TOTAL_IN]  = { 1, 5,              /* sample 0: ch0, ch1 */
                                2, 6,              /* sample 1           */
                                3, 7,              /* sample 2           */
                                4, 8 };            /* sample 3           */
   // clang-format on

   float out[TOTAL_OUT];

   // handle + init args
   AUDIOLIB_STATUS            status;
   AUDIOLIB_upSample_InitArgs kerInitArgs;
   int32_t                    handleSize = AUDIOLIB_upSample_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle      handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   /* ---------------------------------------------------------------------- */
   /* Case 1: PLANAR (isInterleave = 0)                                      */
   /*   in  : NUM_CH x NUM_SAMP        out : NUM_CH x OUT_SAMP                */
   /*   ch0 [1 2 3 4] -> [1 0 0 2 0 0 3 0 0 4 0 0]  (upFactor=3, phase=0)     */
   /* ---------------------------------------------------------------------- */
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = NUM_SAMP; // samples
   bufParamsIn.dim_y     = NUM_CH;   // channels
   bufParamsIn.stride_y  = NUM_SAMP * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = OUT_SAMP;
   bufParamsOut.dim_y     = NUM_CH;
   bufParamsOut.stride_y  = OUT_SAMP * sizeof(float);

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED; // or AUDIOLIB_FUNCTION_NATC
   kerInitArgs.upFactor     = UP_FACTOR;
   kerInitArgs.phase        = PHASE;
   kerInitArgs.isInterleave = 0;

   status = AUDIOLIB_upSample_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_upSample_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_upSample_exec_checkParams(handle, inPlanar, out);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_upSample_exec(handle, inPlanar, out);

   printf("\n PLANAR (upFactor=%d, phase=%d) \n", UP_FACTOR, PHASE);
   for (size_t c = 0; c < NUM_CH; c++) {
      printf("\n ch%zu: ", c);
      for (size_t n = 0; n < OUT_SAMP; n++) {
         printf("%6g ", out[c * OUT_SAMP + n]);
      }
   }
   printf("\n\n");

   /* ---------------------------------------------------------------------- */
   /* Case 2: INTERLEAVED (isInterleave = 1)                                 */
   /*   in  : NUM_SAMP x NUM_CH        out : OUT_SAMP x NUM_CH                */
   /*   kept sample-rows land at output rows phase, phase+upFactor, ...       */
   /* ---------------------------------------------------------------------- */
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = NUM_CH;   // channels
   bufParamsIn.dim_y     = NUM_SAMP; // samples
   bufParamsIn.stride_y  = NUM_CH * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = NUM_CH;
   bufParamsOut.dim_y     = OUT_SAMP;
   bufParamsOut.stride_y  = NUM_CH * sizeof(float);

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.upFactor     = UP_FACTOR;
   kerInitArgs.phase        = PHASE;
   kerInitArgs.isInterleave = 1;

   status = AUDIOLIB_upSample_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_upSample_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_upSample_exec_checkParams(handle, inInter, out);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_upSample_exec(handle, inInter, out);

   printf("\n INTERLEAVED (upFactor=%d, phase=%d) \n", UP_FACTOR, PHASE);
   for (size_t r = 0; r < OUT_SAMP; r++) {
      printf("\n samp%2zu: ", r);
      for (size_t c = 0; c < NUM_CH; c++) {
         printf("%6g ", out[r * NUM_CH + c]);
      }
   }
   printf("\n\n");

   free(handle);
   return 0;
}
