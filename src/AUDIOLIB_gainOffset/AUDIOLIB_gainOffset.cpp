// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_gainOffset_priv.h"

int32_t AUDIOLIB_gainOffset_getHandleSize(AUDIOLIB_gainOffset_InitArgs *pKerInitArgs)
{
   (void) pKerInitArgs; /* Unused parameter suppression */
   return (int32_t) sizeof(AUDIOLIB_gainOffset_PrivArgs);
}

AUDIOLIB_STATUS
AUDIOLIB_gainOffset_init_checkParams(AUDIOLIB_kernelHandle               handle,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                     const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((bufParamsIn->dim_x == 0U) || (bufParamsIn->dim_y == 0U) ||
               (bufParamsIn->dim_x != bufParamsOut->dim_x) || (bufParamsIn->dim_y != bufParamsOut->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainOffset_init(AUDIOLIB_kernelHandle               handle,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                         const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_init \n");

   pKerPrivArgs->dimX      = bufParamsIn->dim_x;
   pKerPrivArgs->dimY      = bufParamsIn->dim_y;
   pKerPrivArgs->strideIn  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOut = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->gain      = pKerInitArgs->gain;
   pKerPrivArgs->offset    = pKerInitArgs->offset;


   if (pKerInitArgs->funcStyle == (int8_t) AUDIOLIB_FUNCTION_OPTIMIZED) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status                = AUDIOLIB_gainOffset_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_ci<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         status                = AUDIOLIB_gainOffset_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_ci<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else if (pKerInitArgs->funcStyle == (int8_t) AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_exec \n");

   AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;
   status                                     = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
