// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_split_priv.h"

int32_t AUDIOLIB_split_getHandleSize(AUDIOLIB_split_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_split_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_split_init_checkParams(AUDIOLIB_kernelHandle          handle,
                                const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                const AUDIOLIB_split_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_split_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->numOutputs <= 0) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else {
         /* Channel axis depends on the input layout. The kernel splits the input evenly,
          * so the input channel count must equal numOutputs * outChannels (this also
          * rejects a non-divisible split that would otherwise silently drop channels). */
         uint32_t numInputChannels = (pKerInitArgs->isInputInterleave != 0U) ? bufParamsIn->dim_x : bufParamsIn->dim_y;
         if (numInputChannels != (pKerInitArgs->numOutputs * pKerInitArgs->outChannels)) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
         else {
            for (uint32_t i = 0; i < pKerInitArgs->numOutputs; i++) {
               uint32_t outChannels =
                   (pKerInitArgs->isInputInterleave != 0U) ? bufParamsOut[i].dim_x : bufParamsOut[i].dim_y;
               if (bufParamsOut[i].data_type != AUDIOLIB_FLOAT32 && bufParamsOut[i].data_type != AUDIOLIB_FLOAT64) {
                  status = AUDIOLIB_ERR_INVALID_TYPE;
                  break;
               }
               else if (bufParamsOut[i].data_type != bufParamsIn->data_type) {
                  status = AUDIOLIB_ERR_INVALID_TYPE;
                  break;
               }
               else if (outChannels != pKerInitArgs->outChannels) {
                  status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  break;
               }
               else {
                  /* Nothing to do here */
               }
            }
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_split_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void **restrict pOut)
{
   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_split_PrivArgs *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;
   ;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_split_exec_checkParams \n");

   if (handle == NULL || pIn == NULL || pOut == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {

      for (uint32_t i = 0; i < pKerPrivArgs->numOutputs; i++) {
         if (pOut[i] == NULL) {
            status = AUDIOLIB_ERR_NULL_POINTER;
            break;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_split_init(AUDIOLIB_kernelHandle          handle,
                                    AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                    AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                    const AUDIOLIB_split_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_split_PrivArgs *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_split_init \n");
   if (pKerPrivArgs == NULL || bufParamsIn == NULL || bufParamsOut == NULL || pKerInitArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      pKerPrivArgs->numOutputs = pKerInitArgs->numOutputs;
      if (!pKerInitArgs->isInputInterleave) {
         pKerPrivArgs->numInputSamples  = bufParamsIn->dim_x; // deinterleaved: dim_x = samples
         pKerPrivArgs->numInputChannels = bufParamsIn->dim_y; // deinterleaved: dim_y = channels
      }
      else {
         pKerPrivArgs->numInputSamples  = bufParamsIn->dim_y; // interleaved: dim_y = samples
         pKerPrivArgs->numInputChannels = bufParamsIn->dim_x; // interleaved: dim_x = channels
      }
      pKerPrivArgs->strideIn          = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
      pKerPrivArgs->strideOut         = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
      pKerPrivArgs->numOutputChannels = pKerPrivArgs->numInputChannels / pKerInitArgs->numOutputs;
      pKerPrivArgs->isInputInterleave = pKerInitArgs->isInputInterleave;

      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_split_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_split_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_split_exec_ci<float>;
            status                = AUDIOLIB_split_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_split_exec_ci<double>;
            status                = AUDIOLIB_split_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_split_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_split_exec \n");

   AUDIOLIB_split_PrivArgs *pKerPrivArgs = (AUDIOLIB_split_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
