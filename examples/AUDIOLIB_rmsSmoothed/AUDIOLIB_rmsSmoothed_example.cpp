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

   // Setup input and output buffers for single-precision datatypes
   // 2 channels x 8 samples per channel (planar)
   float in0[] = {1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f,
                  0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f};

   float out[] = {0.};

   // handles and struct for call to kernel
   AUDIOLIB_STATUS               status;
   AUDIOLIB_rmsSmoothed_InitArgs kerInitArgs;
   AUDIOLIB_rmsSmoothed_SetArgs  kerSetArgs;

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.samplingRate = 48000;
   kerSetArgs.smoothingTime = 1.0f;

   int32_t               handleSize = AUDIOLIB_rmsSmoothed_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams1D_t bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = 8;
   bufParamsIn.dim_y     = 2;
   bufParamsIn.stride_y  = 8 * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 1;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_rmsSmoothed_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_rmsSmoothed_set(handle, &kerSetArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_rmsSmoothed_exec_checkParams(handle, in0, out);

   printf("\n FLOAT \n");

   // exec: the smoothed RMS rises towards the block RMS over successive blocks
   for (size_t frame = 0; (frame < 4) && (status == AUDIOLIB_SUCCESS); frame++) {
      status = AUDIOLIB_rmsSmoothed_exec(handle, in0, out);

      // print results
      printf("\n frame %zu: smoothed RMS = %10g", frame, out[0]);
   }

   printf("\n\n");
   free(handle);

   return 0;
}
