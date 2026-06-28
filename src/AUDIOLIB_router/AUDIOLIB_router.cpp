// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_router_priv.h"

int32_t AUDIOLIB_router_getHandleSize(AUDIOLIB_router_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_router_PrivArgs) + 6 * (pKerInitArgs->numOutputs) * sizeof(uint32_t);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_router_init_checkParams(AUDIOLIB_kernelHandle           handle,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                 const AUDIOLIB_bufParams1D_t   *buffParamsChannelIndex,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                 const AUDIOLIB_router_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_router_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (buffParamsChannelIndex == NULL) || (bufParamsOut == NULL) ||
       (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((bufParamsIn->dim_x > 32 && pKerInitArgs->isInterleave == 1) ||
               (bufParamsIn->dim_y > 32 && pKerInitArgs->isInterleave == 0)) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type ||
               buffParamsChannelIndex->data_type != bufParamsOut->data_type) {

         status = AUDIOLIB_ERR_INVALID_TYPE;
      }

      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_router_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                 const void *restrict pIn,
                                                 const void *restrict pOutChannels,
                                                 const void *restrict pOut,
                                                 const void *restrict pOutScratch)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_router_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOutChannels == NULL) || (pOut == NULL) || (pOutScratch == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_router_init(AUDIOLIB_kernelHandle           handle,
                                     AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                     AUDIOLIB_bufParams1D_t         *bufParamsInChannelIndex,
                                     AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                     const AUDIOLIB_router_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_router_PrivArgs *pKerPrivArgs = (AUDIOLIB_router_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_router_init \n");

   if ((pKerPrivArgs == NULL) || (bufParamsIn == NULL) || (bufParamsInChannelIndex == NULL) || (bufParamsOut == NULL) ||
       (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->isInterleave) {
         pKerPrivArgs->isInterleave   = 1;
         pKerPrivArgs->samples        = bufParamsIn->dim_y;
         pKerPrivArgs->outputChannels = bufParamsInChannelIndex->dim_x;
         pKerPrivArgs->strideIn       = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
         pKerPrivArgs->strideOut      = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
         pKerPrivArgs->inputChannels  = bufParamsIn->dim_x;
      }
      else {
         pKerPrivArgs->isInterleave = 0;

         pKerPrivArgs->outputChannels = bufParamsInChannelIndex->dim_x;
         pKerPrivArgs->samples        = bufParamsIn->dim_x;
         pKerPrivArgs->inputChannels  = bufParamsIn->dim_y;
         pKerPrivArgs->strideIn       = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
         pKerPrivArgs->strideOut      = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
      }
      pKerPrivArgs->data_type = bufParamsOut->data_type;

      uint8_t *ptr = (uint8_t *) (pKerPrivArgs + 1);

      // Assign outputOffsetArray
      pKerPrivArgs->outputOffsetArray = (uint32_t *) ptr;
      ptr += pKerPrivArgs->outputChannels * sizeof(uint32_t);

      pKerPrivArgs->muteArray = (int32_t *) ptr;
      ptr += pKerPrivArgs->outputChannels * sizeof(int32_t);

      pKerPrivArgs->runOffsetArray = (uint32_t *) ptr;
      ptr += pKerPrivArgs->outputChannels * sizeof(uint32_t);

      pKerPrivArgs->runLengthArray = (uint32_t *) ptr;
      ptr += pKerPrivArgs->outputChannels * sizeof(uint32_t);

      pKerPrivArgs->nVecs = (uint32_t *) ptr;
      ptr += pKerPrivArgs->outputChannels * sizeof(uint32_t);

      pKerPrivArgs->runMuteArray = (uint32_t *) ptr;
      ptr += pKerPrivArgs->outputChannels * sizeof(uint32_t);

      pKerPrivArgs->runCount = 0;

      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (pKerPrivArgs->isInterleave == 1) {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_routerInterLeave_exec_cn<float>;
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_routerInterLeave_exec_cn<double>;
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
         else {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_router_exec_cn<float>;
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_router_exec_cn<double>;
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
      }
      else {
         if (pKerPrivArgs->isInterleave == 1) {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_router_interLeave_exec_ci<float>;
               status                = AUDIOLIB_router_init_ci<float>(handle);
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_router_interLeave_exec_ci<double>;
               status                = AUDIOLIB_router_init_ci<double>(handle);
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
         else {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_router_exec_ci<float>;
               status                = AUDIOLIB_router_init_ci<float>(handle);
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_router_exec_ci<double>;
               status                = AUDIOLIB_router_init_ci<double>(handle);
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_router_set(AUDIOLIB_kernelHandle handle, void *restrict pInChannelIndex)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_router_PrivArgs *pKerPrivArgs = (AUDIOLIB_router_PrivArgs *) handle;

   if ((handle == NULL) || (pInChannelIndex == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      uint32_t  strideIn          = pKerPrivArgs->strideIn;
      uint32_t  outputChannels    = pKerPrivArgs->outputChannels;
      uint32_t  inChannels        = pKerPrivArgs->inputChannels;
      uint32_t *pOutChannelLocal  = (uint32_t *) pInChannelIndex;
      uint32_t *outputOffsetArray = pKerPrivArgs->outputOffsetArray;
      int32_t  *muteArray         = pKerPrivArgs->muteArray;
      uint32_t *runOffsetArray    = pKerPrivArgs->runOffsetArray;
      uint32_t *runLengthArray    = pKerPrivArgs->runLengthArray;
      uint32_t *runMuteArray      = pKerPrivArgs->runMuteArray;

      uint32_t i            = 0;
      uint32_t runCount     = 0;
      uint32_t offsetFactor = (pKerPrivArgs->isInterleave == 1) ? 1 : strideIn;

      while (i < outputChannels) {
         uint32_t ch_idx = pOutChannelLocal[i];

         if (ch_idx >= inChannels) {
            outputOffsetArray[i]     = 0;
            muteArray[i]             = 0;
            runMuteArray[runCount]   = 0;
            runOffsetArray[runCount] = 0;
            runLengthArray[runCount] = 1;
            runCount++;
            i++;
            continue;
         }

         // valid channel → start a run
         uint32_t start           = i;
         outputOffsetArray[start] = ch_idx * offsetFactor;
         muteArray[start]         = 1;

         uint32_t j = i + 1;
         while (j < outputChannels) {
            ch_idx = pOutChannelLocal[j];
            if (ch_idx >= inChannels)
               break;
            if (pOutChannelLocal[j] != pOutChannelLocal[j - 1] + 1)
               break;

            outputOffsetArray[j] = ch_idx * offsetFactor;
            muteArray[j]         = 1;
            j++;
         }

         // save run of valid channels
         runOffsetArray[runCount] = outputOffsetArray[start];
         runLengthArray[runCount] = j - start;
         runMuteArray[runCount]   = 1;
         runCount++;

         i = j; // continue from next channel
      }

      pKerPrivArgs->runCount = runCount;

      if (pKerPrivArgs->data_type == AUDIOLIB_FLOAT32) {
         if (pKerPrivArgs->isInterleave == 1) {
            status = AUDIOLIB_router_init_interLeave_set_ci<float>(handle);
         }
         else {
            status = AUDIOLIB_router_init_set_ci<float>(handle);
         }
      }
      else {
         if (pKerPrivArgs->isInterleave == 1) {
            status = AUDIOLIB_router_init_interLeave_set_ci<double>(handle);
         }
         else {
            status = AUDIOLIB_router_init_set_ci<double>(handle);
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_router_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut, void *restrict pOutScratch)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_router_exec \n");

   AUDIOLIB_router_PrivArgs *pKerPrivArgs = (AUDIOLIB_router_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut, pOutScratch);

   return status;
}
