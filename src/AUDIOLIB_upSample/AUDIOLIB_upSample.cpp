// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
 ******************************************************************************
 *  @file     AUDIOLIB_upSample.cpp
 *  @brief    Dispatcher / lifecycle for the AUDIOLIB_upSample kernel.
 *  @version  0.1 - 2025 : Initial Version
 ******************************************************************************
 */

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_upSample_priv.h"

int32_t AUDIOLIB_upSample_getHandleSize(AUDIOLIB_upSample_InitArgs *pKerInitArgs)
{
   (void) pKerInitArgs;
   return (int32_t) sizeof(AUDIOLIB_upSample_PrivArgs);
}

AUDIOLIB_STATUS AUDIOLIB_upSample_init_checkParams(AUDIOLIB_kernelHandle             handle,
                                                   const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                                   const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                                   const AUDIOLIB_upSample_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_init_checkParams\n");

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
   else if ((pKerInitArgs->upFactor < 1U) || (pKerInitArgs->phase >= pKerInitArgs->upFactor)) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if ((bufParamsIn->dim_x == 0U) || (bufParamsIn->dim_y == 0U)) {
      /* Zero channels or samples: the streaming engine cannot run a zero-length stream. */
      status = AUDIOLIB_ERR_INVALID_DIMENSION;
   }
   else {
      /* Output must be upFactor times longer along the sample axis. The product is formed in
       * 64 bits so a wrapped 32-bit result cannot pass the check. */
      uint32_t eleSize    = (uint32_t) AUDIOLIB_sizeof(bufParamsIn->data_type);
      uint32_t numSamples = (pKerInitArgs->isInterleave == 1U) ? bufParamsIn->dim_y : bufParamsIn->dim_x;
      uint32_t outSamples = (pKerInitArgs->isInterleave == 1U) ? bufParamsOut->dim_y : bufParamsOut->dim_x;
      uint32_t inChannels = (pKerInitArgs->isInterleave == 1U) ? bufParamsIn->dim_x : bufParamsIn->dim_y;
      uint32_t outChannels = (pKerInitArgs->isInterleave == 1U) ? bufParamsOut->dim_x : bufParamsOut->dim_y;

      if ((outChannels != inChannels) ||
          ((uint64_t) outSamples != ((uint64_t) pKerInitArgs->upFactor * (uint64_t) numSamples))) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      /* Row strides must be positive, element-aligned and hold one full row. */
      else if ((bufParamsIn->stride_y <= 0) || (bufParamsOut->stride_y <= 0) ||
               (((uint32_t) bufParamsIn->stride_y % eleSize) != 0U) ||
               (((uint32_t) bufParamsOut->stride_y % eleSize) != 0U) ||
               (((uint32_t) bufParamsIn->stride_y / eleSize) < bufParamsIn->dim_x) ||
               (((uint32_t) bufParamsOut->stride_y / eleSize) < bufParamsOut->dim_x)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else {
         /* parameters valid */
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_upSample_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_exec_checkParams\n");

   /* All three pointers are required; scratch is carried inside the handle
    * (bufPblock), so there is no separate scratch argument to validate. */

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_upSample_init(AUDIOLIB_kernelHandle             handle,
                                       AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                       AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                       const AUDIOLIB_upSample_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS             status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_init\n");

   int32_t elemSize = AUDIOLIB_sizeof(bufParamsIn->data_type);

   pKerPrivArgs->dimXIn       = bufParamsIn->dim_x;
   pKerPrivArgs->dimYIn       = bufParamsIn->dim_y;
   pKerPrivArgs->upFactor     = pKerInitArgs->upFactor;
   pKerPrivArgs->phase        = pKerInitArgs->phase;
   pKerPrivArgs->isInterleave = pKerInitArgs->isInterleave;
   pKerPrivArgs->strideIn     = bufParamsIn->stride_y / elemSize;
   pKerPrivArgs->strideOut    = bufParamsOut->stride_y / elemSize;
   pKerPrivArgs->dataType     = bufParamsIn->data_type;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_upSample_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_upSample_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {

      /* Single-channel interleaved data with unit input/output strides is one contiguous
       * row, so it is processed by the (faster) planar path. */
      const bool singleChannelPlanar =
          (pKerPrivArgs->isInterleave == 1U) && (pKerPrivArgs->dimXIn == 1U) && (pKerPrivArgs->strideIn == 1) &&
          (pKerPrivArgs->strideOut == 1);
      pKerPrivArgs->useNonInterleave = singleChannelPlanar ? 1U : 0U;
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {

         if ((pKerPrivArgs->isInterleave == 1U) && (pKerPrivArgs->useNonInterleave == 0U)) {
            if (pKerPrivArgs->phase > 0U) {
               pKerPrivArgs->execute = AUDIOLIB_upSample_execInterleave_ci<true, float>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_upSample_execInterleave_ci<false, float>;
            }
         }
         else {
            if (pKerPrivArgs->phase > 0U) {
               switch (pKerPrivArgs->upFactor) {
               case 2U:
               case 4U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample2_exec_ci<true, float>;
                  break;
               case 3U:
               case 6U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample3_exec_ci<true, float>;
                  break;
               case 5U:
               case 10U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample5_exec_ci<true, float>;
                  break;
               case 7U:
               case 14U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample7_exec_ci<true, float>;
                  break;
               case 8U:
               case 16U:
               case 24U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample8X_exec_ci<true, float>;
                  break;
               case 12U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample6x2_exec_ci<true, float>;
                  break;
               default:
                  pKerPrivArgs->execute = AUDIOLIB_upSampleNonInterleave_exec_ci<true, float>;
                  break;
               }
            }
            else {
               switch (pKerPrivArgs->upFactor) {
               case 2U:
               case 4U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample2_exec_ci<false, float>;
                  break;
               case 3U:
               case 6U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample3_exec_ci<false, float>;
                  break;
               case 5U:
               case 10U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample5_exec_ci<false, float>;
                  break;
               case 7U:
               case 14U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample7_exec_ci<false, float>;
                  break;
               case 8U:
               case 16U:
               case 24U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample8X_exec_ci<false, float>;
                  break;
               case 12U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample6x2_exec_ci<false, float>;
                  break;
               default:
                  pKerPrivArgs->execute = AUDIOLIB_upSampleNonInterleave_exec_ci<false, float>;
                  break;
               }
            }
         }
         status = AUDIOLIB_upSample_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         if ((pKerPrivArgs->isInterleave == 1U) && (pKerPrivArgs->useNonInterleave == 0U)) {
            if (pKerPrivArgs->phase > 0U) {
               pKerPrivArgs->execute = AUDIOLIB_upSample_execInterleave_ci<true, double>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_upSample_execInterleave_ci<false, double>;
            }
         }
         else {
            if (pKerPrivArgs->phase > 0U) {
               switch (pKerPrivArgs->upFactor) {
               case 2U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample2_exec_ci<true, double>;
                  break;
               case 3U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample3_exec_ci<true, double>;
                  break;
               case 4U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample4_exec_ci<true, double>;
                  break;
               case 5U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample5_exec_ci<true, double>;
                  break;
               case 6U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample6_exec_ci<true, double>;
                  break;
               case 7U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample7_exec_ci<true, double>;
                  break;
               case 8U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample8_exec_ci<true, double>;
                  break;
               default:
                  pKerPrivArgs->execute = AUDIOLIB_upSampleNonInterleave_exec_ci<true, double>;
                  break;
               }
            }
            else {
               switch (pKerPrivArgs->upFactor) {
               case 2U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample2_exec_ci<false, double>;
                  break;
               case 3U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample3_exec_ci<false, double>;
                  break;
               case 4U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample4_exec_ci<false, double>;
                  break;
               case 5U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample5_exec_ci<false, double>;
                  break;
               case 6U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample6_exec_ci<false, double>;
                  break;
               case 7U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample7_exec_ci<false, double>;
                  break;
               case 8U:
                  pKerPrivArgs->execute = AUDIOLIB_upSample8_exec_ci<false, double>;
                  break;
               default:
                  pKerPrivArgs->execute = AUDIOLIB_upSampleNonInterleave_exec_ci<false, double>;
                  break;
               }
            }
         }
         status = AUDIOLIB_upSample_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_upSample_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_exec\n");

   AUDIOLIB_upSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;

   return pKerPrivArgs->execute(handle, pIn, pOut);
}

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample.cpp                                      */
/* ======================================================================== */
