// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_ssrc_priv.h"

/* N/A */

/*******************************************************************************
 * INITIALIZATION FUNCTIONS
 ******************************************************************************/
/*
 * Main initialization function for 2x downsampling using C7x intrinsics.
 * Selects the appropriate processing mode (linear, block, or single sample)
 * based on buffer format and input sample count for interleaved data format.
 * For non-interleaved data format, uses linear buffer based processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                 bufferFormat     = pKerPrivArgs->initArgs.bufferFormat;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   uint8_t                 dataFormat       = pKerPrivArgs->initArgs.dataFormat;

   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      // Check if we're using linear buffer format
      if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
         // For linear buffer format, use the specialized implementation
         status = AUDIOLIB_ssrc_downsample2x_linear_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (status == AUDIOLIB_SUCCESS) {
            pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample2x_linear_exec_ci<float>;
         }
      }
      else {
         // For circular buffer format, choose between block and single sample processing
         if (inputSampleCount > (int32_t) AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT) {
            // For larger sample counts, use block processing
            status = AUDIOLIB_ssrc_downsample2x_block_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample2x_block_exec_ci<float>;
            }
         }
         else {
            // For smaller sample counts, use single sample processing
            status = AUDIOLIB_ssrc_downsample2x_ss_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample2x_ss_exec_ci<float>;
            }
         }
      }
   }
   else {
      // For non-interleaved data format, use the specialized implementation
      status =
          AUDIOLIB_ssrc_downsample2x_non_interleaved_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      if (status == AUDIOLIB_SUCCESS) {
         pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample2x_non_interleaved_exec_ci<float>;
      }
   }

   return status;
}

/*******************************************************************************
 * SPECIALIZED INITIALIZATION FUNCTIONS
 ******************************************************************************/

