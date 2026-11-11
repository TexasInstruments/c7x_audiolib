// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_muteNCh_priv.h"
#include <math.h>

int32_t AUDIOLIB_muteNCh_getHandleSize(AUDIOLIB_muteNCh_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_muteNCh_PrivArgs);
   // Calculate the number of channels aligned to the next multiple of ALPHA_COEFF_INDEX for vectorization
   uint32_t alignedChannels = ((pKerInitArgs->numChannels + 7) / ALPHA_COEFF_INDEX) * ALPHA_COEFF_INDEX;
   // Total size = size of the private struct + size of the 5 float arrays
   privBufSize += (5 * sizeof(float) * alignedChannels);
   if (!pKerInitArgs->isInterleaved) // Only allocate for deinterleaved case
   {
      privBufSize += (sizeof(float) * alignedChannels * ALPHA_COEFF_INDEX);
   }
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_muteNCh_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_muteNCh_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_muteNCh_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
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

   if (status == AUDIOLIB_SUCCESS) {
      /* getHandleSize sizes the per-channel arrays from pKerInitArgs->numChannels, but
       * init()/set() index them by the input buffer's channel axis (dim_x when interleaved,
       * else dim_y). They must agree, otherwise those arrays overrun the handle. */
      uint32_t numChannels = (pKerInitArgs->isInterleaved != 0U) ? bufParamsIn->dim_x : bufParamsIn->dim_y;
      if (numChannels != pKerInitArgs->numChannels) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      /* Only a per-channel gain is applied, so the output grid matches the input grid. */
      if ((bufParamsIn->dim_x != bufParamsOut->dim_x) || (bufParamsIn->dim_y != bufParamsOut->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_muteNCh_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_muteNCh_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_muteNCh_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_muteNCh_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_muteNCh_set \n");

   /* Validate the handle and the runtime-settable parameters before applying them. */
   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs->isMute == NULL) {
      /* isMute is a per-channel array of length numChannels. */
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs->fadeType > AUDIOLIB_MUTE_FADE_TYPE_SYMMETRIC) {
      /* fadeType must be one of AUDIOLIB_MUTE_FADE_TYPE_{LINEAR,SMOOTH,HARD,SYMMETRIC}. */
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if (pKerSetArgs->fadeTime < 0.0f) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Each per-channel mute flag must be boolean (0 = unmute, 1 = mute). */
      for (uint32_t ch = 0; ch < pKerPrivArgs->channels; ch++) {
         if ((pKerSetArgs->isMute[ch] != 0) && (pKerSetArgs->isMute[ch] != 1)) {
            status = AUDIOLIB_ERR_INVALID_VALUE;
         }
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      // STEP 1: Set up the targets from user arguments. remainingFadeSamples is recomputed for linear fades below.
      // currentGain is kept, so every fade starts from where its channel is: a channel whose flag did not change is
      // left alone, and a flag changed mid-fade turns the fade round without a jump. startGain is the far end of a full
      // fade, so the linear step is the same for every fade.
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_muteNCh_SetArgs));
      pKerPrivArgs->remainingFadeSamples = 0U;
      for (uint32_t ch = 0; ch < pKerPrivArgs->channels; ch++) {
         bool isMuting                = (pKerSetArgs->isMute[ch] == 1);
         pKerPrivArgs->startGain[ch]  = isMuting ? 1.0f : 0.0f;
         pKerPrivArgs->targetGain[ch] = isMuting ? 0.0f : 1.0f;
      }

      // STEP 2: Calculate fade parameters based on the selected fade type.
      switch (pKerSetArgs->fadeType) {
      case AUDIOLIB_MUTE_FADE_TYPE_LINEAR: {
         float totalFadeSamples =
             (pKerSetArgs->fadeTime * pKerPrivArgs->initArgs.samplingRate) / AUDIOLIB_MS_PER_SECOND;
         pKerPrivArgs->bypassSmoothing = (pKerSetArgs->fadeTime == 0.0f || totalFadeSamples <= 0);

         if (!pKerPrivArgs->bypassSmoothing) {
            for (uint32_t ch = 0; ch < pKerPrivArgs->channels; ch++) {
               // Calculate gainStep.
               pKerPrivArgs->gainStep[ch] =
                   (pKerPrivArgs->targetGain[ch] - pKerPrivArgs->startGain[ch]) / totalFadeSamples;
               // Samples until this channel reaches its target, plus one to absorb rounding of the ramp
               float distance = fabsf(pKerPrivArgs->targetGain[ch] - pKerPrivArgs->currentGain[ch]);
               if (distance > 0.0f) {
                  uint32_t remaining = (uint32_t) ceilf(distance * totalFadeSamples) + 1U;
                  if (remaining > pKerPrivArgs->remainingFadeSamples) {
                     pKerPrivArgs->remainingFadeSamples = remaining;
                  }
               }
            }

            // Pre-calculate vectors only needed for specific optimized paths.
            if (pKerPrivArgs->initArgs.funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
               if (!pKerPrivArgs->initArgs.isInterleaved) {
                  // This is for AUDIOLIB_muteNChLinearFadeDeinterleave_exec_ci
                  for (uint32_t ch = 0; ch < pKerPrivArgs->channels; ch++) {
                     for (uint32_t j = 0; j < ALPHA_COEFF_INDEX; j++) {
                        pKerPrivArgs->precalculatedGainVectors[ch * ALPHA_COEFF_INDEX + j] =
                            (j + 1) * pKerPrivArgs->gainStep[ch];
                     }
                  }
               }
            }
         }
         break;
      }

      case AUDIOLIB_MUTE_FADE_TYPE_SMOOTH: {
         pKerPrivArgs->bypassSmoothing = (pKerSetArgs->fadeTime == 0.0f);
         if (!pKerPrivArgs->bypassSmoothing) {
            float timeConstant                 = pKerSetArgs->fadeTime / AUDIOLIB_MS_PER_SECOND;
            pKerPrivArgs->smoothingCoefficient = expf(-1.0f / (pKerPrivArgs->initArgs.samplingRate * timeConstant));
            pKerPrivArgs->alphaN = powf(pKerPrivArgs->smoothingCoefficient, (float) pKerPrivArgs->samples);

            // This is for AUDIOLIB_muteNChExponentialFadeDeinterleave_exec_ci: alpha^1 .. alpha^ALPHA_COEFF_INDEX
            if (pKerPrivArgs->initArgs.funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
               for (int i = 0; i < ALPHA_COEFF_INDEX; i++) {
                  pKerPrivArgs->alphaCoeff[i] =
                      (i == 0) ? pKerPrivArgs->smoothingCoefficient
                               : pKerPrivArgs->alphaCoeff[i - 1] * pKerPrivArgs->smoothingCoefficient;
               }
               pKerPrivArgs->alphaMultiplier = pKerPrivArgs->alphaCoeff[ALPHA_COEFF_INDEX - 1];
            }

            // The fade runs on the distance from the target, which starts from where the channel is
            for (uint32_t ch = 0; ch < pKerPrivArgs->channels; ch++) {
               pKerPrivArgs->smoothDistance[ch] = pKerPrivArgs->currentGain[ch] - pKerPrivArgs->targetGain[ch];
            }
         }
         break;
      }

      case AUDIOLIB_MUTE_FADE_TYPE_SYMMETRIC: {
         float totalFadeSamples =
             (pKerSetArgs->fadeTime * pKerPrivArgs->initArgs.samplingRate) / AUDIOLIB_MS_PER_SECOND;
         pKerPrivArgs->bypassSmoothing = (pKerSetArgs->fadeTime == 0.0f || totalFadeSamples <= 0);

         if (!pKerPrivArgs->bypassSmoothing) {
            // Constant ratio per sample: the full floor-to-unity span (200 dB) takes fadeTime in either direction.
            // Direction 0 mutes, direction 1 unmutes; a channel's direction is its target gain.
            float ratio[AUDIOLIB_MUTENCH_SYMMETRIC_NUM_DIRECTIONS];
            ratio[1] = powf(1.0f / AUDIOLIB_MUTENCH_SYMMETRIC_FLOOR, 1.0f / totalFadeSamples);
            ratio[0] = 1.0f / ratio[1];
            for (uint32_t dir = 0; dir < AUDIOLIB_MUTENCH_SYMMETRIC_NUM_DIRECTIONS; dir++) {
               for (int i = 0; i < ALPHA_COEFF_INDEX; i++) {
                  pKerPrivArgs->symmetricLaneRatio[dir][i] =
                      (i == 0) ? ratio[dir] : pKerPrivArgs->symmetricLaneRatio[dir][i - 1] * ratio[dir];
               }
               float vectorRatio = pKerPrivArgs->symmetricLaneRatio[dir][ALPHA_COEFF_INDEX - 1];
               for (int k = 0; k < AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION; k++) {
                  pKerPrivArgs->symmetricVectorRatio[dir][k] =
                      (k == 0) ? vectorRatio : pKerPrivArgs->symmetricVectorRatio[dir][k - 1] * vectorRatio;
               }
               pKerPrivArgs->symmetricFrameRatio[dir] = powf(ratio[dir], (float) pKerPrivArgs->samples);
            }

            const float spanDecades = -log10f(AUDIOLIB_MUTENCH_SYMMETRIC_FLOOR);
            for (uint32_t ch = 0; ch < pKerPrivArgs->channels; ch++) {
               uint32_t dir               = (uint32_t) pKerPrivArgs->targetGain[ch];
               pKerPrivArgs->gainStep[ch] = ratio[dir];
               // The ramp starts from the running gain, no lower than one ratio step below the floor, so a channel
               // at 0 reaches the floor on its first sample, as the clamp of the natural C does.
               pKerPrivArgs->currentGain[ch] =
                   fmaxf(pKerPrivArgs->currentGain[ch], AUDIOLIB_MUTENCH_SYMMETRIC_FLOOR / ratio[dir]);
               // Samples until the target: the distance in decades over the 10-decade span, times the fade length,
               // plus one for the step below the floor and one to absorb rounding
               float startGain   = fminf(fmaxf(pKerPrivArgs->currentGain[ch], AUDIOLIB_MUTENCH_SYMMETRIC_FLOOR), 1.0f);
               float targetLevel = (dir == 1U) ? 1.0f : AUDIOLIB_MUTENCH_SYMMETRIC_FLOOR;
               float decades     = fabsf(log10f(targetLevel) - log10f(startGain));
               if (decades > 0.0f) {
                  uint32_t remaining = (uint32_t) ceilf(decades / spanDecades * totalFadeSamples) + 2U;
                  if (remaining > pKerPrivArgs->remainingFadeSamples) {
                     pKerPrivArgs->remainingFadeSamples = remaining;
                  }
               }
            }
         }
         break;
      }

      case AUDIOLIB_MUTE_FADE_TYPE_HARD: {
         pKerPrivArgs->bypassSmoothing = 1;
         break;
      }

      default: {
         /* Unrecognized fadeType: reject so a bad value never falls through
          * with stale configuration. */
         status = AUDIOLIB_ERR_INVALID_VALUE;
         break;
      }
      }

      // A hard change puts every channel on its target at once, so a later fade starts from there.
      if ((status == AUDIOLIB_SUCCESS) && (pKerPrivArgs->bypassSmoothing != 0)) {
         for (uint32_t ch = 0; ch < pKerPrivArgs->channels; ch++) {
            pKerPrivArgs->currentGain[ch] = pKerPrivArgs->targetGain[ch];
         }
      }

      // STEP 3: Assign the processing functions. A linear or symmetric fade switches to the hard path once every
      // channel has reached its target (remainingFadeSamples == 0); a smooth fade never does, so both entries are the
      // same.
      if (status == AUDIOLIB_SUCCESS) {
         FadeType effectiveFadeType =
             pKerPrivArgs->bypassSmoothing ? AUDIOLIB_MUTE_FADE_TYPE_HARD : (FadeType) pKerSetArgs->fadeType;
         const bool isNatC        = (pKerPrivArgs->initArgs.funcStyle == AUDIOLIB_FUNCTION_NATC);
         const bool isInterleaved = pKerPrivArgs->initArgs.isInterleaved;

         pFxnAUDIOLIB_muteNCh_exec hardFxn      = AUDIOLIB_muteNChHard_exec_ci<float>;
         pFxnAUDIOLIB_muteNCh_exec linearFxn    = AUDIOLIB_muteNChLinearFadeDeinterleave_exec_ci<float>;
         pFxnAUDIOLIB_muteNCh_exec smoothFxn    = AUDIOLIB_muteNChExponentialFadeDeinterleave_exec_ci<float>;
         pFxnAUDIOLIB_muteNCh_exec symmetricFxn = AUDIOLIB_muteNChSymmetricFadeDeinterleave_exec_ci<float>;
         if (isNatC) {
            hardFxn      = AUDIOLIB_muteNChHard_exec_cn<float>;
            linearFxn    = AUDIOLIB_muteNChLinearFade_exec_cn<float>;
            smoothFxn    = AUDIOLIB_muteNChSmoothFade_exec_cn<float>;
            symmetricFxn = AUDIOLIB_muteNChSymmetricFade_exec_cn<float>;
         }
         else if (isInterleaved) {
            linearFxn    = AUDIOLIB_muteNChLinearFadeInterleave_exec_ci<float>;
            smoothFxn    = AUDIOLIB_muteNChExponentialFadeInterleave_exec_ci<float>;
            symmetricFxn = AUDIOLIB_muteNChSymmetricFadeInterleave_exec_ci<float>;
         }
         else {
            /* Nothing to do here */
         }

         pFxnAUDIOLIB_muteNCh_exec fadingFxn = hardFxn;
         if (effectiveFadeType == AUDIOLIB_MUTE_FADE_TYPE_LINEAR) {
            fadingFxn = linearFxn;
         }
         else if (effectiveFadeType == AUDIOLIB_MUTE_FADE_TYPE_SMOOTH) {
            fadingFxn = smoothFxn;
            hardFxn   = smoothFxn;
         }
         else if (effectiveFadeType == AUDIOLIB_MUTE_FADE_TYPE_SYMMETRIC) {
            fadingFxn = symmetricFxn;
         }
         else {
            /* AUDIOLIB_MUTE_FADE_TYPE_HARD: both entries are the hard path */
         }
         pKerPrivArgs->execute[AUDIOLIB_MUTENCH_EXEC_FADING]    = fadingFxn;
         pKerPrivArgs->execute[AUDIOLIB_MUTENCH_EXEC_CONVERGED] = hardFxn;
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_muteNCh_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_muteNCh_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_muteNCh_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_muteNCh_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_muteNCh_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_muteNCh_init \n");

   if ((pKerPrivArgs == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      memcpy(&pKerPrivArgs->initArgs, pKerInitArgs, sizeof(AUDIOLIB_muteNCh_InitArgs));

      if (pKerPrivArgs->initArgs.isInterleaved) {
         pKerPrivArgs->samples           = bufParamsIn->dim_y;
         pKerPrivArgs->channels          = bufParamsIn->dim_x;
         pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
         pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
      }
      else {
         pKerPrivArgs->samples           = bufParamsIn->dim_x;
         pKerPrivArgs->channels          = bufParamsIn->dim_y;
         pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
         pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
      }

      // Calculate the number of channels aligned to the next multiple of 8
      uint32_t alignedChannels = ((pKerPrivArgs->channels + 7) / ALPHA_COEFF_INDEX) * ALPHA_COEFF_INDEX;

      // Get a pointer to the memory area immediately following the private args struct
      uint8_t *ptr = (uint8_t *) (pKerPrivArgs + 1);

      // Assign array pointers from the contiguous block of memory
      pKerPrivArgs->gainStep = (float *) ptr;
      ptr += sizeof(float) * alignedChannels;
      pKerPrivArgs->startGain = (float *) ptr;
      ptr += sizeof(float) * alignedChannels;
      pKerPrivArgs->currentGain = (float *) ptr;
      ptr += sizeof(float) * alignedChannels;
      pKerPrivArgs->targetGain = (float *) ptr;
      ptr += sizeof(float) * alignedChannels;
      pKerPrivArgs->smoothDistance = (float *) ptr;
      ptr += sizeof(float) * alignedChannels;
      if (!pKerPrivArgs->initArgs.isInterleaved) {
         pKerPrivArgs->precalculatedGainVectors = (float *) ptr;
      }

      memset(pKerPrivArgs->gainStep, 0, 5 * sizeof(float) * alignedChannels);
      pKerPrivArgs->remainingFadeSamples = 0U;

      if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            status = AUDIOLIB_muteNCh_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_muteNCh_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_muteNCh_exec \n");

   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;

   const uint32_t converged = (uint32_t) (pKerPrivArgs->remainingFadeSamples == 0U);

   status = pKerPrivArgs->execute[converged](handle, pIn, pOut);

   pKerPrivArgs->remainingFadeSamples = (pKerPrivArgs->remainingFadeSamples > pKerPrivArgs->samples)
                                            ? (pKerPrivArgs->remainingFadeSamples - pKerPrivArgs->samples)
                                            : 0U;

   return status;
}
