// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_concat.h"
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NUM_INPUTS (4)  // Number of input buffers to aggregate
#define NUM_SAMPLES (8) // Samples per channel (same for all inputs)
#define IN_CHANNELS (1) // Channels per input buffer
#define TOTAL_CHANNELS (NUM_INPUTS * IN_CHANNELS)

// Input buffers: 4 inputs, each with 1 channel x 8 samples
static float input0[1 * NUM_SAMPLES];
static float input1[1 * NUM_SAMPLES];
static float input2[1 * NUM_SAMPLES];
static float input3[1 * NUM_SAMPLES];

// Array of pointers to input buffers
static void* pInputs[NUM_INPUTS] = {(void*) input0, (void*) input1, (void*) input2, (void*) input3};

// Output buffer: 4 channels x 8 samples (non-interleaved)
static float output[TOTAL_CHANNELS * NUM_SAMPLES];

static void initializeInputData(void)
{
   // Fill input0: [0, 1, 2, 3, 4, 5, 6, 7]
   for (int s = 0; s < NUM_SAMPLES; s++) {
      input0[s] = (float) s;
   }

   // Fill input1: [10, 11, 12, 13, 14, 15, 16, 17]
   for (int s = 0; s < NUM_SAMPLES; s++) {
      input1[s] = (float) (s + 10);
   }

   // Fill input2: [20, 21, 22, 23, 24, 25, 26, 27]
   for (int s = 0; s < NUM_SAMPLES; s++) {
      input2[s] = (float) (s + 20);
   }

   // Fill input3: [30, 31, 32, 33, 34, 35, 36, 37]
   for (int s = 0; s < NUM_SAMPLES; s++) {
      input3[s] = (float) (s + 30);
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
   printf("  - Channels per input: %d\n", IN_CHANNELS);
   printf("  - Samples per channel: %d\n", NUM_SAMPLES);
   printf("  - Total output channels: %d\n", TOTAL_CHANNELS);
   printf("  - Format: Non-interleaved\n");
   printf("==============================================\n\n");

   // Initialize test data
   initializeInputData();

   // Setup initialization arguments
   AUDIOLIB_concat_InitArgs kerInitArgs;
   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.inChannels   = IN_CHANNELS;
   kerInitArgs.numInputs    = NUM_INPUTS;
   kerInitArgs.isInterleave = 0; // Non-interleaved

   // Get handle size and allocate
   int32_t handleSize = AUDIOLIB_concat_getHandleSize(&kerInitArgs);
   printf("Allocated handle size: %d bytes\n\n", handleSize);

   AUDIOLIB_kernelHandle handle = (AUDIOLIB_kernelHandle) malloc(handleSize);
   if (handle == NULL) {
      printf("Error: Failed to allocate handle\n");
      return -1;
   }

   // Setup buffer parameters for each input (all identical)
   AUDIOLIB_bufParams2D_t bufParamsIn[NUM_INPUTS];
   for (int i = 0; i < NUM_INPUTS; i++) {
      bufParamsIn[i].data_type = AUDIOLIB_FLOAT32;
      bufParamsIn[i].dim_x     = NUM_SAMPLES; // Samples (columns)
      bufParamsIn[i].dim_y     = IN_CHANNELS; // Channels (rows)
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
   printf("  Input0[0:3] = %.0f, %.0f, %.0f\n", input0[0], input0[1], input0[2]);
   printf("  Output ch0[0:3] = %.0f, %.0f, %.0f\n", output[0], output[1], output[2]);
   printf("  Input1[0:3] = %.0f, %.0f, %.0f\n", input1[0], input1[1], input1[2]);
   printf("  Output ch1[0:3] = %.0f, %.0f, %.0f\n", output[NUM_SAMPLES], output[NUM_SAMPLES + 1],
          output[NUM_SAMPLES + 2]);

   // Cleanup
   free(handle);

   printf("\n==============================================\n");
   printf("Concat Example Complete (Status: %d)\n", status);
   printf("==============================================\n");

   return (status == AUDIOLIB_SUCCESS) ? 0 : -1;
}
