// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_gainNChTrim_priv.h"

int32_t AUDIOLIB_gainNChTrim_getHandleSize(AUDIOLIB_gainNChTrim_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_gainNChTrim_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_gainNChTrim_init_checkParams(AUDIOLIB_kernelHandle                handle,
                                      const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                      const AUDIOLIB_bufParams1D_t        *bufParamsGain,
                                      const AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                      const AUDIOLIB_gainNChTrim_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrim_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsGain == NULL) || (bufParamsOut == NULL) ||
       (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((bufParamsIn->data_type != bufParamsOut->data_type) ||
               (bufParamsGain->data_type != bufParamsOut->data_type)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else {
         /* Nothing to do here */
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      /* The gain vector holds one value per channel: channels = dim_x when
       * interleaved (channel-major columns), else dim_y (channel rows). */
      uint32_t numChannels = (pKerInitArgs->isInterleave == 1) ? bufParamsIn->dim_x : bufParamsIn->dim_y;
      if (bufParamsGain->dim_x != numChannels) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      /* Input and output describe the same M x N sample grid (only gain is applied),
       * so their dimensions must match. */
      if ((bufParamsIn->dim_x != bufParamsOut->dim_x) || (bufParamsIn->dim_y != bufParamsOut->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainNChTrim_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                      const void *restrict pIn,
                                                      const void *restrict pGain,
                                                      const void *restrict pMasterGain,
                                                      const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrim_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pGain == NULL) || (pMasterGain == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainNChTrim_init(AUDIOLIB_kernelHandle                handle,
                                          AUDIOLIB_bufParams2D_t              *bufParamsIn,
                                          AUDIOLIB_bufParams1D_t              *bufParamsGain,
                                          AUDIOLIB_bufParams2D_t              *bufParamsOut,
                                          const AUDIOLIB_gainNChTrim_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrim_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrim_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrim_init \n");

   if ((pKerPrivArgs == NULL) || (bufParamsIn == NULL) || (bufParamsGain == NULL) || (bufParamsOut == NULL) ||
       (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      pKerPrivArgs->dim_x     = bufParamsIn->dim_x;
      pKerPrivArgs->dim_y     = bufParamsIn->dim_y;
      pKerPrivArgs->strideIn  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
      pKerPrivArgs->strideOut = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);

      if (pKerInitArgs->isInterleave == 1) {

#if __C7X_VEC_SIZE_BITS__ == 256
         pKerPrivArgs->customImplementation =
             (((pKerPrivArgs->dim_x == 2) || (pKerPrivArgs->dim_x == 4)) &&
              (pKerPrivArgs->strideIn == pKerPrivArgs->dim_x) && (pKerPrivArgs->strideOut == pKerPrivArgs->dim_x));
#else
         pKerPrivArgs->customImplementation =
             (((pKerPrivArgs->dim_x == 2) || (pKerPrivArgs->dim_x == 4) || (pKerPrivArgs->dim_x == 8)) &&
              (pKerPrivArgs->strideIn == pKerPrivArgs->dim_x) && ((pKerPrivArgs->strideOut == pKerPrivArgs->dim_x)));
#endif

         pKerPrivArgs->isInterleave = 1;
      }
      else {
         pKerPrivArgs->isInterleave = 0;
      }

      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (pKerInitArgs->isInterleave == 1) {

            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_gainNChTrimInterLeave_exec_cn<float>;
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_gainNChTrimInterLeave_exec_cn<double>;
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
         else {
            if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
               pKerPrivArgs->execute = AUDIOLIB_gainNChTrim_exec_cn<float>;
            }
            else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
               pKerPrivArgs->execute = AUDIOLIB_gainNChTrim_exec_cn<double>;
            }
            else {
               status = AUDIOLIB_ERR_INVALID_TYPE;
            }
         }
      }
      else {

         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {

            pKerPrivArgs->execute = AUDIOLIB_gainNChTrim_exec_ci<float>;
            status =
                AUDIOLIB_gainNChTrim_init_ci<float>(handle, bufParamsIn, bufParamsGain, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_gainNChTrim_exec_ci<double>;
            status =
                AUDIOLIB_gainNChTrim_init_ci<double>(handle, bufParamsIn, bufParamsGain, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_gainNChTrim_exec(AUDIOLIB_kernelHandle handle,
                          void *restrict pIn,
                          void *restrict pGain,
                          void *restrict pMasterGain,
                          void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrim_exec \n");

   AUDIOLIB_gainNChTrim_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrim_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pGain, pMasterGain, pOut);

   return status;
}
