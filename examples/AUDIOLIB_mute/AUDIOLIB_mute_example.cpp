// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_mute.h"
#include "audiolib.h"
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

/******************************************************************************/
/* */
/* Defines & Constants                                                        */
/* */
/******************************************************************************/
#define NUM_CHANNELS (2)
#define NUM_SAMPLES (16)
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
      0.10f, 0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f,
      0.18f, 0.19f, 0.20f, 0.21f, 0.22f, 0.23f, 0.24f, 0.25f,
      // Channel 1
     -0.10f,-0.11f,-0.12f,-0.13f,-0.14f,-0.15f,-0.16f,-0.17f,
     -0.18f,-0.19f,-0.20f,-0.21f,-0.22f,-0.23f,-0.24f,-0.25f
   };

   float outBuf[NUM_CHANNELS * NUM_SAMPLES] = {0.0f};

   // Control flag for muting all channels
   // 1 = mute, 0 = unmute
   int32_t isMute = 1;

   // clang-format on

   // Handles and structs for call to kernel
   AUDIOLIB_STATUS        status;
   AUDIOLIB_mute_InitArgs kerInitArgs; // Struct name changed to _mute_
   AUDIOLIB_mute_SetArgs  kerSetArgs;  // Struct name changed to _mute_

   // Setup runtime-settable parameters
   kerSetArgs.isMute   = isMute;
   kerSetArgs.fadeTime = 5.0f; // Fade time of 5.0 ms
   kerSetArgs.fadeType = 0;    // Use AUDIOLIB_MUTE_FADE_TYPE_LINEAR

   // Setup initialization parameters
   kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_NATC; // Using Natural C for simplicity
   kerInitArgs.samplingRate  = SAMPLING_RATE;
   kerInitArgs.isInterleaved = 0; // Deinterleaved data format

   // Get handle size and allocate memory
   int32_t               handleSize = AUDIOLIB_mute_getHandleSize(&kerInitArgs);
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
      status = AUDIOLIB_mute_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   }

   // Initialize the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_mute_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   }

   // Set runtime parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_mute_set(handle, &kerSetArgs);
   }

   // Check exec parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_mute_exec_checkParams(handle, inBuf, outBuf);
   }

   // Execute the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_mute_exec(handle, inBuf, outBuf);
   }

   // Print results
   printf("Output Buffer Contents (Status: %d)\n", status);
   printf("Mute Flag: %f, Fade Time: %.1f ms, Fade Type: %d\n", kerSetArgs.isMute, kerSetArgs.fadeTime,
          kerSetArgs.fadeType);
   printf("----------------------------------------\n");
   for (size_t ch = 0; ch < NUM_CHANNELS; ch++) {
      printf("Channel %zu:\n", ch);
      for (size_t s = 0; s < NUM_SAMPLES; s++) {
         // Print 8 samples per line for readability
         printf("%12.8f, ", outBuf[ch * NUM_SAMPLES + s]);
         if ((s + 1) % 8 == 0) {
            printf("\n");
         }
      }
      printf("\n");
   }

   free(handle);
   return 0;
}