/*
 * Initialization for 2x downsampling with linear buffer format (interleaved data).
 * Sets up DSPLIB kernels for deinterleave, FIR filtering, block copy, and interleave.
 * Configures streaming engine and agent parameters for downsampling.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_linear_init_ci\n");
#endif

   AUDIOLIB_STATUS         status            = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus      = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs      = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock            = pKerPrivArgs->bufPblock;
   int32_t                 numChannels       = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount  = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 outputSampleCount = pKerPrivArgs->outputSampleCount;
   int32_t                 stateStride       = pKerPrivArgs->inBufferTotalDimX;
   int32_t                 eleSize           = AUDIOLIB_sizeof(pKerInitArgs->sampleDataType);

   // Create buffer parameters for non-interleaved input and output
   DSPLIB_bufParams2D_t interleavedBufParams, nonInterleavedBufParams;
   DSPLIB_bufParams2D_t firBufParamsIn, firBufParamsOut, firBufParamsFilter;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

   // Set up interleaved input buffer parameters
   interleavedBufParams.data_type = DSPLIB_FLOAT32;
   interleavedBufParams.dim_x     = bufParamsIn->dim_x;
   interleavedBufParams.dim_y     = bufParamsIn->dim_y;
   interleavedBufParams.stride_y  = bufParamsIn->stride_y;

   // Set up non-interleaved intermediate buffer parameters
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = pKerPrivArgs->bufParamsState.dim_x;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   // Initialize DSPLIB deinterleaved for input (interleaved to non-interleaved)
   DSPLIB_deinterleaveInitArgs deinterleaveInitArgs;
   deinterleaveInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

   dsplibStatus = DSPLIB_deinterleave_init_checkParams(pKerInitArgs->deinterleaveHandle, &interleavedBufParams,
                                                       &nonInterleavedBufParams, &deinterleaveInitArgs);
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_deinterleave_init(pKerInitArgs->deinterleaveHandle, &interleavedBufParams,
                                              &nonInterleavedBufParams, &deinterleaveInitArgs);
   }

   /**********************************************************************/
   /* Prepare streaming engine 0 to decimate input samples.              */
   /**********************************************************************/
   int32_t icnt0Param = inputSampleCount + AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
   se0Params          = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0    = icnt0Param;
   se0Params.DIM1     = stateStride;
   se0Params.ICNT1    = numChannels;
   se0Params.DECIM    = __SE_DECIM_2;
   se0Params.ELETYPE  = SE_ELETYPE;
   se0Params.VECLEN   = SE_VECLEN;
   se0Params.DIMFMT   = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare SA to store decimated samples                              */
   /**********************************************************************/
   int32_t blkItrCount = AUDIOLIB_ceilingDiv(icnt0Param, eleCount * 2);
   sa0Params           = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0     = icnt0Param / 2;
   sa0Params.DIM1      = stateStride;
   sa0Params.ICNT1     = numChannels;
   sa0Params.VECLEN    = SA_VECLEN;
   sa0Params.DIMFMT    = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks1 = blkItrCount * numChannels;

   /* Initialize FIR kernel parameters for anti-aliasing filter */
   DSPLIB_fir_InitArgs firKerInitArgs;
   firKerInitArgs.funcStyle      = DSPLIB_FUNCTION_OPTIMIZED;
   firKerInitArgs.dataSize       = icnt0Param / 2;
   firKerInitArgs.batchSize      = numChannels;
   firKerInitArgs.filterSize     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
   firKerInitArgs.shift          = 0;
   firKerInitArgs.enableNchCoefs = 0;
   firKerInitArgs.enableMMA      = pKerInitArgs->enableMMA;
   firKerInitArgs.MMA_SIZE       = pKerInitArgs->mmaSize;
   firKerInitArgs.enableQ        = 1;
   firKerInitArgs.Q              = 23;

   /* Initialize FIR input buffer parameters */
   firBufParamsIn.data_type = DSPLIB_FLOAT32;
   firBufParamsIn.dim_x     = icnt0Param / 2;
   firBufParamsIn.dim_y     = numChannels;
   firBufParamsIn.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR output buffer parameters */
   firBufParamsOut.data_type = DSPLIB_FLOAT32;
   firBufParamsOut.dim_x     = outputSampleCount;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR filter buffer parameters */
   firBufParamsFilter.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter.dim_x =
          AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = firBufParamsFilter.dim_x * eleSize;
   }
   else {
      firBufParamsFilter.dim_x    = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC * eleSize;
   }

   /* Check FIR initialization parameters */
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter,
                                                 &firBufParamsOut, &firKerInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter, &firBufParamsOut,
                                     &firKerInitArgs);
   }

   // Set up non-interleaved intermediate buffer parameters for 2d block copy
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   // Initialize DSPLIB blkCopy2D for copying data to history buffer
   DSPLIB_blkCopy2DInitArgs blkCopy2DInitArgs;
   if (dsplibStatus == DSPLIB_SUCCESS) {
      blkCopy2DInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

      dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle1, &nonInterleavedBufParams,
                                                       &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle1, &nonInterleavedBufParams,
                                           &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   // Set up non-interleaved output buffer parameters
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = outputSampleCount;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   // Set up interleaved output buffer parameters
   interleavedBufParams.data_type = DSPLIB_FLOAT32;
   interleavedBufParams.dim_x     = bufParamsOut->dim_x;
   interleavedBufParams.dim_y     = bufParamsOut->dim_y;
   interleavedBufParams.stride_y  = bufParamsOut->stride_y;

   // Initialize DSPLIB interleave for output (non-interleaved to interleaved)
   DSPLIB_interleaveInitArgs interleaveInitArgs;
   if (dsplibStatus == DSPLIB_SUCCESS) {
      interleaveInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

      dsplibStatus = DSPLIB_interleave_init_checkParams(pKerInitArgs->interleaveHandle, &nonInterleavedBufParams,
                                                        &interleavedBufParams, &interleaveInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_interleave_init(pKerInitArgs->interleaveHandle, &nonInterleavedBufParams,
                                            &interleavedBufParams, &interleaveInitArgs);
   }

   /**********************************************************************/
   /* Prepare streaming engine 0 to read output of fir phase 1           */
   /**********************************************************************/

   se1Params         = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0   = outputSampleCount;
   se1Params.DIM1    = stateStride;
   se1Params.ICNT1   = numChannels;
   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   se1Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to read input data from phase 2         */
   /**********************************************************************/

   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = inputSampleCount;
   se2Params.DIM1    = stateStride;
   se2Params.ICNT1   = numChannels;
   se2Params.DECIM   = __SE_DECIM_2;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare SA to store filtered samples                               */
   /**********************************************************************/
   blkItrCount      = AUDIOLIB_ceilingDiv(outputSampleCount, eleCount);
   sa1Params        = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0  = outputSampleCount;
   sa1Params.DIM1   = stateStride;
   sa1Params.ICNT1  = numChannels;
   sa1Params.VECLEN = SA_VECLEN;
   sa1Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks2 = blkItrCount * numChannels;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   if (dsplibStatus != DSPLIB_SUCCESS) {
      status = AUDIOLIB_ERR_FAILURE;
   }

   return status;
}

/*
 * Initialization for 2x downsampling with non-interleaved linear buffer format.
 * Sets up DSPLIB kernels for block copy and FIR filtering.
 * Configures streaming engine and agent parameters for downsampling.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_non_interleaved_init_ci\n");
#endif

   AUDIOLIB_STATUS         status            = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus      = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs      = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock            = pKerPrivArgs->bufPblock;
   int32_t                 numChannels       = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount  = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 outputSampleCount = pKerPrivArgs->outputSampleCount;
   int32_t                 stateStride       = pKerPrivArgs->inBufferTotalDimX;
   int32_t                 eleSize           = AUDIOLIB_sizeof(pKerInitArgs->sampleDataType);

   typedef typename c7x::make_full_vector<dataType>::type vec;
   __SE_TEMPLATE_v1                                       se0Params, se1Params, se2Params;
   __SA_TEMPLATE_v1                                       sa0Params, sa1Params;
   __SE_VECLEN                                            SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN                                            SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE                                           SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t                                                eleCount   = c7x::element_count_of<vec>::value;

   // 1. Set up buffer parameters for non-interleaved data
   DSPLIB_bufParams2D_t nonInterleavedBufParams, srcBufParams;

   srcBufParams.data_type = DSPLIB_FLOAT32;
   srcBufParams.dim_x     = bufParamsIn->dim_x;
   srcBufParams.dim_y     = bufParamsIn->dim_y;
   srcBufParams.stride_y  = bufParamsIn->stride_y;

   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = pKerPrivArgs->bufParamsState.dim_x;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   DSPLIB_blkCopy2DInitArgs blkCopy2DInitArgs;
   blkCopy2DInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;
   dsplibStatus                = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle1, &srcBufParams,
                                                                   &nonInterleavedBufParams, &blkCopy2DInitArgs);
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle1, &srcBufParams, &nonInterleavedBufParams,
                                           &blkCopy2DInitArgs);
   }

   // 2. Set up streaming engine and agent templates
   int32_t icnt0Param = inputSampleCount + AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
   se0Params          = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0    = icnt0Param;
   se0Params.DIM1     = stateStride;
   se0Params.ICNT1    = numChannels;
   se0Params.DECIM    = __SE_DECIM_2;
   se0Params.ELETYPE  = SE_ELETYPE;
   se0Params.VECLEN   = SE_VECLEN;
   se0Params.DIMFMT   = __SE_DIMFMT_2D;

   int32_t blkItrCount = AUDIOLIB_ceilingDiv(icnt0Param, eleCount * 2);
   sa0Params           = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0     = icnt0Param / 2;
   sa0Params.DIM1      = stateStride;
   sa0Params.ICNT1     = numChannels;
   sa0Params.VECLEN    = SA_VECLEN;
   sa0Params.DIMFMT    = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks1 = blkItrCount * numChannels;

   DSPLIB_fir_InitArgs firKerInitArgs;
   firKerInitArgs.funcStyle      = DSPLIB_FUNCTION_OPTIMIZED;
   firKerInitArgs.dataSize       = icnt0Param / 2;
   firKerInitArgs.batchSize      = numChannels;
   firKerInitArgs.filterSize     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
   firKerInitArgs.shift          = 0;
   firKerInitArgs.enableNchCoefs = 0;
   firKerInitArgs.enableMMA      = pKerInitArgs->enableMMA;
   firKerInitArgs.MMA_SIZE       = pKerInitArgs->mmaSize;
   firKerInitArgs.enableQ        = 1;
   firKerInitArgs.Q              = 23;

   DSPLIB_bufParams2D_t firBufParamsIn, firBufParamsOut, firBufParamsFilter;
   firBufParamsIn.data_type = DSPLIB_FLOAT32;
   firBufParamsIn.dim_x     = icnt0Param / 2;
   firBufParamsIn.dim_y     = numChannels;
   firBufParamsIn.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR output buffer parameters */
   firBufParamsOut.data_type = DSPLIB_FLOAT32;
   firBufParamsOut.dim_x     = outputSampleCount;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   firBufParamsFilter.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter.dim_x =
          AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = firBufParamsFilter.dim_x * eleSize;
   }
   else {
      firBufParamsFilter.dim_x    = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC * eleSize;
   }

   dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter,
                                              &firBufParamsOut, &firKerInitArgs);
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter, &firBufParamsOut,
                                     &firKerInitArgs);
   }

   se1Params         = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0   = outputSampleCount;
   se1Params.DIM1    = stateStride;
   se1Params.ICNT1   = numChannels;
   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   se1Params.DIMFMT  = __SE_DIMFMT_2D;

   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = inputSampleCount;
   se2Params.DIM1    = stateStride;
   se2Params.ICNT1   = numChannels;
   se2Params.DECIM   = __SE_DECIM_2;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_2D;

   int32_t blkItrCount2 = AUDIOLIB_ceilingDiv(outputSampleCount, eleCount);
   sa1Params            = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0      = outputSampleCount;
   sa1Params.DIM1       = bufParamsOut->stride_y / eleSize;
   sa1Params.ICNT1      = numChannels;
   sa1Params.VECLEN     = SA_VECLEN;
   sa1Params.DIMFMT     = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks2 = blkItrCount2 * numChannels;

   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle2, &nonInterleavedBufParams,
                                                    &nonInterleavedBufParams, &blkCopy2DInitArgs);
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle2, &nonInterleavedBufParams,
                                           &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   if (dsplibStatus != DSPLIB_SUCCESS) {
      status = AUDIOLIB_ERR_FAILURE;
   }

   return status;
}

