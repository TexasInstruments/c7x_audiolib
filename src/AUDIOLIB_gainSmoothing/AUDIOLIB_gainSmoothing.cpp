// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_gainSmoothing.cpp
*
*  @brief
*
*  @version  0.1 - Oct 2022 : Initial Version [AB]
*
*******************************************************************************
*/
#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_gainSmoothing_priv.h"
#include <cmath>

int32_t AUDIOLIB_gainSmoothing_getHandleSize(AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_gainSmoothing_PrivArgs);
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

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      /* Only single precision is implemented. Accepting FLOAT64 here would let init and
         set succeed while leaving the execute pointer unassigned. */
      if (bufParamsIn->data_type != AUDIOLIB_FLOAT32) {
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

   if ((pIn == NULL) || (pOut == NULL)) {
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
      /* The geometric ramp evaluates pow(target/current, 1/N). A zero start gain divides
         by zero and a negative gain makes pow() return NaN, so both are rejected. The
         one-pole ramp interpolates linearly and is well defined for any finite gain, so
         it is deliberately not constrained here. */
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Nothing to do here */
   }

   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;

      if (pKerPrivArgs->initialized != 1U) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }

      if (status == AUDIOLIB_SUCCESS) {
         // Copy arguments and set the start/end gains for the fade.
         memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_gainSmoothing_SetArgs));
         pKerPrivArgs->currentGain = pKerSetArgs->currentGain;
         pKerPrivArgs->targetGain  = pKerSetArgs->targetGain;

         // Ramp variant for the configured fade type and direction. Direction is baked into
         // the instantiation so that the exec functions never test it.
         pFxnAUDIOLIB_gainSmoothing_exec rampFuncCi = NULL;

         uint32_t nElement = AUDIOLIB_GAINSMOOTHING_LANES_F32;

         // This switch configures the parameters for the chosen fade type
         switch (pKerSetArgs->fadeType) {
         case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC: {
            float totalFadeSamples        = (pKerSetArgs->fadeTime * pKerPrivArgs->initArgs.samplingRate) / 1000.0f;
            pKerPrivArgs->bypassSmoothing = (pKerSetArgs->fadeTime == 0.0f || totalFadeSamples <= 0);
            if (!pKerPrivArgs->bypassSmoothing) {
               pKerPrivArgs->gainStep =
                   std::pow((pKerPrivArgs->targetGain / pKerPrivArgs->currentGain), 1 / totalFadeSamples);

               if (pKerPrivArgs->initArgs.isInterleaved) {
                  for (uint32_t i = 0; i < nElement; i++) {
                     pKerPrivArgs->alphaCoeff[i] = pKerPrivArgs->gainStep;
                  }
               }
               else {
                  float temp = 1;
                  for (uint32_t i = 0; i < nElement; i++) {
                     pKerPrivArgs->alphaCoeff[i] = pKerPrivArgs->gainStep * temp;
                     temp                        = pKerPrivArgs->alphaCoeff[i];
                  }
               }
               pKerPrivArgs->gainStepMultiplier = pKerPrivArgs->alphaCoeff[nElement - 1];
            }
            else {
               pKerPrivArgs->gainStep = 1.0f;
            }
            /* The ramp direction is fixed for the whole ramp, so bake it into the chosen
               instantiation rather than testing it inside the exec function. */
            rampFuncCi = (pKerPrivArgs->targetGain > pKerPrivArgs->currentGain)
                             ? AUDIOLIB_gainSmoothingSymmetricFade_exec_ci<float, true>
                             : AUDIOLIB_gainSmoothingSymmetricFade_exec_ci<float, false>;
            break;
         }

         case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH: {
            pKerPrivArgs->bypassSmoothing = (pKerSetArgs->fadeTime == 0.0f);
            if (!pKerPrivArgs->bypassSmoothing) {
               float timeConstant                 = pKerSetArgs->fadeTime / 1000.0f;
               pKerPrivArgs->smoothingCoefficient = expf(-1.0f / (pKerPrivArgs->initArgs.samplingRate * timeConstant));
               for (uint32_t i = 0; i < nElement; i++) {
                  if (pKerPrivArgs->initArgs.isInterleaved) {
                     pKerPrivArgs->alphaCoeff[i] = pKerPrivArgs->smoothingCoefficient;
                  }
                  else {
                     pKerPrivArgs->alphaCoeff[i] =
                         (i == 0) ? pKerPrivArgs->smoothingCoefficient
                                  : pKerPrivArgs->alphaCoeff[i - 1] * pKerPrivArgs->smoothingCoefficient;
                  }
               }
               pKerPrivArgs->alphaMultiplier = pKerPrivArgs->alphaCoeff[nElement - 1];
            }
            else {
               pKerPrivArgs->smoothingCoefficient = 0;
            }
            /* The one-pole ramp interpolates toward the target with no clamp, so it is
               direction-agnostic and needs only one instantiation. */
            rampFuncCi = AUDIOLIB_gainSmoothingSmoothFade_exec_ci<float>;
            break;
         }

         default:
            status = AUDIOLIB_ERR_INVALID_VALUE;
         }

         // Assign the final execute function pointers based on the chosen style.
         if (pKerPrivArgs->initArgs.funcStyle == AUDIOLIB_FUNCTION_NATC) {
            switch (pKerSetArgs->fadeType) {
            case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC:
               pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_RAMP] =
                   AUDIOLIB_gainSmoothingSymmetricFade_exec_cn<float>;

               break;

            case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH:
               pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_RAMP] =
                   AUDIOLIB_gainSmoothingSmoothFade_exec_cn<float>;

               break;

            default:
               status = AUDIOLIB_ERR_INVALID_VALUE;
            }
            /* The natural C variants handle the constant-gain case internally, so both table
               entries are the same function. */
            pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_CONVERGED] =
                pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_RAMP];
         }
         else {
            if (rampFuncCi == NULL) {
               status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
            }
            else {
               pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_CONVERGED] =
                   AUDIOLIB_gainSmoothingBypass_exec_ci<float>;
               /* fadeTime == 0 means there is no ramp at all, and the coefficient array is left
                  unpopulated in that case, so the ramp variants must never run. */
               pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_RAMP] =
                   pKerPrivArgs->bypassSmoothing ? pKerPrivArgs->execute[AUDIOLIB_GAINSMOOTHING_EXEC_CONVERGED]
                                                 : rampFuncCi;
            }
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
   pKerPrivArgs->initialized                     = 0U;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainSmoothing_init \n");

   memcpy(&pKerPrivArgs->initArgs, pKerInitArgs, sizeof(AUDIOLIB_gainSmoothing_InitArgs));

   pKerPrivArgs->numCols = bufParamsIn->dim_x;
   pKerPrivArgs->numRows = bufParamsIn->dim_y;

   pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
      status = AUDIOLIB_gainSmoothing_init_ci<float>(handle, pKerInitArgs);
   }
   else {
      /*does nothing*/
   }
   if (status == AUDIOLIB_SUCCESS) {
      pKerPrivArgs->initialized = 1U;
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
