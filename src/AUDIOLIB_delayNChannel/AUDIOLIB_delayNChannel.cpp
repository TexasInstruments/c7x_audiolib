// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_delayNChannel_priv.h"

int32_t AUDIOLIB_delayNChannel_getHandleSize(AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_delayNChannel_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_delayNChannel_init_checkParams(AUDIOLIB_kernelHandle                  handle,
                                        const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                        const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                        const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                        const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delayNChannel_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsDelay == NULL) || (bufParamsOut == NULL) ||
       (pKerInitArgs == NULL)) {
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

   if (status == AUDIOLIB_SUCCESS) {
      /* Kernel supports at most MAX_NUM_CHANNELS channels; channels = dim_x when interleaved,
       * else dim_y. Guards the fixed-size delaySize/readIdx/writeIdx[MAX_NUM_CHANNELS] arrays. */
      uint32_t numChannels =
          (pKerInitArgs->interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) ? bufParamsIn->dim_x : bufParamsIn->dim_y;
      if (numChannels > MAX_NUM_CHANNELS) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      /* Only a per-channel delay is applied, so the output grid matches the input grid. */
      if ((bufParamsIn->dim_x != bufParamsOut->dim_x) || (bufParamsIn->dim_y != bufParamsOut->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   AUDIOLIB_DEBUGPRINTFN(0, "Exiting AUDIOLIB_delayNChannel_init_checkParams: %d\n", status);

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_delayNChannel_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                        const void *restrict pIn,
                                                        const void *restrict pDelay,
                                                        const void *restrict pOut,
                                                        const void *restrict pScratch)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delayNChannel_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pDelay == NULL) || (pOut == NULL) || (pScratch == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_delayNChannel_init(AUDIOLIB_kernelHandle                  handle,
                                            AUDIOLIB_bufParams2D_t                *bufParamsIn,
                                            AUDIOLIB_bufParams2D_t                *bufParamsDelay,
                                            AUDIOLIB_bufParams2D_t                *bufParamsOut,
                                            const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                  status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delayNChannel_PrivArgs *pKerPrivArgs = (AUDIOLIB_delayNChannel_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delayNChannel_init \n");

   if ((pKerPrivArgs == NULL) || (bufParamsIn == NULL) || (bufParamsDelay == NULL) || (bufParamsOut == NULL) ||
       (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      pKerPrivArgs->mode                = pKerInitArgs->mode;
      pKerPrivArgs->interleave          = pKerInitArgs->interleave;
      pKerPrivArgs->maxDelay            = pKerInitArgs->maxDelay;
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

      /* Bound numChannels against the fixed-size delaySize/readIdx/writeIdx[MAX_NUM_CHANNELS]
       * arrays before indexing them (guards the copy loop below). */
      if (pKerPrivArgs->numChannels > MAX_NUM_CHANNELS) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      for (uint32_t ch = 0; ch < pKerPrivArgs->numChannels; ch++) {
         pKerPrivArgs->delaySize[ch] = pKerInitArgs->delaySize[ch];
      }

      if (pKerPrivArgs->mode == 1) {
         for (uint32_t ch = 0; ch < pKerPrivArgs->numChannels; ch++) {
            pKerPrivArgs->readIdx[ch]  = 0;
            pKerPrivArgs->writeIdx[ch] = pKerPrivArgs->delaySize[ch];
         }
      }

      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (pKerPrivArgs->interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_delayNChannel_interleave_exec_cn<float>;
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_delayNChannel_interleave_exec_cn<double>;
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
         else {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_delayNChannel_exec_cn<float>;
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_delayNChannel_exec_cn<double>;
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
      }
      else {
         if (pKerPrivArgs->interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_delayNChannel_interleave_exec_ci<float>;
               status = AUDIOLIB_delayNChannel_interleave_init_ci<float>(handle, bufParamsIn, bufParamsDelay,
                                                                         bufParamsOut, pKerInitArgs);
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_delayNChannel_interleave_exec_ci<double>;
               status = AUDIOLIB_delayNChannel_interleave_init_ci<double>(handle, bufParamsIn, bufParamsDelay,
                                                                          bufParamsOut, pKerInitArgs);
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
         else {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_delayNChannel_exec_ci<float>;
               status = AUDIOLIB_delayNChannel_init_ci<float>(handle, bufParamsIn, bufParamsDelay, bufParamsOut,
                                                              pKerInitArgs);
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_delayNChannel_exec_ci<double>;
               status = AUDIOLIB_delayNChannel_init_ci<double>(handle, bufParamsIn, bufParamsDelay, bufParamsOut,
                                                               pKerInitArgs);
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
      }
   }

   AUDIOLIB_DEBUGPRINTFN(0, "Exiting AUDIOLIB_delayNChannel_init func status: %d\n", status);

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_delayNChannel_exec(AUDIOLIB_kernelHandle handle,
                            void *restrict pIn,
                            void *restrict pDelay,
                            void *restrict pOut,
                            void *restrict pScratch)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delayNChannel_exec \n");

   AUDIOLIB_delayNChannel_PrivArgs *pKerPrivArgs = (AUDIOLIB_delayNChannel_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pDelay, pOut, pScratch);

   AUDIOLIB_DEBUGPRINTFN(0, "Exiting AUDIOLIB_delayNChannel_exec function with return status: %d\n", status);

   return status;
}
