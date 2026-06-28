// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "audiolib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
/* */
/* main                                                                       */
/* */
/******************************************************************************/

int main(void)
{
   // -------------------------------------------------------------------------
   // 1. Define Data and Dimensions
   // -------------------------------------------------------------------------
   uint32_t numSamples   = 14;
   uint32_t filterLength = 4;
   uint32_t numChannels  = 1;

   // Input (x): Noisy Signal (Target + Noise)
   float pIn[] = {0.80f, 0.25f, 0.60f, 0.65f, 0.30f, 0.45f, 0.70f, 0.35f, 0.15f, 1.10f, 0.55f, 1.05f, 0.90f, 0.20f};

   // Desired (d): Clean Reference Signal
   float pInDesired[] = {0.71f, 0.13f, 0.50f, 0.54f, 0.19f, 0.38f, 0.56f,
                         0.24f, 0.05f, 0.99f, 0.47f, 0.97f, 0.79f, 0.06f};

   // Output (y): Estimated Clean Signal
   float pOut[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

   // -------------------------------------------------------------------------
   // 2. Allocate Internal Buffers (Specific to NLMS)
   // -------------------------------------------------------------------------

   // State Buffer: Must be Power-of-2 size >= (filterLength + numSamples)
   // 14 + 4 = 18 -> Next Power of 2 is 32.
   float pStateBuffer[32] = {0};

   // Scratch Buffer: Used for accumulation (Size: numChannels * stride)
   float pScratchBuffer[16] = {0};

   // Coefficients Buffer: Stores the adaptive weights (w)
   float pCoefficients[4] = {0};

   // -------------------------------------------------------------------------
   // 3. Kernel Initialization
   // -------------------------------------------------------------------------
   AUDIOLIB_STATUS        status;
   AUDIOLIB_nlms_InitArgs kerInitArgs;

   // Fill in initialization arguments
   kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_OPTIMIZED; // or AUDIOLIB_FUNCTION_NATC
   kerInitArgs.filterLength  = filterLength;
   kerInitArgs.stepSize      = 0.1f; // Learning rate (mu)
   kerInitArgs.numChannels   = numChannels;
   kerInitArgs.isInterleaved = 0; // 0 = Non-interleaved (Planar)

   // Allocate Handle
   int32_t               handleSize = AUDIOLIB_nlms_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   // Configure Buffer Parameters (NLMS uses 2D params)
   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   // Setup Input Buffer Info
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = numSamples;  // Samples per channel
   bufParamsIn.dim_y     = numChannels; // Number of channels
   bufParamsIn.stride_y  = numSamples * sizeof(float);

   // Setup Output Buffer Info
   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = numSamples;
   bufParamsOut.dim_y     = numChannels;
   bufParamsOut.stride_y  = numSamples * sizeof(float);

   status = AUDIOLIB_SUCCESS;

   // init checkparams

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_nlms_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // -------------------------------------------------------------------------
   // 4. Kernel Execution
   // -------------------------------------------------------------------------

   // exec checkparams

   // exec
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_nlms_exec(handle,
                                  pIn,            // Input (Noisy)
                                  pInDesired,     // Desired (Clean Ref)
                                  pStateBuffer,   // Circular State
                                  pScratchBuffer, // Accumulator
                                  pCoefficients,  // Filter Weights
                                  pOut);          // Output (Estimated)
   }

   // -------------------------------------------------------------------------
   // 5. Print Results
   // -------------------------------------------------------------------------
   printf("  %-10s | %-10s | %-10s | %-10s \n", "Input(x)", "Desired(d)", "Output(y)", "Error(d-y)");
   printf("----------------------------------------------------------------\n");

   for (size_t i = 0; i < numSamples; i++) {
      float error = pInDesired[i] - pOut[i];
      printf("%10.5f | %10.5f | %10.5f | %10.5f\n", pIn[i], pInDesired[i], pOut[i], error);
   }

   // Cleanup
   free(handle);

   return 0;
}