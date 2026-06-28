// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_split.h"
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NUM_OUTPUTS (2)                                // Number of output buffers
#define NUM_SAMPLES (8)                                // Samples per channel
#define TOTAL_IN_CHANNELS (4)                          // Total channels in the input buffer
#define OUT_CHANNELS (TOTAL_IN_CHANNELS / NUM_OUTPUTS) // Channels per output

// Input buffer: 4 channels x 8 samples (non-interleaved, row = channel)
static float input[TOTAL_IN_CHANNELS * NUM_SAMPLES];

// Output buffers: 2 outputs, each 2 channels x 8 samples
static float output0[OUT_CHANNELS * NUM_SAMPLES];
static float output1[OUT_CHANNELS * NUM_SAMPLES];

// Array of pointers to output buffers
static void* pOutputs[NUM_OUTPUTS] = {(void*) output0, (void*) output1};

static void initializeInputData(void)
{
   // Fill channel 0: [ 0,  1,  2,  3,  4,  5,  6,  7]
   // Fill channel 1: [10, 11, 12, 13, 14, 15, 16, 17]
   // Fill channel 2: [20, 21, 22, 23, 24, 25, 26, 27]
   // Fill channel 3: [30, 31, 32, 33, 34, 35, 36, 37]
   for (int ch = 0; ch < TOTAL_IN_CHANNELS; ch++) {
      for (int s = 0; s < NUM_SAMPLES; s++) {
         input[ch * NUM_SAMPLES + s] = (float) (ch * 10 + s);
      }
   }
}

static void printInput(void)
{
   printf("\nInput Buffer (non-interleaved, %d channels x %d samples):\n", TOTAL_IN_CHANNELS, NUM_SAMPLES);
   printf("=========================================================\n");

   for (int ch = 0; ch < TOTAL_IN_CHANNELS; ch++) {
      printf("  Channel %d: ", ch);
      for (int s = 0; s < NUM_SAMPLES; s++) {
         printf("%5.0f ", input[ch * NUM_SAMPLES + s]);
      }
      printf("\n");
   }
}

static void printOutputs(void)
{
   printf("\nSplit Outputs:\n");
   printf("=========================================================\n");

   for (int out = 0; out < NUM_OUTPUTS; out++) {
      float* pBuf = (float*) pOutputs[out];
      printf("  Output %d (%d channels x %d samples):\n", out, OUT_CHANNELS, NUM_SAMPLES);
      for (int ch = 0; ch < OUT_CHANNELS; ch++) {
         printf("    Channel %d: ", ch);
         for (int s = 0; s < NUM_SAMPLES; s++) {
            printf("%5.0f ", pBuf[ch * NUM_SAMPLES + s]);
         }
         printf("\n");
      }
   }
}

int main(void)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   printf("==============================================\n");
   printf("AUDIOLIB Split Example\n");
   printf("==============================================\n");
   printf("Configuration:\n");
   printf("  - Total input channels: %d\n", TOTAL_IN_CHANNELS);
   printf("  - Samples per channel:  %d\n", NUM_SAMPLES);
   printf("  - Number of outputs:    %d\n", NUM_OUTPUTS);
   printf("  - Channels per output:  %d\n", OUT_CHANNELS);
   printf("  - Format: Non-interleaved\n");
   printf("==============================================\n");

   // Initialize test data
   initializeInputData();
   printInput();

   // Clear output buffers
   memset(output0, 0, sizeof(output0));
   memset(output1, 0, sizeof(output1));

   // Setup initialization arguments
   AUDIOLIB_split_InitArgs kerInitArgs;
   memset(&kerInitArgs, 0, sizeof(kerInitArgs));
   kerInitArgs.funcStyle         = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.numOutputs        = NUM_OUTPUTS;
   kerInitArgs.outChannels       = OUT_CHANNELS;
   kerInitArgs.isInputInterleave = 0; // Non-interleaved

   // Get handle size and allocate
   int32_t handleSize = AUDIOLIB_split_getHandleSize(&kerInitArgs);
   printf("\nAllocated handle size: %d bytes\n", handleSize);

   AUDIOLIB_kernelHandle handle = (AUDIOLIB_kernelHandle) malloc(handleSize);
   if (handle == NULL) {
      printf("Error: Failed to allocate handle\n");
      return -1;
   }

   // Setup input buffer parameters (non-interleaved: dim_x = samples, dim_y = channels)
   AUDIOLIB_bufParams2D_t bufParamsIn;
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = NUM_SAMPLES;       // Samples per channel
   bufParamsIn.dim_y     = TOTAL_IN_CHANNELS; // Total channels
   bufParamsIn.stride_y  = NUM_SAMPLES * sizeof(float);

   // Setup output buffer parameters (each output has OUT_CHANNELS channels)
   AUDIOLIB_bufParams2D_t bufParamsOut[NUM_OUTPUTS];
   for (int i = 0; i < NUM_OUTPUTS; i++) {
      bufParamsOut[i].data_type = AUDIOLIB_FLOAT32;
      bufParamsOut[i].dim_x     = NUM_SAMPLES;  // Samples per channel
      bufParamsOut[i].dim_y     = OUT_CHANNELS; // Channels per output
      bufParamsOut[i].stride_y  = NUM_SAMPLES * sizeof(float);
   }

   // Validate parameters
   printf("\nValidating parameters...\n");
   status = AUDIOLIB_split_init_checkParams(handle, &bufParamsIn, bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: init_checkParams failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Parameter validation successful!\n");

   // Initialize the kernel
   printf("\nInitializing kernel...\n");
   status = AUDIOLIB_split_init(handle, &bufParamsIn, bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: init failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Kernel initialization successful!\n");

   // Validate execution parameters
   printf("\nValidating execution parameters...\n");
   status = AUDIOLIB_split_exec_checkParams(handle, (const void*) input, (const void**) pOutputs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: exec_checkParams failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Execution parameter validation successful!\n");

   // Execute the kernel
   printf("\nExecuting kernel...\n");
   status = AUDIOLIB_split_exec(handle, (void*) input, pOutputs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: exec failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Kernel execution successful!\n");

   // Print results
   printOutputs();

   // Verify correctness
   printf("\nVerification:\n");
   printf("  Input ch0[0:2]    = %5.0f, %5.0f, %5.0f\n", input[0], input[1], input[2]);
   printf("  Output0 ch0[0:2]  = %5.0f, %5.0f, %5.0f\n", output0[0], output0[1], output0[2]);
   printf("  Input ch2[0:2]    = %5.0f, %5.0f, %5.0f\n", input[2 * NUM_SAMPLES], input[2 * NUM_SAMPLES + 1],
          input[2 * NUM_SAMPLES + 2]);
   printf("  Output1 ch0[0:2]  = %5.0f, %5.0f, %5.0f\n", output1[0], output1[1], output1[2]);

   // Cleanup
   free(handle);

   printf("\n==============================================\n");
   printf("Split Example Complete (Status: %d)\n", status);
   printf("==============================================\n");

   return (status == AUDIOLIB_SUCCESS) ? 0 : -1;
}
