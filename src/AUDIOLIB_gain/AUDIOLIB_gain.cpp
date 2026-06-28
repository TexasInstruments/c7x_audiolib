// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_gain_priv.h"

int32_t AUDIOLIB_gain_getHandleSize(AUDIOLIB_gain_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_gain_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_gain_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams1D_t *bufParamsGain,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_gain_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gain_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsGain == NULL) || (bufParamsOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type ||
               bufParamsGain->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gain_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                               const void *restrict pIn,
                                               const void *restrict pGain,
                                               const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gain_exec_checkParams \n");

   if ((pIn == NULL) || (pGain == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gain_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams1D_t       *bufParamsGain,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_gain_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gain_PrivArgs *pKerPrivArgs = (AUDIOLIB_gain_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gain_init \n");

   pKerPrivArgs->dim_x     = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y     = bufParamsIn->dim_y;
   pKerPrivArgs->strideIn  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOut = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_gain_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_gain_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_gain_exec_ci<float>;
         status = AUDIOLIB_gain_init_ci<float>(handle, bufParamsIn, bufParamsGain, bufParamsOut, pKerInitArgs);
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_gain_exec_ci<double>;
         status = AUDIOLIB_gain_init_ci<double>(handle, bufParamsIn, bufParamsGain, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_gain_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gain_exec \n");

   AUDIOLIB_gain_PrivArgs *pKerPrivArgs = (AUDIOLIB_gain_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pGain, pOut);

   return status;
}