/*
 * Initialization for 2x downsampling using block processing (circular buffer).
 * Used when inputSampleCount exceeds threshold. Sets up streaming engines and agents
 * for multi-channel block FIR filtering and output decimation.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_block_init_ci(AUDIOLIB_kernelHandle   handle,
                                                         AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                         AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                         AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_block_init_ci\n");
#endif

   AUDIOLIB_STATUS         status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs      = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock            = pKerPrivArgs->bufPblock;
   int32_t                 numChannels       = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount  = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 outputSampleCount = pKerPrivArgs->outputSampleCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   pKerPrivArgs->numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   int32_t icnt0Param             = AUDIOLIB_min(numChannels, eleCount);

   // Calculate the number of output blocks needed
   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(outputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch input samples with decimation   */
   /**********************************************************************/
   se0Params               = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0         = icnt0Param;
   se0Params.DIM1          = numChannels * 2; // Decimate by 2 for downsampling
   se0Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se0Params.DIM2          = numChannels * 2;
   se0Params.ICNT2         = (int32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS);
   se0Params.DIM3          = numChannels * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK * 2;
   se0Params.ICNT3         = pKerPrivArgs->numOutputBlocks1;
   se0Params.DIM4          = eleCount;
   se0Params.ICNT4         = pKerPrivArgs->numChannelBlocks;
   se0Params.DECDIM1       = __SE_DECDIM_DIM3;
   se0Params.DECDIM1SD     = __SE_DECDIMSD_DIM1;
   se0Params.DECDIM1_WIDTH = inputSampleCount * numChannels;
   se0Params.DECDIM2       = __SE_DECDIM_DIM4;
   se0Params.DECDIM2_WIDTH = numChannels;
   se0Params.AM0           = __SE_AM_CIRC_CBK0;
   se0Params.AM1           = __SE_AM_CIRC_CBK0;
   se0Params.AM2           = __SE_AM_CIRC_CBK0;
   se0Params.AM3           = __SE_AM_CIRC_CBK0;
   se0Params.AM4           = __SE_AM_CIRC_CBK0;
   /* Determine state Circular Buffer size.
      CB size = 2^(encoding + 9) bytes. */
   uint32_t cbSizeB       = C7000_MIN_CIRCULAR_BUFFER_SIZE_B;
   uint32_t encCbSizeB    = 0;
   uint32_t totBlockSizeB = pKerPrivArgs->inBufferTotalStrideY;
   /* The limit of totBlockSizeB is checked in AUDIOLIB_asrc_init_checkParams() through maxSampleCountPerBlock. */
   while (totBlockSizeB > cbSizeB) {
      encCbSizeB++;
      cbSizeB *= 2;
   }
   se0Params.CBK0    = encCbSizeB;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_5D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to fetch reverse samples.               */
   /**********************************************************************/
   se1Params               = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0         = icnt0Param;
   se1Params.DIM1          = numChannels * 2;
   se1Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se1Params.DIM2          = -numChannels * 2;
   se1Params.ICNT2         = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;
   se1Params.DIM3          = numChannels * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK * 2;
   se1Params.ICNT3         = pKerPrivArgs->numOutputBlocks1;
   se1Params.DIM4          = eleCount;
   se1Params.ICNT4         = pKerPrivArgs->numChannelBlocks;
   se1Params.DECDIM1       = __SE_DECDIM_DIM3;
   se1Params.DECDIM1SD     = __SE_DECDIMSD_DIM1;
   se1Params.DECDIM1_WIDTH = inputSampleCount * numChannels;
   se1Params.DECDIM2       = __SE_DECDIM_DIM4;
   se1Params.DECDIM2_WIDTH = numChannels;
   se1Params.AM0           = __SE_AM_CIRC_CBK0;
   se1Params.AM1           = __SE_AM_CIRC_CBK0;
   se1Params.AM2           = __SE_AM_CIRC_CBK0;
   se1Params.AM3           = __SE_AM_CIRC_CBK0;
   se1Params.AM4           = __SE_AM_CIRC_CBK0;
   se1Params.CBK0          = encCbSizeB;
   se1Params.ELETYPE       = SE_ELETYPE;
   se1Params.VECLEN        = SE_VECLEN;
   se1Params.DIMFMT        = __SE_DIMFMT_5D;

   /**********************************************************************/
   /* Prepare SA to fetch the filter coefficients                        */
   /**********************************************************************/
   sa0Params        = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0  = 1;
   sa0Params.DIM1   = 1;
   sa0Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;
   sa0Params.DIM2   = 0;
   sa0Params.ICNT2  = pKerPrivArgs->numOutputBlocks1;
   sa0Params.DIM3   = 0;
   sa0Params.ICNT3  = pKerPrivArgs->numChannelBlocks;
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store FIR output                            */
   /**********************************************************************/
   sa1Params               = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0         = icnt0Param;
   sa1Params.DIM1          = numChannels;
   sa1Params.ICNT1         = pKerPrivArgs->numOutputBlocks1 * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   sa1Params.DIM2          = eleCount;
   sa1Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa1Params.DECDIM1_WIDTH = numChannels * outputSampleCount;
   sa1Params.DECDIM2       = __SA_DECDIM_DIM2;
   sa1Params.DECDIM2_WIDTH = numChannels;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;

   /**********************************************************************/
   /* Prepare streaming engine 0 to read output of fir phase 1           */
   /**********************************************************************/

   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = numChannels;
   se2Params.DIM1    = numChannels;
   se2Params.ICNT1   = outputSampleCount;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to read input data from phase 2         */
   /**********************************************************************/

   se3Params         = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0   = numChannels;
   se3Params.DIM1    = numChannels * 2;
   se3Params.ICNT1   = outputSampleCount;
   se3Params.AM0     = __SE_AM_CIRC_CBK0;
   se3Params.AM1     = __SE_AM_CIRC_CBK0;
   se3Params.CBK0    = encCbSizeB;
   se3Params.ELETYPE = SE_ELETYPE;
   se3Params.VECLEN  = SE_VECLEN;
   se3Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare SA to store filtered samples                               */
   /**********************************************************************/
   sa2Params        = __gen_SA_TEMPLATE_v1();
   sa2Params.ICNT0  = numChannels;
   sa2Params.DIM1   = numChannels;
   sa2Params.ICNT1  = outputSampleCount;
   sa2Params.VECLEN = SA_VECLEN;
   sa2Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks2 = pKerPrivArgs->numChannelBlocks * outputSampleCount;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;

   return status;
}

