// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_ssrc_priv.h"
#include <math.h>

#define AUDIOLIB_SSRC_MIN(x, y) (((x) < (y)) ? (x) : (y))

AUDIOLIB_STATUS AUDIOLIB_ssrc_init_cn(AUDIOLIB_kernelHandle   handle,
                                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_exec_cn(AUDIOLIB_kernelHandle handle,
                                                 void *restrict pIn,
                                                 void *restrict pState,
                                                 void *restrict pFiltCoeffs,
                                                 void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_exec_cn\n");
#endif

   AUDIOLIB_STATUS         status               = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint32_t                cirBuffAddressMask   = pKerPrivArgs->cirBuffAddressMask;
   uint32_t               *stage1BuffStartIndex = &pKerPrivArgs->stage1BuffStartIndex; /* index into circular buffer */
   dataType               *pInLocal             = (dataType *) pIn;
   dataType               *pOutLocal            = ((dataType *) pOut);
   const dataType         *pFilterLocal         = (dataType *) pFiltCoeffs;
   int32_t                 numChannels          = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount     = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 i, j, k;

   for (i = 0; i < inputSampleCount; i++) {
      for (j = 0; j < numChannels; j++) {
         dataType outTemp       = 0;
         uint32_t dataOffset    = (*stage1BuffStartIndex + i * numChannels + j) & cirBuffAddressMask;
         int32_t  dataOffsetOut = i * (int32_t) AUDIOLIB_SSRC_NUMBER_OF_FILTER_PHASES * numChannels + j;

         uint32_t idx1, idx2;
         for (k = 0; k < static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS); k++) {
            idx1 = (dataOffset + k * numChannels) & cirBuffAddressMask;
            idx2 = (dataOffset + ((AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1) - k) * numChannels) &
                   cirBuffAddressMask;

            dataType inVal1    = pInLocal[idx1];
            dataType inVal2    = pInLocal[idx2];
            dataType filterVal = pFilterLocal[k];
            dataType outProd   = (inVal1 + inVal2) * filterVal;

            outTemp += outProd;
         }

         idx1                                   = (dataOffset + k * numChannels) & cirBuffAddressMask;
         pOutLocal[dataOffsetOut]               = outTemp;
         pOutLocal[dataOffsetOut + numChannels] = pInLocal[idx1];
      }
   }

   *stage1BuffStartIndex = (uint32_t) (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_exec_cn(AUDIOLIB_kernelHandle handle,
                                                 void *restrict pIn,
                                                 void *restrict pState,
                                                 void *restrict pFiltCoeffs,
                                                 void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_exec_cn\n");
#endif

   AUDIOLIB_STATUS         status               = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint32_t                cirBuffAddressMask   = pKerPrivArgs->cirBuffAddressMask;
   uint32_t               *stage1BuffStartIndex = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t               *stage2BuffStartIndex = &pKerPrivArgs->stage2BuffStartIndex;
   uint32_t               *stage2BuffCurrIndex  = &pKerPrivArgs->stage2BuffCurrIndex;
   dataType               *pInLocal             = (dataType *) pIn;
   dataType               *pOutLocal            = ((dataType *) pOut);
   const dataType         *pFilterCoeffsLocal   = (dataType *) pFiltCoeffs;
   const dataType         *pFilterStage1        = pFilterCoeffsLocal + pKerPrivArgs->stage1FiltCoeffsOffset;
   const dataType         *pFilterStage2        = pFilterCoeffsLocal + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 numChannels          = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount     = pKerPrivArgs->initArgs.inputSampleCount;
   uint32_t                inBufferTotalDimX    = pKerPrivArgs->inBufferTotalDimX;
   int32_t                 i, j, k;

   // For interleaved mode, we use the second half of the pIn buffer as the intermediate buffer
   // This is possible because we've allocated double the size for pIn when sampleRateRatio is 4
   // as seen in the test driver file
   dataType *pIntermediateLocal = pInLocal + inBufferTotalDimX; // Point to the second half of pIn buffer

   // Stage 1: 2x upsampling using stage1FiltCoeffs
   for (i = 0; i < inputSampleCount; i++) {
      for (j = 0; j < numChannels; j++) {
         dataType outTemp    = 0;
         uint32_t dataOffset = (*stage1BuffStartIndex + i * numChannels + j) & cirBuffAddressMask;
         uint32_t dataOffsetOut =
             (*stage2BuffCurrIndex + i * (int32_t) AUDIOLIB_SSRC_NUMBER_OF_FILTER_PHASES * numChannels + j) &
             cirBuffAddressMask;

         uint32_t idx1, idx2;
         for (k = 0; k < static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS); k++) {
            idx1 = (dataOffset + k * numChannels) & cirBuffAddressMask;
            idx2 = (dataOffset + ((AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1) - k) * numChannels) &
                   cirBuffAddressMask;

            dataType inVal1    = pInLocal[idx1];
            dataType inVal2    = pInLocal[idx2];
            dataType filterVal = pFilterStage1[k];
            dataType outProd   = (inVal1 + inVal2) * filterVal;

            outTemp += outProd;
         }

         idx1                              = (dataOffset + k * numChannels) & cirBuffAddressMask;
         pIntermediateLocal[dataOffsetOut] = outTemp;
         pIntermediateLocal[(dataOffsetOut + numChannels) & cirBuffAddressMask] = pInLocal[idx1];
      }
   }

   // Update circular buffer index after first stage
   *stage1BuffStartIndex = (uint32_t) (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   // Stage 2: 2x upsampling using stage2FiltCoeffs
   int32_t stage2InputSampleCount = inputSampleCount * 2;
   *stage2BuffCurrIndex = (uint32_t) (*stage2BuffCurrIndex + stage2InputSampleCount * numChannels) & cirBuffAddressMask;
   // We'll use the pIntermediateLocal (second half of pIn) as our stage2 circular buffer
   // The stage2 filter is smaller than stage1, so we have enough space

   // Stage 2: 2x upsampling using stage2FiltCoeffs
   for (i = 0; i < stage2InputSampleCount; i++) {
      for (j = 0; j < numChannels; j++) {
         dataType outTemp       = 0;
         uint32_t dataOffset    = (*stage2BuffStartIndex + i * numChannels + j) & cirBuffAddressMask;
         int32_t  dataOffsetOut = i * (int32_t) AUDIOLIB_SSRC_NUMBER_OF_FILTER_PHASES * numChannels + j;

         uint32_t idx1, idx2;
         for (k = 0; k < static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS); k++) {
            idx1 = (dataOffset + k * numChannels) & cirBuffAddressMask;
            idx2 = (dataOffset + ((AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) - k) * numChannels) &
                   cirBuffAddressMask;
            dataType inVal1    = pIntermediateLocal[idx1];
            dataType inVal2    = pIntermediateLocal[idx2];
            dataType filterVal = pFilterStage2[k];
            dataType outProd   = (inVal1 + inVal2) * filterVal;

            outTemp += outProd;
         }

         idx1                                   = (dataOffset + k * numChannels) & cirBuffAddressMask;
         pOutLocal[dataOffsetOut]               = outTemp;
         pOutLocal[dataOffsetOut + numChannels] = pIntermediateLocal[idx1];
      }
   }

   *stage2BuffStartIndex =
       (uint32_t) (*stage2BuffStartIndex + stage2InputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_exec_cn(AUDIOLIB_kernelHandle handle,
                                                   void *restrict pIn,
                                                   void *restrict pState,
                                                   void *restrict pFiltCoeffs,
                                                   void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_exec_cn\n");
#endif

   AUDIOLIB_STATUS         status               = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint32_t                cirBuffAddressMask   = pKerPrivArgs->cirBuffAddressMask;
   uint32_t               *stage1BuffStartIndex = &pKerPrivArgs->stage1BuffStartIndex; /* index into circular buffer */
   dataType               *pInLocal             = (dataType *) pIn;
   dataType               *pOutLocal            = (dataType *) pOut;
   const dataType         *pFilterLocal         = (dataType *) pFiltCoeffs;
   int32_t                 numChannels          = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount     = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 outputSampleCount    = pKerPrivArgs->outputSampleCount;
   int32_t                 i, j, k;

   // Process each output sample
   for (i = 0; i < outputSampleCount; i++) {
      for (j = 0; j < numChannels; j++) {
         dataType outTemp = 0;
         // For each output sample, we process 2 input samples (2x downsampling)
         uint32_t dataOffset    = (*stage1BuffStartIndex + i * 2 * numChannels + j) & cirBuffAddressMask;
         int32_t  dataOffsetOut = i * numChannels + j;

         // Apply the filter - using symmetric filter coefficients
         uint32_t idx1, idx2;
         for (k = 0; k < static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS); k++) {
            idx1 = (dataOffset + k * 2 * numChannels) & cirBuffAddressMask;
            idx2 =
                (dataOffset + ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) - (k * 2)) * numChannels) &
                cirBuffAddressMask;

            dataType inVal1    = pInLocal[idx1];
            dataType inVal2    = pInLocal[idx2];
            dataType filterVal = pFilterLocal[k];
            dataType outProd   = (inVal1 + inVal2) * filterVal;

            outTemp += outProd;
         }

         // Store the output sample
         idx1                     = (dataOffset + ((k * 2) - 1) * numChannels) & cirBuffAddressMask;
         pOutLocal[dataOffsetOut] = outTemp + pInLocal[idx1] * 0.5f;
      }
   }

   // Update circular buffer index for next call
   *stage1BuffStartIndex = (uint32_t) (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_exec_cn(AUDIOLIB_kernelHandle handle,
                                                   void *restrict pIn,
                                                   void *restrict pState,
                                                   void *restrict pFiltCoeffs,
                                                   void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_exec_cn\n");
#endif

   AUDIOLIB_STATUS         status               = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint32_t                cirBuffAddressMask   = pKerPrivArgs->cirBuffAddressMask;
   uint32_t               *stage1BuffStartIndex = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t               *stage2BuffStartIndex = &pKerPrivArgs->stage2BuffStartIndex;
   uint32_t               *stage2BuffCurrIndex  = &pKerPrivArgs->stage2BuffCurrIndex;
   dataType               *pInLocal             = (dataType *) pIn;
   dataType               *pOutLocal            = (dataType *) pOut;
   const dataType         *pFilterCoeffsLocal   = (dataType *) pFiltCoeffs;
   const dataType         *pFilterStage1        = pFilterCoeffsLocal + pKerPrivArgs->stage1FiltCoeffsOffset;
   const dataType         *pFilterStage2        = pFilterCoeffsLocal + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 numChannels          = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount     = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 outputSampleCount    = pKerPrivArgs->outputSampleCount;
   uint32_t                inBufferTotalDimX    = pKerPrivArgs->inBufferTotalDimX;
   int32_t                 i, j, k;

   // For interleaved mode, we use the second half of the pIn buffer as the intermediate buffer
   dataType *pIntermediateLocal = pInLocal + inBufferTotalDimX; // Point to the second half of pIn buffer

   // Stage 1: 2x downsampling using stage1FiltCoeffs
   int32_t stage1OutputSampleCount = inputSampleCount / 2;

   // Process each output sample for stage 1
   for (i = 0; i < stage1OutputSampleCount; i++) {
      for (j = 0; j < numChannels; j++) {
         dataType outTemp = 0;
         // For each output sample, we process 2 input samples (2x downsampling)
         uint32_t dataOffset    = (*stage1BuffStartIndex + i * 2 * numChannels + j) & cirBuffAddressMask;
         uint32_t dataOffsetOut = (*stage2BuffCurrIndex + i * numChannels + j) & cirBuffAddressMask;

         // Apply the filter - using symmetric filter coefficients
         uint32_t idx1, idx2;
         for (k = 0; k < static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS); k++) {
            idx1 = (dataOffset + k * 2 * numChannels) & cirBuffAddressMask;
            idx2 =
                (dataOffset + ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS - 1) - (k * 2)) * numChannels) &
                cirBuffAddressMask;

            dataType inVal1    = pInLocal[idx1];
            dataType inVal2    = pInLocal[idx2];
            dataType filterVal = pFilterStage1[k];
            dataType outProd   = (inVal1 + inVal2) * filterVal;

            outTemp += outProd;
         }

         // Store the intermediate output sample
         idx1                              = (dataOffset + ((k * 2) - 1) * numChannels) & cirBuffAddressMask;
         pIntermediateLocal[dataOffsetOut] = outTemp + pInLocal[idx1] * 0.5f;
      }
   }

   // Update circular buffer indices after first stage
   *stage1BuffStartIndex = (uint32_t) (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;
   *stage2BuffCurrIndex =
       (uint32_t) (*stage2BuffCurrIndex + stage1OutputSampleCount * numChannels) & cirBuffAddressMask;

   // Process each output sample for stage 2
   for (i = 0; i < outputSampleCount; i++) {
      for (j = 0; j < numChannels; j++) {
         dataType outTemp = 0;
         // For each output sample, we process 2 intermediate samples (2x downsampling)
         uint32_t dataOffset    = (*stage2BuffStartIndex + i * 2 * numChannels + j) & cirBuffAddressMask;
         int32_t  dataOffsetOut = i * numChannels + j;

         // Apply the filter - using symmetric filter coefficients
         uint32_t idx1, idx2;
         for (k = 0; k < static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS); k++) {
            idx1 = (dataOffset + k * 2 * numChannels) & cirBuffAddressMask;
            idx2 =
                (dataOffset + ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) - (k * 2)) * numChannels) &
                cirBuffAddressMask;

            dataType inVal1    = pIntermediateLocal[idx1];
            dataType inVal2    = pIntermediateLocal[idx2];
            dataType filterVal = pFilterStage2[k];
            dataType outProd   = (inVal1 + inVal2) * filterVal;

            outTemp += outProd;
         }

         // Store the final output sample
         idx1                     = (dataOffset + ((k * 2) - 1) * numChannels) & cirBuffAddressMask;
         pOutLocal[dataOffsetOut] = outTemp + pIntermediateLocal[idx1] * 0.5f;
      }
   }

   // Update circular buffer index for next call
   *stage2BuffStartIndex =
       (uint32_t) (*stage2BuffStartIndex + stage1OutputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                 void *restrict pIn,
                                                                 void *restrict pState,
                                                                 void *restrict pFiltCoeffs,
                                                                 void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                 void *restrict pIn,
                                                                 void *restrict pState,
                                                                 void *restrict pFiltCoeffs,
                                                                 void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pState,
                                                                   void *restrict pFiltCoeffs,
                                                                   void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pState,
                                                                   void *restrict pFiltCoeffs,
                                                                   void *restrict pOut);
