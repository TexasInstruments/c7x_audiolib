// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_tableLookup_priv.h"

int32_t AUDIOLIB_tableLookup_getHandleSize(AUDIOLIB_tableLookup_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_tableLookup_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_tableLookup_init_checkParams(AUDIOLIB_kernelHandle                handle,
                                      const AUDIOLIB_bufParams1D_t        *bufParamsIn0,
                                      const AUDIOLIB_bufParams1D_t        *bufParamsIn1,
                                      const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                      const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_tableLookup_init_checkParams \n");

   if (handle == nullptr || bufParamsIn0 == nullptr || bufParamsIn1 == nullptr || bufParamsOut == nullptr ||
       pKerInitArgs == nullptr) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn0->data_type != AUDIOLIB_FLOAT32 || bufParamsIn1->data_type != AUDIOLIB_FLOAT32)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn0->data_type != bufParamsOut->data_type ||
               bufParamsIn1->data_type != bufParamsOut->data_type ||
               bufParamsIn0->data_type != bufParamsIn1->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_tableLookup_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                      const void *restrict pIn0,
                                                      const void *restrict pIn1,
                                                      const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_tableLookup_exec_checkParams \n");

   if ((pIn0 == nullptr) || (pIn1 == nullptr) || (pOut == nullptr)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_tableLookup_init(AUDIOLIB_kernelHandle                handle,
                                          const AUDIOLIB_bufParams1D_t        *bufParamsIn0,
                                          const AUDIOLIB_bufParams1D_t        *bufParamsIn1,
                                          const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                          const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_tableLookup_init \n");

   pKerPrivArgs->srcSamples   = bufParamsIn0->dim_x;
   pKerPrivArgs->tableSamples = bufParamsIn1->dim_x;
   pKerPrivArgs->minVal       = pKerInitArgs->minVal;
   pKerPrivArgs->maxVal       = pKerInitArgs->maxVal;
   pKerPrivArgs->divisor      = (pKerPrivArgs->tableSamples - 1) / (pKerPrivArgs->maxVal - pKerPrivArgs->minVal + 1e-9);

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn0->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_tableLookup_exec_cn<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
#if defined(__C7524__)
      if (pKerPrivArgs->tableSamples > pKerInitArgs->tableLookupSize) {
         if (bufParamsIn0->data_type == AUDIOLIB_FLOAT32) {
            status = AUDIOLIB_tableLookup_unroll_init_ci<float>(handle, bufParamsIn0, bufParamsIn1, bufParamsOut,
                                                                pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_tableLookup_unroll_exec_ci<float>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsIn0->data_type == AUDIOLIB_FLOAT32) {
            status =
                AUDIOLIB_tableLookup_init_ci<float>(handle, bufParamsIn0, bufParamsIn1, bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_tableLookup_exec_ci<float>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
#else
      if (bufParamsIn0->data_type == AUDIOLIB_FLOAT32) {
         status =
             AUDIOLIB_tableLookup_unroll_init_ci<float>(handle, bufParamsIn0, bufParamsIn1, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_tableLookup_unroll_exec_ci<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }

#endif
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_tableLookup_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn0, void *restrict pIn1, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_tableLookup_exec \n");

   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn0, pIn1, pOut);

   return status;
}
