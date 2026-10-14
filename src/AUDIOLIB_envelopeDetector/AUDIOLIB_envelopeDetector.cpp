// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_envelopeDetector_priv.h"
#include "AUDIOLIB_bufParams.h"
#include <math.h>
#include <string.h>

int32_t AUDIOLIB_envelopeDetector_getHandleSize(AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs)
{
   (void) pKerInitArgs;
   return sizeof(AUDIOLIB_envelopeDetector_PrivArgs);
}

AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_init_checkParams(AUDIOLIB_kernelHandle                     handle,
                                           const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                           const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                           const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_envelopeDetector_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else {
         /* Nothing to do here */
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->dim_x == 0U) || (bufParamsIn->dim_y == 0U) || (bufParamsOut->dim_x != bufParamsIn->dim_x) ||
          (bufParamsOut->dim_y != bufParamsIn->dim_y) ||
          (bufParamsIn->stride_y < (int32_t) (bufParamsIn->dim_x * AUDIOLIB_sizeof(bufParamsIn->data_type))) ||
          (bufParamsOut->stride_y < (int32_t) (bufParamsOut->dim_x * AUDIOLIB_sizeof(bufParamsOut->data_type)))) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      /* One float of envelope state is held per channel in a fixed-size array. */
      else if (bufParamsIn->dim_y > (uint32_t) AUDIOLIB_ENVELOPEDETECTOR_MAX_NUM_CHANNELS) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else {
         /* Nothing to do here */
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->samplingRate <= 0) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn,
                                           const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_envelopeDetector_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_init(AUDIOLIB_kernelHandle                     handle,
                                               const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                               const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                               const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_envelopeDetector_init \n");

   AUDIOLIB_envelopeDetector_PrivArgs *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;

   pKerPrivArgs->dataType          = (AUDIOLIB_data_type_e) bufParamsIn->data_type;
   pKerPrivArgs->numSamples        = bufParamsIn->dim_x;
   pKerPrivArgs->numChannels       = bufParamsIn->dim_y;
   pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsOut->data_type);

   pKerPrivArgs->samplingRate = (float) pKerInitArgs->samplingRate;

   /* Every envelope starts at zero, so the first frame behaves as if preceded by silence. */
   for (uint32_t channel = 0U; channel < AUDIOLIB_ENVELOPEDETECTOR_MAX_NUM_CHANNELS; channel++) {
      pKerPrivArgs->envelopeStates[channel] = 0.0f;
   }

   uint64_t group4Cycles = AUDIOLIB_envelopeDetector_group4Cycles((uint32_t) pKerPrivArgs->dataType,
                                                                  pKerPrivArgs->numSamples, pKerPrivArgs->numChannels);
   uint64_t group8Cycles = AUDIOLIB_envelopeDetector_group8Cycles((uint32_t) pKerPrivArgs->dataType,
                                                                  pKerPrivArgs->numSamples, pKerPrivArgs->numChannels);

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_envelopeDetector_exec_cn<float>;
      }
      else {
         pKerPrivArgs->execute = AUDIOLIB_envelopeDetector_exec_cn<double>;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         if (group8Cycles < group4Cycles) {
            pKerPrivArgs->channelsPerGroup = AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS;
            status = AUDIOLIB_envelopeDetector_group8_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_envelopeDetector_group8_exec_ci<float>;
         }
         else {
            pKerPrivArgs->channelsPerGroup = AUDIOLIB_ENVELOPEDETECTOR_GROUP4_CHANNELS;
            status = AUDIOLIB_envelopeDetector_group4_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_envelopeDetector_group4_exec_ci<float>;
         }
      }
      else {
         if (group8Cycles < group4Cycles) {
            pKerPrivArgs->channelsPerGroup = AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS;
            status = AUDIOLIB_envelopeDetector_group8_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_envelopeDetector_group8_exec_ci<double>;
         }
         else {
            pKerPrivArgs->channelsPerGroup = AUDIOLIB_ENVELOPEDETECTOR_GROUP4_CHANNELS;
            status = AUDIOLIB_envelopeDetector_group4_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            pKerPrivArgs->execute = AUDIOLIB_envelopeDetector_group4_exec_ci<double>;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_set(AUDIOLIB_kernelHandle                    handle,
                                              const AUDIOLIB_envelopeDetector_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_envelopeDetector_set \n");

   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((pKerSetArgs->attackTime <= 0.0f) || (pKerSetArgs->releaseTime <= 0.0f)) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_envelopeDetector_PrivArgs *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_envelopeDetector_SetArgs));

      float samplingRate               = pKerPrivArgs->samplingRate;
      pKerPrivArgs->attackCoefficient  = 1.0f - expf(-1.0f / (samplingRate * pKerSetArgs->attackTime));
      pKerPrivArgs->releaseCoefficient = 1.0f - expf(-1.0f / (samplingRate * pKerSetArgs->releaseTime));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_get(AUDIOLIB_kernelHandle              handle,
                                              AUDIOLIB_envelopeDetector_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_envelopeDetector_PrivArgs *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_envelopeDetector_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_envelopeDetector_PrivArgs *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;

   if ((pKerPrivArgs == NULL) || (pIn == NULL) || (pOut == NULL)) {
      return AUDIOLIB_ERR_NULL_POINTER;
   }

   return pKerPrivArgs->execute(handle, pIn, pOut);
}
