// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_typeConversion_priv.h"

int32_t AUDIOLIB_typeConversion_getHandleSize(AUDIOLIB_typeConversion_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_typeConversion_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_typeConversion_init_checkParams(AUDIOLIB_kernelHandle                   handle,
                                         const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                         const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_typeConversion_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type == AUDIOLIB_INT16 &&
           (bufParamsOut->data_type == AUDIOLIB_FLOAT32 || bufParamsOut->data_type == AUDIOLIB_FLOAT64)) ||
          (bufParamsIn->data_type == AUDIOLIB_INT32 &&
           (bufParamsOut->data_type == AUDIOLIB_FLOAT32 || bufParamsOut->data_type == AUDIOLIB_FLOAT64)) ||
          (bufParamsOut->data_type == AUDIOLIB_INT16 &&
           (bufParamsIn->data_type == AUDIOLIB_FLOAT32 || bufParamsIn->data_type == AUDIOLIB_FLOAT64)) ||
          (bufParamsOut->data_type == AUDIOLIB_INT32 &&
           (bufParamsIn->data_type == AUDIOLIB_FLOAT32 || bufParamsIn->data_type == AUDIOLIB_FLOAT64))) {
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      /* Input and output describe the same M x N sample grid (only the element
       * type changes), so their dimensions must match. */
      if ((bufParamsIn->dim_x != bufParamsOut->dim_x) || (bufParamsIn->dim_y != bufParamsOut->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_typeConversion_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                         const void *restrict pIn,
                                         const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_typeConversion_exec_checkParams \n");

   if ((pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_typeConversion_init(AUDIOLIB_kernelHandle                   handle,
                                             AUDIOLIB_bufParams2D_t                 *bufParamsIn,
                                             AUDIOLIB_bufParams2D_t                 *bufParamsOut,
                                             const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_typeConversion_init \n");
   pKerPrivArgs->samples   = bufParamsIn->dim_x;
   pKerPrivArgs->channels  = bufParamsIn->dim_y;
   pKerPrivArgs->strideIn  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOut = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   if (bufParamsIn->data_type == AUDIOLIB_INT16) {
      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (bufParamsOut->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_q15ToFloat_exec_cn<float>;
         }
         else if (bufParamsOut->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_q15ToFloat_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsOut->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_q15ToFloat_exec_ci<float>;
            status                = AUDIOLIB_q15ToFloat_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsOut->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_q15ToFloat_exec_ci<double>;
            status = AUDIOLIB_q15ToFloat_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   else if (bufParamsOut->data_type == AUDIOLIB_INT16) {
      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ15_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ15_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ15_exec_ci<float>;
            status                = AUDIOLIB_floatToQ15_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ15_exec_ci<double>;
            status = AUDIOLIB_floatToQ15_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   else if (bufParamsIn->data_type == AUDIOLIB_INT32) {
      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (bufParamsOut->data_type == AUDIOLIB_FLOAT32 && pKerInitArgs->testQ31 == 0) {
            pKerPrivArgs->execute = AUDIOLIB_q23ToFloat_exec_cn<float>;
         }
         else if (bufParamsOut->data_type == AUDIOLIB_FLOAT32 && pKerInitArgs->testQ31 == 1) {
            pKerPrivArgs->execute = AUDIOLIB_q31ToFloat_exec_cn<float>;
         }
         else if (bufParamsOut->data_type == AUDIOLIB_FLOAT64 && pKerInitArgs->testQ31 == 0) {
            pKerPrivArgs->execute = AUDIOLIB_q23ToFloat_exec_cn<double>;
         }
         else if (bufParamsOut->data_type == AUDIOLIB_FLOAT64 && pKerInitArgs->testQ31 == 1) {
            pKerPrivArgs->execute = AUDIOLIB_q31ToFloat_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsOut->data_type == AUDIOLIB_FLOAT32 && pKerInitArgs->testQ31 == 0) {
            pKerPrivArgs->execute = AUDIOLIB_q23ToFloat_exec_ci<float>;
            status                = AUDIOLIB_q23ToFloat_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsOut->data_type == AUDIOLIB_FLOAT64 && pKerInitArgs->testQ31 == 0) {
            pKerPrivArgs->execute = AUDIOLIB_q23ToFloat_exec_ci<double>;
            status = AUDIOLIB_q23ToFloat_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsOut->data_type == AUDIOLIB_FLOAT32 && pKerInitArgs->testQ31 == 1) {
            pKerPrivArgs->execute = AUDIOLIB_q31ToFloat_exec_ci<float>;
            status                = AUDIOLIB_q23ToFloat_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsOut->data_type == AUDIOLIB_FLOAT64 && pKerInitArgs->testQ31 == 1) {
            pKerPrivArgs->execute = AUDIOLIB_q31ToFloat_exec_ci<double>;
            status = AUDIOLIB_q23ToFloat_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   else if (bufParamsOut->data_type == AUDIOLIB_INT32) {
      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32 && pKerInitArgs->testQ31 == 0) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ23_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64 && pKerInitArgs->testQ31 == 0) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ23_exec_cn<double>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT32 && pKerInitArgs->testQ31 == 1) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ31_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64 && pKerInitArgs->testQ31 == 1) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ31_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32 && pKerInitArgs->testQ31 == 0) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ23_exec_ci<float>;
            status                = AUDIOLIB_floatToQ23_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT32 && pKerInitArgs->testQ31 == 1) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ31_exec_ci<float>;
            status                = AUDIOLIB_floatToQ23_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64 && pKerInitArgs->testQ31 == 0) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ23_exec_ci<double>;
            status = AUDIOLIB_floatToQ23_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64 && pKerInitArgs->testQ31 == 1) {
            pKerPrivArgs->execute = AUDIOLIB_floatToQ31_exec_ci<double>;
            status = AUDIOLIB_floatToQ23_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   else {
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_typeConversion_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_typeConversion_exec \n");

   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);
   return status;
}
