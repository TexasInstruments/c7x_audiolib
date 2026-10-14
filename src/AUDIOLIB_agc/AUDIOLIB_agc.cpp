// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_agc_priv.h"
#include "AUDIOLIB_bufParams.h"
#include <string.h>

int32_t AUDIOLIB_agc_getHandleSize(AUDIOLIB_agc_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_agc_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_agc_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_agc_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_agc_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
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

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->dim_x == 0U) || (bufParamsIn->dim_y == 0U) || (bufParamsOut->dim_x != bufParamsIn->dim_x) ||
          (bufParamsOut->dim_y != bufParamsIn->dim_y) ||
          (bufParamsIn->stride_y < (int32_t) (bufParamsIn->dim_x * AUDIOLIB_sizeof(bufParamsIn->data_type))) ||
          (bufParamsOut->stride_y < (int32_t) (bufParamsOut->dim_x * AUDIOLIB_sizeof(bufParamsOut->data_type)))) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->samplingRate <= AUDIOLIB_AGC_SAMPLING_RATE_LOWER_BOUND) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_agc_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_agc_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_agc_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_agc_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_agc_PrivArgs *pKerPrivArgs = (AUDIOLIB_agc_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_agc_init \n");

   pKerPrivArgs->dataType = (AUDIOLIB_data_type_e) bufParamsIn->data_type;
   pKerPrivArgs->dim_x    = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y    = bufParamsIn->dim_y;
   pKerPrivArgs->strideIn = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOut = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->initArgs = *pKerInitArgs;

   pKerPrivArgs->oneOverNumSamples = 1.0f / ((float) pKerPrivArgs->dim_x * (float) pKerPrivArgs->dim_y);

   pKerPrivArgs->currentGain = pKerInitArgs->currentGain;
   pKerPrivArgs->targetGain  = AUDIOLIB_AGC_UNITY_GAIN;
   pKerPrivArgs->energy      = 0.0f;
   pKerPrivArgs->gainComputed = 0U;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_agc_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_agc_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_agc_exec_ci<float>;
         status                = AUDIOLIB_agc_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_agc_exec_ci<double>;
         status                = AUDIOLIB_agc_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_agc_set(AUDIOLIB_kernelHandle handle, const AUDIOLIB_agc_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_agc_set \n");

   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((pKerSetArgs->smoothingTime <= AUDIOLIB_AGC_SMOOTHING_TIME_LOWER_BOUND) ||
          (pKerSetArgs->ratio < AUDIOLIB_AGC_MIN_RATIO) || (pKerSetArgs->maxGain < AUDIOLIB_AGC_MIN_MAX_GAIN_DB) ||
          (pKerSetArgs->maxAttenuation < AUDIOLIB_AGC_MIN_MAX_ATTENUATION_DB) ||
          (pKerSetArgs->recoveryRate < AUDIOLIB_AGC_MIN_RECOVERY_RATE) ||
          ((pKerSetArgs->enableRecovery != AUDIOLIB_AGC_RECOVERY_DISABLE) &&
           (pKerSetArgs->enableRecovery != AUDIOLIB_AGC_RECOVERY_ENABLE))) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_agc_PrivArgs *pKerPrivArgs = (AUDIOLIB_agc_PrivArgs *) handle;
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_agc_SetArgs));

      float samplingRate     = (float) pKerPrivArgs->initArgs.samplingRate;
      float blockTime        = (float) pKerPrivArgs->dim_x / samplingRate;
      float smoothingTimeSec = pKerSetArgs->smoothingTime / AUDIOLIB_AGC_MS_PER_SECOND;
      float recoveryStepDB   = pKerSetArgs->recoveryRate * blockTime;

      pKerPrivArgs->oneOverSlope         = 1.0f - (1.0f / pKerSetArgs->ratio);
      pKerPrivArgs->smoothingCoeffSample = 1.0f - expf(-1.0f / (samplingRate * smoothingTimeSec));
      pKerPrivArgs->smoothingCoeffBlock  = 1.0f - expf(-blockTime / smoothingTimeSec);
      pKerPrivArgs->recoveryRateUp       = powf(10.0f, recoveryStepDB / 20.0f);
      pKerPrivArgs->recoveryRateDown     = powf(10.0f, -recoveryStepDB / 20.0f);
      /* The gain evolves once per sample across all channels, i.e. dim_x * dim_y steps per block */
      pKerPrivArgs->blockDecay = powf(1.0f - pKerPrivArgs->smoothingCoeffSample,
                                      (float) pKerPrivArgs->dim_x * (float) pKerPrivArgs->dim_y);

      if (pKerPrivArgs->initArgs.funcStyle != AUDIOLIB_FUNCTION_NATC) {
         if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
            status = AUDIOLIB_agc_set_ci<float>(handle, pKerSetArgs);
         }
         else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
            status = AUDIOLIB_agc_set_ci<double>(handle, pKerSetArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_agc_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_agc_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_agc_PrivArgs *pKerPrivArgs = (AUDIOLIB_agc_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_agc_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_agc_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_agc_exec \n");

   AUDIOLIB_agc_PrivArgs *pKerPrivArgs = (AUDIOLIB_agc_PrivArgs *) handle;

   if ((pKerPrivArgs == NULL) || (pIn == NULL) || (pOut == NULL)) {
      return AUDIOLIB_ERR_NULL_POINTER;
   }

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
