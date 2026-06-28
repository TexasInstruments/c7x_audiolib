// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_balance_priv.h"
#include "AUDIOLIB_bufParams.h"

int32_t AUDIOLIB_balance_getHandleSize(AUDIOLIB_balance_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_balance_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_balance_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_balance_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_balance_init_checkParams \n");

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32)) {
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

AUDIOLIB_STATUS AUDIOLIB_balance_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                  const void *restrict pInL,
                                                  const void *restrict pInR,
                                                  const void *restrict pOutL,
                                                  const void *restrict pOutR)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_balance_exec_checkParams \n");

   if ((pInL == NULL) || (pInR == NULL) || (pOutL == NULL) || (pOutR == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_balance_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_balance_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_balance_PrivArgs *pKerPrivArgs = (AUDIOLIB_balance_PrivArgs *) handle;
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_balance_SetArgs));

      float angle               = (1.0f + pKerSetArgs->balance) * (M_PI / 4.0f);
      pKerPrivArgs->targetGainL = cosf(angle);
      pKerPrivArgs->targetGainR = sinf(angle);
      if (pKerSetArgs->smoothingTime != 0.0f) {
         pKerPrivArgs->bypassSmoothing = 0;

         pKerPrivArgs->smoothingCoefficient =
             expf(-1.0f / (pKerSetArgs->samplingRate * (pKerSetArgs->smoothingTime / 1000.0f)));

         // Pre-calculate a series of alpha^N coefficients and a multiplier.
         pKerPrivArgs->alphaCoeff[0] = 1.0f;
         for (int i = 1; i < ALPHA_COEFF_INDEX; i++) {
            pKerPrivArgs->alphaCoeff[i] = pKerPrivArgs->alphaCoeff[i - 1] * pKerPrivArgs->smoothingCoefficient;
         }

         float vecAlphaUpdate = pKerPrivArgs->alphaCoeff[ALPHA_COEFF_INDEX - 1];
         vecAlphaUpdate       = vecAlphaUpdate * (pKerPrivArgs->smoothingCoefficient);

         pKerPrivArgs->alphaMultiplier = vecAlphaUpdate;
      }
      else {
         pKerPrivArgs->bypassSmoothing = 1;
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_balance_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_balance_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_balance_PrivArgs *pKerPrivArgs = (AUDIOLIB_balance_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_balance_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_balance_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_balance_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_balance_PrivArgs *pKerPrivArgs = (AUDIOLIB_balance_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_balance_init \n");

   pKerPrivArgs->samples           = bufParamsIn->dim_x;
   pKerPrivArgs->channels          = bufParamsIn->dim_y;
   pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   // initialise currentGainL and currentGainR to zero
   pKerPrivArgs->currentGainL = 0.0f;
   pKerPrivArgs->currentGainR = 0.0f;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_balance_exec_cn<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status                = AUDIOLIB_balance_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_balance_exec_ci<float>;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_balance_exec(AUDIOLIB_kernelHandle handle,
                      void *restrict pInL,
                      void *restrict pInR,
                      void *restrict pOutL,
                      void *restrict pOutR)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_balance_exec \n");

   AUDIOLIB_balance_PrivArgs *pKerPrivArgs = (AUDIOLIB_balance_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pInL, pInR, pOutL, pOutR);

   return status;
}
