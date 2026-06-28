// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_whiteNoiseGenerator_priv.h"

int32_t AUDIOLIB_whiteNoiseGenerator_getHandleSize(AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_whiteNoiseGenerator_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_whiteNoiseGenerator_init_checkParams(AUDIOLIB_kernelHandle                        handle,
                                              const AUDIOLIB_bufParams1D_t                *bufParamsOut,
                                              const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_whiteNoiseGenerator_init_checkParams \n");

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

AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_whiteNoiseGenerator_exec_checkParams \n");

   if (pOut == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_init(AUDIOLIB_kernelHandle                        handle,
                                                  AUDIOLIB_bufParams1D_t                      *bufParamsOut,
                                                  const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_whiteNoiseGenerator_init \n");

   pKerPrivArgs->samples = bufParamsOut->dim_x;
   pKerPrivArgs->range   = pKerInitArgs->range;
   uint32_t samples      = pKerPrivArgs->samples;
   memcpy(&pKerPrivArgs->initArgs, pKerInitArgs, sizeof(AUDIOLIB_whiteNoiseGenerator_InitArgs));

   uint64_t aN    = 1;
   uint64_t cN    = 0;
   uint64_t baseA = AUDIOLIB_LCG_MULTIPLIER;
   uint64_t baseC = AUDIOLIB_LCG_INCREMENT;

   const uint32_t a = AUDIOLIB_LCG_MULTIPLIER;
   const uint32_t c = AUDIOLIB_LCG_INCREMENT;

   pKerPrivArgs->seed      = pKerInitArgs->seed;
   pKerPrivArgs->states[0] = pKerInitArgs->seed;

   for (int i = 1; i < STATE_INDEX; i++) {
      pKerPrivArgs->states[i] = (a * pKerPrivArgs->states[i - 1] + c); // x_n = (a * x_{n-1} + c)
   }

   // binary exponentiation (Olog(n)) to get the jumpHead constants for updating the states for the next frame
   while (samples > 0) {
      if (samples & 1) {
         aN = (aN * baseA) & AUDIOLIB_LCG_MOD_MASK;
         cN = (cN * baseA + baseC) & AUDIOLIB_LCG_MOD_MASK;
      }
      baseC = (baseC * (baseA + 1)) & AUDIOLIB_LCG_MOD_MASK;
      baseA = (baseA * baseA) & AUDIOLIB_LCG_MOD_MASK;
      samples >>= 1;
   }

   pKerPrivArgs->jumpHeadMultiplierNextFrame = (uint32_t) aN;
   pKerPrivArgs->jumpHeadIncNextFrame        = (uint32_t) cN;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsOut->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_whiteNoiseGenerator_exec_cn<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsOut->data_type == AUDIOLIB_FLOAT32) {
         status                = AUDIOLIB_whiteNoiseGenerator_init_ci<float>(handle, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_whiteNoiseGenerator_exec_ci<float>;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_whiteNoiseGenerator_exec(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_whiteNoiseGenerator_exec \n");

   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pOut);

   return status;
}
