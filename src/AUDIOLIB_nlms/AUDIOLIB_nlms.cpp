// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_nlms_priv.h"

int32_t AUDIOLIB_nlms_getHandleSize(AUDIOLIB_nlms_InitArgs *pKerInitArgs)
{
   int32_t privArgsSize = (sizeof(AUDIOLIB_nlms_PrivArgs) + 7) & ~7;

   return privArgsSize;
}
AUDIOLIB_STATUS
AUDIOLIB_nlms_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               AUDIOLIB_bufParams2D_t       *bufParamsIn,
                               AUDIOLIB_bufParams2D_t       *bufParamsOut,
                               const AUDIOLIB_nlms_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_nlms_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL)) {
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

AUDIOLIB_STATUS AUDIOLIB_nlms_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                               void *restrict pIn,
                                               void *restrict pInRef,
                                               void *restrict pStateBuffer,   // Circular State Buffer
                                               void *restrict pScratchBuffer, // Accumulator Buffer
                                               void *restrict pCoefficients,
                                               void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_nlms_exec_checkParams \n");

   if ((pIn == NULL) || (pInRef == NULL) || (pOut == NULL) || (pScratchBuffer == NULL) || (pCoefficients == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_nlms_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_nlms_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_nlms_PrivArgs *pKerPrivArgs = (AUDIOLIB_nlms_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_nlms_init \n");

   pKerPrivArgs->dim_x             = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y             = bufParamsIn->dim_y;
   pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->stepSize          = pKerInitArgs->stepSize;
   pKerPrivArgs->filterLength      = pKerInitArgs->filterLength;
   pKerPrivArgs->regularization    = 0.001;
   pKerPrivArgs->totalSamples      = bufParamsIn->dim_x;

   // === CIRCULAR BUFFER SETUP ===
   uint32_t minRequired = pKerInitArgs->filterLength + pKerPrivArgs->totalSamples;

   uint32_t circBuffSize = 512;
   while (circBuffSize < minRequired) {
      circBuffSize *= 2;
   }
   pKerPrivArgs->circBuffSize        = circBuffSize;
   pKerPrivArgs->circBuffMask        = circBuffSize - 1;
   pKerPrivArgs->strideStateElements = circBuffSize;

   // Initialize the single shared indices.
   pKerPrivArgs->readIdx  = 0;
   pKerPrivArgs->writeIdx = pKerInitArgs->filterLength;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {

      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_nlms_exec_cn<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {

      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {

         pKerPrivArgs->execute = AUDIOLIB_nlms_exec_ci<float>;
         status                = AUDIOLIB_nlms_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_nlms_exec(AUDIOLIB_kernelHandle handle,
                   void *restrict pIn,
                   void *restrict pInRef,
                   void *restrict pStateBuffer,   // Circular State Buffer
                   void *restrict pScratchBuffer, // Accumulator Buffer
                   void *restrict pCoefficients,
                   void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_nlms_exec \n");

   AUDIOLIB_nlms_PrivArgs *pKerPrivArgs = (AUDIOLIB_nlms_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pInRef, pStateBuffer, pScratchBuffer, pCoefficients, pOut);

   return status;
}
