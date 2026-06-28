// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <stdint.h>

#define M (3)
#define K (3)

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{

   // clang-format off

  // Setup input and output buffers for single- and double-precision datatypes
   float inL[] = {0.71649936, 0.13543484, 0.50923542,
		  0.54119591, 0.19242506, 0.38308575,
                  0.24567145, 0.05629663, 0.99152828};
   float inR[] = {0.4799542,  0.97309674, 0.79839982,
        0.06691247, 0.71649936, 0.13543484,
                  0.50923542, 0.54119591, 0.19242506};
   float outL[] = {0., 0., 0.,
		  0., 0., 0.,
		  0., 0., 0.};
   float outR[] = {0., 0., 0.,
        0., 0., 0.,
        0., 0., 0.};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS           status;
   AUDIOLIB_balance_InitArgs kerInitArgs;
   AUDIOLIB_balance_SetArgs  kerSetArgs;

   kerSetArgs.balance       = 0;      // Set balance to 0
   kerSetArgs.smoothingTime = 100.0f; // Set smoothing time to 100 ms
   kerSetArgs.samplingRate  = 48000;  // Set sampling rate to 48 kHz

   int32_t               handleSize = AUDIOLIB_balance_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = K;
   bufParamsIn.dim_y     = M;
   bufParamsIn.stride_y  = M * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = M * sizeof(float);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_balance_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_balance_set(handle, &kerSetArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_balance_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_balance_exec_checkParams(handle, inL, inR, outL, outR);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_balance_exec(handle, inL, inR, outL, outR);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%10g, ", outL[i * M + j]);
      }
   }

   printf("\n\n");
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%10g, ", outR[i * M + j]);
      }
   }
   printf("\n\n");

   return 0;
}
