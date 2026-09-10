// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_ceil_priv.h"

int32_t AUDIOLIB_ceil_getHandleSize(AUDIOLIB_ceil_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_ceil_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_ceil_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_ceil_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_ceil_init_checkParams\n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((bufParamsIn->dim_x != bufParamsOut->dim_x) || (bufParamsIn->dim_y != bufParamsOut->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_ceil_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_ceil_exec_checkParams\n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_ceil_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_ceil_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ceil_PrivArgs *pKerPrivArgs = (AUDIOLIB_ceil_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_ceil_init\n");

   pKerPrivArgs->dim_x     = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y     = bufParamsIn->dim_y;
   pKerPrivArgs->inStride  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->outStride = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->dataType  = bufParamsIn->data_type;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_ceil_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_ceil_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         /* init_ci computes nVecs, which selects the unroll factor, so it has
            to run before the execution variant is chosen */
         status = AUDIOLIB_ceil_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);

         if (pKerPrivArgs->nVecs >= AUDIOLIB_CEIL_SP_UNROLL4_MIN_VECS) {
            pKerPrivArgs->execute = AUDIOLIB_ceil_exec_ci_unroll4<float>;
         }
         else {
            pKerPrivArgs->execute = AUDIOLIB_ceil_exec_ci<float>;
         }
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         status                = AUDIOLIB_ceil_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_ceil_exec_ci<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   AUDIOLIB_DEBUGPRINTFN(0, "Exit AUDIOLIB_ceil_init status %d\n", status);

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_ceil_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_ceil_exec\n");

   AUDIOLIB_ceil_PrivArgs *pKerPrivArgs = (AUDIOLIB_ceil_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
