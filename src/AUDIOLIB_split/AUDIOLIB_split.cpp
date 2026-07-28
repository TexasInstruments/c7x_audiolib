// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_split_priv.h"

int32_t AUDIOLIB_split_getHandleSize(AUDIOLIB_split_InitArgs *pKerInitArgs)
{
   if (pKerInitArgs == NULL || pKerInitArgs->numOutputs == 0) {
      return -1;
   }
   uint32_t N          = pKerInitArgs->numOutputs;
   int32_t  baseSize   = ((int32_t) sizeof(AUDIOLIB_split_PrivArgs) + 63) & ~63;
   int32_t  arraySize  = ((int32_t)(2U * N * sizeof(uint32_t)) + 63) & ~63;
   int32_t  pblockSize =
       (int32_t)(SE_PARAM_SIZE + N * SE_PARAM_SIZE + N * SA_PARAM_SIZE + 2U * N * sizeof(uint32_t));
   return baseSize + arraySize + pblockSize;
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
      if (pKerInitArgs->numOutputs <= 0 || pKerInitArgs->outChannels == NULL) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else {
         /* Channel axis depends on the input layout. The input channel count must equal
          * the sum of the per-output channel counts (this also rejects a split that would
          * otherwise silently drop or duplicate channels). */
         uint32_t numInputChannels =
             (pKerInitArgs->isInputInterleave != 0U) ? bufParamsIn->dim_x : bufParamsIn->dim_y;
         uint32_t totalOutChannels = 0;
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
            else if (outChannels != pKerInitArgs->outChannels[i]) {
               status = AUDIOLIB_ERR_INVALID_DIMENSION;
               break;
            }
            else {
               totalOutChannels += outChannels;
            }
         }

         if (status == AUDIOLIB_SUCCESS && numInputChannels != totalOutChannels) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
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
      if (pKerInitArgs->numOutputs == 0) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      pKerPrivArgs->numOutputs        = pKerInitArgs->numOutputs;
      pKerPrivArgs->isInputInterleave = pKerInitArgs->isInputInterleave;

      /* Set up tail pointers within the caller-allocated handle buffer. */
      uint32_t N      = pKerInitArgs->numOutputs;
      uint8_t *tail   = (uint8_t *) handle + (((int32_t) sizeof(AUDIOLIB_split_PrivArgs) + 63) & ~63);
      pKerPrivArgs->outChannels = (uint32_t *) tail;
      pKerPrivArgs->strideOut   = (uint32_t *)(tail + N * sizeof(uint32_t));
      pKerPrivArgs->bufPblock   = tail + (((int32_t)(2U * N * sizeof(uint32_t)) + 63) & ~63);

      if (!pKerInitArgs->isInputInterleave) {
         pKerPrivArgs->numInputSamples  = bufParamsIn->dim_x;  // deinterleaved: dim_x = samples
         pKerPrivArgs->numInputChannels = bufParamsIn->dim_y;  // deinterleaved: dim_y = channels
      }
      else {
         pKerPrivArgs->numInputSamples  = bufParamsIn->dim_y;  // interleaved: dim_y = samples
         pKerPrivArgs->numInputChannels = bufParamsIn->dim_x;  // interleaved: dim_x = channels
      }
      pKerPrivArgs->strideIn = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);

      pKerPrivArgs->outChannelsUniform = 1;
      for (uint32_t i = 0; i < pKerInitArgs->numOutputs; i++) {
         pKerPrivArgs->outChannels[i] =
             pKerInitArgs->isInputInterleave ? bufParamsOut[i].dim_x : bufParamsOut[i].dim_y;
         pKerPrivArgs->strideOut[i] = bufParamsOut[i].stride_y / AUDIOLIB_sizeof(bufParamsOut[i].data_type);
         if (pKerPrivArgs->outChannels[i] != pKerPrivArgs->outChannels[0]) {
            pKerPrivArgs->outChannelsUniform = 0;
         }
      }

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
         /* Planar, or interleaved with uniform channel counts, use the single-load fast path;
          * only interleaved with differing channel counts needs the per-output load. */
         uint8_t useSingleLoad =
             ((pKerInitArgs->isInputInterleave == 0U) || (pKerPrivArgs->outChannelsUniform != 0U)) ? 1U : 0U;
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute =
                (useSingleLoad != 0U) ? AUDIOLIB_split_exec_ci<float> : AUDIOLIB_splitPerOutput_exec_ci<float>;
            status = AUDIOLIB_split_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute =
                (useSingleLoad != 0U) ? AUDIOLIB_split_exec_ci<double> : AUDIOLIB_splitPerOutput_exec_ci<double>;
            status = AUDIOLIB_split_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
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
