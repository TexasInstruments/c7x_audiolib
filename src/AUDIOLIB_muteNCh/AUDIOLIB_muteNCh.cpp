// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_muteNCh_priv.h"

int32_t AUDIOLIB_muteNCh_getHandleSize(AUDIOLIB_muteNCh_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_muteNCh_PrivArgs);
   // Calculate the number of channels aligned to the next multiple of ALPHA_COEFF_INDEX for vectorization
   uint32_t alignedChannels = ((pKerInitArgs->numChannels + 7) / ALPHA_COEFF_INDEX) * ALPHA_COEFF_INDEX;
   // Total size = size of the private struct + size of the 4 float arrays
   privBufSize += (4 * sizeof(float) * alignedChannels);
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
   else if (pKerSetArgs->fadeType > AUDIOLIB_MUTE_FADE_TYPE_HARD) {
      /* fadeType must be one of AUDIOLIB_MUTE_FADE_TYPE_{LINEAR,SMOOTH,HARD}. */
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
      // STEP 1: Set up initial state from user arguments.
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_muteNCh_SetArgs));
      for (uint32_t ch = 0; ch < pKerPrivArgs->channels; ch++) {
         bool isMuting                 = (pKerSetArgs->isMute[ch] == 1);
         pKerPrivArgs->currentGain[ch] = isMuting ? 1.0f : 0.0f;
         pKerPrivArgs->startGain[ch]   = pKerPrivArgs->currentGain[ch];
         pKerPrivArgs->targetGain[ch]  = isMuting ? 0.0f : 1.0f;
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
            }

            // Pre-calculate vectors only needed for specific optimized paths.
            if (pKerPrivArgs->initArgs.funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
               if (pKerPrivArgs->initArgs.isInterleaved) {
                  // This is for AUDIOLIB_muteNChLinearFadeInterleave_exec_ci
                  for (int i = 0; i < ALPHA_COEFF_INDEX; i++) {
                     pKerPrivArgs->alphaCoeff[i] = (i + 1) * pKerPrivArgs->gainStep[0];
                  }
               }
               else {
                  // This is for AUDIOLIB_muteNChLinearFade(Unrolled)Deinterleave_exec_ci
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

            // This is for the optimized smooth fade functions.
            if (pKerPrivArgs->initArgs.funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
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

      // STEP 3: Assign the correct processing function based on the configuration.
      if (status == AUDIOLIB_SUCCESS) {
         FadeType effectiveFadeType =
             pKerPrivArgs->bypassSmoothing ? AUDIOLIB_MUTE_FADE_TYPE_HARD : (FadeType) pKerSetArgs->fadeType;

         if (pKerPrivArgs->initArgs.funcStyle == AUDIOLIB_FUNCTION_NATC) {
            switch (effectiveFadeType) {
            case AUDIOLIB_MUTE_FADE_TYPE_LINEAR:
               pKerPrivArgs->execute = AUDIOLIB_muteNChLinearFade_exec_cn<float>;
               break;
            case AUDIOLIB_MUTE_FADE_TYPE_SMOOTH:
               pKerPrivArgs->execute = AUDIOLIB_muteNChSmoothFade_exec_cn<float>;
               break;
            case AUDIOLIB_MUTE_FADE_TYPE_HARD:
               pKerPrivArgs->execute = AUDIOLIB_muteNChHard_exec_cn<float>;
               break;
            default: /* Nothing to do here */
               break;
            }
         }
         else // AUDIOLIB_FUNCTION_OPTIMIZED
         {
            const bool isInterleaved = pKerPrivArgs->initArgs.isInterleaved;
            const bool useUnrolled   = (pKerPrivArgs->samples % 8 == 0);

            switch (effectiveFadeType) {
            case AUDIOLIB_MUTE_FADE_TYPE_LINEAR:
               if (isInterleaved) {
                  pKerPrivArgs->execute = AUDIOLIB_muteNChLinearFadeInterleave_exec_ci<float>;
               }
               else {
                  pKerPrivArgs->execute = useUnrolled ? AUDIOLIB_muteNChLinearFadeUnrolledDeinterleave_exec_ci<float>
                                                      : AUDIOLIB_muteNChLinearFadeDeinterleave_exec_ci<float>;
               }
               break;

            case AUDIOLIB_MUTE_FADE_TYPE_SMOOTH:
               if (isInterleaved) {
                  pKerPrivArgs->execute = AUDIOLIB_muteNChExponentialFadeInterleave_exec_ci<float>;
               }
               else {
                  pKerPrivArgs->execute = AUDIOLIB_muteNChExponentialFadeDeinterleave_exec_ci<float>;
               }
               break;

            case AUDIOLIB_MUTE_FADE_TYPE_HARD:
               pKerPrivArgs->execute = isInterleaved ? AUDIOLIB_hardMuteInterleave_exec_ci<float>
                                                     : AUDIOLIB_hardMuteDeinterleave_exec_ci<float>;
               break;

            default: /* Nothing to do here */
               break;
            }
         }
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
      if (!pKerPrivArgs->initArgs.isInterleaved) {
         pKerPrivArgs->precalculatedGainVectors = (float *) ptr;
      }

      memset(pKerPrivArgs->gainStep, 0, 4 * sizeof(float) * alignedChannels);

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

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
