// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <cmath>
#include <cstdlib>
#ifdef C7X
#include <dsplib.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#if defined(_HOST_BUILD) || defined(ARM_A53)
#include <malloc.h>
#endif
#ifdef ARM_A53
#include "../generated/ti_board_config.h"
#include "../generated/ti_board_open_close.h"
#include "../generated/ti_drivers_config.h"
#include "../generated/ti_drivers_open_close.h"
#endif

#ifdef C7X
#define AUDIOLIB_DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif
#ifdef ARM_A53
#define AUDIOLIB_DEBUG_PRINT(fmt, ...) DebugP_log(fmt, ##__VA_ARGS__)
#endif

#ifdef C7X
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
#endif
const float audiolib_ssrc_downsampleStage1[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage1_downsample2x_rfmt.h"
};

#ifdef C7X
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
#endif
const float audiolib_ssrc_downsampleStage2[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage2_downsample2x_rfmt.h"
};

#ifdef C7X
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
#endif
const float audiolib_ssrc_upsampleStage1[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage1_upsample2x_rfmt.h"
};

#ifdef C7X
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
#endif
const float audiolib_ssrc_upsampleStage2[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage2_upsample2x_rfmt.h"
};

int main(void)
{
#ifdef ARM_A53
   int32_t systemstatus = SystemP_SUCCESS;
   /* System initialization */
   System_init();
   /* Board initialization */
   Board_init();
   /* Drivers initialization */
   Drivers_open();
   /* Open drivers */
   systemstatus = Board_driversOpen();
   DebugP_assert(systemstatus == SystemP_SUCCESS);
#endif
   /* Parameters */
   const ssrc_sample_rate_t inputSampleRate  = SSRC_SAMPLE_RATE_48000;
   const ssrc_sample_rate_t outputSampleRate = SSRC_SAMPLE_RATE_96000;
   const uint8_t            numChannels      = 4;
   const uint32_t           inputSampleCount = 64; /* Must be power of 2 for circular buffer */
   const uint32_t           sampleDataType   = AUDIOLIB_FLOAT32;
   const uint8_t            dataFormat       = AUDIOLIB_DATA_FORMAT_INTERLEAVED;
   AUDIOLIB_ssrc_InitArgs   kerInitArgs      = {0};

   /* Buffer parameters */
   AUDIOLIB_bufParams2D_t bufParamsIn = {sampleDataType, numChannels, inputSampleCount, numChannels * sizeof(float)};
   uint32_t outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(inputSampleRate, outputSampleRate, inputSampleCount);
   AUDIOLIB_bufParams2D_t bufParamsOut = {sampleDataType, numChannels, outFrameLength, numChannels * sizeof(float)};

   /* Get circular buffer parameters */
   AUDIOLIB_ssrc_bufferParams_t cbParams;
   AUDIOLIB_STATUS status = AUDIOLIB_ssrc_getCircularBufferParams(inputSampleRate, outputSampleRate, inputSampleCount,
                                                                  numChannels, sampleDataType, &cbParams);

   if (status != AUDIOLIB_SUCCESS) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("Failed to get circular buffer parameters\r\n"));
      return 1;
   }

   /* Allocate buffers with proper alignment for circular buffer */
   float *in = (float *) memalign(cbParams.alignment, cbParams.bufferSize);

   /* Prepare input data (interleaved: each row is a sample, each column is a channel) */
   float    baseFreq   = 1000.0f; /* Base frequency in Hz */
   float    fsin       = (float) AUDIOLIB_ssrc_convertSampleRateToInt(inputSampleRate);
   uint32_t cbElements = cbParams.alignment / sizeof(float);

   /* Fill the circular buffer with sine wave data */
   for (uint32_t s = 0; s < inputSampleCount; ++s) {
      for (uint32_t c = 0; c < numChannels; ++c) {
         uint32_t idx = (s * numChannels + c) % cbElements;
         in[idx]      = sinf(2.0f * 3.14159265f * baseFreq * s / fsin + c * 0.5f);
      }
   }

   /* Allocate output buffer */
   float *out = (float *) memalign(AUDIOLIB_L2DATA_ALIGNMENT, numChannels * outFrameLength * sizeof(float));

   /* Allocate filter coefficients buffer */
   int32_t filtCoeffSizeBytes = AUDIOLIB_ssrc_getFilterCoeffSize(inputSampleRate, outputSampleRate,
                                                                 AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR,
#ifdef C7X
                                                                 0, DSPLIB_MMA_SIZE_32_BIT,
#endif
                                                                 sampleDataType);

   float *filtCoeffs = (float *) memalign(AUDIOLIB_L2DATA_ALIGNMENT, filtCoeffSizeBytes);

   /* Copy filter coefficients */
   status = AUDIOLIB_ssrc_copyFilterCoeffs(
       inputSampleRate, outputSampleRate, AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR,
#ifdef C7X
       0, DSPLIB_MMA_SIZE_32_BIT,
#endif
       sampleDataType, audiolib_ssrc_downsampleStage1, audiolib_ssrc_downsampleStage2, audiolib_ssrc_upsampleStage1,
       audiolib_ssrc_upsampleStage2, filtCoeffSizeBytes, filtCoeffs);

   if (status != AUDIOLIB_SUCCESS) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("Failed to copy filter coefficients\r\n"));
      return 1;
   }

   /* Allocate kernel handle */
   int32_t               handleSize = AUDIOLIB_ssrc_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   /* Initialize kernel arguments */
   kerInitArgs.funcStyle        = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.inputSampleCount = inputSampleCount;
   kerInitArgs.sampleDataType   = sampleDataType;
   kerInitArgs.inputSampleRate  = inputSampleRate;
   kerInitArgs.outputSampleRate = outputSampleRate;
   kerInitArgs.numChannels      = numChannels;
   kerInitArgs.dataFormat       = dataFormat;
   kerInitArgs.bufferFormat     = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
