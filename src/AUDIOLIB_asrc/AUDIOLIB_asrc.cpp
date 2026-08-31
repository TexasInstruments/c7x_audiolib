// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_asrc_priv.h"
#include <string.h>

#define AUDIOLIB_ASRC_MIN(x, y) (((x) < (y)) ? (x) : (y))

int32_t AUDIOLIB_asrc_getHandleSize(AUDIOLIB_asrc_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_asrc_PrivArgs);
   return privBufSize;
}

int32_t AUDIOLIB_asrc_getNonInterleavedDataBufSize(uint8_t  numChannels,
                                                   uint32_t sampleCount,
                                                   uint32_t sampleDataType,
                                                   uint32_t dataFormat)
{
   int32_t nonInterleavedDataBufSize;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_getNonInterleavedDataBufSize\n");
#endif

#ifdef ARM_A53
   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
#endif
      if (sampleCount >= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS) {
         nonInterleavedDataBufSize = (int32_t) numChannels * (int32_t) sampleCount *
                                     (int32_t) AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR *
                                     (int32_t) AUDIOLIB_sizeof(sampleDataType);
      }
      else {
         nonInterleavedDataBufSize = (int32_t) numChannels * (int32_t) AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS *
                                     (int32_t) AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR *
                                     (int32_t) AUDIOLIB_sizeof(sampleDataType);
      }
#ifdef ARM_A53
   }
   else {
      if (sampleCount >= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS) {
         nonInterleavedDataBufSize = (int32_t) numChannels * (int32_t) sampleCount *
                                     (int32_t) AUDIOLIB_ASRC_QUADRUPLE_BUFFERING_FACTOR *
                                     (int32_t) AUDIOLIB_sizeof(sampleDataType);
      }
      else {
         nonInterleavedDataBufSize = (int32_t) numChannels * (int32_t) AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS *
                                     (int32_t) AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR *
                                     (int32_t) AUDIOLIB_sizeof(sampleDataType);
      }
   }
#endif
   return nonInterleavedDataBufSize;
}

int32_t AUDIOLIB_asrc_getFilterCoeffSize(uint32_t sampleDataType)
{
   int32_t filterCoeffSizeInBytes;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_getFilterCoeffSize\n");
#endif

   filterCoeffSizeInBytes = (int32_t) AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS *
                            (int32_t) (AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES + 3) *
                            (int32_t) AUDIOLIB_sizeof(sampleDataType);
   return filterCoeffSizeInBytes;
}

int32_t AUDIOLIB_asrc_getFilterRembufSize(uint8_t numChannels, uint32_t sampleDataType, uint32_t frameModuloFactor)
{
   int32_t filterRembufSizeInBytes;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_getFilterRembufSize\n");
#endif

   filterRembufSizeInBytes =
       (int32_t) numChannels * (int32_t) (frameModuloFactor - 1) * (int32_t) AUDIOLIB_sizeof(sampleDataType);
   return filterRembufSizeInBytes;
}

int32_t
AUDIOLIB_asrc_getOutBufferLength(sample_rate_t inputSampleRate, sample_rate_t outputSampleRate, uint32_t sampleCount)
{
   float   sampleRateRatio;
   int32_t outFrameLength;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_getFilterRembufSize\n");
#endif

   // Calculate the asynchronous sample rate conversion ratio
   if (inputSampleRate > SAMPLE_RATE_NA && inputSampleRate <= SAMPLE_RATE_48000) {
      sampleRateRatio =
          (float) convertSampleRateToInt(outputSampleRate) / (float) convertSampleRateToInt(inputSampleRate);
   }
   else {
      sampleRateRatio = 0.0f;
   }

   // Calculate output frame length based on asynchronous sample rate conversion ratio
   outFrameLength = (int32_t) ceil(sampleCount * sampleRateRatio) + AUDIOLIB_ASRC_MAX_MODULO_FACTOR;

   return outFrameLength;
}

uint32_t AUDIOLIB_asrc_getFilterLength(void)
{
   /* Return the length of the filter. */
   return (uint32_t) AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
}

