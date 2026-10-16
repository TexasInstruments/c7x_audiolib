// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_gainSmoothing.cpp
*
*  @brief    Public API of AUDIOLIB_gainSmoothing: parameter checks, init, set/get and
*            the exec dispatcher.
*
*  @version  0.1 - Oct 2026 : Initial Version
*
*******************************************************************************
*/
#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_gainSmoothing_priv.h"
#include <cmath>

int32_t AUDIOLIB_gainSmoothing_getHandleSize(AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs)
{
   (void) pKerInitArgs; /* The handle size does not depend on the init arguments. */

   int32_t privBufSize = (int32_t) sizeof(AUDIOLIB_gainSmoothing_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_gainSmoothing_init_checkParams(AUDIOLIB_kernelHandle                  handle,
                                        const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                        const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                        const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainSmoothing_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      /* Only single precision is implemented. Accepting FLOAT64 here would let init and
         set succeed while leaving the execute pointer unassigned. */
      if (bufParamsIn->data_type != (uint32_t) AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (pKerInitArgs->samplingRate <= 0.0f) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else if (pKerInitArgs->isInterleaved > 1U) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else if ((bufParamsIn->dim_x == 0U) || (bufParamsIn->dim_y == 0U) ||
               (bufParamsOut->dim_x != bufParamsIn->dim_x) || (bufParamsOut->dim_y != bufParamsIn->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                        const void *restrict pIn,
                                                        const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainSmoothing_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_gainSmoothing_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainSmoothing_set \n");

   /* Validate the handle and the runtime-settable parameters before applying them. */
   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs->fadeType > AUDIOLIB_GAINSMOOTHING_FADE_TYPE_MAX) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if (pKerSetArgs->fadeTime < 0.0f) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if ((pKerSetArgs->fadeType == AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC) &&
            ((pKerSetArgs->currentGain <= 0.0f) || (pKerSetArgs->targetGain <= 0.0f))) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Nothing to do here */
   }

   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;

      // Copy arguments and set the start/end gains for the fade.
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_gainSmoothing_SetArgs));
      pKerPrivArgs->currentGain = pKerSetArgs->currentGain;
      pKerPrivArgs->targetGain  = pKerSetArgs->targetGain;

      pFxnAUDIOLIB_gainSmoothing_exec rampFuncCi = NULL;

      const uint32_t nElement = AUDIOLIB_GAINSMOOTHING_LANES_F32;

      // This switch configures the parameters for the chosen fade type
      switch (pKerSetArgs->fadeType) {
      case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC: {

         /* fadeTime < 0 was rejected above, so <= 0 means exactly 0 (no ramp). */
         const float totalFadeSamples  = (pKerSetArgs->fadeTime * pKerPrivArgs->samplingRate) / 1000.0f;
         pKerPrivArgs->bypassSmoothing = (uint8_t) ((pKerSetArgs->fadeTime <= 0.0f) || (totalFadeSamples <= 0.0f));
         pKerPrivArgs->fadeSamplesLeft = totalFadeSamples;
         if (pKerPrivArgs->bypassSmoothing == 0U) {
            pKerPrivArgs->gainStep =
                std::pow((pKerPrivArgs->targetGain / pKerPrivArgs->currentGain), 1.0f / totalFadeSamples);

            if (pKerPrivArgs->isInterleaved == 0U) {
               pKerPrivArgs->alphaCoeff[0] = pKerPrivArgs->gainStep;
               for (uint32_t i = 1; i < nElement; i++) {
                  pKerPrivArgs->alphaCoeff[i] = pKerPrivArgs->alphaCoeff[i - 1] * pKerPrivArgs->gainStep;
               }
               pKerPrivArgs->gainStepMultiplier = pKerPrivArgs->alphaCoeff[nElement - 1];
               pKerPrivArgs->gainStepMultiplierBi =
                   pKerPrivArgs->gainStepMultiplier * pKerPrivArgs->gainStepMultiplier;
            }
            pKerPrivArgs->factorPerBlock =
                (float) std::pow((double) pKerPrivArgs->gainStep, (double) pKerPrivArgs->inSamples);
         }
         else {
            pKerPrivArgs->gainStep = 1.0f;
         }

         if (pKerPrivArgs->isInterleaved == 0U) {
            rampFuncCi = AUDIOLIB_gainSmoothingSymmetricFade_exec_planar_ci<float>;
         }
         else {
            rampFuncCi = AUDIOLIB_gainSmoothingSymmetricFade_exec_lane_ci<float>;
         }
         break;
      }

      case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH: {
         pKerPrivArgs->bypassSmoothing = (uint8_t) (pKerSetArgs->fadeTime <= 0.0f);
         pKerPrivArgs->fadeSamplesLeft = 0.0f; /* SYMMETRIC only; cleared so a type switch leaves no stale count */
         if (pKerPrivArgs->bypassSmoothing == 0U) {
            const float timeConstant           = pKerSetArgs->fadeTime / 1000.0f;
            pKerPrivArgs->smoothingCoefficient = expf(-1.0f / (pKerPrivArgs->samplingRate * timeConstant));

            if (pKerPrivArgs->isInterleaved == 0U) {
               pKerPrivArgs->alphaCoeff[0] = pKerPrivArgs->smoothingCoefficient;
               for (uint32_t i = 1; i < nElement; i++) {
                  pKerPrivArgs->alphaCoeff[i] = pKerPrivArgs->alphaCoeff[i - 1] * pKerPrivArgs->smoothingCoefficient;
               }
               pKerPrivArgs->alphaMultiplier   = pKerPrivArgs->alphaCoeff[nElement - 1];
               pKerPrivArgs->alphaMultiplierBi = pKerPrivArgs->alphaMultiplier * pKerPrivArgs->alphaMultiplier;
            }
            pKerPrivArgs->factorPerBlock =
                (float) std::pow((double) pKerPrivArgs->smoothingCoefficient, (double) pKerPrivArgs->inSamples);
         }
         else {
            pKerPrivArgs->smoothingCoefficient = 0.0f;
         }

         rampFuncCi = (pKerPrivArgs->isInterleaved == 0U) ? AUDIOLIB_gainSmoothingSmoothFade_exec_planar_ci<float>
                                                          : AUDIOLIB_gainSmoothingSmoothFade_exec_lane_ci<float>;

         break;
      }

      default:
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }

      // Assign the final execute function pointers based on the chosen style.
      if (pKerPrivArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         switch (pKerSetArgs->fadeType) {
         case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC:
            pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_RAMP] =
                AUDIOLIB_gainSmoothingSymmetricFade_exec_cn<float>;
            break;

         case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH:
            pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_RAMP] = AUDIOLIB_gainSmoothingSmoothFade_exec_cn<float>;
            break;

         default:
            status = AUDIOLIB_ERR_INVALID_VALUE;
            break;
         }
         pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_CONVERGED] =
             pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_RAMP];
      }
      else {
         if (rampFuncCi == NULL) {
            status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
         }
         else {
            pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_CONVERGED] = AUDIOLIB_gainSmoothingBypass_exec_ci<float>;
            pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_RAMP] =
                (pKerPrivArgs->bypassSmoothing == 1U) ? pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_CONVERGED]
                                                   : rampFuncCi;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_gainSmoothing_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_gainSmoothing_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_init(AUDIOLIB_kernelHandle                  handle,
                                            AUDIOLIB_bufParams2D_t                *bufParamsIn,
                                            AUDIOLIB_bufParams2D_t                *bufParamsOut,
                                            const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                  status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainSmoothing_init \n");

   pKerPrivArgs->isInterleaved = pKerInitArgs->isInterleaved;
   pKerPrivArgs->samplingRate  = pKerInitArgs->samplingRate;
   pKerPrivArgs->funcStyle     = pKerInitArgs->funcStyle;

   pKerPrivArgs->numCols           = bufParamsIn->dim_x;
   pKerPrivArgs->numRows           = bufParamsIn->dim_y;
   pKerPrivArgs->strideInElements  = (uint32_t) (bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type));
   pKerPrivArgs->strideOutElements = (uint32_t) (bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type));

   if (pKerPrivArgs->isInterleaved == 1U) {
      pKerPrivArgs->inSamples = pKerPrivArgs->numRows;
   }
   else {
      pKerPrivArgs->inSamples = pKerPrivArgs->numCols;
   }

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
      status = AUDIOLIB_gainSmoothing_init_ci<float>(handle, pKerInitArgs);
   }
   else {
      /*does nothing*/
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_gainSmoothing_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainSmoothing_exec \n");

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;

   const uint32_t converged =
       (uint32_t) (fabsf(pKerPrivArgs->currentGain - pKerPrivArgs->targetGain) < AUDIOLIB_GAINSMOOTHING_GAIN_EPSILON);

   status = pKerPrivArgs->execute[converged](handle, pIn, pOut);

   return status;
}
