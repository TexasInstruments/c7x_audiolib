// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_inputAggregator.h"
#include "AUDIOLIB_types.h"
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************/
/*                                                                            */
/* Configuration                                                              */
/*                                                                            */
/******************************************************************************/

#define NUM_INPUTS (3)   // Number of input buffers to aggregate
#define NUM_SAMPLES (64) // Samples per channel (same for all inputs)

// Channel counts for each input buffer
static int32_t inChannels[NUM_INPUTS] = {2, 4, 2}; // Total: 8 channels

// Calculate total channels
#define TOTAL_CHANNELS (2 + 4 + 2) // Sum of inChannels

/******************************************************************************/
/*                                                                            */
/* Input Data (Non-interleaved format: channels x samples)                    */
/*                                                                            */
/******************************************************************************/

// Input 0: 2 channels x NUM_SAMPLES
static float input0[2 * NUM_SAMPLES];

// Input 1: 4 channels x NUM_SAMPLES
static float input1[4 * NUM_SAMPLES];

// Input 2: 2 channels x NUM_SAMPLES
static float input2[2 * NUM_SAMPLES];

// Array of pointers to input buffers
static void* pInputs[NUM_INPUTS] = {(void*) input0, (void*) input1, (void*) input2};

// Output buffer: TOTAL_CHANNELS x NUM_SAMPLES
static float output[TOTAL_CHANNELS * NUM_SAMPLES];

/******************************************************************************/
/*                                                                            */
/* Helper Functions                                                           */
/*                                                                            */
/******************************************************************************/

/**
 * @brief Initialize input buffers with test data
 */
static void initializeInputData(void)
{
   // Fill input0 (2 channels)
   for (int ch = 0; ch < 2; ch++) {
      for (int s = 0; s < NUM_SAMPLES; s++) {
         input0[ch * NUM_SAMPLES + s] = (float) (ch * 100 + s) * 0.01f;
      }
   }

   // Fill input1 (4 channels)
   for (int ch = 0; ch < 4; ch++) {
      for (int s = 0; s < NUM_SAMPLES; s++) {
         input1[ch * NUM_SAMPLES + s] = (float) (ch * 100 + s + 200) * 0.01f;
      }
   }

   // Fill input2 (2 channels)
   for (int ch = 0; ch < 2; ch++) {
      for (int s = 0; s < NUM_SAMPLES; s++) {
         input2[ch * NUM_SAMPLES + s] = (float) (ch * 100 + s + 600) * 0.01f;
      }
   }
}

/**
 * @brief Print buffer contents for verification
 */
static void printOutput(void)
{
   printf("\nAggregated Output (first 8 samples per channel):\n");
   printf("================================================\n");

   for (int ch = 0; ch < TOTAL_CHANNELS; ch++) {
      printf("Channel %d: ", ch);
      for (int s = 0; s < 8 && s < NUM_SAMPLES; s++) {
         printf("%7.2f ", output[ch * NUM_SAMPLES + s]);
      }
      printf("...\n");
   }
}

