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
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

static const uint32_t M         = 3U;
static const uint32_t K         = 3U;
static const uint32_t NUM_ELEMS = M * K;

/******************************************************************************/
/*                                                                            */
/* main                                                                       */
/*                                                                            */
/******************************************************************************/

int main(void)
{
   // clang-format off

   // Setup input and output buffers for single- and double-precision datatypes
   float in0[NUM_ELEMS] = {0.71649936f, 0.13543484f, 0.50923542f,
                           0.54119591f, 0.19242506f, 0.38308575f,
                           0.24567145f, 0.05629663f, 0.99152828f};

   float out[NUM_ELEMS] = {0.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 0.0f,
                           0.0f, 0.0f, 0.0f};

   double in0D[NUM_ELEMS] = {0.71649936, 0.13543484, 0.50923542,
                             0.54119591, 0.19242506, 0.38308575,
                             0.24567145, 0.05629663, 0.99152828};

   double outD[NUM_ELEMS] = {0.0, 0.0, 0.0,
                             0.0, 0.0, 0.0,
                             0.0, 0.0, 0.0};

   // clang-format on

   // handles and struct for call to kernel
   AUDIOLIB_STATUS              status = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainOffset_InitArgs kerInitArgs;

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.gain      = 1.0f;
   kerInitArgs.offset    = 0.22f;

   const int32_t         handleSize = AUDIOLIB_gainOffset_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = NULL;

   if (handleSize > 0) {
      handle = malloc(static_cast<size_t>(handleSize));
   }

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   AUDIOLIB_bufParams2D_t bufParamsIn0;
   AUDIOLIB_bufParams2D_t bufParamsOut;

   /*-------------------------------- FLOAT --------------------------------*/

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = static_cast<uint32_t>(AUDIOLIB_FLOAT32);
   bufParamsIn0.dim_x     = K;
   bufParamsIn0.dim_y     = M;
   bufParamsIn0.stride_y  = static_cast<int32_t>(K * static_cast<uint32_t>(sizeof(float)));

   bufParamsOut.data_type = static_cast<uint32_t>(AUDIOLIB_FLOAT32);
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = static_cast<int32_t>(K * static_cast<uint32_t>(sizeof(float)));

   // init checkparams
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainOffset_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);
   }

   // init
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainOffset_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);
   }

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainOffset_exec_checkParams(handle, in0, out);
   }

   // exec
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainOffset_exec(handle, in0, out);
   }

   (void) printf("\n FLOAT \n");

   // print results
   for (uint32_t i = 0U; i < M; i++) {
      (void) printf("\n\n");
      for (uint32_t j = 0U; j < K; j++) {
         (void) printf("%10g, ", static_cast<double>(out[(i * K) + j]));
      }
   }
   (void) printf("\n\n");

   /*-------------------------------- DOUBLE -------------------------------*/

   // fill in input and output buffer parameters
   bufParamsIn0.data_type = static_cast<uint32_t>(AUDIOLIB_FLOAT64);
   bufParamsIn0.dim_x     = K;
   bufParamsIn0.dim_y     = M;
   bufParamsIn0.stride_y  = static_cast<int32_t>(K * static_cast<uint32_t>(sizeof(double)));

   bufParamsOut.data_type = static_cast<uint32_t>(AUDIOLIB_FLOAT64);
   bufParamsOut.dim_x     = K;
   bufParamsOut.dim_y     = M;
   bufParamsOut.stride_y  = static_cast<int32_t>(K * static_cast<uint32_t>(sizeof(double)));

   // init checkparams
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainOffset_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);
   }

   // init
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainOffset_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);
   }

   // exec checkparams
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainOffset_exec_checkParams(handle, in0D, outD);
   }

   // exec
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_gainOffset_exec(handle, in0D, outD);
   }

   (void) printf("\n DOUBLE \n");

   // print results
   for (uint32_t i = 0U; i < M; i++) {
      (void) printf("\n\n");
      for (uint32_t j = 0U; j < K; j++) {
         (void) printf("%10g, ", outD[(i * K) + j]);
      }
   }
   (void) printf("\n\n");

   if (status != AUDIOLIB_SUCCESS) {
      (void) printf("\n AUDIOLIB_gainOffset example failed, status = %" PRId32 "\n", static_cast<int32_t>(status));
   }

   free(handle);

   return (status == AUDIOLIB_SUCCESS) ? 0 : 1;
}
