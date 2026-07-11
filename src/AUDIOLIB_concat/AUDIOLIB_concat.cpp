// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_concat_priv.h"

int32_t AUDIOLIB_concat_getHandleSize(AUDIOLIB_concat_InitArgs *pKerInitArgs)
{
   if (pKerInitArgs == NULL || pKerInitArgs->numInputs == 0) {
      return -1;
   }
   uint32_t N          = pKerInitArgs->numInputs;
   int32_t  baseSize   = ((int32_t) sizeof(AUDIOLIB_concat_PrivArgs) + 63) & ~63;
   int32_t  arraySize  = ((int32_t)(2U * N * sizeof(uint32_t)) + 63) & ~63;
   int32_t  pblockSize =
       (int32_t)(N * SE_PARAM_SIZE + SA_PARAM_SIZE + N * SA_PARAM_SIZE + 2U * N * sizeof(uint32_t));
   return baseSize + arraySize + pblockSize;
}

AUDIOLIB_STATUS
AUDIOLIB_concat_init_checkParams(AUDIOLIB_kernelHandle           handle,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                 const AUDIOLIB_concat_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concat_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->numInputs == 0 || pKerInitArgs->inChannels == NULL) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else {
         uint32_t totalInChannels = 0;
         for (uint32_t i = 0; i < pKerInitArgs->numInputs; i++) {
            if (pKerInitArgs->inChannels[i] == 0) {
               status = AUDIOLIB_ERR_INVALID_VALUE;
               break;
            }
            if (bufParamsIn[i].data_type != AUDIOLIB_FLOAT32 && bufParamsIn[i].data_type != AUDIOLIB_FLOAT64) {
               status = AUDIOLIB_ERR_INVALID_TYPE;
               break;
            }
            if (bufParamsIn[i].data_type != bufParamsOut->data_type) {
               status = AUDIOLIB_ERR_INVALID_TYPE;
               break;
            }
            /* Each input may have its own channel count; validate against inChannels[i] */
            if (pKerInitArgs->isInterleave) {
               if (bufParamsIn[i].dim_x != pKerInitArgs->inChannels[i]) {
                  status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  break;
               }
            }
            else {
               if (bufParamsIn[i].dim_y != pKerInitArgs->inChannels[i]) {
                  status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  break;
               }
            }
            totalInChannels += pKerInitArgs->inChannels[i];
         }

         if (status == AUDIOLIB_SUCCESS) {
            /* Cross-check: caller-declared total must match computed sum */
            if (pKerInitArgs->totalInChannels != totalInChannels) {
               status = AUDIOLIB_ERR_INVALID_VALUE;
            }
         }

         if (status == AUDIOLIB_SUCCESS) {
            if (pKerInitArgs->isInterleave) {
               if (bufParamsOut->dim_x != totalInChannels) {
                  status = AUDIOLIB_ERR_INVALID_DIMENSION;
               }
            }
            else {
               if (bufParamsOut->dim_y != totalInChannels) {
                  status = AUDIOLIB_ERR_INVALID_DIMENSION;
               }
            }
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_concat_exec_checkParams(AUDIOLIB_kernelHandle handle, const void **restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_concat_PrivArgs *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concat_exec_checkParams \n");

   if (handle == NULL || pIn == NULL || pOut == NULL || pKerPrivArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      if (pKerPrivArgs->numInputs <= 0) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else {
         for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
            if (pIn[i] == NULL) {
               status = AUDIOLIB_ERR_NULL_POINTER;
               break;
            }
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_concat_init(AUDIOLIB_kernelHandle           handle,
                                     AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                     AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                     const AUDIOLIB_concat_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_concat_PrivArgs *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concat_init \n");

   if (pKerPrivArgs == NULL || bufParamsIn == NULL || bufParamsOut == NULL || pKerInitArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->numInputs == 0) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      pKerPrivArgs->numInputs    = pKerInitArgs->numInputs;
      pKerPrivArgs->isInterleave = pKerInitArgs->isInterleave;

      /* Set up tail pointers within the caller-allocated handle buffer. */
      uint32_t N      = pKerInitArgs->numInputs;
      uint8_t *tail   = (uint8_t *) handle + (((int32_t) sizeof(AUDIOLIB_concat_PrivArgs) + 63) & ~63);
      pKerPrivArgs->inChannels = (uint32_t *) tail;
      pKerPrivArgs->strideIn   = (uint32_t *)(tail + N * sizeof(uint32_t));
      pKerPrivArgs->bufPblock  = tail + (((int32_t)(2U * N * sizeof(uint32_t)) + 63) & ~63);

      pKerPrivArgs->inSamples         = pKerInitArgs->isInterleave ? bufParamsIn[0].dim_y : bufParamsIn[0].dim_x;
      pKerPrivArgs->totalInChannels   = 0;
      pKerPrivArgs->inChannelsUniform = 1;
      for (uint32_t i = 0; i < pKerInitArgs->numInputs; i++) {
         pKerPrivArgs->inChannels[i] =
             pKerInitArgs->isInterleave ? bufParamsIn[i].dim_x : bufParamsIn[i].dim_y;
         pKerPrivArgs->strideIn[i] = bufParamsIn[i].stride_y / AUDIOLIB_sizeof(bufParamsIn[i].data_type);
         pKerPrivArgs->totalInChannels += pKerPrivArgs->inChannels[i];
         if (pKerPrivArgs->inChannels[i] != pKerPrivArgs->inChannels[0]) {
            pKerPrivArgs->inChannelsUniform = 0;
         }
      }

      pKerPrivArgs->strideOut = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {

         if (bufParamsIn[0].data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_concat_exec_cn<float>;
         }
         else if (bufParamsIn[0].data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_concat_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {

         /* Planar, or interleaved with uniform channel counts, use the single-store fast
          * path; only interleaved with differing channel counts needs the per-input store. */
         uint8_t useSingleStore =
             ((pKerInitArgs->isInterleave == 0U) || (pKerPrivArgs->inChannelsUniform != 0U)) ? 1U : 0U;
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute =
                (useSingleStore != 0U) ? AUDIOLIB_concat_exec_ci<float> : AUDIOLIB_concatPerInputStore_exec_ci<float>;
            status = AUDIOLIB_concat_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute =
                (useSingleStore != 0U) ? AUDIOLIB_concat_exec_ci<double> : AUDIOLIB_concatPerInputStore_exec_ci<double>;
            status = AUDIOLIB_concat_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_concat_exec(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concat_exec \n");

   AUDIOLIB_concat_PrivArgs *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
