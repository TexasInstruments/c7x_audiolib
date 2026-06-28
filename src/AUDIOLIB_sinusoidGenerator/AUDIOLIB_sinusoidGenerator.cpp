// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_sinusoidGenerator_priv.h"
#include <stdio.h>
int32_t AUDIOLIB_sinusoidGenerator_getHandleSize(AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_sinusoidGenerator_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_sinusoidGenerator_init_checkParams(AUDIOLIB_kernelHandle                      handle,
                                            const AUDIOLIB_bufParams1D_t              *bufParamsOut,
                                            const AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_sinusoidGenerator_init_checkParams \n");

   if ((handle == NULL) || (bufParamsOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsOut->data_type != AUDIOLIB_FLOAT32) && (bufParamsOut->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }

      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_sinusoidGenerator_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_sinusoidGenerator_exec_checkParams \n");

   if (pOut == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_set(AUDIOLIB_kernelHandle               handle,
                                               AUDIOLIB_sinusoidGenerator_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS                      status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_sinusoidGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_sinusoidGenerator_set \n");

   memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_sinusoidGenerator_SetArgs));
   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      float PI            = 3.14159265358979323846;
      pKerPrivArgs->phase = pKerSetArgs->startPhase * PI / 180.0;
      if (pKerSetArgs->smoothingTime != 0.0f) {
         pKerPrivArgs->phaseIncTarget = 2 * PI * pKerSetArgs->targetFrequency / pKerSetArgs->samplingRate;
         pKerPrivArgs->phaseInc       = 2 * PI * pKerSetArgs->startFrequency / pKerSetArgs->samplingRate;
         pKerPrivArgs->smoothingCoefficient =
             1 - expf(-1 / (pKerSetArgs->samplingRate * pKerSetArgs->smoothingTime / 1000));
         pKerPrivArgs->oneMinusSmoothingCoefficient = 1 - pKerPrivArgs->smoothingCoefficient;
         pKerPrivArgs->alphaCoeff[0]                = 1.0f;
         pKerPrivArgs->ramp[0]                      = 0.0f;
         for (int i = 1; i < ALPHA_COEFF_INDEX; i++) {
            pKerPrivArgs->alphaCoeff[i] = pKerPrivArgs->alphaCoeff[i - 1] * pKerPrivArgs->oneMinusSmoothingCoefficient;
            pKerPrivArgs->ramp[i]       = i;
         }
         float vecAlphaUpdate = pKerPrivArgs->alphaCoeff[ALPHA_COEFF_INDEX - 1];
         vecAlphaUpdate       = vecAlphaUpdate * (pKerPrivArgs->oneMinusSmoothingCoefficient);

         pKerPrivArgs->alphaMultiplier = vecAlphaUpdate;
      }
      else {
         pKerPrivArgs->bypassSmoothing              = 1;
         pKerPrivArgs->smoothingCoefficient         = 1.0f;
         pKerPrivArgs->oneMinusSmoothingCoefficient = 0.0f;
         pKerPrivArgs->alphaMultiplier              = 0.0f;
         pKerPrivArgs->phaseIncTarget               = 2 * PI * pKerSetArgs->targetFrequency / pKerSetArgs->samplingRate;
         pKerSetArgs->startFrequency                = pKerSetArgs->targetFrequency;
         pKerPrivArgs->phaseInc                     = pKerPrivArgs->phaseIncTarget;

         for (int i = 0; i < ALPHA_COEFF_INDEX; i++) {
            pKerPrivArgs->alphaCoeff[i] = 0.0f;
            pKerPrivArgs->ramp[i]       = i;
         }
      }
      pKerPrivArgs->lastSampleIdx = ((pKerPrivArgs->numSamples - 1) % ALPHA_COEFF_INDEX);
   }
   return status;
}

AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_get(AUDIOLIB_kernelHandle               handle,
                                               AUDIOLIB_sinusoidGenerator_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_sinusoidGenerator_get \n");

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_sinusoidGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_sinusoidGenerator_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_init(AUDIOLIB_kernelHandle                      handle,
                                                AUDIOLIB_bufParams1D_t                    *bufParamsOut,
                                                const AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs)
{

   AUDIOLIB_STATUS                      status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_sinusoidGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_sinusoidGenerator_init \n");

   pKerPrivArgs->numSamples = bufParamsOut->dim_x;
   memcpy(&pKerPrivArgs->initArgs, pKerInitArgs, sizeof(AUDIOLIB_sinusoidGenerator_InitArgs));

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsOut->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_sinusoidGenerator_exec_cn<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (!pKerInitArgs->executionMode) {

         if (bufParamsOut->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_sinusoidGenerator_exec_scalar_ci<float>;
            status                = AUDIOLIB_sinusoidGenerator_init_ci<float>(handle, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {
         if (bufParamsOut->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_sinusoidGenerator_exec_vector_ci<float>;
            status                = AUDIOLIB_sinusoidGenerator_init_ci<float>(handle, bufParamsOut, pKerInitArgs);
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_sinusoidGenerator_exec(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_sinusoidGenerator_exec \n");

   AUDIOLIB_sinusoidGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pOut);

   return status;
}
