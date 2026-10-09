// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../audiolib.h"
#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_preEmphasis_priv.h"

int32_t AUDIOLIB_preEmphasis_getHandleSize(AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs)
{
   (void) pKerInitArgs; /* Unused parameter suppression */
   return (int32_t) sizeof(AUDIOLIB_preEmphasis_PrivArgs);
}

AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_init_checkParams(AUDIOLIB_kernelHandle                handle,
                                      const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                      const AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                      const AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_preEmphasis_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      if (bufParamsIn->data_type != AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((pKerInitArgs->alpha < 0.0f) || (pKerInitArgs->alpha > 1.0f)) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else if (((pKerInitArgs->isInterleave != 0U) ? bufParamsIn->dim_x : bufParamsIn->dim_y) >
               (uint32_t) AUDIOLIB_PREEMPHASIS_MAX_NUM_CHANNELS) {
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
AUDIOLIB_preEmphasis_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_preEmphasis_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_preEmphasis_init(AUDIOLIB_kernelHandle                handle,
                                          const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                          const AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                          const AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_preEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_preEmphasis_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_preEmphasis_init \n");

   pKerPrivArgs->isInterleave = pKerInitArgs->isInterleave;

   pKerPrivArgs->samples =
       (pKerInitArgs->isInterleave != 0U) ? (int32_t) bufParamsIn->dim_y : (int32_t) bufParamsIn->dim_x;
   pKerPrivArgs->numChannels =
       (pKerInitArgs->isInterleave != 0U) ? (int32_t) bufParamsIn->dim_x : (int32_t) bufParamsIn->dim_y;

   pKerPrivArgs->strideIn  = (int32_t) (bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type));
   pKerPrivArgs->strideOut = (int32_t) (bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type));
   pKerPrivArgs->alpha     = pKerInitArgs->alpha;

   for (uint32_t n = 0U; n < AUDIOLIB_PREEMPHASIS_MAX_NUM_CHANNELS; n++) {
      pKerPrivArgs->xPrev[n] = 0.0f;
   }

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_OPTIMIZED) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_preEmphasis_init_ci<float>(pKerPrivArgs, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = (pKerInitArgs->isInterleave != 0U)
                                     ? AUDIOLIB_preEmphasis_interleave_exec_ci<float>
                                     : ((pKerPrivArgs->samples <= AUDIOLIB_PREEMPHASIS_LANES_F32)
                                            ? AUDIOLIB_preEmphasis_nonInterleaveSingleVec_exec_ci<float>
                                            : AUDIOLIB_preEmphasis_nonInterleave_exec_ci<float>);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      pKerPrivArgs->execute = AUDIOLIB_preEmphasis_exec_cn<float>;
   }
   else {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_preEmphasis_exec \n");

   AUDIOLIB_preEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_preEmphasis_PrivArgs *) handle;
   status                                      = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
