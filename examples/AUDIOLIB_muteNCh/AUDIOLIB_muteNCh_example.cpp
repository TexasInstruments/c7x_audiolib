// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"

#include "AUDIOLIB_muteNCh.h"
#include "audiolib.h"
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#define NUM_CHANNELS (4)
#define NUM_SAMPLES (8)
#define SAMPLING_RATE (48000)

/******************************************************************************/
/* */
/* main                                                                       */
/* */
/******************************************************************************/
int main(void)
{
   // clang-format off

   // Setup input and output buffers (deinterleaved format)
   // Format: [ch0_s0, ch0_s1, ..., ch1_s0, ch1_s1, ...]
   float inBuf[] = {
      // Channel 0
      0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f,
      // Channel 1
      0.11f, 0.22f, 0.33f, 0.44f, 0.55f, 0.66f, 0.77f, 0.88f,
      // Channel 2
      -0.1f, -0.2f, -0.3f, -0.4f, -0.5f, -0.6f, -0.7f, -0.8f,
      // Channel 3
      -0.11f, -0.22f, -0.33f, -0.44f, -0.55f, -0.66f, -0.77f, -0.88f
   };

   float outBuf[NUM_CHANNELS * NUM_SAMPLES] = {0.0f};

   // Control array for muting channels
   // 1 = mute this channel, 0 = unmute this channel
   int32_t isMuteArray[NUM_CHANNELS] = {1, 0, 1, 0};

   // clang-format on

   // Handles and structs for call to kernel
   AUDIOLIB_STATUS           status;
   AUDIOLIB_muteNCh_InitArgs kerInitArgs;
   AUDIOLIB_muteNCh_SetArgs  kerSetArgs;

   // Setup runtime-settable parameters
   kerSetArgs.isMute   = isMuteArray;
   kerSetArgs.fadeTime = 1.0f; // Fade time of 1.0 ms
   kerSetArgs.fadeType = 0;    // Use AUDIOLIB_MUTE_FADE_TYPE_LINEAR

   // Setup initialization parameters
   kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.samplingRate  = SAMPLING_RATE;
   kerInitArgs.isInterleaved = 0; // Deinterleaved data format
   kerInitArgs.numChannels   = NUM_CHANNELS;

   int32_t               handleSize = AUDIOLIB_muteNCh_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   // Fill in input and output buffer parameters for deinterleaved data
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = NUM_SAMPLES;
   bufParamsIn.dim_y     = NUM_CHANNELS;
   bufParamsIn.stride_y  = NUM_SAMPLES * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = NUM_SAMPLES;
   bufParamsOut.dim_y     = NUM_CHANNELS;
   bufParamsOut.stride_y  = NUM_SAMPLES * sizeof(float);

   status = AUDIOLIB_SUCCESS;

   // Check init parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   }

   // Initialize the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   }

   // Set runtime parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_muteNCh_set(handle, &kerSetArgs);
   }

   // Check exec parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_muteNCh_exec_checkParams(handle, inBuf, outBuf);
   }

   // Execute the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_muteNCh_exec(handle, inBuf, outBuf);
   }

   // Print results
   printf("Output Buffer Contents (Status: %d)\n", status);
   printf("----------------------------------------\n");
   for (size_t ch = 0; ch < NUM_CHANNELS; ch++) {
      printf("Channel %zu (Mute=%d):\n", ch, isMuteArray[ch]);
      for (size_t s = 0; s < NUM_SAMPLES; s++) {
         printf("%12.8f, ", outBuf[ch * NUM_SAMPLES + s]);
      }
      printf("\n\n");
   }

   free(handle);
   return 0;
}
