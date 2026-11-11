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
   float in0[] = {0.71649936, 0.13543484, 0.50923542,
		  0.54119591, 0.19242506, 0.38308575,
                  0.24567145, 0.05629663, 0.99152828};
   
   float out[] = {0., 0., 0.,
		  0., 0., 0.,
		  0., 0., 0.};

   double in0D[] = {0.71649936, 0.13543484, 0.50923542,
		  0.54119591, 0.19242506, 0.38308575,
                  0.24567145, 0.05629663, 0.99152828};
   
   double outD[] = {0., 0., 0.,
		  0., 0., 0.,
		  0., 0., 0.};


   float asymmetricLimiterLower = 0.15;
   float asymmetricLimiterUpper = 0.50;

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS        status;
   AUDIOLIB_asymmetricLimiter_InitArgs kerInitArgs;
   AUDIOLIB_asymmetricLimiter_SetArgs  kerSetArgs;
   int32_t                handleSize = AUDIOLIB_asymmetricLimiter_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle  handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = K;
   bufParamsIn0.dim_y     = M;
   bufParamsIn0.stride_y  = K * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = K * sizeof(float);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerSetArgs.asymmetricLimiterLower = asymmetricLimiterLower;
   kerSetArgs.asymmetricLimiterUpper = asymmetricLimiterUpper;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_asymmetricLimiter_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_asymmetricLimiter_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // set
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_asymmetricLimiter_set(handle, &kerSetArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_asymmetricLimiter_exec_checkParams(handle, in0, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_asymmetricLimiter_exec(handle, in0, out);

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
   bufParamsIn0.stride_y  = K * sizeof(double);

   bufParamsOut.data_type = AUDIOLIB_FLOAT64;
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = K * sizeof(double);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerSetArgs.asymmetricLimiterLower = asymmetricLimiterLower;
   kerSetArgs.asymmetricLimiterUpper = asymmetricLimiterUpper;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_asymmetricLimiter_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_asymmetricLimiter_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // set
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_asymmetricLimiter_set(handle, &kerSetArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_asymmetricLimiter_exec_checkParams(handle, in0, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_asymmetricLimiter_exec(handle, in0D, outD);

   printf("\n DOUBLE \n");

   // print results
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%10g, ", outD[i * M + j]);
      }
   }
   printf("\n\n");
   free(handle);
   return 0;
}
