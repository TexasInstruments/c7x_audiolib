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
   0.0000000f, 0.1370123f, 0.2714404f, 0.4007488f,
   0.0000000f, 0.1434926f, 0.2840154f, 0.4186597f, 
   0.0000000f, 0.1499668f, 0.2965416f, 0.4364092f,
   0.0000000f, 0.1564345f, 0.3090170f, 0.4539905f, 
   0.0000000f, 0.1628955f, 0.3214395f, 0.4713967f,
   0.0000000f, 0.1693495f, 0.3338069f, 0.4886212f};

float in1[] = {0., 0., 0., 0., 0., 0., 0.};

int32_t channelConfig[] = {14, 15, 16, 17, 18, 19, 20};

float out[] = {0., 0., 0., 0., 
   0., 0., 0., 0., 
   0., 0., 0., 0., 
   0., 0., 0., 0., 
   0., 0., 0., 0., 
   0., 0., 0., 0.,
   0., 0., 0., 0.};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS                status;
   AUDIOLIB_balanceFader_InitArgs kerInitArgs;
   AUDIOLIB_balanceFader_SetArgs  kerSetArgs;

   kerSetArgs.balance              = 1; // Set balance to 0
   kerSetArgs.fade                 = 1;
   kerSetArgs.lfeBalanceGainFactor = 0.6;
   kerSetArgs.lfeFaderGainFactor   = 0.2;
   kerSetArgs.sideGainFactor       = 0.4;
   kerSetArgs.lfeBalanceMode       = 2;
   kerSetArgs.lfeFaderMode         = 3;

   kerSetArgs.channelConfig = (int32_t *) malloc(7 * sizeof(int32_t));
   memcpy(kerSetArgs.channelConfig, channelConfig, 7 * sizeof(int32_t));

   int32_t               handleSize = AUDIOLIB_balanceFader_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;
   AUDIOLIB_bufParams1D_t bufParamsGain;

   // fill in input and output buffer parameters
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = 4;
   bufParamsIn.dim_y     = 7;
   bufParamsIn.stride_y  = 4 * sizeof(float);

   bufParamsGain.data_type = AUDIOLIB_FLOAT32;
   bufParamsGain.dim_x     = 7;

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 4;
   bufParamsOut.dim_y     = 7;
   bufParamsOut.stride_y  = 4 * sizeof(float);

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.isInterleave = 0;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status =
          AUDIOLIB_balanceFader_init_checkParams(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_balanceFader_init(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_balanceFader_set(handle, in1, &kerSetArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_balanceFader_exec_checkParams(handle, in0, in1, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_balanceFader_exec(handle, in0, in1, out);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < 7; i++) {
      printf("\n\n");
      for (size_t j = 0; j < 4; j++) {
         printf("%f, ", out[i * 4 + j]);
      }
   }

   printf("\n\n");
   free(kerSetArgs.channelConfig);
   free(handle);
   return 0;
}
