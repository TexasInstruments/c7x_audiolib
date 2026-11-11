// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_asymmetricLimiter_priv.h"

int32_t AUDIOLIB_asymmetricLimiter_getHandleSize(AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_asymmetricLimiter_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_asymmetricLimiter_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_set(AUDIOLIB_kernelHandle handle,
                                               const AUDIOLIB_asymmetricLimiter_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_asymmetricLimiter_set \n");

   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_asymmetricLimiter_PrivArgs *pKerPrivArgs = (AUDIOLIB_asymmetricLimiter_PrivArgs *) handle;
      pKerPrivArgs->setArgs                             = *pKerSetArgs;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_get(AUDIOLIB_kernelHandle handle,
                                               AUDIOLIB_asymmetricLimiter_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_asymmetricLimiter_get \n");

   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_asymmetricLimiter_PrivArgs *pKerPrivArgs = (AUDIOLIB_asymmetricLimiter_PrivArgs *) handle;
      *pKerSetArgs                                      = pKerPrivArgs->setArgs;
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_asymmetricLimiter_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_asymmetricLimiter_PrivArgs *pKerPrivArgs = (AUDIOLIB_asymmetricLimiter_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_asymmetricLimiter_init \n");

   pKerPrivArgs->dim_x              = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y              = bufParamsIn->dim_y;
   pKerPrivArgs->strideIn           = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOut          = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_asymmetricLimiter_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_asymmetricLimiter_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_asymmetricLimiter_exec_ci<float>;
         status                = AUDIOLIB_asymmetricLimiter_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_asymmetricLimiter_exec_ci<double>;
         status                = AUDIOLIB_asymmetricLimiter_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_asymmetricLimiter_exec \n");

   AUDIOLIB_asymmetricLimiter_PrivArgs *pKerPrivArgs = (AUDIOLIB_asymmetricLimiter_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
