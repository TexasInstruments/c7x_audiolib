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
                                              const AUDIOLIB_bufParams2D_t                *bufParamsOut,
                                              const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_whiteNoiseGenerator_init_checkParams \n");

   if ((handle == NULL) || (bufParamsOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   if (status == AUDIOLIB_SUCCESS) {
      if (bufParamsOut->data_type != AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((bufParamsOut->dim_y == 0U) ||
               (bufParamsOut->dim_y > (uint32_t) AUDIOLIB_WHITENOISEGENERATOR_MAX_NUM_CHANNELS)) {
         /* Only this many channel spacings fit in the LCG's 2^32 period without overlap. */
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
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

/* Multiplier and increment that advance the LCG by `steps` states in one step:
 * X_n+steps = (multiplier * X_n + increment) mod 2^32. Binary exponentiation, O(log steps). */
static void AUDIOLIB_whiteNoiseGenerator_jumpConstants(uint32_t steps, uint32_t *pMultiplier, uint32_t *pIncrement)
{
   uint64_t aN    = 1;
   uint64_t cN    = 0;
   uint64_t baseA = AUDIOLIB_LCG_MULTIPLIER;
   uint64_t baseC = AUDIOLIB_LCG_INCREMENT;

   while (steps > 0) {
      if (steps & 1) {
         aN = (aN * baseA) & AUDIOLIB_LCG_MOD_MASK;
         cN = (cN * baseA + baseC) & AUDIOLIB_LCG_MOD_MASK;
      }
      baseC = (baseC * (baseA + 1)) & AUDIOLIB_LCG_MOD_MASK;
      baseA = (baseA * baseA) & AUDIOLIB_LCG_MOD_MASK;
      steps >>= 1;
   }

   *pMultiplier = (uint32_t) aN;
   *pIncrement  = (uint32_t) cN;
}

AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_init(AUDIOLIB_kernelHandle                        handle,
                                                  AUDIOLIB_bufParams2D_t                      *bufParamsOut,
                                                  const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_whiteNoiseGenerator_init \n");

   pKerPrivArgs->samples     = (int32_t) bufParamsOut->dim_x;
   pKerPrivArgs->numChannels = (int32_t) bufParamsOut->dim_y;
   pKerPrivArgs->strideOut   = (int32_t) (bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type));
   pKerPrivArgs->range       = pKerInitArgs->range;

   memcpy(&pKerPrivArgs->initArgs, pKerInitArgs, sizeof(AUDIOLIB_whiteNoiseGenerator_InitArgs));

   const uint32_t a = AUDIOLIB_LCG_MULTIPLIER;
   const uint32_t c = AUDIOLIB_LCG_INCREMENT;

   // jumpHead constants advance channel 0's states to the next frame (every other channel follows from it)
   AUDIOLIB_whiteNoiseGenerator_jumpConstants((uint32_t) pKerPrivArgs->samples,
                                              &pKerPrivArgs->jumpHeadMultiplierNextFrame,
                                              &pKerPrivArgs->jumpHeadIncNextFrame);

   // channel c starts c * CHANNEL_SPACING steps after channel 0
   AUDIOLIB_whiteNoiseGenerator_jumpConstants(AUDIOLIB_WHITENOISEGENERATOR_CHANNEL_SPACING,
                                              &pKerPrivArgs->channelMultiplier, &pKerPrivArgs->channelIncrement);

   /* The _ci loop runs whole iterations of numVectors vectors over every row. Pick 5 or 8 by the same cycle
    * model perfEst reports: 8 wins when the padded vectors it saves over all rows outweigh its 8 extra cycles
    * per call; on a tie the shorter unroll is kept. */
   uint64_t shortUnrollCycles = AUDIOLIB_whiteNoiseGenerator_execCycles(
       pKerPrivArgs->numChannels, pKerPrivArgs->samples, AUDIOLIB_WHITENOISEGENERATOR_SHORT_UNROLL);
   uint64_t longUnrollCycles = AUDIOLIB_whiteNoiseGenerator_execCycles(pKerPrivArgs->numChannels, pKerPrivArgs->samples,
                                                                       AUDIOLIB_WHITENOISEGENERATOR_LONG_UNROLL);
   pKerPrivArgs->numVectors  = (longUnrollCycles < shortUnrollCycles) ? AUDIOLIB_WHITENOISEGENERATOR_LONG_UNROLL
                                                                      : AUDIOLIB_WHITENOISEGENERATOR_SHORT_UNROLL;
   pKerPrivArgs->numIterations   = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, STATE_INDEX * pKerPrivArgs->numVectors);
   pKerPrivArgs->totalIterations = pKerPrivArgs->numIterations * pKerPrivArgs->numChannels;

   // steps from the first state vector to each of the others, and per loop iteration
   for (int32_t v = 0; v < AUDIOLIB_WHITENOISEGENERATOR_LONG_UNROLL; v++) {
      AUDIOLIB_whiteNoiseGenerator_jumpConstants((uint32_t) (v * STATE_INDEX), &pKerPrivArgs->vectorMultiplier[v],
                                                 &pKerPrivArgs->vectorIncrement[v]);
   }
   AUDIOLIB_whiteNoiseGenerator_jumpConstants((uint32_t) (pKerPrivArgs->numVectors * STATE_INDEX),
                                              &pKerPrivArgs->iterationMultiplier, &pKerPrivArgs->iterationIncrement);

   // a row's last iteration jumps instead of stepping, from (numIterations - 1) iterations into the row
   uint32_t rowSteps = (uint32_t) ((pKerPrivArgs->numIterations - 1) * pKerPrivArgs->numVectors * STATE_INDEX);
   AUDIOLIB_whiteNoiseGenerator_jumpConstants(AUDIOLIB_WHITENOISEGENERATOR_CHANNEL_SPACING - rowSteps,
                                              &pKerPrivArgs->rowJumpMultiplier, &pKerPrivArgs->rowJumpIncrement);
   if (pKerPrivArgs->numIterations == 1) {
      // every iteration is a row's last, so the step is always the row jump, whatever SA1's predicate
      pKerPrivArgs->iterationMultiplier = pKerPrivArgs->rowJumpMultiplier;
      pKerPrivArgs->iterationIncrement  = pKerPrivArgs->rowJumpIncrement;
   }

   // channel 0's states start at its first output: one step after the seed
   pKerPrivArgs->states[0] = (a * pKerInitArgs->seed + c);
   for (int32_t i = 1; i < STATE_INDEX; i++) {
      pKerPrivArgs->states[i] = (a * pKerPrivArgs->states[i - 1] + c); // x_n = (a * x_{n-1} + c)
   }

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
         pKerPrivArgs->execute = (pKerPrivArgs->numVectors == AUDIOLIB_WHITENOISEGENERATOR_LONG_UNROLL)
                                     ? AUDIOLIB_whiteNoiseGenerator_unroll8_exec_ci<float>
                                     : AUDIOLIB_whiteNoiseGenerator_unroll5_exec_ci<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
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
