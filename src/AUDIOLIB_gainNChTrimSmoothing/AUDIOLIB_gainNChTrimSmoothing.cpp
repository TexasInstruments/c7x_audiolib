// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_gainNChTrimSmoothing.cpp
*
*  @brief
*
*  @version  0.1 - Oct 2026 : Initial Version
*
*******************************************************************************
*/
#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_gainNChTrimSmoothing_priv.h"
#include <cmath>
#include <cstring> // memset, memcpy: cl7x does not pull these in through <cmath>

int32_t AUDIOLIB_gainNChTrimSmoothing_getHandleSize(AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_gainNChTrimSmoothing_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothing_init_checkParams(AUDIOLIB_kernelHandle                         handle,
                                               const AUDIOLIB_bufParams2D_t                 *bufParamsIn,
                                               const AUDIOLIB_bufParams2D_t                 *bufParamsOut,
                                               const AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrimSmoothing_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      uint32_t numChannels = (pKerInitArgs->isInterleaved != 0U) ? bufParamsIn->dim_x : bufParamsIn->dim_y;

      /* Only single precision is implemented. */
      if (bufParamsIn->data_type != AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((bufParamsIn->dim_x != bufParamsOut->dim_x) || (bufParamsIn->dim_y != bufParamsOut->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else if ((numChannels == 0U) || (numChannels > AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
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

AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                               const void *restrict pIn,
                                                               const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrimSmoothing_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_set(AUDIOLIB_kernelHandle                  handle,
                                                  AUDIOLIB_gainNChTrimSmoothing_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrimSmoothing_set \n");

   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs->trimGain == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs->fadeType > AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_MAX) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if (!(pKerSetArgs->fadeTime >= 0.0f)) {
      /* Written as a negated >= so that a NaN fadeTime is rejected too. */
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Nothing to do here */
   }

   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;

   uint32_t totalFadeSamples = 0U;

   if (status == AUDIOLIB_SUCCESS) {
      /* Fade length in samples, computed in single precision and truncated, as the
         reference model does. Only read once handle and pKerSetArgs are known valid. */
      float fadeSamplesF = (pKerSetArgs->fadeTime / 1000.0f) * pKerPrivArgs->samplingRate;
      if (fadeSamplesF >= (float) INT32_MAX) {
         /* remainingFadeSamples is signed 32-bit. */
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else {
         totalFadeSamples = (uint32_t) fadeSamplesF;
      }
   }

   if ((status == AUDIOLIB_SUCCESS) && (pKerSetArgs->fadeType == AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SYMMETRIC)) {
      for (uint32_t ch = 0; ch < pKerPrivArgs->numChannels; ch++) {
         float targetGain = pKerSetArgs->masterGain * pKerSetArgs->trimGain[ch];
         if (targetGain < 0.0f) {
            status = AUDIOLIB_ERR_INVALID_VALUE;
         }
         if ((pKerSetArgs->initialGain != NULL) && (pKerSetArgs->initialGain[ch] < 0.0f)) {
            status = AUDIOLIB_ERR_INVALID_VALUE;
         }
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_gainNChTrimSmoothing_SetArgs));

      /* One-pole weight of the target, k = 1 - exp(-1 / (fs * tau)), as in the reference model;
         fadeTime 0 gives k = 1, i.e. the target immediately. The kernels use the weight of the
         previous gain, 1 - k = exp(-1 / (fs * tau)), which is 0 for fadeTime 0. */
      float prevWeight = 0.0f;
      if (pKerSetArgs->fadeTime > 0.0f) {
         prevWeight =
             (float) (exp(-1.0 / ((double) pKerPrivArgs->samplingRate * (double) pKerSetArgs->fadeTime / 1000.0)));
      }
      pKerPrivArgs->smoothingCoeff = prevWeight;

      for (uint32_t ch = 0; ch < pKerPrivArgs->numChannels; ch++) {
         if (pKerSetArgs->initialGain != NULL) {
            /* Seed the channel at rest on the given gain. */
            pKerPrivArgs->currentGain[ch]          = pKerSetArgs->initialGain[ch];
            pKerPrivArgs->targetGain[ch]           = pKerSetArgs->initialGain[ch];
            pKerPrivArgs->gainStep[ch]             = 1.0f;
            pKerPrivArgs->remainingFadeSamples[ch] = 0;
         }

         float targetGain = pKerSetArgs->masterGain * pKerSetArgs->trimGain[ch];

         if (pKerSetArgs->fadeType == AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH) {
            pKerPrivArgs->targetGain[ch]           = targetGain;
            pKerPrivArgs->gainStep[ch]             = 1.0f;
            pKerPrivArgs->remainingFadeSamples[ch] = 0;
            continue;
         }

         /* Compare with the previous target, not the running gain: mid-fade the running
            gain never equals the target, and a channel whose gains did not change must
            keep its fade instead of restarting it with a full fadeTime. */
         float      prevTarget = pKerPrivArgs->targetGain[ch];
         const bool isChanged =
             !(fabsf(targetGain - prevTarget) <=
               (AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_ATOL + AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_RTOL * fabsf(prevTarget)));

         if ((!isChanged) && (pKerPrivArgs->remainingFadeSamples[ch] > 0)) {
            continue;
         }

         /* A channel at rest may still be off its target, e.g. after an unconverged SMOOTH
            run; fade it from where it is instead of snapping to the target. */
         float      curGain   = pKerPrivArgs->currentGain[ch];
         const bool offTarget =
             !(fabsf(targetGain - curGain) <=
               (AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_ATOL + AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_RTOL * fabsf(targetGain)));

         if ((isChanged || offTarget) && (totalFadeSamples > 0U)) {
            float fromGain = fmaxf(pKerPrivArgs->currentGain[ch], AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_FLOOR);
            float ratio    = fmaxf(targetGain, AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_FLOOR) / fromGain;

            pKerPrivArgs->currentGain[ch]          = fromGain;
            pKerPrivArgs->gainStep[ch]             = (float) pow((double) ratio, 1.0 / (double) totalFadeSamples);
            pKerPrivArgs->remainingFadeSamples[ch] = (int32_t) totalFadeSamples;
         }
         else {
            pKerPrivArgs->currentGain[ch]          = targetGain;
            pKerPrivArgs->gainStep[ch]             = 1.0f;
            pKerPrivArgs->remainingFadeSamples[ch] = 0;
         }
         pKerPrivArgs->targetGain[ch] = targetGain;
      }

      /* Every channel advances one frame per exec(), so the channel with the most fade left
         decides when the whole kernel has converged. */
      uint32_t maxRemaining = 0U;
      for (uint32_t ch = 0; ch < pKerPrivArgs->numChannels; ch++) {
         uint32_t remaining = (uint32_t) pKerPrivArgs->remainingFadeSamples[ch];
         maxRemaining       = (remaining > maxRemaining) ? remaining : maxRemaining;
      }
      pKerPrivArgs->maxRemainingFadeSamples = maxRemaining;

      pFxnAUDIOLIB_gainNChTrimSmoothing_exec rampFxn;
      pFxnAUDIOLIB_gainNChTrimSmoothing_exec convergedFxn;

      if (pKerPrivArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         /* The natural C variants handle the constant-gain case internally, so both table
            entries are the same function. */
         rampFxn      = (pKerSetArgs->fadeType == AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH)
                            ? AUDIOLIB_gainNChTrimSmoothingSmooth_exec_cn<float>
                            : AUDIOLIB_gainNChTrimSmoothingSymmetric_exec_cn<float>;
         convergedFxn = rampFxn;
      }
      else {
         /* The optimized kernels read per-lane power/target tables that depend on the stream
            layout chosen in init_ci; the C7x side fills them and picks the kernels. SMOOTH
            with a target weight of exactly 1 is the target itself, so it takes the
            constant-gain kernel. Row padding is masked by the SE/SA templates, so there is
            no frame-length-dependent variant to choose. */
         status = AUDIOLIB_gainNChTrimSmoothing_set_ci<float>(handle, pKerSetArgs->fadeType,
                                                              (uint8_t) (prevWeight == 0.0f), &rampFxn,
                                                              &convergedFxn);
      }

      pKerPrivArgs->execute[AUDIOLIB_GAINNCHTRIMSMOOTHING_EXEC_RAMP]      = rampFxn;
      pKerPrivArgs->execute[AUDIOLIB_GAINNCHTRIMSMOOTHING_EXEC_CONVERGED] = convergedFxn;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_get(AUDIOLIB_kernelHandle                  handle,
                                                  AUDIOLIB_gainNChTrimSmoothing_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_gainNChTrimSmoothing_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_init(AUDIOLIB_kernelHandle                         handle,
                                                   AUDIOLIB_bufParams2D_t                       *bufParamsIn,
                                                   AUDIOLIB_bufParams2D_t                       *bufParamsOut,
                                                   const AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrimSmoothing_init \n");

   pKerPrivArgs->isInterleaved = pKerInitArgs->isInterleaved;
   pKerPrivArgs->samplingRate  = pKerInitArgs->samplingRate;
   pKerPrivArgs->funcStyle     = pKerInitArgs->funcStyle;

   pKerPrivArgs->numSamples  = (pKerInitArgs->isInterleaved != 0U) ? bufParamsIn->dim_y : bufParamsIn->dim_x;
   pKerPrivArgs->numChannels = (pKerInitArgs->isInterleaved != 0U) ? bufParamsIn->dim_x : bufParamsIn->dim_y;

   pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   /* Unity gain, no fade in progress, until set() is called. The whole arrays are cleared,
      not just numChannels entries, because the optimized kernels load full vectors. */
   memset(&pKerPrivArgs->setArgs, 0, sizeof(AUDIOLIB_gainNChTrimSmoothing_SetArgs));
   pKerPrivArgs->maxRemainingFadeSamples = 0U;
   pKerPrivArgs->smoothingCoeff          = 0.0f;
   for (uint32_t ch = 0; ch < AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS; ch++) {
      pKerPrivArgs->currentGain[ch]          = 1.0f;
      pKerPrivArgs->targetGain[ch]           = 1.0f;
      pKerPrivArgs->gainStep[ch]             = 1.0f;
      pKerPrivArgs->remainingFadeSamples[ch] = 0;
   }
   /* No fade in progress, so both entries apply the (unity) target until set() is called. */
   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      pKerPrivArgs->execute[AUDIOLIB_GAINNCHTRIMSMOOTHING_EXEC_RAMP] =
          AUDIOLIB_gainNChTrimSmoothingSymmetric_exec_cn<float>;
      /* The natural C kernels handle the constant-gain case themselves. */
      pKerPrivArgs->bypassFxn = NULL;
   }
   else {
      /* One constant-gain kernel serves both layouts: init_ci stores its templates. */
      pKerPrivArgs->bypassFxn = AUDIOLIB_gainNChTrimSmoothingBypass_exec_ci<float>;
      pKerPrivArgs->execute[AUDIOLIB_GAINNCHTRIMSMOOTHING_EXEC_RAMP] = pKerPrivArgs->bypassFxn;
      status = AUDIOLIB_gainNChTrimSmoothing_init_ci<float>(handle, pKerInitArgs);
   }
   pKerPrivArgs->execute[AUDIOLIB_GAINNCHTRIMSMOOTHING_EXEC_CONVERGED] =
       pKerPrivArgs->execute[AUDIOLIB_GAINNCHTRIMSMOOTHING_EXEC_RAMP];

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothing_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNChTrimSmoothing_exec \n");

   AUDIOLIB_gainNChTrimSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrimSmoothing_PrivArgs *) handle;

   const uint32_t converged = (uint32_t) (pKerPrivArgs->maxRemainingFadeSamples == 0U);

   status = pKerPrivArgs->execute[converged](handle, pIn, pOut);

   pKerPrivArgs->maxRemainingFadeSamples = (pKerPrivArgs->maxRemainingFadeSamples > pKerPrivArgs->numSamples)
                                               ? (pKerPrivArgs->maxRemainingFadeSamples - pKerPrivArgs->numSamples)
                                               : 0U;

   return status;
}