/*
 * Initialization for 2x downsampling using single sample processing (circular buffer).
 * Used when inputSampleCount is below threshold. Sets up streaming engines and agents
 * for per-sample FIR filtering and output decimation.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_ss_init_ci(AUDIOLIB_kernelHandle   handle,
                                                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                      AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                      AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_ss_init_ci\n");
#endif

   AUDIOLIB_STATUS         status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs      = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock            = pKerPrivArgs->bufPblock;
   int32_t                 numChannels       = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 outputSampleCount = pKerPrivArgs->outputSampleCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   pKerPrivArgs->numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   int32_t icnt0Param             = AUDIOLIB_min(numChannels, eleCount);

   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch input samples with decimation   */
   /**********************************************************************/
   se0Params               = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0         = icnt0Param;
   se0Params.DIM1          = numChannels * 2;
   se0Params.ICNT1         = (int32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS);
   se0Params.DIM2          = eleCount;
   se0Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se0Params.DIM3          = numChannels * 2;
   se0Params.ICNT3         = outputSampleCount;
   se0Params.DECDIM1       = __SE_DECDIM_DIM2;
   se0Params.DECDIM1_WIDTH = numChannels;
   se0Params.AM0           = __SE_AM_CIRC_CBK0;
   se0Params.AM1           = __SE_AM_CIRC_CBK0;
   se0Params.AM2           = __SE_AM_CIRC_CBK0;
   se0Params.AM3           = __SE_AM_CIRC_CBK0;
   /* Determine state Circular Buffer size.
      CB size = 2^(encoding + 9) bytes. */
   uint32_t cbSizeB       = C7000_MIN_CIRCULAR_BUFFER_SIZE_B;
   uint32_t encCbSizeB    = 0;
   uint32_t totBlockSizeB = pKerPrivArgs->inBufferTotalStrideY;
   /* The limit of totBlockSizeB is checked in AUDIOLIB_asrc_init_checkParams() through maxSampleCountPerBlock. */
   while (totBlockSizeB > cbSizeB) {
      encCbSizeB++;
      cbSizeB *= 2;
   }
   se0Params.CBK0    = encCbSizeB;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to fetch reverse samples.               */
   /**********************************************************************/
   se1Params               = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0         = icnt0Param;
   se1Params.DIM1          = -numChannels * 2;
   se1Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;
   se1Params.DIM2          = eleCount;
   se1Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se1Params.DIM3          = numChannels * 2;
   se1Params.ICNT3         = outputSampleCount;
   se1Params.DECDIM1       = __SE_DECDIM_DIM2;
   se1Params.DECDIM1_WIDTH = numChannels;
   se1Params.AM0           = __SE_AM_CIRC_CBK0;
   se1Params.AM1           = __SE_AM_CIRC_CBK0;
   se1Params.AM2           = __SE_AM_CIRC_CBK0;
   se1Params.AM3           = __SE_AM_CIRC_CBK0;
   se1Params.CBK0          = encCbSizeB;
   se1Params.ELETYPE       = SE_ELETYPE;
   se1Params.VECLEN        = SE_VECLEN;
   se1Params.DIMFMT        = __SE_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA to fetch the filter coefficients                        */
   /**********************************************************************/
   sa0Params        = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0  = 1;
   sa0Params.DIM1   = 1;
   sa0Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;
   sa0Params.DIM2   = 0;
   sa0Params.ICNT2  = pKerPrivArgs->numChannelBlocks;
   sa0Params.DIM3   = 0;
   sa0Params.ICNT3  = outputSampleCount;
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store output                                */
   /**********************************************************************/
   sa1Params               = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0         = icnt0Param;
   sa1Params.DIM1          = eleCount;
   sa1Params.ICNT1         = pKerPrivArgs->numChannelBlocks;
   sa1Params.DIM2          = numChannels;
   sa1Params.ICNT2         = outputSampleCount;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa1Params.DECDIM1_WIDTH = numChannels;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;

   /**********************************************************************/
   /* Prepare streaming engine 0 to read output of fir phase 1           */
   /**********************************************************************/

   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = numChannels;
   se2Params.DIM1    = numChannels;
   se2Params.ICNT1   = outputSampleCount;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to read input data from phase 2         */
   /**********************************************************************/

   se3Params         = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0   = numChannels;
   se3Params.DIM1    = numChannels * 2;
   se3Params.ICNT1   = outputSampleCount;
   se3Params.AM0     = __SE_AM_CIRC_CBK0;
   se3Params.AM1     = __SE_AM_CIRC_CBK0;
   se3Params.CBK0    = encCbSizeB;
   se3Params.ELETYPE = SE_ELETYPE;
   se3Params.VECLEN  = SE_VECLEN;
   se3Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare SA to store filtered samples                               */
   /**********************************************************************/
   sa2Params        = __gen_SA_TEMPLATE_v1();
   sa2Params.ICNT0  = numChannels;
   sa2Params.DIM1   = numChannels;
   sa2Params.ICNT1  = outputSampleCount;
   sa2Params.VECLEN = SA_VECLEN;
   sa2Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks2 = pKerPrivArgs->numChannelBlocks * outputSampleCount;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;

   return status;
}

