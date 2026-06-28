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
  float in0[] = {0.0f, 1.0f, 2.0f, 3.0f, 
   0.0f, 1.0f, 2.0f, 3.0f};

float out[] = {0.,
   0.};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS                   status;
   AUDIOLIB_blockStatistics_InitArgs kerInitArgs;
   AUDIOLIB_blockStatistics_SetArgs  kerSetArgs;

   kerSetArgs.funcStyle      = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerSetArgs.statisticsType = 8;

   int32_t               handleSize = AUDIOLIB_blockStatistics_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = 4;
   bufParamsIn.dim_y     = 2;
   bufParamsIn.stride_y  = 4 * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 1;
   bufParamsOut.dim_y     = 2;
   bufParamsOut.stride_y  = 1 * sizeof(float);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_blockStatistics_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_blockStatistics_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_blockStatistics_set(handle, &kerSetArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_blockStatistics_exec_checkParams(handle, in0, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_blockStatistics_exec(handle, in0, out);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < 2; i++) {
      printf("\n\n");
      for (size_t j = 0; j < 1; j++) {
         printf("%10g, ", out[i * 1 + j]);
      }
   }

   printf("\n\n");
   free(handle);

   return 0;
}
