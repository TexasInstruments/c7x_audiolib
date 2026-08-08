// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/******************************************************************************/
/*                                                                            */
/* Includes                                                                   */
/*                                                                            */
/******************************************************************************/

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_gainSmoothing.h"
#include "audiolib.h"
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

/******************************************************************************/
/*                                                                            */
/* Defines & Constants                                                        */
/*                                                                            */
/******************************************************************************/

static const uint32_t NUM_CHANNELS     = 2U;
static const uint32_t NUM_SAMPLES      = 16U;
static const uint32_t NUM_ELEMS        = NUM_CHANNELS * NUM_SAMPLES;
static const uint32_t SAMPLES_PER_LINE = 8U;
static const float    SAMPLING_RATE    = 48000.0f;

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{
   // clang-format off

   // Setup input and output buffers (deinterleaved format)
   // Format: [ch0_s0, ch0_s1, ..., ch1_s0, ch1_s1, ...]
   float inBuf[NUM_ELEMS] = {
      // Channel 0
      0.10f, 0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f,
      0.18f, 0.19f, 0.20f, 0.21f, 0.22f, 0.23f, 0.24f, 0.25f,
      // Channel 1
     -0.10f,-0.11f,-0.12f,-0.13f,-0.14f,-0.15f,-0.16f,-0.17f,
     -0.18f,-0.19f,-0.20f,-0.21f,-0.22f,-0.23f,-0.24f,-0.25f
   };

   float outBuf[NUM_ELEMS] = {0.0f};

   // clang-format on

   // Handles and structs for call to kernel
   AUDIOLIB_STATUS                 status = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainSmoothing_InitArgs kerInitArgs;
   AUDIOLIB_gainSmoothing_SetArgs  kerSetArgs;

   // Setup runtime-settable parameters
   kerSetArgs.currentGain = 1.0f;
   kerSetArgs.targetGain  = 3.0f;
   kerSetArgs.fadeTime    = 1.0f; // Fade time of 1.0 ms
   kerSetArgs.fadeType    = static_cast<uint8_t>(AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC);

   // Setup initialization parameters
   kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_NATC; // Using Natural C for simplicity
   kerInitArgs.samplingRate  = SAMPLING_RATE;
   kerInitArgs.isInterleaved = 0U; // Deinterleaved data format

   // Get handle size and allocate memory
   const int32_t         handleSize = AUDIOLIB_gainSmoothing_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = NULL;

   if (handleSize > 0) {
      handle = malloc(static_cast<size_t>(handleSize));
   }

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams2D_t bufParamsOut;

   // Fill in input and output buffer parameters for deinterleaved data
   bufParamsIn.data_type = static_cast<uint32_t>(AUDIOLIB_FLOAT32);
   bufParamsIn.dim_x     = NUM_SAMPLES;
   bufParamsIn.dim_y     = NUM_CHANNELS;
   bufParamsIn.stride_y  = static_cast<int32_t>(NUM_SAMPLES * static_cast<uint32_t>(sizeof(float)));

   bufParamsOut.data_type = static_cast<uint32_t>(AUDIOLIB_FLOAT32);
   bufParamsOut.dim_x     = NUM_SAMPLES;
   bufParamsOut.dim_y     = NUM_CHANNELS;
   bufParamsOut.stride_y  = static_cast<int32_t>(NUM_SAMPLES * static_cast<uint32_t>(sizeof(float)));

   // Check init parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   }

   // Initialize the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   }

   // Set runtime parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainSmoothing_set(handle, &kerSetArgs);
   }

   // Check exec parameters
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainSmoothing_exec_checkParams(handle, inBuf, outBuf);
   }

   // Execute the kernel
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainSmoothing_exec(handle, inBuf, outBuf);
   }

   // Print results
   (void) printf("Output Buffer Contents (Status: %" PRId32 ")\n", static_cast<int32_t>(status));
   (void) printf("Target Gain: %f, Fade Time: %.1f ms, Fade Type: %" PRIu32 "\n",
                 static_cast<double>(kerSetArgs.targetGain), static_cast<double>(kerSetArgs.fadeTime),
                 static_cast<uint32_t>(kerSetArgs.fadeType));
   (void) printf("----------------------------------------\n");
   for (uint32_t ch = 0U; ch < NUM_CHANNELS; ch++) {
      (void) printf("Channel %" PRIu32 ":\n", ch);
      for (uint32_t s = 0U; s < NUM_SAMPLES; s++) {
         // Print 8 samples per line for readability
         (void) printf("%12.8f, ", static_cast<double>(outBuf[(ch * NUM_SAMPLES) + s]));
         if (((s + 1U) % SAMPLES_PER_LINE) == 0U) {
            (void) printf("\n");
         }
      }
      (void) printf("\n");
   }

   free(handle);

   return (status == AUDIOLIB_SUCCESS) ? 0 : 1;
}
