// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_agc_priv.h"
#include <cstdint>

void AUDIOLIB_agc_updateTargetGain(AUDIOLIB_agc_PrivArgs *pKerPrivArgs, float energySum)
{
   const AUDIOLIB_agc_SetArgs *pPrm = &pKerPrivArgs->setArgs;

   float blockEnergy    = energySum * pKerPrivArgs->oneOverNumSamples;
   float energy         = (pKerPrivArgs->energy * (1.0f - pKerPrivArgs->smoothingCoeffBlock)) +
                          (blockEnergy * pKerPrivArgs->smoothingCoeffBlock);
   pKerPrivArgs->energy = energy;

   /* log10(0) is -inf, which is always below the activation threshold */
   float level = (energy > 0.0f) ? (10.0f * log10f(energy)) : -FLT_MAX;

   pKerPrivArgs->gainComputed = (level >= pPrm->activationThreshold) ? 1U : 0U;

   if (level >= pPrm->activationThreshold) {
      float gainDB = (pPrm->targetLevel - level) * pKerPrivArgs->oneOverSlope;
      gainDB       = (gainDB > pPrm->maxGain) ? pPrm->maxGain : gainDB;
      gainDB       = (gainDB < -pPrm->maxAttenuation) ? -pPrm->maxAttenuation : gainDB;

      pKerPrivArgs->targetGain = powf(10.0f, gainDB / 20.0f);
   }
   else if (pPrm->enableRecovery != AUDIOLIB_AGC_RECOVERY_DISABLE) {
      float targetGain = pKerPrivArgs->targetGain;
      if (targetGain > AUDIOLIB_AGC_UNITY_GAIN) {
         targetGain *= pKerPrivArgs->recoveryRateDown;
         targetGain = (targetGain < AUDIOLIB_AGC_UNITY_GAIN) ? AUDIOLIB_AGC_UNITY_GAIN : targetGain;
      }
      else if (targetGain < AUDIOLIB_AGC_UNITY_GAIN) {
         targetGain *= pKerPrivArgs->recoveryRateUp;
         targetGain = (targetGain > AUDIOLIB_AGC_UNITY_GAIN) ? AUDIOLIB_AGC_UNITY_GAIN : targetGain;
      }
      else {
         /* Already at 0 dB */
      }
      pKerPrivArgs->targetGain = targetGain;
   }
   else {
      /* Hold the target gain */
   }
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_agc_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_agc_PrivArgs *pKerPrivArgs = (AUDIOLIB_agc_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_agc_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t dim_x    = pKerPrivArgs->dim_x;
   uint32_t dim_y    = pKerPrivArgs->dim_y;
   int32_t  strideIn = pKerPrivArgs->strideIn;

   dataType energySum = 0;
   for (uint32_t m = 0; m < dim_y; m++) {
#pragma UNROLL(1)
      for (uint32_t n = 0; n < dim_x; n++) {
         dataType x = pInLocal[m * strideIn + n];
         energySum += x * x;
      }
   }

   AUDIOLIB_agc_updateTargetGain(pKerPrivArgs, (float) energySum);

   dataType targetGain  = (dataType) pKerPrivArgs->targetGain;
   dataType decay       = (dataType) (1.0f - pKerPrivArgs->smoothingCoeffSample);
   dataType currentGain = (dataType) pKerPrivArgs->currentGain;
   int32_t  strideOut   = pKerPrivArgs->strideOut;

   for (uint32_t m = 0; m < dim_y; m++) {
      for (uint32_t n = 0; n < dim_x; n++) {
         currentGain                  = targetGain + (decay * (currentGain - targetGain));
         pOutLocal[m * strideOut + n] = pInLocal[m * strideIn + n] * currentGain;
      }
   }

   pKerPrivArgs->currentGain = (float) currentGain;

   return status;
}

/* explicit instantiation for the different data type versions */
template AUDIOLIB_STATUS
AUDIOLIB_agc_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_agc_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
