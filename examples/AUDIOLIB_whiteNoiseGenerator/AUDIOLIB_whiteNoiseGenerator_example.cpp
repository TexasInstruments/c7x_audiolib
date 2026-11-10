// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_whiteNoiseGenerator.h" // Kernel header
#include "audiolib.h"
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

// Define the frame size for our example: NUM_CHANNELS rows of NUM_SAMPLES samples each
#define NUM_SAMPLES (16)
#define NUM_CHANNELS (4)

/******************************************************************************/
/* */
/* main                                                                       */
/* */
/******************************************************************************/
int main(void)
{
   // clang-format off

   // Setup output buffers for two frames of noise, one row per channel
   float outBuf_Frame1[NUM_CHANNELS][NUM_SAMPLES] = {{0.0f}};
   float outBuf_Frame2[NUM_CHANNELS][NUM_SAMPLES] = {{0.0f}};

   // clang-format on

   // Handles and structs for call to kernel
   AUDIOLIB_STATUS                       status;
   AUDIOLIB_whiteNoiseGenerator_InitArgs kerInitArgs;

   // Setup initialization parameters
   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED; // Use C7x optimized kernel
   kerInitArgs.seed      = 42;                          // One seed for all channels
   kerInitArgs.range     = 1.0f;                        // Output noise in range [-1.0, 1.0]

   int32_t               handleSize = AUDIOLIB_whiteNoiseGenerator_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   if (handle == NULL) {
      printf("Failed to allocate handle memory\n");
      return -1;
   }

   AUDIOLIB_bufParams2D_t bufParamsOut;

   // Fill in output buffer parameters: each channel is a row, rows stride_y bytes apart.
   // Every channel gets its own uncorrelated noise; channel 0 equals the single-channel output.
   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = NUM_SAMPLES;
   bufParamsOut.dim_y     = NUM_CHANNELS;
   bufParamsOut.stride_y  = NUM_SAMPLES * sizeof(float);

   status = AUDIOLIB_SUCCESS;

   // Check init parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_whiteNoiseGenerator_init_checkParams(handle, &bufParamsOut, &kerInitArgs);
   }

   // Initialize the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_whiteNoiseGenerator_init(handle, &bufParamsOut, &kerInitArgs);
   }

   // --- Execute Frame 1 ---

   // Check exec parameters for Frame 1
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_whiteNoiseGenerator_exec_checkParams(handle, outBuf_Frame1);
   }

   // Execute the kernel for Frame 1
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_whiteNoiseGenerator_exec(handle, outBuf_Frame1);
   }

   // --- Execute Frame 2 ---
   // We run exec again on a *different* buffer.
   // The kernel will continue from its internal state, demonstrating
   // state continuity across frames.

   // Check exec parameters for Frame 2
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_whiteNoiseGenerator_exec_checkParams(handle, outBuf_Frame2);
   }

   // Execute the kernel for Frame 2
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_whiteNoiseGenerator_exec(handle, outBuf_Frame2);
   }

   // Print results
   printf("--- Frame 1 Output (Seed=42, %d channels) ---\n", NUM_CHANNELS);
   printf("Status: %d\n", status);
   for (size_t ch = 0; ch < NUM_CHANNELS; ch++) {
      printf("Channel %zu:\n", ch);
      for (size_t s = 0; s < NUM_SAMPLES; s++) {
         printf("%12.8f, ", outBuf_Frame1[ch][s]);
         if ((s + 1) % 8 == 0)
            printf("\n");
      }
   }

   printf("\n--- Frame 2 Output (State Continued) ---\n");
   printf("Status: %d\n", status);
   for (size_t ch = 0; ch < NUM_CHANNELS; ch++) {
      printf("Channel %zu:\n", ch);
      for (size_t s = 0; s < NUM_SAMPLES; s++) {
         printf("%12.8f, ", outBuf_Frame2[ch][s]);
         if ((s + 1) % 8 == 0)
            printf("\n");
      }
   }

   free(handle);
   return 0;
}
