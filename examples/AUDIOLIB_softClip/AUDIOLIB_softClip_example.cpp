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
   float in0[] = {-0.5175042152404785, 1.8287558555603027, 8.619150161743164,
                  -4.234958648681641, -9.167564392089844, -0.7614397406578064,
                  8.430334091186523, -3.5051281452178955, -5.849630832672119,};
   
   float out[] = {0., 0., 0.,
		  0., 0., 0.,
		  0., 0., 0.};

   // handles and struct for call to kernel
   AUDIOLIB_STATUS        status;
   AUDIOLIB_softClip_InitArgs kerInitArgs;
   AUDIOLIB_softClip_SetArgs kerSetArgs;
   int32_t                handleSize = AUDIOLIB_softClip_getHandleSize(&kerInitArgs);
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

   kerSetArgs.threshold   = 0.4f; // Set threshold to 0.4
   kerSetArgs.endKnee     = 1.0f; // Set endKnee to 1
   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_softClip_init_checkParams(handle, &bufParamsIn0,&bufParamsOut, &kerInitArgs);

   // set
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_softClip_set(handle, &kerSetArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_softClip_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      AUDIOLIB_softClip_exec_checkParams(handle, in0, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_softClip_exec(handle, in0, out);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < K; i++) {
      printf("\n\n");
      for (size_t j = 0; j < M; j++) {
         printf("%10g, ", out[i * M + j]);
      }
   }
   printf("\n\n");
   free(handle);
   return 0;
}
