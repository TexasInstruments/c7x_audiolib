// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_inputAggregator_priv.h"

int32_t AUDIOLIB_inputAggregator_getHandleSize(AUDIOLIB_inputAggregator_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_inputAggregator_PrivArgs);
   privBufSize += 3 * sizeof(uint32_t) * pKerInitArgs->numInputs;
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_inputAggregator_init_checkParams(AUDIOLIB_kernelHandle                    handle,
                                          const AUDIOLIB_bufParams2D_t            *bufParamsIn,
                                          const AUDIOLIB_bufParams2D_t            *bufParamsOut,
                                          const AUDIOLIB_inputAggregator_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_inputAggregator_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->numInputs <= 0) {
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
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_inputAggregator_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                          const void **restrict pIn,
                                                          const void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_inputAggregator_PrivArgs *pKerPrivArgs = (AUDIOLIB_inputAggregator_PrivArgs *) handle;
   ;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_inputAggregator_exec_checkParams \n");

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

AUDIOLIB_STATUS AUDIOLIB_inputAggregator_init(AUDIOLIB_kernelHandle                    handle,
                                              AUDIOLIB_bufParams2D_t                  *bufParamsIn,
                                              AUDIOLIB_bufParams2D_t                  *bufParamsOut,
                                              const AUDIOLIB_inputAggregator_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_inputAggregator_PrivArgs *pKerPrivArgs = (AUDIOLIB_inputAggregator_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_inputAggregator_init \n");
   if (pKerPrivArgs == NULL || bufParamsIn == NULL || bufParamsOut == NULL || pKerInitArgs == NULL) {
      return AUDIOLIB_ERR_NULL_POINTER;
   }

   if (pKerInitArgs->numInputs <= 0) {
      return AUDIOLIB_ERR_INVALID_VALUE;
   }

   pKerPrivArgs->numInputs        = pKerInitArgs->numInputs;
   uint8_t *ptr                   = (uint8_t *) (pKerPrivArgs + 1);
   pKerPrivArgs->numInputChannels = (uint32_t *) ptr;
   ptr += pKerInitArgs->numInputs * sizeof(uint32_t);
   pKerPrivArgs->numInputSamples = (uint32_t *) ptr;
   ptr += pKerInitArgs->numInputs * sizeof(uint32_t);
   pKerPrivArgs->strideIn = (uint32_t *) ptr;

   pKerPrivArgs->isInputInterleave  = pKerInitArgs->isInputInterleave;
   pKerPrivArgs->isOutputInterleave = pKerInitArgs->isOutputInterleave;

   if (!pKerInitArgs->isInputInterleave) {
      for (uint32_t i = 0; i < pKerInitArgs->numInputs; i++) {
         pKerPrivArgs->numInputSamples[i]  = bufParamsIn[i].dim_x;
         pKerPrivArgs->numInputChannels[i] = bufParamsIn[i].dim_y;
         pKerPrivArgs->strideIn[i]         = bufParamsIn[0].stride_y / AUDIOLIB_sizeof(bufParamsIn[0].data_type);
      }
   }
   else {
      for (uint32_t i = 0; i < pKerInitArgs->numInputs; i++) {
         pKerPrivArgs->numInputSamples[i]  = bufParamsIn[i].dim_y;
         pKerPrivArgs->numInputChannels[i] = bufParamsIn[i].dim_x;
         pKerPrivArgs->strideIn[i]         = bufParamsIn[i].stride_y / AUDIOLIB_sizeof(bufParamsIn[i].data_type);
      }
   }
   // Set output parameters
   if (!pKerInitArgs->isOutputInterleave) {
      pKerPrivArgs->numOutputSamples  = bufParamsOut->dim_x;
      pKerPrivArgs->numOutputChannels = bufParamsOut->dim_y;
   }
   else {
      pKerPrivArgs->numOutputSamples  = bufParamsOut->dim_y;
      pKerPrivArgs->numOutputChannels = bufParamsOut->dim_x;
   }

   pKerPrivArgs->strideOut = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {

      if (bufParamsIn[0].data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_inputAggregator_exec_cn<float>;
      }
      else if (bufParamsIn[0].data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_inputAggregator_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {

      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {

         if (!pKerInitArgs->isInputInterleave && !pKerInitArgs->isOutputInterleave) {
            pKerPrivArgs->execute = AUDIOLIB_inputAggregatorDeinterleaveToDeinterleave_exec_ci<float>;
         }
         else {
            pKerPrivArgs->execute = AUDIOLIB_inputAggregatorGeneric_exec_ci<float>;
         }
         status = AUDIOLIB_inputAggregator_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         if (!pKerInitArgs->isInputInterleave && !pKerInitArgs->isOutputInterleave) {
            pKerPrivArgs->execute = AUDIOLIB_inputAggregatorDeinterleaveToDeinterleave_exec_ci<double>;
         }
         else {
            pKerPrivArgs->execute = AUDIOLIB_inputAggregatorGeneric_exec_ci<double>;
         }
         status = AUDIOLIB_inputAggregator_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_inputAggregator_exec(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_inputAggregator_exec \n");

   AUDIOLIB_inputAggregator_PrivArgs *pKerPrivArgs = (AUDIOLIB_inputAggregator_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
