// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_crossfade_priv.h"

int32_t AUDIOLIB_crossfade_getHandleSize(AUDIOLIB_crossfade_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_crossfade_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_crossfade_init_checkParams(AUDIOLIB_kernelHandle              handle,
                                    const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                    const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                    const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                    const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                    const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                    const AUDIOLIB_crossfade_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_crossfade_init_checkParams \n");

   if (handle == NULL || bufParamsIn1 == NULL || bufParamsIn2 == NULL || bufParamsIn3 == NULL || bufParamsIn3 == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (((bufParamsIn0->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn0->data_type != AUDIOLIB_FLOAT64)) ||
          ((bufParamsIn1->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn1->data_type != AUDIOLIB_FLOAT64)) ||
          ((bufParamsIn2->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn2->data_type != AUDIOLIB_FLOAT64)) ||
          ((bufParamsIn3->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn3->data_type != AUDIOLIB_FLOAT64))) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn0->data_type != bufParamsOut->data_type ||
               bufParamsIn1->data_type != bufParamsOut->data_type ||
               bufParamsIn2->data_type != bufParamsOut->data_type ||
               bufParamsIn3->data_type != bufParamsOut->data_type) {

         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn2->dim_x != bufParamsIn3->dim_x) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else if (((bufParamsIn0->dim_x != bufParamsIn2->dim_x || bufParamsIn1->dim_x != bufParamsIn3->dim_x) &&
                (pKerInitArgs->isInterleave == 0)) ||
               ((bufParamsIn0->dim_y != bufParamsIn2->dim_x || bufParamsIn0->dim_y != bufParamsIn3->dim_x) &&
                (pKerInitArgs->isInterleave == 1))) {

         status = AUDIOLIB_ERR_INVALID_VALUE;
      }

      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                    const void *restrict pIn0,
                                                    const void *restrict pIn1,
                                                    const void *restrict pIn2,
                                                    const void *restrict pIn3,
                                                    const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_crossfade_exec_checkParams \n");

   if ((pIn0 == NULL) || (pIn1 == NULL) || (pIn2 == NULL) || (pIn3 == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_crossfade_init(AUDIOLIB_kernelHandle              handle,
                                        const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                        const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                        const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                        const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                        const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                        const AUDIOLIB_crossfade_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS              status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_crossfade_init \n");

   if (pKerInitArgs->isInterleave == 1) {

      pKerPrivArgs->samples  = bufParamsIn0->dim_y;
      pKerPrivArgs->channels = bufParamsIn0->dim_x;
   }
   else {
      pKerPrivArgs->samples  = bufParamsIn0->dim_x;
      pKerPrivArgs->channels = bufParamsIn0->dim_y;
   }
   pKerPrivArgs->strideInElements  = bufParamsIn0->stride_y / AUDIOLIB_sizeof(bufParamsIn0->data_type);
   pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->isInterleave      = pKerInitArgs->isInterleave;

   if (pKerPrivArgs->isInterleave == 1) {

      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (bufParamsIn0->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_crossfadeInterleave_exec_cn<float>;
         }
         else if (bufParamsIn0->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_crossfadeInterleave_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsIn0->data_type == AUDIOLIB_FLOAT32) {
            status = AUDIOLIB_crossfadeInterleave_init_ci<float>(handle, bufParamsIn0, bufParamsIn1, bufParamsIn2,
                                                                 bufParamsIn3, bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_crossfadeInterleave_exec_ci<float>;
         }

         else if (bufParamsIn0->data_type == AUDIOLIB_FLOAT64) {
            status = AUDIOLIB_crossfadeInterleave_init_ci<double>(handle, bufParamsIn0, bufParamsIn1, bufParamsIn2,
                                                                  bufParamsIn3, bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_crossfadeInterleave_exec_ci<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   else {
      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (bufParamsIn0->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_crossfade_exec_cn<float>;
         }
         else if (bufParamsIn0->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_crossfade_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsIn0->data_type == AUDIOLIB_FLOAT32) {
            status = AUDIOLIB_crossfade_init_ci<float>(handle, bufParamsIn0, bufParamsIn1, bufParamsIn2, bufParamsIn3,
                                                       bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_crossfade_exec_ci<float>;
         }

         else if (bufParamsIn0->data_type == AUDIOLIB_FLOAT64) {
            status = AUDIOLIB_crossfade_init_ci<double>(handle, bufParamsIn0, bufParamsIn1, bufParamsIn2, bufParamsIn3,
                                                        bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_crossfade_exec_ci<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_crossfade_exec(AUDIOLIB_kernelHandle handle,
                        void *restrict pIn0,
                        void *restrict pIn1,
                        void *restrict pIn2,
                        void *restrict pIn3,
                        void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_crossfade_exec \n");

   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn0, pIn1, pIn2, pIn3, pOut);

   return status;
}
