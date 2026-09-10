// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "audiolib.h"
#include <stdint.h>

#define NUM_CHANNELS (3)
#define NUM_SAMPLES (3)

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{
   // Setup input and output buffers for single- and double-precision datatypes.
   // The inputs pair each magnitude with its negation, because the floor of a
   // negative value rounds away from zero while the floor of a positive value
   // rounds toward it. Whole numbers are included to show that they pass
   // through unchanged.
   float in0[] = {2.3F, -2.3F, 2.7F, -2.7F, 0.5F, -0.5F, 3.0F, -3.0F, 0.0F};

   float out[] = {0., 0., 0., 0., 0., 0., 0., 0., 0.};

   double in0D[] = {2.3, -2.3, 2.7, -2.7, 0.5, -0.5, 3.0, -3.0, 0.0};

   double outD[] = {0., 0., 0., 0., 0., 0., 0., 0., 0.};

   AUDIOLIB_STATUS        status;
   AUDIOLIB_floor_InitArgs kerInitArgs;
   int32_t                handleSize = AUDIOLIB_floor_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle  handle     = malloc(handleSize);

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = NUM_SAMPLES;
   bufParamsIn0.dim_y     = NUM_CHANNELS;
   bufParamsIn0.stride_y  = NUM_SAMPLES * sizeof(float);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = NUM_SAMPLES;
   bufParamsOut.dim_y     = NUM_CHANNELS;
   bufParamsOut.stride_y  = NUM_SAMPLES * sizeof(float);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_floor_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_floor_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_floor_exec_checkParams(handle, in0, out);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_floor_exec(handle, in0, out);

   printf("\n FLOAT \n");

   // print results
   for (size_t i = 0; i < NUM_CHANNELS; i++) {
      printf("\n\n");
      for (size_t j = 0; j < NUM_SAMPLES; j++) {
         printf("%10g -> %10g, ", in0[i * NUM_SAMPLES + j], out[i * NUM_SAMPLES + j]);
      }
   }
   printf("\n\n");

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = AUDIOLIB_FLOAT64;
   bufParamsIn0.dim_x     = NUM_SAMPLES;
   bufParamsIn0.dim_y     = NUM_CHANNELS;
   bufParamsIn0.stride_y  = NUM_SAMPLES * sizeof(double);

   bufParamsOut.data_type = AUDIOLIB_FLOAT64;
   bufParamsOut.dim_x     = NUM_SAMPLES;
   bufParamsOut.dim_y     = NUM_CHANNELS;
   bufParamsOut.stride_y  = NUM_SAMPLES * sizeof(double);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

   status = AUDIOLIB_SUCCESS;

   // init checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_floor_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // init
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_floor_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_floor_exec_checkParams(handle, in0D, outD);

   // exec
   if (status == AUDIOLIB_SUCCESS)
      status = AUDIOLIB_floor_exec(handle, in0D, outD);

   printf("\n DOUBLE \n");

   // print results
   for (size_t i = 0; i < NUM_CHANNELS; i++) {
      printf("\n\n");
      for (size_t j = 0; j < NUM_SAMPLES; j++) {
         printf("%10g -> %10g, ", in0D[i * NUM_SAMPLES + j], outD[i * NUM_SAMPLES + j]);
      }
   }
   printf("\n\n");

   free(handle);

   return 0;
}
