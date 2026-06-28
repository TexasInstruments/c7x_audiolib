// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_delay_priv.h"

int32_t AUDIOLIB_delay_getHandleSize(AUDIOLIB_delay_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_delay_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_delay_init_checkParams(AUDIOLIB_kernelHandle          handle,
                                const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                const AUDIOLIB_bufParams2D_t  *bufParamsDelay,
                                const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                const AUDIOLIB_delay_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delay_init_checkParams \n");

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type ||
               bufParamsDelay->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else {
         /* Nothing to do here */
      }

      if ((pKerInitArgs->mode != 0) && (pKerInitArgs->mode != 1)) {
         status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
      }
      else {
         /* Nothing to do here */
      }
   }

   AUDIOLIB_DEBUGPRINTFN(0, "Exiting AUDIOLIB_delay_init_checkParams: %d\n", status);

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_delay_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                const void *restrict pIn,
                                                const void *restrict pDelay,
                                                const void *restrict pOut,
                                                const void *restrict pScratch)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delay_exec_checkParams \n");

   if ((pIn == NULL) || (pDelay == NULL) || (pOut == NULL) || (pScratch == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_delay_init(AUDIOLIB_kernelHandle          handle,
                                    AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                    AUDIOLIB_bufParams2D_t        *bufParamsDelay,
                                    AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                    const AUDIOLIB_delay_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delay_init \n");

   pKerPrivArgs->mode                = pKerInitArgs->mode;
   pKerPrivArgs->interleave          = pKerInitArgs->interleave;
   pKerPrivArgs->delaySize           = pKerInitArgs->delaySize;
   pKerPrivArgs->delayBuffSize       = bufParamsDelay->stride_y / AUDIOLIB_sizeof(bufParamsDelay->data_type);
   pKerPrivArgs->strideInElements    = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOutElements   = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->strideDelayElements = bufParamsDelay->stride_y / AUDIOLIB_sizeof(bufParamsDelay->data_type);

   if (pKerInitArgs->interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      pKerPrivArgs->numSamples  = bufParamsIn->dim_y;
      pKerPrivArgs->numChannels = bufParamsIn->dim_x;
   }
   else {
      pKerPrivArgs->numSamples  = bufParamsIn->dim_x;
      pKerPrivArgs->numChannels = bufParamsIn->dim_y;
   }

   if (pKerPrivArgs->mode == 1) {
      pKerPrivArgs->readIdx  = 0;
      pKerPrivArgs->writeIdx = pKerPrivArgs->delaySize;
   }

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (pKerPrivArgs->interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_delay_interleave_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_delay_interleave_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_delay_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_delay_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   else {
      if (pKerPrivArgs->interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_delay_interleave_exec_ci<float>;
            status = AUDIOLIB_delay_interleave_init_ci<float>(handle, bufParamsIn, bufParamsDelay, bufParamsOut,
                                                              pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_delay_interleave_exec_ci<double>;
            status = AUDIOLIB_delay_interleave_init_ci<double>(handle, bufParamsIn, bufParamsDelay, bufParamsOut,
                                                               pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_delay_exec_ci<float>;
            status = AUDIOLIB_delay_init_ci<float>(handle, bufParamsIn, bufParamsDelay, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_delay_exec_ci<double>;
            status = AUDIOLIB_delay_init_ci<double>(handle, bufParamsIn, bufParamsDelay, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   AUDIOLIB_DEBUGPRINTFN(0, "Exiting AUDIOLIB_delay_init func status: %d\n", status);

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_delay_exec(AUDIOLIB_kernelHandle handle,
                    void *restrict pIn,
                    void *restrict pDelay,
                    void *restrict pOut,
                    void *restrict pScratch)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delay_exec \n");

   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pDelay, pOut, pScratch);

   AUDIOLIB_DEBUGPRINTFN(0, "Exiting AUDIOLIB_delay_exec function with return status: %d\n", status);

   return status;
}
