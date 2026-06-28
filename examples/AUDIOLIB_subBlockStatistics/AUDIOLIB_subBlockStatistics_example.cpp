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
  float in0[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
   8.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 0.0f};

   float out[] = {0., 0.};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS                      status;
   AUDIOLIB_subBlockStatistics_InitArgs kerInitArgs;
   AUDIOLIB_subBlockStatistics_SetArgs  kerSetArgs;

   kerSetArgs.funcStyle        = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerSetArgs.statisticsType   = 0;
   kerSetArgs.flagPerChanStats = 0;

   int32_t               handleSize = AUDIOLIB_subBlockStatistics_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = 8;
   bufParamsIn.dim_y     = 2;
   bufParamsIn.stride_y  = 8 * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 2; // inputBlockSize / subBlockSize
   bufParamsOut.dim_y     = 1;
   bufParamsOut.stride_y  = 2 * sizeof(float);

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.subBlockSize = 4;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_subBlockStatistics_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_subBlockStatistics_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_subBlockStatistics_set(handle, &kerSetArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_subBlockStatistics_exec_checkParams(handle, in0, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_subBlockStatistics_exec(handle, in0, out);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < 1; i++) {
      printf("\n\n");
      for (size_t j = 0; j < 2; j++) {
         printf("%10g, ", out[i * 2 + j]);
      }
   }
   free(handle);
   printf("\n\n");

   return 0;
}
