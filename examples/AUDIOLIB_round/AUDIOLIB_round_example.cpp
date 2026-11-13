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
   float in0[] = {1.4f, 2.5f, 3.6f,
                  4.1f, 5.9f, 6.3f,
                  7.7f, 8.2f, 9.8f};

   float out[] = {0.f, 0.f, 0.f,
                  0.f, 0.f, 0.f,
                  0.f, 0.f, 0.f};

   double in0D[] = {1.4, 2.5, 3.6,
                    4.1, 5.9, 6.3,
                    7.7, 8.2, 9.8};

   double outD[] = {0., 0., 0.,
                    0., 0., 0.,
                    0., 0., 0.};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS         status;
   AUDIOLIB_round_InitArgs kerInitArgs;
   int32_t                 handleSize = AUDIOLIB_round_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle   handle     = malloc(handleSize);

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
      status = AUDIOLIB_round_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_round_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_round_exec_checkParams(handle, in0, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_round_exec(handle, in0, out);

   printf("\n FLOAT ROUND \n");

   // print results
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%10g, ", out[i * M + j]);
      }
   }
   printf("\n\n");

   // fill in input and output buffer parameters for double
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
      status = AUDIOLIB_round_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_round_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_round_exec_checkParams(handle, in0D, outD);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_round_exec(handle, in0D, outD);

   printf("\n DOUBLE ROUND \n");

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
