// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_downSample_priv.h"

int32_t AUDIOLIB_downSample_getHandleSize(AUDIOLIB_downSample_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_downSample_PrivArgs);
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
   else if (pKerInitArgs->downSampleRatio < 1u) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if (pKerInitArgs->phase >= pKerInitArgs->downSampleRatio) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Nothing to do here */
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

   if ((pKerPrivArgs == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      return AUDIOLIB_ERR_NULL_POINTER;
   }

   pKerPrivArgs->downSampleRatio = pKerInitArgs->downSampleRatio;
   pKerPrivArgs->phase           = pKerInitArgs->phase;

   pKerPrivArgs->dim_x     = bufParamsIn->dim_x;
   pKerPrivArgs->dim_y     = bufParamsIn->dim_y;
   pKerPrivArgs->strideIn  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOut = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->dataType  = bufParamsIn->data_type; // cached for perfEst

   pKerPrivArgs->isInterleave = (pKerInitArgs->isInterleave == 1) ? 1 : 0;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (pKerPrivArgs->isInterleave == 0) {

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
            pKerPrivArgs->execute = AUDIOLIB_downSampleInterLeave_exec_cn<float>;
         }
         else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
            pKerPrivArgs->execute = AUDIOLIB_downSampleInterLeave_exec_cn<double>;
         }
         else {
            status = AUDIOLIB_ERR_INVALID_TYPE;
         }
      }
   }
   else {
      uint8_t interleave1ChNotAlign =
          (pKerPrivArgs->isInterleave == 1 && (pKerPrivArgs->strideIn == (uint32_t) pKerPrivArgs->dim_x) &&
           (pKerPrivArgs->dim_x == 1U) && pKerPrivArgs->downSampleRatio == 2)
              ? 1U
              : 0U;

      uint8_t interleave2ChNotAlign =
          (pKerPrivArgs->isInterleave == 1 && (pKerPrivArgs->strideIn == (uint32_t) pKerPrivArgs->dim_x) &&
           (pKerPrivArgs->dim_x == 2U) && pKerPrivArgs->downSampleRatio == 2 &&
           bufParamsIn->data_type == AUDIOLIB_FLOAT32)
              ? 1U
              : 0U;

      pKerPrivArgs->usesRatio2Path = interleave1ChNotAlign | interleave2ChNotAlign;

      // init_ci sets up the SE/SA templates and the usesRatio2Path flag; the
      // matching C7x execution variant is then selected from the configuration.
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_downSample_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);

         if (pKerPrivArgs->isInterleave) {
            if (pKerPrivArgs->usesRatio2Path) {
               // dim_x == 2 fetches two channels at a time; dim_x == 1 uses the
               // generic decimate-by-2 stream pair.
               pKerPrivArgs->execute = (pKerPrivArgs->dim_x == 2) ? AUDIOLIB_downSampleRatio2TwoCh_exec_ci<float>
                                                                  : AUDIOLIB_downSampleRatio2_exec_ci<float>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_downSampleGeneric_exec_ci<float>;
            }
         }
         else {
            switch (pKerPrivArgs->downSampleRatio) {
            case 2:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio2_exec_ci<float>;
               break;
            case 3:
            case 6:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio3_6_exec_ci<float>;
               break;
            case 4:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio4_exec_ci<float>;
               break;
            case 5:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio5_exec_ci<float>;
               break;
            case 7:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio7_exec_ci<float>;
               break;
            case 8:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio8_exec_ci<float>;
               break;
            case 12:
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

         if (pKerPrivArgs->isInterleave) {
            // The two-channel fetch variant is float-only; double always uses the
            // generic decimate-by-2 stream pair.
            if (pKerPrivArgs->usesRatio2Path) {
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio2_exec_ci<double>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_downSampleGeneric_exec_ci<double>;
            }
         }
         else {
            switch (pKerPrivArgs->downSampleRatio) {
            case 2:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio2_exec_ci<double>;
               break;
            case 3:
               pKerPrivArgs->execute = AUDIOLIB_downSampleRatio3_6_exec_ci<double>;
               break;
            case 4:
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

   if (handle == NULL) {
      return AUDIOLIB_ERR_NULL_POINTER;
   }

   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   status                                     = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
