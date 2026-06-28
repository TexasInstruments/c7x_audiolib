// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../AUDIOLIB_gainNCh/AUDIOLIB_gainNCh_priv.h"
#include "../audiolib.h"
#include "AUDIOLIB_balanceFader_priv.h"
#include "AUDIOLIB_bufParams.h"

int32_t AUDIOLIB_balanceFader_getHandleSize(AUDIOLIB_balanceFader_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_balanceFader_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_balanceFader_init_checkParams(AUDIOLIB_kernelHandle                 handle,
                                       const AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                       const AUDIOLIB_bufParams1D_t         *bufParamsGain,
                                       const AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                       const AUDIOLIB_balanceFader_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_balanceFader_init_checkParams \n");

   if (handle == NULL || bufParamsIn == NULL || bufParamsGain == NULL || bufParamsOut == NULL || pKerInitArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type ||
               bufParamsGain->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else {
         /* Nothing to do here */
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      /* The gain vector holds one value per channel: channels = dim_x when interleaved
       * (channel-major columns), else dim_y. set()/exec index the gain by this count,
       * so it must match the input channel axis. */
      uint32_t numChannels = (pKerInitArgs->isInterleave == 1) ? bufParamsIn->dim_x : bufParamsIn->dim_y;
      if (bufParamsGain->dim_x != numChannels) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_balanceFader_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                       const void *restrict pIn,
                                                       const void *restrict pGain,
                                                       const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_balanceFader_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pGain == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_balanceFader_set(AUDIOLIB_kernelHandle handle,
                                          void *restrict pGain,
                                          AUDIOLIB_balanceFader_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   if (handle == NULL || pGain == NULL || pKerSetArgs == NULL || pKerSetArgs->channelConfig == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_balanceFader_PrivArgs *pKerPrivArgs = (AUDIOLIB_balanceFader_PrivArgs *) handle;
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_balanceFader_SetArgs));

      float *restrict pGainLocal = (float *) pGain;

      // --- 2. Calculate Fundamental Gain Components ---

      // Balance-related components
      float balanceAngle = (M_PI / 4.0f) * (pKerSetArgs->balance + 1.0f);
      float leftGain     = cosf(balanceAngle);
      float rightGain    = sinf(balanceAngle);

      // Fader-related components
      float faderAngle = (M_PI / 4.0f) * (pKerSetArgs->fade + 1.0f);
      float frontGain  = sinf(faderAngle);
      float backGain   = cosf(faderAngle);
      float sideGain   = 0.0f;

      if (pKerSetArgs->fade < 0.0f) { // Fading to rear
         sideGain = pKerSetArgs->sideGainFactor * backGain + (1 - pKerSetArgs->sideGainFactor) * frontGain;
      }
      else { // Fading to front or center
         sideGain = backGain;
      }

      float topMiddleGain = 0.5f * (frontGain + backGain);
      // --- 3. Loop Through User's Channel Config and Calculate Gains On-The-Fly ---
      for (int32_t i = 0; i < pKerPrivArgs->numChannels; i++) {
         int32_t channelType = pKerSetArgs->channelConfig[i];
         // Use a switch to calculate and assign the gain for only this channel
         switch (channelType) {
         // --- Base Layer ---
         case AUDIOLIB_CHANNEL_FRONT_LEFT:
         case AUDIOLIB_CHANNEL_FRONT_LEFT_TREBBLE:
         case AUDIOLIB_CHANNEL_FRONT_LEFT_MID:
         case AUDIOLIB_CHANNEL_FRONT_LEFT_WOOFER:
            pGainLocal[i] = leftGain * frontGain;
            break;
         case AUDIOLIB_CHANNEL_FRONT_RIGHT:
         case AUDIOLIB_CHANNEL_FRONT_RIGHT_TREBBLE:
         case AUDIOLIB_CHANNEL_FRONT_RIGHT_MID:
         case AUDIOLIB_CHANNEL_FRONT_RIGHT_WOOFER:
            pGainLocal[i] = rightGain * frontGain;
            break;
         case AUDIOLIB_CHANNEL_FRONT_CENTER:
            pGainLocal[i] = 1.0f * frontGain;
            break;
         case AUDIOLIB_CHANNEL_SIDE_LEFT:
            pGainLocal[i] = leftGain * sideGain;
            break;
         case AUDIOLIB_CHANNEL_SIDE_RIGHT:
            pGainLocal[i] = rightGain * sideGain;
            break;
         case AUDIOLIB_CHANNEL_REAR_LEFT:
            pGainLocal[i] = leftGain * backGain;
            break;
         case AUDIOLIB_CHANNEL_REAR_RIGHT:
            pGainLocal[i] = rightGain * backGain;
            break;

         // --- Height Layer ---
         case AUDIOLIB_CHANNEL_TOP_FRONT_LEFT:
            pGainLocal[i] = leftGain * frontGain;
            break;
         case AUDIOLIB_CHANNEL_TOP_FRONT_RIGHT:
            pGainLocal[i] = rightGain * frontGain;
            break;
         case AUDIOLIB_CHANNEL_TOP_MIDDLE_LEFT:
            pGainLocal[i] = leftGain * topMiddleGain;
            break;
         case AUDIOLIB_CHANNEL_TOP_MIDDLE_RIGHT:
            pGainLocal[i] = rightGain * topMiddleGain;
            break;
         case AUDIOLIB_CHANNEL_TOP_REAR_LEFT:
            pGainLocal[i] = leftGain * backGain;
            break;
         case AUDIOLIB_CHANNEL_TOP_REAR_RIGHT:
            pGainLocal[i] = rightGain * backGain;
            break;
         case AUDIOLIB_CHANNEL_OVERHEAD:
            pGainLocal[i] = 1.0f * topMiddleGain;
            break;

         // --- LFE (Subwoofer) ---
         case AUDIOLIB_CHANNEL_LOW_FREQUENCY_EFFECTS: {
            float lfeBalanceGain = 1.0f; // Default
            if (pKerSetArgs->lfeBalanceMode == AUDIOLIB_LFE_BALANCE_PARTIAL) {
               lfeBalanceGain = pKerSetArgs->lfeBalanceGainFactor +
                                (1 - pKerSetArgs->lfeBalanceGainFactor) * cosf(2.0f * balanceAngle);
            }
            else if (pKerSetArgs->lfeBalanceMode == AUDIOLIB_LFE_BALANCE_FULL) {
               lfeBalanceGain = 0.5f * (leftGain + rightGain);
            }

            float lfeFaderGain = 1.0f; // Default
            if (pKerSetArgs->lfeFaderMode == AUDIOLIB_LFE_FADER_AVERAGE) {
               lfeFaderGain = 0.5f * (frontGain + backGain);
            }
            else if (pKerSetArgs->lfeFaderMode == AUDIOLIB_LFE_FADER_POSITION_BASED) {
               lfeFaderGain =
                   (1 - pKerSetArgs->lfeFaderGainFactor) * frontGain + pKerSetArgs->lfeFaderGainFactor * backGain;
            }
            else if (pKerSetArgs->lfeFaderMode == AUDIOLIB_LFE_FADER_FULL) {
               lfeFaderGain = backGain;
            }

            pGainLocal[i] = lfeBalanceGain * lfeFaderGain;
         } break;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_balanceFader_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_balanceFader_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_balanceFader_PrivArgs *pKerPrivArgs = (AUDIOLIB_balanceFader_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_balanceFader_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_balanceFader_init(AUDIOLIB_kernelHandle                 handle,
                                           const AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                           const AUDIOLIB_bufParams1D_t         *bufParamsGain,
                                           const AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                           const AUDIOLIB_balanceFader_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                 status              = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNCh_InitArgs      *pKerInitArgsGainNCh = (AUDIOLIB_gainNCh_InitArgs *) pKerInitArgs;
   AUDIOLIB_balanceFader_PrivArgs *pKerPrivArgs        = (AUDIOLIB_balanceFader_PrivArgs *) handle;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_balanceFader_init \n");

   // Initialize gainNCh fields
   pKerPrivArgs->gainNChArgs.dim_x        = bufParamsIn->dim_x;
   pKerPrivArgs->gainNChArgs.dim_y        = bufParamsIn->dim_y;
   pKerPrivArgs->gainNChArgs.strideIn     = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->gainNChArgs.strideOut    = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->gainNChArgs.isInterleave = pKerInitArgs->isInterleave;

   // initialise balanceFader fields
   pKerPrivArgs->samples           = bufParamsIn->dim_y;
   pKerPrivArgs->isInterleave      = pKerInitArgs->isInterleave;
   pKerPrivArgs->numChannels       = bufParamsGain->dim_x;
   pKerPrivArgs->strideInElements  = pKerPrivArgs->gainNChArgs.strideIn;
   pKerPrivArgs->strideOutElements = pKerPrivArgs->gainNChArgs.strideOut;

   if (pKerInitArgs->isInterleave == 1) {
#if __C7X_VEC_SIZE_BITS__ == 256
      pKerPrivArgs->gainNChArgs.customImplementation =
          (((pKerPrivArgs->gainNChArgs.dim_x == 2) || (pKerPrivArgs->gainNChArgs.dim_x == 4)) &&
           (pKerPrivArgs->gainNChArgs.strideIn == pKerPrivArgs->gainNChArgs.dim_x) &&
           (pKerPrivArgs->gainNChArgs.strideOut == pKerPrivArgs->gainNChArgs.dim_x));
#else
      pKerPrivArgs->gainNChArgs.customImplementation =
          (((pKerPrivArgs->gainNChArgs.dim_x == 2) || (pKerPrivArgs->gainNChArgs.dim_x == 4) ||
            (pKerPrivArgs->gainNChArgs.dim_x == 8)) &&
           (pKerPrivArgs->gainNChArgs.strideIn == pKerPrivArgs->gainNChArgs.dim_x) &&
           (pKerPrivArgs->gainNChArgs.strideOut == pKerPrivArgs->gainNChArgs.dim_x));
#endif
      pKerPrivArgs->gainNChArgs.isInterleave = 1;
   }
   else {
      pKerPrivArgs->gainNChArgs.isInterleave = 0;
   }

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_gainNCh_init_ci<float>(&pKerPrivArgs->gainNChArgs, bufParamsIn, bufParamsGain, bufParamsOut,
                                                  pKerInitArgsGainNCh);
         pKerPrivArgs->gainNChArgs.execute = AUDIOLIB_gainNCh_exec_ci<float>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (pKerPrivArgs->gainNChArgs.isInterleave == 0) {
         pKerPrivArgs->gainNChArgs.execute = AUDIOLIB_gainNCh_exec_cn<float>;
      }
      else {
         pKerPrivArgs->gainNChArgs.execute = AUDIOLIB_gainNChInterLeave_exec_cn<float>;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_balanceFader_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_balanceFader_exec \n");

   AUDIOLIB_balanceFader_PrivArgs *pKerPrivArgs = (AUDIOLIB_balanceFader_PrivArgs *) handle;
   status = pKerPrivArgs->gainNChArgs.execute(&pKerPrivArgs->gainNChArgs, pIn, pGain, pOut);
   return status;
}

void AUDIOLIB_balanceFader_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_balanceFader_PrivArgs *pKerPrivArgs = (AUDIOLIB_balanceFader_PrivArgs *) handle;
   AUDIOLIB_gainNCh_perfEst(&pKerPrivArgs->gainNChArgs, archCycles, estCycles);
}