AUDIOLIB_STATUS
AUDIOLIB_asrc_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_asrc_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   int32_t         outFrameLength;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_init_checkParams\n");
#endif
   if (handle == nullptr) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (bufParamsIn == nullptr || bufParamsOut == nullptr || pKerInitArgs == nullptr) {
         status = AUDIOLIB_ERR_NULL_POINTER;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      // Validate funcStyle
      if (pKerInitArgs->funcStyle < AUDIOLIB_FUNCTION_NATC || pKerInitArgs->funcStyle > AUDIOLIB_FUNCTION_OPTIMIZED) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }
   if (status == AUDIOLIB_SUCCESS) {
      /* Circular buffer size must be a multiple of 2 for the streaming engine circular buffer mode.*/
      /* Circular buffer addressing is only programed to use 16 MB for now (can be incrased). So we have to limit the
       * maxSampleCountPerBlock */
      /* 16 MB = 16 * 1024 * 1024 bytes */
      if (pKerInitArgs->maxSampleCountPerBlock == 0 || pKerInitArgs->maxSampleCountPerBlock % 2 != 0 ||
          pKerInitArgs->maxSampleCountPerBlock >
              (((16 * 1024 * 1024) / AUDIOLIB_ASRC_QUADRUPLE_BUFFERING_FACTOR) / AUDIOLIB_sizeof(AUDIOLIB_FLOAT32))) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }
   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->sampleDataType != AUDIOLIB_FLOAT32) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->inputSampleRate < SAMPLE_RATE_32000 || pKerInitArgs->inputSampleRate > SAMPLE_RATE_48000) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }
   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->outputSampleRate < SAMPLE_RATE_32000 || pKerInitArgs->outputSampleRate > SAMPLE_RATE_48000) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->numChannels == 0 || pKerInitArgs->numChannels > AUDIOLIB_ASRC_MAX_NUM_CHANNELS) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->dataFormat != AUDIOLIB_DATA_FORMAT_INTERLEAVED &&
          pKerInitArgs->dataFormat != AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->frameModuloFactor > AUDIOLIB_ASRC_MAX_MODULO_FACTOR) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

#ifdef C7X
   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (pKerInitArgs->matTransHandle == nullptr) {
            status = AUDIOLIB_ERR_NULL_POINTER;
         }
      }
   }
