// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/******************************************************************************/
/*                                                                            */
/* Includes                                                                   */
/*                                                                            */
/******************************************************************************/

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#define NUM_CHANNELS (2)
#define NUM_SAMPLES (8)
#define NUM_FRAMES (4)

template <typename dataType>
static AUDIOLIB_STATUS
runAgc(AUDIOLIB_kernelHandle handle, uint32_t dataType_e, const dataType in[NUM_FRAMES][NUM_CHANNELS * NUM_SAMPLES])
{
   AUDIOLIB_STATUS           status = AUDIOLIB_SUCCESS;
   AUDIOLIB_agc_InitArgs kerInitArgs;
   AUDIOLIB_agc_SetArgs  kerSetArgs;
   AUDIOLIB_bufParams2D_t    bufParamsIn;
   AUDIOLIB_bufParams2D_t    bufParamsOut;
   dataType                  frameIn[NUM_CHANNELS * NUM_SAMPLES];
   dataType                  frameOut[NUM_CHANNELS * NUM_SAMPLES];

   bufParamsIn.data_type = dataType_e;
   bufParamsIn.dim_x     = NUM_SAMPLES;
   bufParamsIn.dim_y     = NUM_CHANNELS;
   bufParamsIn.stride_y  = NUM_SAMPLES * sizeof(dataType);

   bufParamsOut.data_type = dataType_e;
   bufParamsOut.dim_x     = NUM_SAMPLES;
   bufParamsOut.dim_y     = NUM_CHANNELS;
   bufParamsOut.stride_y  = NUM_SAMPLES * sizeof(dataType);

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.samplingRate = 48000;
   kerInitArgs.currentGain  = 1.0f;

   kerSetArgs.targetLevel         = -20.0f; // dB
   kerSetArgs.maxAttenuation      = 100.0f; // dB
   kerSetArgs.maxGain             = 12.0f;  // dB
   kerSetArgs.ratio               = 10.0f;  // dB/dB
   kerSetArgs.activationThreshold = -50.0f; // dB
   kerSetArgs.smoothingTime       = 1.0f;   // msec
   kerSetArgs.enableRecovery      = AUDIOLIB_AGC_RECOVERY_ENABLE;
   kerSetArgs.recoveryRate        = 20.0f; // dB/sec

   // init checkparams
   status = AUDIOLIB_agc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_agc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // set
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_agc_set(handle, &kerSetArgs);

   for (uint32_t frame = 0; (frame < NUM_FRAMES) && (status == AUDIOLIB_SUCCESS); frame++) {
      for (uint32_t i = 0; i < NUM_CHANNELS * NUM_SAMPLES; i++) {
         frameIn[i] = in[frame][i];
      }

      // exec checkparams
      status = AUDIOLIB_agc_exec_checkParams(handle, frameIn, frameOut);

      // exec
      if (status == AUDIOLIB_SUCCESS)
         status = AUDIOLIB_agc_exec(handle, frameIn, frameOut);

      // print output for this frame
      printf("frame %u output : ", frame);
      for (uint32_t i = 0; i < NUM_CHANNELS * NUM_SAMPLES; i++) {
         printf("%10g, ", (double) frameOut[i]);
      }
      printf("\n");
   }

   return status;
}

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{

   // clang-format off

   float in[NUM_FRAMES][NUM_CHANNELS * NUM_SAMPLES] = {
      { 0.71649936, -0.13543484,  0.50923542, -0.54119591,  0.19242506, -0.38308575,  0.84567145, -0.95629663,
       -0.99152828,  0.61234567, -0.24567145,  0.05629663,  0.87654321, -0.43210987,  0.32109876, -0.76543210},
      {-0.65432109,  0.54321098, -0.43210987,  0.93210987, -0.82109876,  0.71098765, -0.10987654,  0.49876543,
        0.38765432, -0.97654321,  0.16543210, -0.85432109,  0.74321098, -0.63210987,  0.52109876, -0.21098765},
      { 0.00007164, -0.00001354,  0.00005092, -0.00005411,  0.00001924, -0.00003830,  0.00008456, -0.00009562,
       -0.00009915,  0.00006123, -0.00002456,  0.00000562,  0.00008765, -0.00004321,  0.00003210, -0.00007654},
      {-0.00006543,  0.00005432, -0.00004321,  0.00009321, -0.00008210,  0.00007109, -0.00001098,  0.00004987,
        0.00003876, -0.00009765,  0.00001654, -0.00008543,  0.00007432, -0.00006321,  0.00005210, -0.00002109},
   };

   // clang-format on

   double inD[NUM_FRAMES][NUM_CHANNELS * NUM_SAMPLES];
   for (uint32_t frame = 0; frame < NUM_FRAMES; frame++) {
      for (uint32_t i = 0; i < NUM_CHANNELS * NUM_SAMPLES; i++) {
         inD[frame][i] = (double) in[frame][i];
      }
   }

   AUDIOLIB_STATUS           status;
   AUDIOLIB_agc_InitArgs kerInitArgs;
   int32_t                   handleSize = AUDIOLIB_agc_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle     handle     = malloc(handleSize);

   printf("\n FLOAT \n");
   status = runAgc<float>(handle, AUDIOLIB_FLOAT32, in);

   printf("\n DOUBLE \n");
   if (status == AUDIOLIB_SUCCESS)
      status = runAgc<double>(handle, AUDIOLIB_FLOAT64, inD);


   free(handle);
   return (status == AUDIOLIB_SUCCESS) ? 0 : 1;
}
