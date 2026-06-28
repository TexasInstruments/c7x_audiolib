// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "audiolib.h"
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

   // Setup input and output buffers for single- and double-precision datatypes
   float in0[] = {0.71649936, 0.13543484, 0.50923542, 0.54119591, 0.19242506,
                  0.38308575, 0.56363197, 0.24567145, 0.05629663};

   float out[] = {0., 0., 0., 0., 0., 0., 0., 0., 0.};

   double in0D[] = {0.71649936, 0.13543484, 0.50923542, 0.54119591, 0.19242506,
                    0.38308575, 0.24567145, 0.05629663, 0.99152828};

   double outD[] = {0., 0., 0., 0., 0., 0., 0., 0., 0.};

   AUDIOLIB_STATUS        status;
   AUDIOLIB_dB10_InitArgs kerInitArgs;
   int32_t                handleSize = AUDIOLIB_dB10_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle  handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = K;
   bufParamsIn0.dim_y     = M;
   bufParamsIn0.stride_y  = M * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = M * sizeof(float);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_dB10_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_dB10_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_dB10_exec_checkParams(handle, in0, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_dB10_exec(handle, in0, out);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%10g, ", out[i * M + j]);
      }
   }
   printf("\n\n");

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = AUDIOLIB_FLOAT64;
   bufParamsIn0.dim_x     = K;
   bufParamsIn0.dim_y     = M;
   bufParamsIn0.stride_y  = M * sizeof(double);

   bufParamsOut.data_type = AUDIOLIB_FLOAT64;
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = M * sizeof(double);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_dB10_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_dB10_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_dB10_exec_checkParams(handle, in0D, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_dB10_exec(handle, in0D, outD);

   printf("\n DOUBLE \n");

   // print results
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%10g, ", outD[i * M + j]);
      }
   }
   printf("\n\n");

   return 0;
}