/*******************************************************************************
 * EXECUTION FUNCTIONS
 ******************************************************************************/
/*
 * Execution for 2x downsampling using block processing (circular buffer).
 * Processes input in blocks using streaming engines and agents for FIR filtering and output decimation.
 * Handles circular buffer index updates.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_block_exec_ci(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pState,
                                                         void *restrict pFiltCoeffs,
                                                         void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_block_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status               = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock               = pKerPrivArgs->bufPblock;
   uint32_t               *stage1BuffStartIndex = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t                cirBuffAddressMask   = pKerPrivArgs->cirBuffAddressMask;
   dataType               *pInLocal             = (dataType *) pIn;
   dataType               *pOutLocal            = (dataType *) pOut;
   const dataType         *pFilterLocal         = (dataType *) pFiltCoeffs;
   int32_t                 numChannels          = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount     = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 numOutputBlocks1     = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2     = pKerPrivArgs->numOutputBlocks2;
   int32_t                 chCount, outputBlockCount, tapCount;

   // Use Block FIR direct interleaved mode with streaming engines
   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);

   __SE0_OPEN(&pInLocal[*stage1BuffStartIndex], se0Params);
   __SE1_OPEN(&pInLocal[(*stage1BuffStartIndex +
                         ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels)) &
                        cirBuffAddressMask],
              se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   // Process each channel block
   for (chCount = 0; chCount < numChannels; chCount += eleCount) {
      // Process each output block
      for (outputBlockCount = 0; outputBlockCount < numOutputBlocks1; outputBlockCount++) {
         // Initialize accumulators for filtered outputs
         vec vecAcc0 = (vec) 0;
         vec vecAcc1 = (vec) 0;
         vec vecAcc2 = (vec) 0;
         vec vecAcc3 = (vec) 0;

         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS);
              tapCount++) {
            // Duplicate each filter coefficient
            dataType *VB1Dup    = c7x::strm_agen<0, dataType>::get_adv(pFilterLocal);
            vec       vecCoeffs = __vload_dup(VB1Dup);

            // Get input samples from forward direction
            vec vecInput1_0 = c7x::strm_eng<0, vec>::get_adv();
            // Get input samples from reverse direction
            vec vecInput2_0 = c7x::strm_eng<1, vec>::get_adv();
            // Compute MAC with symmetric taps
            vecAcc0 += vecCoeffs * (vecInput1_0 + vecInput2_0);

            vec vecInput1_1 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInput2_1 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc1 += vecCoeffs * (vecInput1_1 + vecInput2_1);

            vec vecInput1_2 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInput2_2 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc2 += vecCoeffs * (vecInput1_2 + vecInput2_2);

            vec vecInput1_3 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInput2_3 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc3 += vecCoeffs * (vecInput1_3 + vecInput2_3);
         }

         __vpred opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc0);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc1);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc2);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc3);
      }
   }

   // Close streaming engines and address generators
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   /* Complete anti-aliasing filtering by adding phase 2 filtering */
   __SE0_OPEN(pOutLocal, se2Params);
   __SE1_OPEN(&pInLocal[(*stage1BuffStartIndex +
                         ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels)) &
                        cirBuffAddressMask],
              se3Params);
   __SA2_OPEN(sa2Params);

   for (outputBlockCount = 0; outputBlockCount < numOutputBlocks2; outputBlockCount++) {
      vec vecFirPhase1 = c7x::strm_eng<0, vec>::get_adv();
      vec vecPhase2    = c7x::strm_eng<1, vec>::get_adv();

      vec vecDownsampled = vecFirPhase1 + vecPhase2 * 0.5f;

      __vpred opWr    = c7x::strm_agen<2, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<2, vec>::get_adv(pOutLocal);
      __vstore_pred(opWr, opWrPtr, vecDownsampled);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA2_CLOSE();

   // Update circular buffer index for next call
   *stage1BuffStartIndex = (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

/*
 * Execution for 2x downsampling using single sample processing (circular buffer).
 * Processes each sample individually to achieve 2x downsampling.
 * Uses streaming engines and agents for FIR filtering and output decimation.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_ss_exec_ci(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pState,
                                                      void *restrict pFiltCoeffs,
                                                      void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_ss_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status               = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock               = pKerPrivArgs->bufPblock;
   uint32_t               *stage1BuffStartIndex = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t                cirBuffAddressMask   = pKerPrivArgs->cirBuffAddressMask;
   dataType               *pInLocal             = (dataType *) pIn;
   dataType               *pOutLocal            = (dataType *) pOut;
   const dataType         *pFilterLocal         = (dataType *) pFiltCoeffs;
   int32_t                 numChannels          = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount     = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 outputSampleCount    = pKerPrivArgs->outputSampleCount;
   int32_t                 numOutputBlocks2     = pKerPrivArgs->numOutputBlocks2;
   int32_t                 chCount, outputCount, tapCount;

   // Use single sample processing with streaming engines
   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);

   __SE0_OPEN(&pInLocal[*stage1BuffStartIndex], se0Params);
   __SE1_OPEN(&pInLocal[(*stage1BuffStartIndex +
                         ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels)) &
                        cirBuffAddressMask],
              se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   // Process each output sample
   for (outputCount = 0; outputCount < outputSampleCount; outputCount++) {
      // Process each channel block
      for (chCount = 0; chCount < numChannels; chCount += eleCount) {
         // Initialize accumulators
         vec vecAcc0 = (vec) 0;
         vec vecAcc1 = (vec) 0;
         vec vecAcc2 = (vec) 0;
         vec vecAcc3 = (vec) 0;

         // Process the FIR filter with unrolling
         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS /
                                                            AUDIOLIB_SSRC_SS_UNROLL_FACTOR);
              tapCount++) {
            /* ---- UR0 ---------------------------------------------------------- */
            /* Load next coefficient */
            vec vecCoeffs0 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterLocal));
            /* Load forward input sample */
            vec vecInputF0 = c7x::strm_eng<0, vec>::get_adv();
            /* Load reverse input sample */
            vec vecInputR0 = c7x::strm_eng<1, vec>::get_adv();
            /* MAC */
            vecAcc0 += vecCoeffs0 * (vecInputF0 + vecInputR0);

            /* ---- UR1 ---------------------------------------------------------- */
            vec vecCoeffs1 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterLocal));
            vec vecInputF1 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInputR1 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc1 += vecCoeffs1 * (vecInputF1 + vecInputR1);

            /* ---- UR2 ---------------------------------------------------------- */
            vec vecCoeffs2 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterLocal));
            vec vecInputF2 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInputR2 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc2 += vecCoeffs2 * (vecInputF2 + vecInputR2);

            /* ---- UR3 ---------------------------------------------------------- */
            vec vecCoeffs3 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterLocal));
            vec vecInputF3 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInputR3 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc3 += vecCoeffs3 * (vecInputF3 + vecInputR3);
         }

         // Combine partial sums
         vecAcc0 = vecAcc0 + vecAcc1;
         vecAcc2 = vecAcc2 + vecAcc3;
         vecAcc0 = vecAcc0 + vecAcc2;

         // Write output
         __vpred opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc0);
      }
   }

   // Close streaming engines and agents
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   /* Complete anti-aliasing filtering by adding phase 2 filtering */
   __SE0_OPEN(pOutLocal, se2Params);
   __SE1_OPEN(&pInLocal[(*stage1BuffStartIndex +
                         ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels)) &
                        cirBuffAddressMask],
              se3Params);
   __SA2_OPEN(sa2Params);

   for (outputCount = 0; outputCount < numOutputBlocks2; outputCount++) {
      vec vecFirPhase1 = c7x::strm_eng<0, vec>::get_adv();
      vec vecPhase2    = c7x::strm_eng<1, vec>::get_adv();

      vec vecDownsampled = vecFirPhase1 + vecPhase2 * 0.5f;

      __vpred opWr    = c7x::strm_agen<2, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<2, vec>::get_adv(pOutLocal);
      __vstore_pred(opWr, opWrPtr, vecDownsampled);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA2_CLOSE();

   // Update circular buffer index for next call
   *stage1BuffStartIndex = (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

/*
 * Execution for 2x downsampling with linear buffer format (interleaved data).
 * Performs deinterleave, decimation, FIR filtering, and interleaving.
 * Final output is re-interleaved.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pState,
                                                          void *restrict pFiltCoeffs,
                                                          void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_linear_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                               = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus __attribute__((unused)) = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs                         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                               = pKerPrivArgs->bufPblock;
   dataType               *pInLocal                             = (dataType *) pIn;
   dataType               *pOutLocal                            = (dataType *) pOut;
   dataType               *pStateLocal                          = (dataType *) pState;
   dataType               *pFilterLocal                         = (dataType *) pFiltCoeffs;
   int32_t                 inputSampleCount                     = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 numOutputBlocks1                     = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2                     = pKerPrivArgs->numOutputBlocks2;
   int32_t                 vecCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   /* Step 1: Transform interleaved input data to non-interleaved format using deinterleave kernel and store input
   samples to state buffer (after the filter history) */
   dsplibStatus = DSPLIB_deinterleave_exec(pKerPrivArgs->initArgs.deinterleaveHandle, pInLocal,
                                           &pStateLocal[pKerPrivArgs->stage1BuffCurrIndex]);

   /* Step 2: Decimate the non-interleaved input data for downsampling */
   __SE0_OPEN(pStateLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (vecCount = 0; vecCount < numOutputBlocks1; vecCount++) {
      vec vecDecimated = c7x::strm_eng<0, vec>::get_adv();

      __vpred opWr    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);
      __vstore_pred(opWr, opWrPtr, vecDecimated);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   /* Step 3: Apply anti-aliasing filter */
   // First, apply the phase 1 fir filter
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle1, &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex],
                                  pFilterLocal, &pStateLocal[pKerPrivArgs->scratch2BuffStartIndex]);

   /* Step 4: Complete anti-aliasing filtering by adding phase 2 filtering */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch2BuffStartIndex], se1Params);
   __SE1_OPEN(&pStateLocal[AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1], se2Params);
   __SA0_OPEN(sa1Params);

   for (vecCount = 0; vecCount < numOutputBlocks2; vecCount++) {
      vec vecFirPhase1 = c7x::strm_eng<0, vec>::get_adv();
      vec vecPhase2    = c7x::strm_eng<1, vec>::get_adv();

      vec vecDownsampled = vecFirPhase1 + vecPhase2 * 0.5f;

      __vpred opWr    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);
      __vstore_pred(opWr, opWrPtr, vecDownsampled);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   /* Step 5: Transform non-interleaved output data back to interleaved format using interleave kernel */
   dsplibStatus = DSPLIB_interleave_exec(pKerPrivArgs->initArgs.interleaveHandle,
                                         &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex], pOutLocal);

   /* Step 6: Copy the last filterLength samples to the beginning of the state buffer for next time
      This creates the history for the next processing block */
   dsplibStatus =
       DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle1, &pStateLocal[inputSampleCount], pStateLocal);

   return status;
}

