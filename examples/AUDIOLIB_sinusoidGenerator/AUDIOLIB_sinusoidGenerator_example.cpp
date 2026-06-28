// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_sinusoidGenerator.h"
#include "audiolib.h"
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

/******************************************************************************/
/* */
/* Defines & Constants                                                        */
/* */
/******************************************************************************/
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

   // Setup output buffer
   float outBuf[NUM_SAMPLES] = {0.0f};

   // clang-format on

   // Handles and structs for call to kernel
   AUDIOLIB_STATUS                     status;
   AUDIOLIB_sinusoidGenerator_InitArgs kerInitArgs;
   AUDIOLIB_sinusoidGenerator_SetArgs  kerSetArgs;

   // Setup runtime-settable parameters
   kerSetArgs.startFrequency  = 0.0f;    // Ignored when smoothingTime is 0
   kerSetArgs.targetFrequency = 1000.0f; // 1000 Hz tone
   kerSetArgs.startPhase      = 0.0f;    // Start at 0 degrees
   kerSetArgs.smoothingTime   = 0.0f;    // 0.0 ms = instant frequency, no smoothing
   kerSetArgs.samplingRate    = SAMPLING_RATE;
   kerSetArgs.data_type       = AUDIOLIB_FLOAT32;

   // Setup initialization parameters
   kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_NATC; // Using Natural C for simplicity
   kerInitArgs.executionMode = 0;                      // 0 for scalar, 1 for vector (NATC ignores this)

   // Get handle size and allocate memory
   int32_t               handleSize = AUDIOLIB_sinusoidGenerator_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams1D_t bufParamsOut;

   // Fill in output buffer parameters
   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = NUM_SAMPLES;

   status = AUDIOLIB_SUCCESS;

   // Check init parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_sinusoidGenerator_init_checkParams(handle, &bufParamsOut, &kerInitArgs);
   }

   // Initialize the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_sinusoidGenerator_init(handle, &bufParamsOut, &kerInitArgs);
   }

   // Set runtime parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_sinusoidGenerator_set(handle, &kerSetArgs);
   }

   // Check exec parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_sinusoidGenerator_exec_checkParams(handle, outBuf);
   }

   // Execute the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_sinusoidGenerator_exec(handle, outBuf);
   }

   // Print results
   printf("Output Buffer Contents (Status: %d)\n", status);
   printf("Target Freq: %.1f Hz, Start Phase: %.1f deg, Smooth Time: %.1f ms\n", kerSetArgs.targetFrequency,
          kerSetArgs.startPhase, kerSetArgs.smoothingTime);
   printf("----------------------------------------\n");
   printf("Channel 0:\n");
   for (size_t s = 0; s < NUM_SAMPLES; s++) {
      // Print 8 samples per line for readability
      printf("%12.8f, ", outBuf[s]);
      if ((s + 1) % 8 == 0) {
         printf("\n");
      }
   }
   printf("\n");

   free(handle);
   return 0;
}
