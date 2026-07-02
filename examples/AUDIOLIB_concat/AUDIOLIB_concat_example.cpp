// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_concat.h"
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NUM_INPUTS (3)  // Number of input buffers to aggregate
#define NUM_SAMPLES (8) // Samples per channel (same for all inputs)

// Dynamic channel support: each input buffer may carry a DIFFERENT channel count.
#define IN0_CHANNELS (1)
#define IN1_CHANNELS (2)
#define IN2_CHANNELS (1)
#define TOTAL_CHANNELS (IN0_CHANNELS + IN1_CHANNELS + IN2_CHANNELS)

// Input buffers (non-interleaved, row = channel), each sized to its own channel count
static float input0[IN0_CHANNELS * NUM_SAMPLES];
static float input1[IN1_CHANNELS * NUM_SAMPLES];
static float input2[IN2_CHANNELS * NUM_SAMPLES];

// Array of pointers to input buffers
static void* pInputs[NUM_INPUTS] = {(void*) input0, (void*) input1, (void*) input2};

// Per-input channel counts (drives the dynamic channel handling in the kernel)
static uint32_t inChannels[NUM_INPUTS] = {IN0_CHANNELS, IN1_CHANNELS, IN2_CHANNELS};

// Output buffer: TOTAL_CHANNELS channels x 8 samples (non-interleaved)
static float output[TOTAL_CHANNELS * NUM_SAMPLES];

static void initializeInputData(void)
{
   // input0 ch0: [ 0,  1,  2,  3,  4,  5,  6,  7]
   for (int s = 0; s < NUM_SAMPLES; s++) {
      input0[s] = (float) s;
   }

   // input1 ch0: [10..17], ch1: [20..27]
   for (int c = 0; c < IN1_CHANNELS; c++) {
      for (int s = 0; s < NUM_SAMPLES; s++) {
         input1[c * NUM_SAMPLES + s] = (float) (10 * (c + 1) + s);
      }
   }

   // input2 ch0: [30..37]
   for (int s = 0; s < NUM_SAMPLES; s++) {
      input2[s] = (float) (30 + s);
   }
}

static void printOutput(void)
{
   printf("\nAggregated Output (non-interleaved):\n");
   printf("====================================\n");

   for (int ch = 0; ch < TOTAL_CHANNELS; ch++) {
      printf("Channel %d: ", ch);
      for (int s = 0; s < NUM_SAMPLES; s++) {
         printf("%.0f ", output[ch * NUM_SAMPLES + s]);
      }
      printf("\n");
   }
}

int main(void)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   printf("==============================================\n");
   printf("AUDIOLIB Concat Example\n");
   printf("==============================================\n");
   printf("Configuration:\n");
   printf("  - Number of inputs: %d\n", NUM_INPUTS);
   printf("  - Channels per input: %d, %d, %d\n", IN0_CHANNELS, IN1_CHANNELS, IN2_CHANNELS);
   printf("  - Samples per channel: %d\n", NUM_SAMPLES);
   printf("  - Total output channels: %d\n", TOTAL_CHANNELS);
   printf("  - Format: Non-interleaved\n");
   printf("==============================================\n\n");

   // Initialize test data
   initializeInputData();

   // Setup initialization arguments
   AUDIOLIB_concat_InitArgs kerInitArgs;
   memset(&kerInitArgs, 0, sizeof(kerInitArgs));
   kerInitArgs.funcStyle       = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.inChannels      = inChannels; // Per-input channel counts
   kerInitArgs.numInputs       = NUM_INPUTS;
   kerInitArgs.totalInChannels = TOTAL_CHANNELS;
   kerInitArgs.isInterleave    = 0; // Non-interleaved

   // Get handle size and allocate
   int32_t handleSize = AUDIOLIB_concat_getHandleSize(&kerInitArgs);
   printf("Allocated handle size: %d bytes\n\n", handleSize);

   AUDIOLIB_kernelHandle handle = (AUDIOLIB_kernelHandle) malloc(handleSize);
   if (handle == NULL) {
      printf("Error: Failed to allocate handle\n");
      return -1;
   }

   // Setup buffer parameters for each input (dim_y = that input's channel count)
   AUDIOLIB_bufParams2D_t bufParamsIn[NUM_INPUTS];
   for (int i = 0; i < NUM_INPUTS; i++) {
      bufParamsIn[i].data_type = AUDIOLIB_FLOAT32;
      bufParamsIn[i].dim_x     = NUM_SAMPLES;    // Samples (columns)
      bufParamsIn[i].dim_y     = inChannels[i];  // Channels (rows), per input
      bufParamsIn[i].stride_y  = NUM_SAMPLES * sizeof(float);
   }

   // Setup output buffer parameters
   AUDIOLIB_bufParams2D_t bufParamsOut;
   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = NUM_SAMPLES;    // Samples
   bufParamsOut.dim_y     = TOTAL_CHANNELS; // Total channels
   bufParamsOut.stride_y  = NUM_SAMPLES * sizeof(float);

   // Validate parameters
   printf("Validating parameters...\n");
   status = AUDIOLIB_concat_init_checkParams(handle, bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: init_checkParams failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Parameter validation successful!\n\n");

   // Initialize the kernel
   printf("Initializing kernel...\n");
   status = AUDIOLIB_concat_init(handle, bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: init failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Kernel initialization successful!\n\n");

   // Validate execution parameters
   printf("Validating execution parameters...\n");
   status = AUDIOLIB_concat_exec_checkParams(handle, (const void**) pInputs, (const void*) output);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: exec_checkParams failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Execution parameter validation successful!\n\n");

   // Execute the kernel
   printf("Executing kernel...\n");
   status = AUDIOLIB_concat_exec(handle, pInputs, (void*) output);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: exec failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Kernel execution successful!\n");

   // Print results
   printOutput();

   // Verify correctness
   printf("\nVerification:\n");
   printf("  Input1 ch1[0:3] = %.0f, %.0f, %.0f\n", input1[NUM_SAMPLES], input1[NUM_SAMPLES + 1],
          input1[NUM_SAMPLES + 2]);
   printf("  Output ch2[0:3] = %.0f, %.0f, %.0f\n", output[2 * NUM_SAMPLES], output[2 * NUM_SAMPLES + 1],
          output[2 * NUM_SAMPLES + 2]);

   // Cleanup
   free(handle);

   printf("\n==============================================\n");
   printf("Concat Example Complete (Status: %d)\n", status);
   printf("==============================================\n");

   return (status == AUDIOLIB_SUCCESS) ? 0 : -1;
}
