// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_rmsSmoothed_priv.h"

int32_t AUDIOLIB_rmsSmoothed_getHandleSize(AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_rmsSmoothed_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_init_checkParams(AUDIOLIB_kernelHandle                handle,
                                      const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                      const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                      const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_rmsSmoothed_init_checkParams \n");

   if (handle == nullptr || bufParamsIn == nullptr || bufParamsOut == nullptr || pKerInitArgs == nullptr) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->dim_x == 0 || bufParamsIn->dim_y == 0 || bufParamsOut->dim_x != 1) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else if ((bufParamsIn->dim_y > 1) &&
               (bufParamsIn->stride_y < (int32_t) (bufParamsIn->dim_x * AUDIOLIB_sizeof(bufParamsIn->data_type)))) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else if (pKerInitArgs->samplingRate <= 0) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_rmsSmoothed_exec_checkParams \n");

   if ((handle == nullptr) || (pIn == nullptr) || (pOut == nullptr)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_rmsSmoothed_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_rmsSmoothed_set \n");

   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs->smoothingTime < 0.0f) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Nothing to do here */
   }

   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_rmsSmoothed_PrivArgs *pKerPrivArgs = (AUDIOLIB_rmsSmoothed_PrivArgs *) handle;

      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_rmsSmoothed_SetArgs));

      if (pKerSetArgs->smoothingTime == 0.0f) {
         pKerPrivArgs->smoothingCoefficient = 0.0f;
      }
      else {
         pKerPrivArgs->smoothingCoefficient =
             expf(-2.0f * M_PI * pKerPrivArgs->blockSize /
                  (pKerPrivArgs->initArgs.samplingRate * (pKerSetArgs->smoothingTime / 1000.0f)));
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_rmsSmoothed_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_rmsSmoothed_PrivArgs *pKerPrivArgs = (AUDIOLIB_rmsSmoothed_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_rmsSmoothed_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_init(AUDIOLIB_kernelHandle                handle,
                                          const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                          const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                          const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_rmsSmoothed_PrivArgs *pKerPrivArgs = (AUDIOLIB_rmsSmoothed_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_rmsSmoothed_init \n");

   pKerPrivArgs->blockSize        = bufParamsIn->dim_x;
   pKerPrivArgs->numChannels      = bufParamsIn->dim_y;
   pKerPrivArgs->strideInElements = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->initArgs         = *pKerInitArgs;
   pKerPrivArgs->invNumSamples    = 1.0f / (float) (pKerPrivArgs->blockSize * pKerPrivArgs->numChannels);
   pKerPrivArgs->smoothedEnergy   = 0.0f;

   /* No smoothing until AUDIOLIB_rmsSmoothed_set is called */
   pKerPrivArgs->setArgs.smoothingTime = 0.0f;
   pKerPrivArgs->smoothingCoefficient  = 0.0f;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_rmsSmoothed_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_rmsSmoothed_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status                = AUDIOLIB_rmsSmoothed_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_rmsSmoothed_exec_ci<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         status                = AUDIOLIB_rmsSmoothed_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_rmsSmoothed_exec_ci<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_rmsSmoothed_exec \n");

   AUDIOLIB_rmsSmoothed_PrivArgs *pKerPrivArgs = (AUDIOLIB_rmsSmoothed_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