/*
 * Execution for 2x downsampling with non-interleaved linear buffer format.
 * Performs block copy, decimation, FIR filtering, and writes output in non-interleaved format.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pState,
                                                                   void *restrict pFiltCoeffs,
                                                                   void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_non_interleaved_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                               = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus __attribute__((unused)) = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs                         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                               = pKerPrivArgs->bufPblock;
   dataType               *pInLocal                             = (dataType *) pIn;
   dataType               *pOutLocal                            = (dataType *) pOut;
   dataType               *pStateLocal                          = (dataType *) pState;
   dataType               *pFilterLocal                         = (dataType *) pFiltCoeffs;
   int32_t                 inputSampleCount                     = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 numOutputBlocks1                     = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2                     = pKerPrivArgs->numOutputBlocks2;
   int32_t                 vecCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   /* Step 1: Copy input data to state buffer (after the filter history) */
   dsplibStatus = DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle1, pInLocal,
                                        &pStateLocal[pKerPrivArgs->stage1BuffCurrIndex]);

   /* Step 2: Decimate the non-interleaved input data for downsampling */
   __SE0_OPEN(pStateLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (vecCount = 0; vecCount < numOutputBlocks1; vecCount++) {
      vec     vecDecimated = c7x::strm_eng<0, vec>::get_adv();
      __vpred opWr         = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr      = c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);
      __vstore_pred(opWr, opWrPtr, vecDecimated);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   /* Step 3: Apply anti-aliasing filter */
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle1, &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex],
                                  pFilterLocal, &pStateLocal[pKerPrivArgs->scratch2BuffStartIndex]);

   /* Step 4: Complete anti-aliasing filtering by adding phase 2 filtering */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch2BuffStartIndex], se1Params);
   __SE1_OPEN(&pStateLocal[AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1], se2Params);
   __SA0_OPEN(sa1Params);

   for (vecCount = 0; vecCount < numOutputBlocks2; vecCount++) {
      vec     vecFirPhase1   = c7x::strm_eng<0, vec>::get_adv();
      vec     vecPhase2      = c7x::strm_eng<1, vec>::get_adv();
      vec     vecDownsampled = vecFirPhase1 + vecPhase2 * 0.5f;
      __vpred opWr           = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr        = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(opWr, opWrPtr, vecDownsampled);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   /* Step 5: Copy the last filterLength samples to the beginning of the state buffer for next time */
   dsplibStatus =
       DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle2, &pStateLocal[inputSampleCount], pStateLocal);

   return status;
}

/*******************************************************************************
 * TEMPLATE INSTANTIATIONS
 ******************************************************************************/

// Initialization function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_linear_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_block_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                         AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                         AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                         AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_ss_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                      AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                      AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS
AUDIOLIB_ssrc_downsample2x_non_interleaved_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

// Execution function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_linear_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                          void *restrict pIn,
                                                                          void *restrict pState,
                                                                          void *restrict pFiltCoeffs,
                                                                          void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_block_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                         void *restrict pIn,
                                                                         void *restrict pState,
                                                                         void *restrict pFiltCoeffs,
                                                                         void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_ss_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                      void *restrict pIn,
                                                                      void *restrict pState,
                                                                      void *restrict pFiltCoeffs,
                                                                      void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_non_interleaved_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                   void *restrict pIn,
                                                                                   void *restrict pState,
                                                                                   void *restrict pFiltCoeffs,
                                                                                   void *restrict pOut);
