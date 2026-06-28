// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_mute_priv.h"

int32_t AUDIOLIB_mute_getHandleSize(AUDIOLIB_mute_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_mute_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_mute_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_mute_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_mute_init_checkParams \n");

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

AUDIOLIB_STATUS
AUDIOLIB_mute_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_mute_exec_checkParams \n");

   if ((pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_mute_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_mute_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_mute_set \n");

   /* Validate the handle and the runtime-settable parameters before applying them. */
   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs->isMute > 1U) {
      /* isMute is a boolean switch: 1 = mute, 0 = unmute. */
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if (pKerSetArgs->fadeType > AUDIOLIB_MUTE_FADE_TYPE_HARD) {
      /* fadeType must be one of AUDIOLIB_MUTE_FADE_TYPE_{LINEAR,SMOOTH,HARD}. */
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if (pKerSetArgs->fadeTime < 0.0f) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Nothing to do here */
   }

   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;

      // Copy arguments and set the start/end gains for the fade.
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_mute_SetArgs));
      pKerPrivArgs->currentGain = pKerSetArgs->isMute ? 1.0f : 0.0f;
      pKerPrivArgs->targetGain  = pKerSetArgs->isMute ? 0.0f : 1.0f;

      // Temporary function pointers for the optimized implementations.
      pFxnAUDIOLIB_mute_exec muteFuncCi   = NULL;
      pFxnAUDIOLIB_mute_exec unmuteFuncCi = NULL;

      // This switch configures the parameters for the chosen fade type
      switch (pKerSetArgs->fadeType) {
      case AUDIOLIB_MUTE_FADE_TYPE_LINEAR: {
         float totalFadeSamples =
             (pKerSetArgs->fadeTime * pKerPrivArgs->initArgs.samplingRate) / AUDIOLIB_MS_PER_SECOND;
         pKerPrivArgs->bypassSmoothing = (pKerSetArgs->fadeTime == 0.0f || totalFadeSamples <= 0);

         if (!pKerPrivArgs->bypassSmoothing) {
            pKerPrivArgs->gainStep = (pKerPrivArgs->targetGain - pKerPrivArgs->currentGain) / totalFadeSamples;
            for (int i = 0; i < ALPHA_COEFF_INDEX; i++) {
               if (pKerPrivArgs->initArgs.isInterleaved) {
                  pKerPrivArgs->alphaCoeff[i]    = pKerPrivArgs->gainStep;
                  pKerPrivArgs->gainStepAdder[i] = pKerPrivArgs->gainStep;
               }
               else {
                  pKerPrivArgs->alphaCoeff[i]    = (i + 1) * pKerPrivArgs->gainStep;
                  pKerPrivArgs->gainStepAdder[i] = 8 * pKerPrivArgs->gainStep;
               }
            }
            pKerPrivArgs->alphaMultiplier = 0.0f;
         }
         muteFuncCi   = AUDIOLIB_muteLinearFade_exec_ci<float>;
         unmuteFuncCi = AUDIOLIB_unMuteLinearFade_exec_ci<float>;
         break;
      }

      case AUDIOLIB_MUTE_FADE_TYPE_SMOOTH: {
         pKerPrivArgs->bypassSmoothing = (pKerSetArgs->fadeTime == 0.0f);
         if (!pKerPrivArgs->bypassSmoothing) {
            float timeConstant                 = pKerSetArgs->fadeTime / AUDIOLIB_MS_PER_SECOND;
            pKerPrivArgs->smoothingCoefficient = expf(-1.0f / (pKerPrivArgs->initArgs.samplingRate * timeConstant));
            for (int i = 0; i < ALPHA_COEFF_INDEX; i++) {
               if (pKerPrivArgs->initArgs.isInterleaved) {
                  pKerPrivArgs->alphaCoeff[i] = pKerPrivArgs->smoothingCoefficient;
               }
               else {
                  pKerPrivArgs->alphaCoeff[i] =
                      (i == 0) ? pKerPrivArgs->smoothingCoefficient
                               : pKerPrivArgs->alphaCoeff[i - 1] * pKerPrivArgs->smoothingCoefficient;
               }
            }
            pKerPrivArgs->alphaMultiplier = pKerPrivArgs->alphaCoeff[ALPHA_COEFF_INDEX - 1];
         }
         muteFuncCi   = AUDIOLIB_muteSmoothFade_exec_ci<float>;
         unmuteFuncCi = AUDIOLIB_unMuteSmoothFade_exec_ci<float>;
         break;
      }

      case AUDIOLIB_MUTE_FADE_TYPE_HARD: {
         pKerPrivArgs->bypassSmoothing = 1;
         muteFuncCi                    = AUDIOLIB_hardMute_exec_ci<float>;
         unmuteFuncCi                  = AUDIOLIB_hardUnMute_exec_ci<float>;
         break;
      }

      default: {
         /* Unreachable: fadeType range is validated above, but keep the
          * branch explicit so an unrecognized value never falls through
          * with stale configuration. */
         status = AUDIOLIB_ERR_INVALID_VALUE;
         break;
      }
      }

      if (status == AUDIOLIB_SUCCESS) {
         // Assign the final execute function pointer based on the chosen style.
         if (pKerPrivArgs->initArgs.funcStyle == AUDIOLIB_FUNCTION_NATC) {
            // If smoothing is bypassed, always use the 'Hard' function.
            if (pKerPrivArgs->bypassSmoothing) {
               pKerPrivArgs->execute = AUDIOLIB_muteHard_exec_cn<float>;
            }
            else {
               // Otherwise, select the appropriate natural C fade function.
               switch (pKerPrivArgs->setArgs.fadeType) {
               case AUDIOLIB_MUTE_FADE_TYPE_LINEAR:
                  pKerPrivArgs->execute = AUDIOLIB_muteLinearFade_exec_cn<float>;
                  break;

               case AUDIOLIB_MUTE_FADE_TYPE_SMOOTH:
                  pKerPrivArgs->execute = AUDIOLIB_muteSmoothFade_exec_cn<float>;
                  break;

               default:
                  /* Nothing to do here */
                  break;
               }
            }
         }
         else {
            // Use the optimized implementation for the selected fade type and direction.
            pKerPrivArgs->execute = pKerPrivArgs->setArgs.isMute ? muteFuncCi : unmuteFuncCi;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_mute_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_mute_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_mute_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_mute_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_mute_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_mute_init \n");

   memcpy(&pKerPrivArgs->initArgs, pKerInitArgs, sizeof(AUDIOLIB_mute_InitArgs));

   pKerPrivArgs->samples           = bufParamsIn->dim_x;
   pKerPrivArgs->channels          = bufParamsIn->dim_y;
   pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_mute_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_mute_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_mute_exec \n");

   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