#endif

   if (status == AUDIOLIB_SUCCESS) {
      outFrameLength = AUDIOLIB_asrc_getOutBufferLength(pKerInitArgs->inputSampleRate, pKerInitArgs->outputSampleRate,
                                                        pKerInitArgs->maxSampleCountPerBlock);
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   if (status == AUDIOLIB_SUCCESS) {
      if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (bufParamsIn->dim_y != pKerInitArgs->maxSampleCountPerBlock) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
      else {
         if (bufParamsIn->dim_y != pKerInitArgs->numChannels) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (bufParamsIn->dim_x != pKerInitArgs->numChannels) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
      else {
         if (bufParamsIn->dim_x != pKerInitArgs->maxSampleCountPerBlock) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (bufParamsIn->stride_y !=
             (int32_t) (pKerInitArgs->numChannels * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType))) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
      else {
         if (pKerInitArgs->maxSampleCountPerBlock >= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS) {
            if (bufParamsIn->stride_y !=
                (int32_t) (pKerInitArgs->maxSampleCountPerBlock * AUDIOLIB_ASRC_QUADRUPLE_BUFFERING_FACTOR *
                           AUDIOLIB_sizeof(pKerInitArgs->sampleDataType))) {
               status = AUDIOLIB_ERR_INVALID_DIMENSION;
            }
         }
         else {
            if (bufParamsIn->stride_y !=
                (int32_t) (AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS * AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR *
                           AUDIOLIB_sizeof(pKerInitArgs->sampleDataType))) {
               status = AUDIOLIB_ERR_INVALID_DIMENSION;
            }
         }
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (bufParamsOut->dim_x != pKerInitArgs->numChannels) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
      else {
         if (bufParamsOut->dim_x != (uint32_t) outFrameLength) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (bufParamsOut->dim_y != (uint32_t) outFrameLength) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
      else {
         if (bufParamsOut->dim_y != pKerInitArgs->numChannels) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
   }

   if (status == AUDIOLIB_SUCCESS) {
      if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         if (bufParamsOut->stride_y != pKerInitArgs->numChannels * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType)) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
      else {
         if (bufParamsOut->stride_y != outFrameLength * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType)) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_asrc_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                               const void *restrict pIn,
                                               const void *restrict pNonInterleavedData,
                                               const void *restrict pFiltCoeffs,
                                               const void *restrict pFilterRembuf,
                                               const void *restrict pOut,
                                               const AUDIOLIB_asrc_ExecInArgs  *pKerInArgs,
                                               const AUDIOLIB_asrc_ExecOutArgs *pKerOutArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_asrc_PrivArgs *) handle;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_exec_checkParams\n");
#endif
   if ((handle == nullptr) || (pIn == nullptr) || (pFiltCoeffs == nullptr) || (pFilterRembuf == nullptr) ||
       (pOut == nullptr) || (pKerInArgs == nullptr) || (pKerOutArgs == nullptr) ||
       (pNonInterleavedData == nullptr && pKerPrivArgs->initArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((pKerInArgs->inputSampleCount > pKerPrivArgs->initArgs.maxSampleCountPerBlock) ||
          (pKerInArgs->inputSampleCount <= 0)) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_asrc_init(AUDIOLIB_kernelHandle   handle,
                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                   AUDIOLIB_asrc_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
#ifdef C7X
   DSPLIB_STATUS dsplib_status = DSPLIB_SUCCESS;
#endif
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs    = (AUDIOLIB_asrc_PrivArgs *) handle;
   float                   sampleRateRatio = 0.0;
#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT Enter AUDIOLIB_asrc_init\n");
#endif
   pKerPrivArgs->initArgs = *pKerInitArgs;

   int32_t ele_size = AUDIOLIB_sizeof(pKerInitArgs->sampleDataType);

   if (status == AUDIOLIB_SUCCESS) {
      pKerPrivArgs->outputsPerInputRatio = 1.0;
      pKerPrivArgs->fracOutputsRemaining = 0.0;
      pKerPrivArgs->remBufCount          = 0.0;

      uint32_t inputSampleRateInt = convertSampleRateToInt(pKerInitArgs->inputSampleRate);
      if (inputSampleRateInt == 0) {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
      else {
         sampleRateRatio = (float) convertSampleRateToInt(pKerInitArgs->outputSampleRate) / (float) inputSampleRateInt;
         pKerPrivArgs->maxOutputsPerInputRatio =
             sampleRateRatio + ((float) AUDIOLIB_ASRC_MAX_MODULO_FACTOR - (float) pKerInitArgs->frameModuloFactor) /
                                   pKerInitArgs->maxSampleCountPerBlock;
         pKerPrivArgs->outBufferDimX = bufParamsOut->dim_x;

         if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
            if (pKerInitArgs->maxSampleCountPerBlock >= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS) {
               pKerPrivArgs->inBufferTotalDimX =
                   pKerPrivArgs->initArgs.maxSampleCountPerBlock * AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR;
            }
            else {
               pKerPrivArgs->inBufferTotalDimX =
                   AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS * AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR;
            }

            pKerPrivArgs->bufParamsFilterRembuf.data_type = AUDIOLIB_FLOAT32;
            pKerPrivArgs->bufParamsFilterRembuf.dim_x     = (uint32_t) pKerPrivArgs->initArgs.numChannels;
            pKerPrivArgs->bufParamsFilterRembuf.dim_y     = (uint32_t) (pKerInitArgs->frameModuloFactor - 1);
            pKerPrivArgs->bufParamsFilterRembuf.stride_y =
                (uint32_t) pKerPrivArgs->initArgs.numChannels * (uint32_t) ele_size;
         }
         else {
            if (pKerInitArgs->maxSampleCountPerBlock >= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS) {
               pKerPrivArgs->inBufferTotalDimX =
                   pKerPrivArgs->initArgs.maxSampleCountPerBlock * AUDIOLIB_ASRC_QUADRUPLE_BUFFERING_FACTOR;
            }
            else {
               pKerPrivArgs->inBufferTotalDimX =
                   AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS * AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR;
            }

            pKerPrivArgs->bufParamsFilterRembuf.data_type = AUDIOLIB_FLOAT32;
            pKerPrivArgs->bufParamsFilterRembuf.dim_x     = (uint32_t) (pKerInitArgs->frameModuloFactor - 1);
            pKerPrivArgs->bufParamsFilterRembuf.dim_y     = (uint32_t) pKerPrivArgs->initArgs.numChannels;
            pKerPrivArgs->bufParamsFilterRembuf.stride_y =
                (uint32_t) (pKerInitArgs->frameModuloFactor - 1) * (uint32_t) ele_size;
         }
         pKerPrivArgs->inBufferTotalStrideY = pKerPrivArgs->inBufferTotalDimX * ele_size;
         pKerPrivArgs->cirBuffStartIndex =
             (int32_t) pKerPrivArgs->inBufferTotalDimX - (int32_t) (AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS - 1);

         if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
            status = AUDIOLIB_asrc_init_cn(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
#ifdef C7X
                  // Create DSPLIB_bufParams2D_t structures
                  DSPLIB_bufParams2D_t dsplibBufParamsIn;
                  DSPLIB_bufParams2D_t dsplibNonInterleavedData;

                  // Copy values from AUDIOLIB to DSPLIB structure
                  dsplibBufParamsIn.data_type = DSPLIB_FLOAT32;
                  dsplibBufParamsIn.dim_x     = bufParamsIn->dim_x;
                  dsplibBufParamsIn.dim_y     = bufParamsIn->dim_y;
                  dsplibBufParamsIn.stride_y  = bufParamsIn->stride_y;

                  dsplibNonInterleavedData.data_type = DSPLIB_FLOAT32;
                  dsplibNonInterleavedData.dim_x     = pKerPrivArgs->initArgs.maxSampleCountPerBlock;
                  dsplibNonInterleavedData.dim_y     = (uint32_t) pKerPrivArgs->initArgs.numChannels;
                  dsplibNonInterleavedData.stride_y  = pKerPrivArgs->inBufferTotalStrideY;

                  pKerPrivArgs->matTransKerInitArgs.funcStyle = DSPLIB_FUNCTION_NATC;

                  dsplib_status =
                      DSPLIB_matTrans_init_checkParams(pKerPrivArgs->initArgs.matTransHandle, &dsplibBufParamsIn,
                                                       &dsplibNonInterleavedData, &pKerPrivArgs->matTransKerInitArgs);

                  // Map DSPLIB_STATUS to AUDIOLIB_STATUS
                  if (dsplib_status == DSPLIB_SUCCESS) {
                     dsplib_status =
                         DSPLIB_matTrans_init(pKerPrivArgs->initArgs.matTransHandle, &dsplibBufParamsIn,
                                              &dsplibNonInterleavedData, &pKerPrivArgs->matTransKerInitArgs);
                     pKerPrivArgs->execute = AUDIOLIB_asrc_exec_cn_interleaved<float>;
                  }
                  else {
                     status = AUDIOLIB_ERR_FAILURE; // Default to generic failure
                  }
#endif
#ifdef ARM_A53
                  pKerPrivArgs->widthInNonInterleavedData   = bufParamsIn->dim_x;
                  pKerPrivArgs->heightInNonInterleavedData  = bufParamsIn->dim_y;
                  pKerPrivArgs->strideInNonInterleavedData  = bufParamsIn->stride_y;
                  pKerPrivArgs->strideOutNonInterleavedData = pKerPrivArgs->inBufferTotalStrideY;

                  pKerPrivArgs->execute = AUDIOLIB_asrc_exec_cn_interleaved<float>;
#endif
               }
               else {
                  pKerPrivArgs->execute = AUDIOLIB_asrc_exec_cn_non_interleaved<float>;
               }
            }
         }
         else {
            if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
#ifdef C7X
               // Create DSPLIB_bufParams2D_t structures
               DSPLIB_bufParams2D_t dsplibBufParamsIn;
               DSPLIB_bufParams2D_t dsplibNonInterleavedData;

               // Copy values from AUDIOLIB to DSPLIB structure
               dsplibBufParamsIn.data_type = DSPLIB_FLOAT32;
               dsplibBufParamsIn.dim_x     = bufParamsIn->dim_x;
               dsplibBufParamsIn.dim_y     = bufParamsIn->dim_y;
               dsplibBufParamsIn.stride_y  = bufParamsIn->stride_y;

               dsplibNonInterleavedData.data_type = DSPLIB_FLOAT32;
               dsplibNonInterleavedData.dim_x     = pKerPrivArgs->initArgs.maxSampleCountPerBlock;
               dsplibNonInterleavedData.dim_y     = (uint32_t) pKerPrivArgs->initArgs.numChannels;
               dsplibNonInterleavedData.stride_y  = pKerPrivArgs->inBufferTotalStrideY;

               pKerPrivArgs->matTransKerInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

               dsplib_status =
                   DSPLIB_matTrans_init_checkParams(pKerPrivArgs->initArgs.matTransHandle, &dsplibBufParamsIn,
                                                    &dsplibNonInterleavedData, &pKerPrivArgs->matTransKerInitArgs);
               // Map DSPLIB_STATUS to AUDIOLIB_STATUS
               if (dsplib_status == DSPLIB_SUCCESS) {
                  dsplib_status = DSPLIB_matTrans_init(pKerPrivArgs->initArgs.matTransHandle, &dsplibBufParamsIn,
                                                       &dsplibNonInterleavedData, &pKerPrivArgs->matTransKerInitArgs);
                  status = AUDIOLIB_asrc_init_ci_interleaved<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
                  pKerPrivArgs->execute = AUDIOLIB_asrc_exec_ci_interleaved<float>;
               }
               else {
                  status = AUDIOLIB_ERR_FAILURE; // Default to generic failure
               }
#endif
#ifdef ARM_A53
               pKerPrivArgs->widthInNonInterleavedData   = bufParamsIn->dim_x;
               pKerPrivArgs->heightInNonInterleavedData  = bufParamsIn->dim_y;
               pKerPrivArgs->strideInNonInterleavedData  = bufParamsIn->stride_y;
               pKerPrivArgs->strideOutNonInterleavedData = pKerPrivArgs->inBufferTotalStrideY;
               pKerPrivArgs->history_length              = AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS - 1;

               status = AUDIOLIB_asrc_init_ci_interleaved<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
               pKerPrivArgs->execute = AUDIOLIB_asrc_exec_ci_interleaved<float>;
#endif
            }
            else {
#ifdef ARM_A53
               pKerPrivArgs->history_length = AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS - 1;
#endif
               status = AUDIOLIB_asrc_init_ci_non_interleaved<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
               pKerPrivArgs->execute = AUDIOLIB_asrc_exec_ci_non_interleaved<float>;
            }
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_asrc_set(AUDIOLIB_kernelHandle handle,
                                  uint8_t               mode,
                                  double                asrcRatio,
                                  void *restrict pNonInterleavedData,
                                  void *restrict pIn)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_asrc_PrivArgs *) handle;
   uint32_t                dataType;
   int32_t                 i;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT Enter AUDIOLIB_asrc_set\n");
#endif

   if (handle == nullptr) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      dataType = pKerPrivArgs->initArgs.sampleDataType;
      if (mode == AUDIOLIB_ASRC_MODE_RESET) {

         pKerPrivArgs->fracOutputsRemaining = 0.0;
         pKerPrivArgs->remBufCount          = 0.0;

         if (pKerPrivArgs->initArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
            if (pIn != nullptr && pNonInterleavedData != nullptr) {
               if (pKerPrivArgs->initArgs.maxSampleCountPerBlock >= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS) {
                  int32_t offset =
                      (pKerPrivArgs->initArgs.maxSampleCountPerBlock * AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR) -
                      AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
                  for (i = 0; i < (int32_t) pKerPrivArgs->initArgs.numChannels; i++) {
                     float *pNonInterleavedDataLocal = (float *) pNonInterleavedData + offset;
                     memset((void *) pNonInterleavedDataLocal, 0,
                            AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS *
                                AUDIOLIB_sizeof(dataType)); /* init/clear portion of intermediate buffer */
                     offset += pKerPrivArgs->initArgs.maxSampleCountPerBlock * AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR;
                  }
#ifdef ARM_A53
                  const int32_t history_length = pKerPrivArgs->history_length;
                  const int32_t effective_dim  = history_length + pKerPrivArgs->initArgs.maxSampleCountPerBlock;

                  for (i = 0; i < (int32_t) pKerPrivArgs->initArgs.numChannels; i++) {
                     float *pChannelHistoryStart = (float *) pNonInterleavedData + i * effective_dim;
                     memset((void *) pChannelHistoryStart, 0, history_length * AUDIOLIB_sizeof(dataType));
                  }
#endif
               }
               else {
                  int32_t offset = AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
                  for (i = 0; i < (int32_t) pKerPrivArgs->initArgs.numChannels; i++) {
                     float *pNonInterleavedDataLocal = (float *) pNonInterleavedData + offset;
                     memset((void *) pNonInterleavedDataLocal, 0,
                            AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS *
                                AUDIOLIB_sizeof(dataType)); /* init/clear portion of pIn/State buffer */
                     offset += AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS * AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR;
                  }
#ifdef ARM_A53
                  const int32_t history_length = pKerPrivArgs->history_length;
                  const int32_t effective_dim  = history_length + pKerPrivArgs->initArgs.maxSampleCountPerBlock;

                  for (i = 0; i < (int32_t) pKerPrivArgs->initArgs.numChannels; i++) {
                     float *pChannelHistoryStart = (float *) pNonInterleavedData + i * effective_dim;
                     memset((void *) pChannelHistoryStart, 0, history_length * AUDIOLIB_sizeof(dataType));
                  }
#endif
               }
            }
            else {
               status = AUDIOLIB_ERR_NULL_POINTER; /* pNonInterleavedData must not be NULL for reset */
            }
         }
         else {
            if (pIn != nullptr) {
               if (pKerPrivArgs->initArgs.maxSampleCountPerBlock >= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS) {
                  int32_t offset =
                      (pKerPrivArgs->initArgs.maxSampleCountPerBlock * AUDIOLIB_ASRC_QUADRUPLE_BUFFERING_FACTOR) -
                      AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
                  for (i = 0; i < (int32_t) pKerPrivArgs->initArgs.numChannels; i++) {
                     float *pInLocal = (float *) pIn + offset;
                     memset((void *) pInLocal, 0,
                            AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS *
                                AUDIOLIB_sizeof(dataType)); /* init/clear portion of pIn/State buffer */
                     offset += pKerPrivArgs->initArgs.maxSampleCountPerBlock * AUDIOLIB_ASRC_QUADRUPLE_BUFFERING_FACTOR;
                  }
#ifdef ARM_A53
                  for (i = 0; i < (int32_t) pKerPrivArgs->initArgs.numChannels; i++) {
                     float *pChannelHistoryStart = (float *) pNonInterleavedData + i * pKerPrivArgs->inBufferTotalDimX;
                     memset((void *) pChannelHistoryStart, 0,
                            pKerPrivArgs->history_length *
                                AUDIOLIB_sizeof(dataType)); /* Clear history section for each channel */
                  }
#endif
               }
               else {
                  int32_t offset = AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
                  for (i = 0; i < (int32_t) pKerPrivArgs->initArgs.numChannels; i++) {
                     float *pInLocal = (float *) pIn + offset;
                     memset((void *) pInLocal, 0,
                            AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS *
                                AUDIOLIB_sizeof(dataType)); /* init/clear portion of pIn/State buffer */
                     offset += AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS * AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR;
                  }
#ifdef ARM_A53
                  for (i = 0; i < (int32_t) pKerPrivArgs->initArgs.numChannels; i++) {
                     float *pChannelHistoryStart = (float *) pNonInterleavedData + i * pKerPrivArgs->inBufferTotalDimX;
                     memset((void *) pChannelHistoryStart, 0,
                            pKerPrivArgs->history_length *
                                AUDIOLIB_sizeof(dataType)); /* Clear history section for each channel */
                  }
#endif
               }
            }
            else {
               status = AUDIOLIB_ERR_NULL_POINTER; /* pNonInterleavedData must not be NULL for reset */
            }
         }
      }
      else if (mode == AUDIOLIB_ASRC_MODE_SET) {
         if (asrcRatio <= 0.033f || asrcRatio > AUDIOLIB_ASRC_MAX_IO_SAMPLE_RATE_RATIO) {
            status = AUDIOLIB_ERR_INVALID_VALUE;
         }
         else {
            /* Set the outputsPerInputRatio using asrcRatio */
            pKerPrivArgs->outputsPerInputRatio = asrcRatio;
         }
      }
      else {
         status = AUDIOLIB_ERR_INVALID_VALUE;
      }
   }

   return status;
}

/** call exec_check before this */
AUDIOLIB_STATUS
AUDIOLIB_asrc_exec(AUDIOLIB_kernelHandle handle,
                   void *restrict pIn,
                   void *restrict pNonInterleavedData,
                   void *restrict pFiltCoeffs,
                   void *restrict pFilterRembuf,
                   void *restrict pOut,
                   const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                   AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_asrc_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf, pOut, pKerInArgs,
                                  pKerOutArgs);

   return status;
}

void AUDIOLIB_asrc_perfEst(AUDIOLIB_kernelHandle         handle,
                           const AUDIOLIB_bufParams2D_t *bufParamsIn,
                           const AUDIOLIB_bufParams2D_t *bufParamsOut,
                           uint64_t                     *archCycles,
                           uint64_t                     *estCycles)
{
   // Assembly analysis:
   // For both interleaved and non-interleaved C7x implementations,
   // the main compute loop has: Total cycles (est.): 42 + trip_cnt * 14

   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_asrc_PrivArgs *) handle;
   uint32_t                numChannels  = pKerPrivArgs->initArgs.numChannels;
   uint32_t                outputSamples =
       AUDIOLIB_asrc_getOutBufferLength(pKerPrivArgs->initArgs.inputSampleRate, pKerPrivArgs->initArgs.outputSampleRate,
                                        pKerPrivArgs->initArgs.maxSampleCountPerBlock) -
       AUDIOLIB_ASRC_MAX_MODULO_FACTOR;

   // Fixed overhead from assembly
   const uint64_t fixedOverhead        = 689;
   const uint64_t externalLoopOverhead = 95;
   const uint64_t perSampleCycles      = 14;

   uint64_t computeCycles =
       fixedOverhead + (outputSamples * (externalLoopOverhead + (42 + (numChannels + 1) / 2 * perSampleCycles)));

   *archCycles = computeCycles;
   *estCycles  = computeCycles;
}

uint32_t convertSampleRateToInt(sample_rate_t rate)
{
   uint32_t result = 0;

   switch (rate) {
   case SAMPLE_RATE_NA:
      result = 0;
      break;
   case SAMPLE_RATE_32000:
      result = 32000;
      break;
   case SAMPLE_RATE_44100:
      result = 44100;
      break;
   case SAMPLE_RATE_48000:
      result = 48000;
      break;
   default:
      result = 0;
      break;
   }

   return result;
}
