// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <stdint.h>

#define M (10)
#define K (10)

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{

   // clang-format off

  // Setup input and output buffers for single- and double-precision datatypes
   float in0[M];
   float in1[K];

   for(uint32_t i=0;i<M;i++)
   {
      in0[i]=i*1.145+(i);
   }
   for(uint32_t i=0;i<K;i++)
   {
      in1[i]=i*0.245+(i);
   }

   float out[M] = {0};

   // handles and struct for call to kernel
   AUDIOLIB_STATUS               status;
   AUDIOLIB_tableInterpolate_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_tableInterpolate_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams1D_t bufParamsIn0, bufParamsIn1, bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = M;

   bufParamsIn1.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn1.dim_x     = K;

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = M;

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.minVal = 0;
   kerInitArgs.maxVal = 10;
   kerInitArgs.tableInterpolateSize=256;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_tableInterpolate_init_checkParams(handle, &bufParamsIn0,&bufParamsIn1, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_tableInterpolate_init(handle, &bufParamsIn0,&bufParamsIn1, &bufParamsOut, &kerInitArgs);

#if defined(__C7524__)
   if (kerInitArgs.tableInterpolateSize >= K) {
      if (status == AUDIOLIB_SUCCESS) {
         AUDIOLIB_tableInterpolate_set_ci(handle, in1);
      }
   }
#endif

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_tableInterpolate_exec_checkParams(handle, in0, in1, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_tableInterpolate_exec(handle, in0, in1, out);

   printf("\n RESULT \n");

   // print results

   for (size_t j = 0; j < M; j++) {
         printf("%f, ", out[j]);
      }
   printf("\n\n");

   free(handle);
   return 0;
}
