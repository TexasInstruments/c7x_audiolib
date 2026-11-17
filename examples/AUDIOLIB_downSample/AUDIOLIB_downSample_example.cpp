// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_downSample.h"
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Downsampler = integer-factor decimation: keep every downSampleRatio-th sample,
// starting at offset `phase`. There is NO anti-aliasing filter (use AUDIOLIB_ssrc
// for filtered rate conversion).
//
// This example uses the planar (non-interleaved) layout: each row is one channel,
// dim_x = samples per channel, dim_y = channels.
//
//   channel 0 = [ 1  2  3  4  5  6  7  8  9 ]
//   channel 1 = [11 12 13 14 15 16 17 18 19 ]
//
// With downSampleRatio = 3 and phase = 0 the kept sample indices are 0, 3, 6, so:
//
//   channel 0 -> [ 1  4  7 ]
//   channel 1 -> [11 14 17 ]

#define NUM_CHANNELS      (2)
#define NUM_SAMPLES       (9)
#define DOWNSAMPLE_RATIO  (3)
#define PHASE             (0)

// Output samples per channel = ceil((NUM_SAMPLES - PHASE) / DOWNSAMPLE_RATIO)
#define OUT_SAMPLES       (((NUM_SAMPLES - PHASE) + DOWNSAMPLE_RATIO - 1) / DOWNSAMPLE_RATIO)

static float input[NUM_CHANNELS * NUM_SAMPLES];
static float output[NUM_CHANNELS * OUT_SAMPLES];

static void initializeInputData(void)
{
   // Channel ch holds [ch*10 + 1, ch*10 + 2, ...].
   for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      for (int s = 0; s < NUM_SAMPLES; s++) {
         input[ch * NUM_SAMPLES + s] = (float) (ch * 10 + s + 1);
      }
   }
}

static void printPlanar(const char *title, const float *buf, int channels, int samples)
{
   printf("\n%s (%d channels x %d samples):\n", title, channels, samples);
   for (int ch = 0; ch < channels; ch++) {
      printf("  Channel %d: ", ch);
      for (int s = 0; s < samples; s++) {
         printf("%5.0f ", buf[ch * samples + s]);
      }
      printf("\n");
   }
}

int main(void)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   printf("==============================================\n");
   printf("AUDIOLIB downSample Example\n");
   printf("==============================================\n");
   printf("Configuration:\n");
   printf("  - Input channels:     %d\n", NUM_CHANNELS);
   printf("  - Samples per channel: %d\n", NUM_SAMPLES);
   printf("  - Downsample ratio:   %d\n", DOWNSAMPLE_RATIO);
   printf("  - Phase:              %d\n", PHASE);
   printf("  - Output samples:     %d\n", OUT_SAMPLES);
   printf("  - Format: Non-interleaved (planar)\n");
   printf("==============================================\n");

   initializeInputData();
   printPlanar("Input Buffer", input, NUM_CHANNELS, NUM_SAMPLES);

   memset(output, 0, sizeof(output));

   // Init arguments
   AUDIOLIB_downSample_InitArgs kerInitArgs;
   memset(&kerInitArgs, 0, sizeof(kerInitArgs));
   kerInitArgs.funcStyle       = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.isInterleave    = 0; // planar: channels x samples
   kerInitArgs.downSampleRatio = DOWNSAMPLE_RATIO;
   kerInitArgs.phase           = PHASE;

   // Allocate the kernel handle
   int32_t handleSize = AUDIOLIB_downSample_getHandleSize(&kerInitArgs);
   printf("\nAllocated handle size: %d bytes\n", handleSize);

   AUDIOLIB_kernelHandle handle = (AUDIOLIB_kernelHandle) malloc(handleSize);
   if (handle == NULL) {
      printf("Error: Failed to allocate handle\n");
      return -1;
   }

   // Input buffer parameters (planar: dim_x = samples, dim_y = channels)
   AUDIOLIB_bufParams2D_t bufParamsIn;
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = NUM_SAMPLES;
   bufParamsIn.dim_y     = NUM_CHANNELS;
   bufParamsIn.stride_y  = NUM_SAMPLES * sizeof(float);

   // Output buffer parameters (dim_x = decimated samples, dim_y = channels)
   AUDIOLIB_bufParams2D_t bufParamsOut;
   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = OUT_SAMPLES;
   bufParamsOut.dim_y     = NUM_CHANNELS;
   bufParamsOut.stride_y  = OUT_SAMPLES * sizeof(float);

   // Validate init parameters
   printf("\nValidating parameters...\n");
   status = AUDIOLIB_downSample_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: init_checkParams failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Parameter validation successful!\n");

   // Initialize the kernel
   printf("\nInitializing kernel...\n");
   status = AUDIOLIB_downSample_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: init failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Kernel initialization successful!\n");

   // Validate execution parameters
   status = AUDIOLIB_downSample_exec_checkParams(handle, (const void *) input, (const void *) output);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: exec_checkParams failed with status %d\n", status);
      free(handle);
      return -1;
   }

   // Execute the kernel
   printf("\nExecuting kernel...\n");
   status = AUDIOLIB_downSample_exec(handle, (void *) input, (void *) output);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: exec failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Kernel execution successful!\n");

   printPlanar("Output Buffer", output, NUM_CHANNELS, OUT_SAMPLES);

   // Verify against the expected decimation result
   int fail = 0;
   for (int ch = 0; ch < NUM_CHANNELS; ch++) {
      for (int s = 0; s < OUT_SAMPLES; s++) {
         float expected = input[ch * NUM_SAMPLES + (PHASE + s * DOWNSAMPLE_RATIO)];
         if (output[ch * OUT_SAMPLES + s] != expected) {
            printf("  Mismatch ch %d sample %d: got %.0f expected %.0f\n", ch, s,
                   output[ch * OUT_SAMPLES + s], expected);
            fail = 1;
         }
      }
   }

   free(handle);

   printf("\n==============================================\n");
   printf("downSample Example Complete (Status: %d, %s)\n", status, fail ? "FAIL" : "PASS");
   printf("==============================================\n");

   return (status == AUDIOLIB_SUCCESS && !fail) ? 0 : -1;
}
