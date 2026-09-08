// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <stdint.h>

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{

   // clang-format off

  // Setup input and output buffers for single-precision datatypes
  float in0[] = {0.0000000f, 0.1305262f, 0.2588190f, 0.3826834f, 
   0.0000000f, 0.1370123f, 0.2714404f, 0.4007488f};

float in1[] = {0., 0., 0.};

int32_t channelConfig[] = {AUDIOLIB_CHANNEL_FRONT_LEFT , AUDIOLIB_CHANNEL_FRONT_RIGHT , AUDIOLIB_CHANNEL_FRONT_CENTER};

float out[] = {0., 0., 0., 0., 
   0., 0., 0., 0., 
   0., 0., 0., 0., 
   0., 0., 0., 0., 
   0., 0., 0., 0., 
   0., 0., 0., 0.};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS                status;
   AUDIOLIB_panning_InitArgs kerInitArgs;
   AUDIOLIB_panning_SetArgs  kerSetArgs;

   kerSetArgs.samplingRate = 48000;
   kerSetArgs.smoothingTime  = 0;
   kerSetArgs.sourceAzimuth = 15;
   kerInitArgs.channelConfig = (int32_t *) malloc(3 * sizeof(int32_t));

   memcpy(kerInitArgs.channelConfig, channelConfig, 3 * sizeof(int32_t));

   kerInitArgs.numChannels = 3;
   int32_t               handleSize = AUDIOLIB_panning_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;
   AUDIOLIB_bufParams1D_t bufParamsGain;

   // fill in input and output buffer parameters
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = 1;
   bufParamsIn.dim_y     = 8;
   bufParamsIn.stride_y  = 8 * sizeof(float);

   bufParamsGain.data_type = AUDIOLIB_FLOAT32;
   bufParamsGain.dim_x     = 3;

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 8;
   bufParamsOut.dim_y     = 3;
   bufParamsOut.stride_y  = 8 * sizeof(float);

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.isInterleave = 0;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status =
          AUDIOLIB_panning_init_checkParams(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_panning_init(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_panning_set(handle, in1, &kerSetArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_panning_exec_checkParams(handle, in0, in1, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_panning_exec(handle, in0, in1, out);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < 3; i++) {
      printf("\n\n");
      for (size_t j = 0; j < 8; j++) {
         printf("%f, ", out[i * 8 + j]);
      }
   }

   printf("\n\n");
   free(kerInitArgs.channelConfig);
   free(handle);
   return 0;
}
