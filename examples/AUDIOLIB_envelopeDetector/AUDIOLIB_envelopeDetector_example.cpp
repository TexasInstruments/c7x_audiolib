// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0


#include "audiolib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#define NUM_CHANNELS 2
#define SAMPLES_PER_BLOCK 256
#define NUM_BLOCKS 10
#define SAMPLING_RATE 48000
#define ATTACK_TIME 0.010f     /* seconds */
#define RELEASE_TIME 0.100f    /* seconds */

void generate_test_signal(float *pSignal, uint32_t numChannels, uint32_t numSamples, uint32_t blockNum)
{
   for (uint32_t m = 0; m < numChannels; m++) {
      for (uint32_t n = 0; n < numSamples; n++) {
         uint32_t sampleIndex = blockNum * numSamples + n;
         float freq = 1000.0f + 500.0f * (blockNum % 3);  /* Varying frequency */
         float amplitude = 0.1f * (1.0f + 0.5f * (blockNum / (float)NUM_BLOCKS));  /* Growing amplitude */
         pSignal[m * numSamples + n] = amplitude * sinf(2.0f * 3.14159265f * freq * sampleIndex / SAMPLING_RATE);
      }
   }
}

void print_buffer(const char *name, float *pBuffer, uint32_t numChannels, uint32_t numSamples)
{
   printf("\n%s (first 8 samples per channel):\n", name);
   for (uint32_t m = 0; m < numChannels; m++) {
      printf("  Channel %u: ", m);
      for (uint32_t n = 0; n < (numSamples < 8 ? numSamples : 8); n++) {
         printf("%.6f ", pBuffer[m * numSamples + n]);
      }
      printf("\n");
   }
}

int main(void)
{
   AUDIOLIB_STATUS status;
   AUDIOLIB_kernelHandle kernelHandle;
   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;
   AUDIOLIB_envelopeDetector_InitArgs initArgs;
   AUDIOLIB_envelopeDetector_SetArgs  setArgs;

   printf("=== AUDIOLIB_envelopeDetector Example ===\n");
   printf("Configuration:\n");
   printf("  Channels: %u\n", NUM_CHANNELS);
   printf("  Samples per block: %u\n", SAMPLES_PER_BLOCK);
   printf("  Sampling rate: %u Hz\n", SAMPLING_RATE);
   printf("  Attack time: %.3f sec\n", ATTACK_TIME);
   printf("  Release time: %.3f sec\n", RELEASE_TIME);
   printf("  Number of blocks: %u\n\n", NUM_BLOCKS);

   /* Allocate input and output buffers */
   float *pInputBuffer = (float *) malloc(NUM_CHANNELS * SAMPLES_PER_BLOCK * sizeof(float));
   float *pOutputBuffer = (float *) malloc(NUM_CHANNELS * SAMPLES_PER_BLOCK * sizeof(float));

   if (!pInputBuffer || !pOutputBuffer) {
      printf("ERROR: Memory allocation failed\n");
      return -1;
   }

   /* Initialize buffer parameters */
   bufParamsIn.dim_x = SAMPLES_PER_BLOCK;
   bufParamsIn.dim_y = NUM_CHANNELS;
   bufParamsIn.stride_y = SAMPLES_PER_BLOCK * sizeof(float);
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;

   bufParamsOut.dim_x = SAMPLES_PER_BLOCK;
   bufParamsOut.dim_y = NUM_CHANNELS;
   bufParamsOut.stride_y = SAMPLES_PER_BLOCK * sizeof(float);
   bufParamsOut.data_type = AUDIOLIB_FLOAT32;

   /* Setup initialization arguments */
   initArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
   initArgs.samplingRate = SAMPLING_RATE;

   /* Setup set arguments */
   setArgs.attackTime = ATTACK_TIME;
   setArgs.releaseTime = RELEASE_TIME;

   /* Query and allocate kernel handle */
   int32_t handleSize = AUDIOLIB_envelopeDetector_getHandleSize(&initArgs);
   kernelHandle = (AUDIOLIB_kernelHandle) malloc(handleSize);

   if (!kernelHandle) {
      printf("ERROR: Failed to allocate kernel handle\n");
      free(pInputBuffer);
      free(pOutputBuffer);
      return -1;
   }

   /* Validate the parameters, then initialize the kernel */
   status = AUDIOLIB_envelopeDetector_init_checkParams(kernelHandle, &bufParamsIn, &bufParamsOut, &initArgs);
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_envelopeDetector_init(kernelHandle, &bufParamsIn, &bufParamsOut, &initArgs);
   }
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_envelopeDetector_set(kernelHandle, &setArgs);
   }
   if (status != AUDIOLIB_SUCCESS) {
      printf("ERROR: Kernel initialization failed with status %d\n", status);
      free(kernelHandle);
      free(pInputBuffer);
      free(pOutputBuffer);
      return -1;
   }

   printf("Kernel initialized successfully\n\n");

   /* Process multiple blocks */
   for (uint32_t block = 0; block < NUM_BLOCKS; block++) {
      printf("Processing block %u...\n", block);

      /* Generate test signal */
      generate_test_signal(pInputBuffer, NUM_CHANNELS, SAMPLES_PER_BLOCK, block);

      if (block == 0) {
         print_buffer("Input signal (block 0)", pInputBuffer, NUM_CHANNELS, SAMPLES_PER_BLOCK);
      }

      /* Execute kernel */
      status = AUDIOLIB_envelopeDetector_exec(kernelHandle, (void *) pInputBuffer, (void *) pOutputBuffer);
      if (status != AUDIOLIB_SUCCESS) {
         printf("ERROR: Kernel execution failed with status %d\n", status);
         free(kernelHandle);
         free(pInputBuffer);
         free(pOutputBuffer);
         return -1;
      }

      if (block == 0) {
         print_buffer("Envelope output (block 0)", pOutputBuffer, NUM_CHANNELS, SAMPLES_PER_BLOCK);
      }

      /* Print envelope statistics */
      for (uint32_t m = 0; m < NUM_CHANNELS; m++) {
         float maxEnv = 0.0f;
         float avgEnv = 0.0f;
         for (uint32_t n = 0; n < SAMPLES_PER_BLOCK; n++) {
            float env = pOutputBuffer[m * SAMPLES_PER_BLOCK + n];
            if (env > maxEnv) maxEnv = env;
            avgEnv += env;
         }
         avgEnv /= SAMPLES_PER_BLOCK;
         printf("  Ch%u: max_envelope=%.6f, avg_envelope=%.6f\n", m, maxEnv, avgEnv);
      }
   }

   free(kernelHandle);
   free(pInputBuffer);
   free(pOutputBuffer);

   return 0;
}
