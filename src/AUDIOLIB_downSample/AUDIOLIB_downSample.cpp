// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_downSample_priv.h"

int32_t AUDIOLIB_downSample_getHandleSize(AUDIOLIB_downSample_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = (int32_t) sizeof(AUDIOLIB_downSample_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_downSample_init_checkParams(AUDIOLIB_kernelHandle               handle,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                     const AUDIOLIB_downSample_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSample_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
      status = AUDIOLIB_ERR_INVALID_TYPE;
   }
   else if (bufParamsIn->data_type != bufParamsOut->data_type) {
      status = AUDIOLIB_ERR_INVALID_TYPE;
   }
   else if (pKerInitArgs->isInterleave > 1U) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if ((pKerInitArgs->downSampleRatio < 1U) || (pKerInitArgs->phase >= pKerInitArgs->downSampleRatio)) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if ((bufParamsIn->dim_x == 0U) || (bufParamsIn->dim_y == 0U)) {
      /* Zero channels or samples: the streaming engine cannot run a zero-length stream. */
      status = AUDIOLIB_ERR_INVALID_DIMENSION;
   }
   else if (((pKerInitArgs->isInterleave == 1U) ? bufParamsIn->dim_y : bufParamsIn->dim_x) <= pKerInitArgs->phase) {
      /* No sample is kept: the streaming engine cannot run a zero-length stream. */
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Row strides must hold one full row: the input row of dim_x elements, and the output row of
       * dim_x channels (interleaved) or ceil((dim_x - phase) / downSampleRatio) samples (planar). */
      uint32_t eleSize    = (uint32_t) AUDIOLIB_sizeof(bufParamsIn->data_type);
      uint32_t outRowEles = (pKerInitArgs->isInterleave == 1U)
                                ? bufParamsIn->dim_x
                                : (((bufParamsIn->dim_x - pKerInitArgs->phase - 1U) / pKerInitArgs->downSampleRatio) + 1U);

      if ((bufParamsIn->stride_y <= 0) || (bufParamsOut->stride_y <= 0) ||
          (((uint32_t) bufParamsIn->stride_y % eleSize) != 0U) || (((uint32_t) bufParamsOut->stride_y % eleSize) != 0U) ||
          (((uint32_t) bufParamsIn->stride_y / eleSize) < bufParamsIn->dim_x) ||
          (((uint32_t) bufParamsOut->stride_y / eleSize) < outRowEles)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_downSample_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSample_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_downSample_init(AUDIOLIB_kernelHandle               handle,
                                         AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                         AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                         const AUDIOLIB_downSample_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSample_init \n");

   pKerPrivArgs->downSampleRatio = pKerInitArgs->downSampleRatio;
   pKerPrivArgs->phase           = pKerInitArgs->phase;

   pKerPrivArgs->dim_x     = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y     = bufParamsIn->dim_y;
   pKerPrivArgs->strideIn  = (uint32_t) bufParamsIn->stride_y / (uint32_t) AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOut = (uint32_t) bufParamsOut->stride_y / (uint32_t) AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->dataType  = bufParamsIn->data_type; /* cached for perfEst */

   pKerPrivArgs->isInterleave = (pKerInitArgs->isInterleave == 1U) ? 1U : 0U;

   /* Clear the execute pointer so an unsupported type leaves it NULL rather than
    * uninitialized; exec checks for NULL before calling it. */
   pKerPrivArgs->execute = NULL;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (pKerPrivArgs->isInterleave == 0U) {

         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_downSample_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_downSample_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
      else {

         if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
            pKerPrivArgs->execute = AUDIOLIB_downSampleInterleave_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_downSampleInterleave_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   else {
     
      uint32_t inSamples  = (pKerPrivArgs->isInterleave == 1U) ? pKerPrivArgs->dim_y : pKerPrivArgs->dim_x;
      uint32_t outSamples = ((inSamples - pKerPrivArgs->phase - 1U) / pKerPrivArgs->downSampleRatio) + 1U;
      bool     ratio2Ok   = (pKerPrivArgs->downSampleRatio == 2U) && (outSamples >= 2U);
      bool     contiguous = (pKerPrivArgs->strideIn == pKerPrivArgs->dim_x);
      bool     interOk    = contiguous && ((pKerPrivArgs->dim_x == 1U) ||
                                       ((pKerPrivArgs->dim_x == 2U) && (bufParamsIn->data_type == AUDIOLIB_FLOAT32)));

      pKerPrivArgs->usesRatio2Path = (ratio2Ok && ((pKerPrivArgs->isInterleave == 0U) || interOk)) ? 1U : 0U;

      /* init_ci sets up the SE/SA templates from the usesRatio2Path flag; the
       * matching C7x execution variant is then selected from the configuration. */
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_downSample_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);

         if (pKerPrivArgs->usesRatio2Path == 1U) {
            /* interleaved dim_x == 2 fetches two channels at a time; planar and
             * interleaved dim_x == 1 use the generic decimate-by-2 stream pair. */
            pKerPrivArgs->execute = ((pKerPrivArgs->isInterleave == 1U) && (pKerPrivArgs->dim_x == 2U))
                                        ? AUDIOLIB_downSampleRatio2TwoCh_exec_ci<float>
                                        : AUDIOLIB_downSampleRatio2_exec_ci<float>;
         }
         else if (pKerPrivArgs->isInterleave == 1U) {
            pKerPrivArgs->execute = AUDIOLIB_downSampleGeneric_exec_ci<float>;
         }
         else {
            switch (pKerPrivArgs->downSampleRatio) {
            case 3U:
            case 6U:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio3_6_exec_ci<float>;
               break;
            case 4U:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio4_exec_ci<float>;
               break;
            case 5U:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio5_exec_ci<float>;
               break;
            case 7U:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio7_exec_ci<float>;
               break;
            case 8U:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio8_exec_ci<float>;
               break;
            case 12U:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio12_exec_ci<float>;
               break;
            default:
               pKerPrivArgs->execute = AUDIOLIB_downSampleGeneric_exec_ci<float>;
               break;
            }
         }
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         status = AUDIOLIB_downSample_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);

         if (pKerPrivArgs->usesRatio2Path == 1U) {
            /* The two-channel fetch variant is float-only; double always uses the
             * generic decimate-by-2 stream pair. */
            pKerPrivArgs->execute = AUDIOLIB_downSampleRatio2_exec_ci<double>;
         }
         else if (pKerPrivArgs->isInterleave == 1U) {
            pKerPrivArgs->execute = AUDIOLIB_downSampleGeneric_exec_ci<double>;
         }
         else {
            switch (pKerPrivArgs->downSampleRatio) {
            case 3U:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio3_6_exec_ci<double>;
               break;
            case 4U:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio4_exec_ci<double>;
               break;
            default:
               pKerPrivArgs->execute = AUDIOLIB_downSampleGeneric_exec_ci<double>;
               break;
            }
         }
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_downSample_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSample_exec \n");

   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;

   if (pKerPrivArgs->execute != NULL) {
      status = pKerPrivArgs->execute(handle, pIn, pOut);
   }
   else {
      status = AUDIOLIB_ERR_INVALID_TYPE;
   }

   return status;
}
