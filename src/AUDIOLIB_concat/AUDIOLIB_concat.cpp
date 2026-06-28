// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_concat_priv.h"

int32_t AUDIOLIB_concat_getHandleSize(AUDIOLIB_concat_InitArgs *pKerInitArgs)
{
   return sizeof(AUDIOLIB_concat_PrivArgs);
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
      if (pKerInitArgs->numInputs == 0 || pKerInitArgs->inChannels <= 0) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else if (pKerInitArgs->numInputs > MAX_SE_PARAMS) {
         /* Each input consumes one SE-template slot in the handle's bufPblock, which
          * is sized for MAX_SE_PARAMS inputs; reject more to avoid overrunning it. */
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else {
         for (uint32_t i = 0; i < pKerInitArgs->numInputs; i++) {
            if (bufParamsIn[i].data_type != AUDIOLIB_FLOAT32 && bufParamsIn[i].data_type != AUDIOLIB_FLOAT64) {
               status = AUDIOLIB_ERR_INVALID_TYPE;
               break;
            }
            if (bufParamsIn[i].data_type != bufParamsOut->data_type) {
               status = AUDIOLIB_ERR_INVALID_TYPE;
               break;
            }
            if (pKerInitArgs->isInterleave) {
               if (bufParamsIn[i].dim_x != (uint32_t) pKerInitArgs->inChannels) {
                  status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  break;
               }
            }
            else {
               if (bufParamsIn[i].dim_y != (uint32_t) pKerInitArgs->inChannels) {
                  status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  break;
               }
            }
         }

         if (status == AUDIOLIB_SUCCESS) {
            uint32_t totalOutChannels = (uint32_t) pKerInitArgs->inChannels * pKerInitArgs->numInputs;
            if (pKerInitArgs->isInterleave) {
               if (bufParamsOut->dim_x != totalOutChannels) {
                  status = AUDIOLIB_ERR_INVALID_DIMENSION;
               }
            }
            else {
               if (bufParamsOut->dim_y != totalOutChannels) {
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
      pKerPrivArgs->numInputs = pKerInitArgs->numInputs;

      pKerPrivArgs->isInterleave = pKerInitArgs->isInterleave;

      if (!pKerInitArgs->isInterleave) {
         pKerPrivArgs->inSamples  = bufParamsIn[0].dim_x;
         pKerPrivArgs->inChannels = pKerInitArgs->inChannels;
      }
      else {
         pKerPrivArgs->inSamples  = bufParamsIn[0].dim_y;
         pKerPrivArgs->inChannels = pKerInitArgs->inChannels;
      }

      pKerPrivArgs->strideIn  = bufParamsIn[0].stride_y / AUDIOLIB_sizeof(bufParamsIn[0].data_type);
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

         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {

            pKerPrivArgs->execute = AUDIOLIB_concat_exec_ci<float>;
            status                = AUDIOLIB_concat_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_concat_exec_ci<double>;
            status                = AUDIOLIB_concat_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
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
