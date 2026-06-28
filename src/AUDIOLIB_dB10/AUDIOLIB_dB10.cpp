// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_dB10_priv.h"

int32_t AUDIOLIB_dB10_getHandleSize(AUDIOLIB_dB10_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_dB10_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_dB10_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_dB10_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_dB10_init_checkParams\n");
#endif
   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
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
AUDIOLIB_dB10_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_dB10_exec_checkParams\n");
#endif
   if ((pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_dB10_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_dB10_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT Enter AUDIOLIB_dB10_init\n");
#endif

   pKerPrivArgs->dim_x     = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y     = bufParamsIn->dim_y;
   pKerPrivArgs->inStride  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->outStride = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->dataType  = bufParamsIn->data_type;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_dB10_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_dB10_exec_cn<double>;
      }

      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
#if AUDIOLIB_DEBUGPRINT
         printf("AUDIOLIB_DEBUGPRINT  CP 2 status %d\n", status);
#endif
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {

#if AUDIOLIB_DEBUGPRINT
         printf("AUDIOLIB_DEBUGPRINT bufParamsIn->data_type == AUDIOLIB_FLOAT32\n");
#endif
         if (bufParamsIn->dim_x == 1) {
            if (bufParamsIn->dim_y == 1) {
               pKerPrivArgs->execute = AUDIOLIB_dB10_scaler_1x1_exec_ci<float>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_dB10_scaler_Mx1_exec_ci<float>;
            }
            status = AUDIOLIB_dB10_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else {
#if defined(__C7524__)
            pKerPrivArgs->execute = AUDIOLIB_dB10_vector_sp_exec_ci<float>;
#elif defined(__C7504__)
            if (bufParamsIn->dim_x > 1 && bufParamsIn->dim_y == 1) {
               pKerPrivArgs->execute = AUDIOLIB_dB10_scaler_sp_1xN_exec_ci<float>;
            }
            else if (pKerPrivArgs->inStride == pKerPrivArgs->dim_x && pKerPrivArgs->outStride == pKerPrivArgs->dim_x) {
               pKerPrivArgs->execute = AUDIOLIB_dB10_scaler_sp_1xN_exec_ci<float>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_dB10_scaler_sp_MxN_exec_ci<float>;
            }
#endif
            status = AUDIOLIB_dB10_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         if (bufParamsIn->dim_x == 1 && bufParamsIn->dim_y == 1) {
            pKerPrivArgs->execute = AUDIOLIB_dB10_scaler_1x1_exec_ci<double>;
         }
         else {
            if (bufParamsIn->dim_x < 32) {
               pKerPrivArgs->execute = AUDIOLIB_dB10_vector_dp_split1_exec_ci<double>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_dB10_vector_dp_split2_exec_ci<double>;
            }
         }
         status = AUDIOLIB_dB10_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT  CP 3 status %d\n", status);
#endif
   return status;
}

AUDIOLIB_STATUS AUDIOLIB_dB10_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT Enter AUDIOLIB_dB10_exec\n");
#endif

   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
