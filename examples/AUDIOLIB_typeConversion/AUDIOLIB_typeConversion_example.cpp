// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_types.h"
#include "audiolib.h"
#include <cstdio>
#include <stdint.h>
#include <stdlib.h>

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
   float pIn0[] = {1.0154616832733154, 0.46621352434158325, 0.5326930284500122, 
      -0.6739658713340759, -0.3593469262123108, -0.7759705185890198, 
      0.4773215055465698, -0.419317364692688, 1.0749090909957886};
   
   int32_t pOut[M * K] = {0};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS                  status;
   AUDIOLIB_typeConversion_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_typeConversion_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsIn1, bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = K;
   bufParamsIn0.dim_y     = M;
   bufParamsIn0.stride_y  = M * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_INT32;
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = M * sizeof(int32_t);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.testQ31   = 0;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_typeConversion_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_typeConversion_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_typeConversion_exec_checkParams(handle, pIn0, pOut);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_typeConversion_exec(handle, pIn0, pOut);

   printf("\n FLOAT \n");

   printf("\n\n");
   printf("First test case:");
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%d, ", pOut[i * M + j]);
      }
   }
   printf("\n\n");

   // For second test case

   double pIn1[M * K] = {0.71649936, 0.13543484, 0.50923542, 0.54119591, 0.19242506,
                         0.38308575, 0.24567145, 0.05629663, 0.99152828};

   // fill in input and output buffer parameters
   bufParamsIn1.data_type = AUDIOLIB_FLOAT64;
   bufParamsIn1.dim_x     = K;
   bufParamsIn1.dim_y     = M;
   bufParamsIn1.stride_y  = M * sizeof(double);

   bufParamsOut.data_type = AUDIOLIB_INT32;
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = M * sizeof(int32_t);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.testQ31   = 1;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_typeConversion_init_checkParams(handle, &bufParamsIn1, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_typeConversion_init(handle, &bufParamsIn1, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_typeConversion_exec_checkParams(handle, pIn1, pOut);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_typeConversion_exec(handle, pIn1, pOut);

   printf("\n DOUBLE \n");

   printf("\n\n");
   printf("second test case:\n");
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%d, ", pOut[i * M + j]);
      }
   }
   printf("\n\n");
   free(handle);

   return 0;
}