/******************************************************************************/
/*                                                                            */
/* Main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   printf("==============================================\n");
   printf("AUDIOLIB Input Aggregator Example\n");
   printf("==============================================\n");
   printf("Configuration:\n");
   printf("  - Number of inputs: %d\n", NUM_INPUTS);
   printf("  - Samples per channel: %d\n", NUM_SAMPLES);
   printf("  - Channel counts: [%d, %d, %d]\n", inChannels[0], inChannels[1], inChannels[2]);
   printf("  - Total output channels: %d\n", TOTAL_CHANNELS);
   printf("  - Input format: Non-interleaved\n");
   printf("  - Output format: Non-interleaved\n");
   printf("==============================================\n\n");

   // Initialize test data
   initializeInputData();

   // Setup initialization arguments
   AUDIOLIB_inputAggregator_InitArgs kerInitArgs;
   kerInitArgs.funcStyle          = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.inChannels         = inChannels;
   kerInitArgs.numInputs          = NUM_INPUTS;
   kerInitArgs.totalInChannels    = TOTAL_CHANNELS;
   kerInitArgs.isInputInterleave  = 0; // Non-interleaved input
   kerInitArgs.isOutputInterleave = 0; // Non-interleaved output

   // Get handle size and allocate
   int32_t handleSize = AUDIOLIB_inputAggregator_getHandleSize(&kerInitArgs);
   printf("Allocated handle size: %d bytes\n", handleSize);

   AUDIOLIB_kernelHandle handle = (AUDIOLIB_kernelHandle) malloc(handleSize);
   if (handle == NULL) {
      printf("Error: Failed to allocate handle\n");
      return -1;
   }

   // Setup buffer parameters for each input
   AUDIOLIB_bufParams2D_t bufParamsIn[NUM_INPUTS];

   // Input 0: 2 channels
   bufParamsIn[0].data_type = AUDIOLIB_FLOAT32;
   bufParamsIn[0].dim_x     = NUM_SAMPLES;   // Samples (columns)
   bufParamsIn[0].dim_y     = inChannels[0]; // Channels (rows)
   bufParamsIn[0].stride_y  = NUM_SAMPLES * sizeof(float);

   // Input 1: 4 channels
   bufParamsIn[1].data_type = AUDIOLIB_FLOAT32;
   bufParamsIn[1].dim_x     = NUM_SAMPLES;
   bufParamsIn[1].dim_y     = inChannels[1];
   bufParamsIn[1].stride_y  = NUM_SAMPLES * sizeof(float);

   // Input 2: 2 channels
   bufParamsIn[2].data_type = AUDIOLIB_FLOAT32;
   bufParamsIn[2].dim_x     = NUM_SAMPLES;
   bufParamsIn[2].dim_y     = inChannels[2];
   bufParamsIn[2].stride_y  = NUM_SAMPLES * sizeof(float);

   // Setup output buffer parameters
   AUDIOLIB_bufParams2D_t bufParamsOut;
   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = NUM_SAMPLES;    // Samples
   bufParamsOut.dim_y     = TOTAL_CHANNELS; // Total channels
   bufParamsOut.stride_y  = NUM_SAMPLES * sizeof(float);

   // Initialize the kernel
   printf("Initializing input aggregator kernel...\n");

   status = AUDIOLIB_inputAggregator_init_checkParams(handle, bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: init_checkParams failed with status %d\n", status);
      free(handle);
      return -1;
   }

   status = AUDIOLIB_inputAggregator_init(handle, bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: init failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Initialization successful!\n\n");

   // Execute the kernel
   printf("Executing input aggregator...\n");

   status = AUDIOLIB_inputAggregator_exec_checkParams(handle, (const void**) pInputs, (const void*) output);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: exec_checkParams failed with status %d\n", status);
      free(handle);
      return -1;
   }

   status = AUDIOLIB_inputAggregator_exec(handle, pInputs, (void*) output);
   if (status != AUDIOLIB_SUCCESS) {
      printf("Error: exec failed with status %d\n", status);
      free(handle);
      return -1;
   }
   printf("Execution successful!\n");

   // Print results
   printOutput();

   // Verify: first channel of output should match first channel of input0
   printf("\nVerification:\n");
   printf("  Input0[0][0:3]  = %.2f, %.2f, %.2f\n", input0[0], input0[1], input0[2]);
   printf("  Output[0][0:3]  = %.2f, %.2f, %.2f\n", output[0], output[1], output[2]);

   // Channel 2 of output should match first channel of input1
   printf("  Input1[0][0:3]  = %.2f, %.2f, %.2f\n", input1[0], input1[1], input1[2]);
   printf("  Output[2][0:3]  = %.2f, %.2f, %.2f\n", output[2 * NUM_SAMPLES], output[2 * NUM_SAMPLES + 1],
          output[2 * NUM_SAMPLES + 2]);

   // Cleanup
   free(handle);

   printf("\n==============================================\n");
   printf("Input Aggregator Example Complete (Status: %d)\n", status);
   printf("==============================================\n");

   return (status == AUDIOLIB_SUCCESS) ? 0 : -1;
}