#ifdef C7X
   kerInitArgs.enableMMA = 0;
   kerInitArgs.mmaSize   = DSPLIB_MMA_SIZE_32_BIT;
#endif

   /* Check parameters before initialization */
   status = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("init_checkParams failed with status: %d\r\n"), status);
      free(in);
      free(out);
      free(filtCoeffs);
      free(handle);
      return 1;
   }

   /* Initialize kernel */
   status = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != AUDIOLIB_SUCCESS) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("init failed with status: %d\r\n"), status);
      free(in);
      free(out);
      free(filtCoeffs);
      free(handle);
      return 1;
   }

   /* Reset state */
   status = AUDIOLIB_ssrc_set(handle, AUDIOLIB_SSRC_MODE_RESET, in, NULL);
   if (status != AUDIOLIB_SUCCESS) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("reset failed\r\n"));
      free(in);
      free(out);
      free(filtCoeffs);
      free(handle);
      return 1;
   }

   /* Execute SSRC */
   status = AUDIOLIB_ssrc_exec(handle, in, NULL, filtCoeffs, out);
   if (status != AUDIOLIB_SUCCESS) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("exec failed\r\n"));
      free(in);
      free(out);
      free(filtCoeffs);
      free(handle);
      return 1;
   }

   /* Print results (first channel only) */
   AUDIOLIB_DEBUG_PRINT(const_cast<char *>("SSRC output (first 10 samples, channel 0):\r\n"));
   for (uint32_t i = 0; i < 10 && i < outFrameLength; ++i) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("%10g\r\n"), out[i * numChannels]);
   }

   AUDIOLIB_DEBUG_PRINT(const_cast<char *>("SSRC conversion successful!\r\n"));
   AUDIOLIB_DEBUG_PRINT(const_cast<char *>("Input samples: %d\r\n"), inputSampleCount);
   AUDIOLIB_DEBUG_PRINT(const_cast<char *>("Output samples: %d\r\n"), outFrameLength);
   AUDIOLIB_DEBUG_PRINT(const_cast<char *>("Input sample rate: %d Hz\r\n"),
                        AUDIOLIB_ssrc_convertSampleRateToInt(inputSampleRate));
   AUDIOLIB_DEBUG_PRINT(const_cast<char *>("Output sample rate: %d Hz\r\n"),
                        AUDIOLIB_ssrc_convertSampleRateToInt(outputSampleRate));
   AUDIOLIB_DEBUG_PRINT(const_cast<char *>("Number of channels: %d\r\n"), numChannels);

   /* Free resources */
   free(in);
   free(out);
   free(filtCoeffs);
   free(handle);

#ifdef ARM_A53
   /* Close drivers */
   Board_driversClose();
   /* Close drivers */
   Drivers_close();
   /* Board Deinitialization */
   Board_deinit();
   /* System Deinitialization */
   System_deinit();
#endif
   return status == AUDIOLIB_SUCCESS ? 0 : 1;
}
