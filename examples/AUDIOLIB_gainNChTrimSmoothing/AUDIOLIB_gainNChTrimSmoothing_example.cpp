// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <malloc.h>
#include <stdint.h>

#define NUM_CHANNELS (2)
#define NUM_SAMPLES  (8)
#define NUM_FRAMES   (3)

/* Input is all ones, so every printed output sample is the gain applied to it. */
static void runFrames(AUDIOLIB_kernelHandle handle, float *in, float *out, int32_t numFrames)
{
   for (int32_t frame = 0; frame < numFrames; frame++) {
      AUDIOLIB_STATUS status = AUDIOLIB_gainNChTrimSmoothing_exec_checkParams(handle, in, out);

      if (status == AUDIOLIB_SUCCESS)
         status = AUDIOLIB_gainNChTrimSmoothing_exec(handle, in, out);

      if (status != AUDIOLIB_SUCCESS) {
         printf("exec failed, status = %d\n", status);
         return;
      }

      printf("\n frame %d", frame);
      for (int32_t ch = 0; ch < NUM_CHANNELS; ch++) {
         printf("\n   ch%d: ", ch);
         for (int32_t s = 0; s < NUM_SAMPLES; s++) {
            printf("%9.6f, ", out[ch * NUM_SAMPLES + s]);
         }
      }
      printf("\n");
   }
}

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{
   AUDIOLIB_STATUS                        status = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_InitArgs kerInitArgs;
   AUDIOLIB_gainNChTrimSmoothing_SetArgs  kerSetArgs;

   // Planar float buffers: NUM_CHANNELS rows of NUM_SAMPLES samples, 64-byte aligned
   float *in  = (float *) memalign(AUDIOLIB_L2DATA_ALIGNMENT, NUM_CHANNELS * NUM_SAMPLES * sizeof(float));
   float *out = (float *) memalign(AUDIOLIB_L2DATA_ALIGNMENT, NUM_CHANNELS * NUM_SAMPLES * sizeof(float));

   for (int32_t i = 0; i < NUM_CHANNELS * NUM_SAMPLES; i++) {
      in[i]  = 1.0f;
      out[i] = 0.0f;
   }

   kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.samplingRate  = 8000.0f;
   kerInitArgs.isInterleaved = 0;

   // The handle holds per-channel state read with full-width vector loads: align it
   int32_t               handleSize = AUDIOLIB_gainNChTrimSmoothing_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = memalign(AUDIOLIB_L2DATA_ALIGNMENT, handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   // fill in input and output buffer parameters (planar: dim_x = samples, dim_y = channels)
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = NUM_SAMPLES;
   bufParamsIn.dim_y     = NUM_CHANNELS;
   bufParamsIn.stride_y  = NUM_SAMPLES * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = NUM_SAMPLES;
   bufParamsOut.dim_y     = NUM_CHANNELS;
   bufParamsOut.stride_y  = NUM_SAMPLES * sizeof(float);

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_gainNChTrimSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // SYMMETRIC: start both channels at unity and ramp to master * trim over 2 ms
   // (16 samples at 8 kHz, i.e. two frames), constant dB per sample
   float initialGain[NUM_CHANNELS] = {1.0f, 1.0f};
   float trimGain[NUM_CHANNELS]    = {0.5f, 0.25f};

   kerSetArgs.initialGain = initialGain;
   kerSetArgs.trimGain    = trimGain;
   kerSetArgs.masterGain  = 1.0f;
   kerSetArgs.fadeTime    = 2.0f;
   kerSetArgs.fadeType    = AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SYMMETRIC;

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_gainNChTrimSmoothing_set(handle, &kerSetArgs);

   if (status == AUDIOLIB_SUCCESS) {
      printf("\n SYMMETRIC fade, 2 ms: ch0 1.0 -> 0.5, ch1 1.0 -> 0.25 \n");
      runFrames(handle, in, out, NUM_FRAMES);
   }

   // SMOOTH: keep the running gains (initialGain NULL) and approach a new master gain
   // with a 1 ms time constant; the gain never snaps to the target
   float trimGainSmooth[NUM_CHANNELS] = {1.0f, 1.0f};

   kerSetArgs.initialGain = NULL;
   kerSetArgs.trimGain    = trimGainSmooth;
   kerSetArgs.masterGain  = 0.8f;
   kerSetArgs.fadeTime    = 1.0f;
   kerSetArgs.fadeType    = AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH;

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_gainNChTrimSmoothing_set(handle, &kerSetArgs);

   if (status == AUDIOLIB_SUCCESS) {
      printf("\n SMOOTH fade, tau 1 ms: both channels -> 0.8 \n");
      runFrames(handle, in, out, NUM_FRAMES);
   }

   if (status != AUDIOLIB_SUCCESS)
      printf("\n gainNChTrimSmoothing example failed, status = %d \n", status);

   printf("\n\n");
   free(in);
   free(out);
   free(handle);
   return 0;
}
