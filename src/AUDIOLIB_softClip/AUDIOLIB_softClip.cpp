// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_softClip_priv.h"

int32_t AUDIOLIB_softClip_getHandleSize(AUDIOLIB_softClip_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_softClip_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_softClip_init_checkParams(AUDIOLIB_kernelHandle             handle,
                                   const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                   const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                   const AUDIOLIB_softClip_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_softClip_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (bufParamsIn->data_type != AUDIOLIB_FLOAT32) {
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

AUDIOLIB_STATUS
AUDIOLIB_softClip_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_softClip_exec_checkParams \n");

   if ((pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_softClip_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_softClip_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS             status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_softClip_PrivArgs *pKerPrivArgs = (AUDIOLIB_softClip_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_softClip_set \n");

   memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_softClip_SetArgs));
   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      pKerPrivArgs->threshold = pKerSetArgs->threshold;
      pKerPrivArgs->endKnee   = pKerSetArgs->endKnee;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_softClip_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_softClip_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_softClip_get \n");

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_softClip_PrivArgs *pKerPrivArgs = (AUDIOLIB_softClip_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_softClip_SetArgs));
      pKerPrivArgs->threshold = pKerSetArgs->threshold;
      pKerPrivArgs->endKnee   = pKerSetArgs->endKnee;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_softClip_init(AUDIOLIB_kernelHandle             handle,
                                       AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                       AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                       const AUDIOLIB_softClip_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS             status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_softClip_PrivArgs *pKerPrivArgs = (AUDIOLIB_softClip_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_softClip_init \n");

   pKerPrivArgs->dim_x     = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y     = bufParamsIn->dim_y;
   pKerPrivArgs->inStride  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->outStride = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_softClip_exec_cn<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_softClip_exec_ci<float>;
         status                = AUDIOLIB_softClip_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_softClip_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_softClip_exec \n");

   AUDIOLIB_softClip_PrivArgs *pKerPrivArgs = (AUDIOLIB_softClip_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
