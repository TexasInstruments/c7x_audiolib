// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../audiolib.h"
#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_deEmphasis_priv.h"

int32_t AUDIOLIB_deEmphasis_getHandleSize(AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs)
{
   (void) pKerInitArgs; /* Unused parameter suppression */
   return (int32_t) sizeof(AUDIOLIB_deEmphasis_PrivArgs);
}

AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_init_checkParams(AUDIOLIB_kernelHandle               handle,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                     const AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_deEmphasis_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      uint32_t numChannels =
          (pKerInitArgs->isInterleave != 0U) ? (uint32_t) bufParamsIn->dim_x : (uint32_t) bufParamsIn->dim_y;

      if (bufParamsIn->data_type != AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((pKerInitArgs->alpha < 0.0f) || (pKerInitArgs->alpha > 1.0f)) {
         /* alpha == 1.0 is permitted: it degenerates to a pure integrator, which is
          * only marginally stable but is a legitimate test vector. */
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else if (numChannels > (uint32_t) AUDIOLIB_DEEMPHASIS_MAX_NUM_CHANNELS) {
         /* One float of filter state is held per channel in a fixed-size array. */
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_deEmphasis_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_deEmphasis_init(AUDIOLIB_kernelHandle               handle,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                         const AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_deEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_deEmphasis_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_deEmphasis_init \n");

   pKerPrivArgs->isInterleave = pKerInitArgs->isInterleave;

   pKerPrivArgs->samples =
       (pKerInitArgs->isInterleave != 0U) ? (int32_t) bufParamsIn->dim_y : (int32_t) bufParamsIn->dim_x;
   pKerPrivArgs->numChannels =
       (pKerInitArgs->isInterleave != 0U) ? (uint32_t) bufParamsIn->dim_x : (uint32_t) bufParamsIn->dim_y;

   pKerPrivArgs->strideIn  = (int32_t) (bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type));
   pKerPrivArgs->strideOut = (int32_t) (bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type));
   pKerPrivArgs->alpha     = pKerInitArgs->alpha;

   /* y[-1] starts at zero, so the very first frame behaves as if preceded by silence. */
   for (uint32_t n = 0U; n < AUDIOLIB_DEEMPHASIS_MAX_NUM_CHANNELS; n++) {
      pKerPrivArgs->yPrev[n] = 0.0f;
   }

   /* Coefficient powers for the non-interleaved scan. alphaPow[i] = alpha^(i+1). */
   float pow = 1.0f;
   for (uint32_t n = 0U; n < (uint32_t) AUDIOLIB_DEEMPHASIS_LANES_F32; n++) {
      pow                       = pow * pKerInitArgs->alpha;
      pKerPrivArgs->alphaPow[n] = pow;
   }
   pKerPrivArgs->alphaPow1  = pKerPrivArgs->alphaPow[0];
   pKerPrivArgs->alphaPow2  = pKerPrivArgs->alphaPow[1];
   pKerPrivArgs->alphaPow4  = pKerPrivArgs->alphaPow[3];
   pKerPrivArgs->alphaPow12 = pKerPrivArgs->alphaPow[AUDIOLIB_DEEMPHASIS_LANES_F32 - 1] * pKerPrivArgs->alphaPow[3];
   pKerPrivArgs->alphaPow16 = pKerPrivArgs->alphaPow[AUDIOLIB_DEEMPHASIS_LANES_F32 - 1] *
                              pKerPrivArgs->alphaPow[AUDIOLIB_DEEMPHASIS_LANES_F32 - 1];

   /* VPERM controls for the non-interleaved scan: lane l of the delayed vector takes lane
    * l - 4 (zero below lane 4), and every lane of the broadcast takes the last lane. */
   const uint32_t laneBytes = (uint32_t) sizeof(float);
   for (uint32_t l = 0U; l < (uint32_t) AUDIOLIB_DEEMPHASIS_LANES_F32; l++) {
      for (uint32_t b = 0U; b < laneBytes; b++) {
         pKerPrivArgs->delay4Ctrl[(l * laneBytes) + b] =
             (l < 4U) ? (uint8_t) AUDIOLIB_DEEMPHASIS_PERM_ZERO : (uint8_t) (((l - 4U) * laneBytes) + b);
         pKerPrivArgs->bcastLastCtrl[(l * laneBytes) + b] =
             (uint8_t) ((((uint32_t) AUDIOLIB_DEEMPHASIS_LANES_F32 - 1U) * laneBytes) + b);
      }
   }

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         /* The non-interleaved loop filters channels in pairs; whether one channel is left
          * over is fixed by the configuration, so the variant is selected here. A single
          * channel has its own variant, which opens only SE0/SA0. */
         static const pFxnAUDIOLIB_deEmphasis_exec execNonInterleaveByChannelParity[2] = {
             AUDIOLIB_deEmphasis_nonInterleave_exec_ci<float, false>,
             AUDIOLIB_deEmphasis_nonInterleave_exec_ci<float, true>};

         status = AUDIOLIB_deEmphasis_init_ci<float>(pKerPrivArgs, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (pKerInitArgs->isInterleave != 0U) {
            pKerPrivArgs->execute = AUDIOLIB_deEmphasis_interleave_exec_ci<float>;
         }
         else if (pKerPrivArgs->numChannels == 1U) {
            pKerPrivArgs->execute = AUDIOLIB_deEmphasis_nonInterleaveSingleChannel_exec_ci<float>;
         }
         else {
            pKerPrivArgs->execute = execNonInterleaveByChannelParity[pKerPrivArgs->numChannels & 1U];
         }
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      pKerPrivArgs->execute = AUDIOLIB_deEmphasis_exec_cn<float>;
   }
   else {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_deEmphasis_exec \n");

   AUDIOLIB_deEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_deEmphasis_PrivArgs *) handle;
   status                                     = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
