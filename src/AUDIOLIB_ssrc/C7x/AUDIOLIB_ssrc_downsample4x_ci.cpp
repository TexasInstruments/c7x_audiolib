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
 * Main initialization function for 4x downsampling using C7x intrinsics.
 * Selects the appropriate processing mode (linear, block, or single sample)
 * based on buffer format and input sample count for interleaved data format.
 * For non-interleaved data format, uses linear buffer based processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                 bufferFormat     = pKerPrivArgs->initArgs.bufferFormat;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   uint8_t                 dataFormat       = pKerPrivArgs->initArgs.dataFormat;

   // Check data format first
   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      // Check if we're using linear buffer format
      if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
         // For linear buffer format, use the specialized implementation
         status = AUDIOLIB_ssrc_downsample4x_linear_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (status == AUDIOLIB_SUCCESS) {
            pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample4x_linear_exec_ci<float>;
         }
      }
      else {
         // For circular buffer format, choose between block and single sample processing
         if (inputSampleCount > (int32_t) AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT) {
            // For larger sample counts, use block processing
            status = AUDIOLIB_ssrc_downsample4x_block_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample4x_block_exec_ci<float>;
            }
         }
         else {
            // For smaller sample counts, use single sample processing
            status = AUDIOLIB_ssrc_downsample4x_ss_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample4x_ss_exec_ci<float>;
            }
         }
      }
   }
   else {
      // For non-interleaved data format, use the specialized implementation
      status =
          AUDIOLIB_ssrc_downsample4x_non_interleaved_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      if (status == AUDIOLIB_SUCCESS) {
         pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample4x_non_interleaved_exec_ci<float>;
      }
   }

   return status;
}

/*******************************************************************************
 * SPECIALIZED INITIALIZATION FUNCTIONS
 ******************************************************************************/
/*
 * Initialization for 4x downsampling with linear buffer format (interleaved data).
 * Sets up DSPLIB kernels for deinterleave, FIR filtering, block copy, and interleave.
 * Configures streaming engine and agent parameters for both downsampling stages.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample2x_linear_init_ci\n");
#endif

   AUDIOLIB_STATUS         status                  = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus            = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs            = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                  = pKerPrivArgs->bufPblock;
   int32_t                 numChannels             = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount        = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 intermediateSampleCount = inputSampleCount / 2;
   int32_t                 outputSampleCount       = pKerPrivArgs->outputSampleCount;
   int32_t                 stateStride             = pKerPrivArgs->inBufferTotalDimX;
   int32_t                 eleSize                 = AUDIOLIB_sizeof(pKerInitArgs->sampleDataType);

   // Create buffer parameters for non-interleaved input and output
   DSPLIB_bufParams2D_t interleavedBufParams, nonInterleavedBufParams;
   DSPLIB_bufParams2D_t firBufParamsIn, firBufParamsOut, firBufParamsFilter;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se4Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se5Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa3Params; // =__gen_SA_TEMPLATE_v1();
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
   int32_t icnt0Param = inputSampleCount + AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS - 1;
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
   firKerInitArgs.filterSize     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
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
   firBufParamsOut.dim_x     = intermediateSampleCount;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR filter buffer parameters */
   firBufParamsFilter.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter.dim_x =
          AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = firBufParamsFilter.dim_x * eleSize;
   }
   else {
      firBufParamsFilter.dim_x    = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC * eleSize;
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
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS - 1;
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

   /* Prepare streaming engine 0 to read output of fir phase 1           */
   se1Params         = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0   = intermediateSampleCount;
   se1Params.DIM1    = stateStride;
   se1Params.ICNT1   = numChannels;
   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   se1Params.DIMFMT  = __SE_DIMFMT_2D;

   /* Prepare streaming engine 1 to read input data from phase 2         */
   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = inputSampleCount;
   se2Params.DIM1    = stateStride;
   se2Params.ICNT1   = numChannels;
   se2Params.DECIM   = __SE_DECIM_2;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_2D;

   /* Prepare SA to store filtered samples                               */
   blkItrCount      = AUDIOLIB_ceilingDiv(intermediateSampleCount, eleCount);
   sa1Params        = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0  = intermediateSampleCount;
   sa1Params.DIM1   = stateStride;
   sa1Params.ICNT1  = numChannels;
   sa1Params.VECLEN = SA_VECLEN;
   sa1Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks2 = blkItrCount * numChannels;

   /**********************************************************************/
   /* Stage 2                                                            */
   /**********************************************************************/

   /* Prepare streaming engine 0 to decimate input samples.              */
   icnt0Param        = intermediateSampleCount + AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
   se3Params         = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0   = icnt0Param;
   se3Params.DIM1    = stateStride;
   se3Params.ICNT1   = numChannels;
   se3Params.DECIM   = __SE_DECIM_2;
   se3Params.ELETYPE = SE_ELETYPE;
   se3Params.VECLEN  = SE_VECLEN;
   se3Params.DIMFMT  = __SE_DIMFMT_2D;

   /* Prepare SA to store decimated samples                              */
   blkItrCount      = AUDIOLIB_ceilingDiv(icnt0Param, eleCount * 2);
   sa2Params        = __gen_SA_TEMPLATE_v1();
   sa2Params.ICNT0  = icnt0Param / 2;
   sa2Params.DIM1   = stateStride;
   sa2Params.ICNT1  = numChannels;
   sa2Params.VECLEN = SA_VECLEN;
   sa2Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks3 = blkItrCount * numChannels;

   /* Initialize FIR kernel parameters for anti-aliasing filter */
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
      dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle2, &firBufParamsIn, &firBufParamsFilter,
                                                 &firBufParamsOut, &firKerInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle2, &firBufParamsIn, &firBufParamsFilter, &firBufParamsOut,
                                     &firKerInitArgs);
   }

   // Set up non-interleaved intermediate buffer parameters for 2d block copy
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   // Initialize DSPLIB blkCopy2D for copying data to history buffer
   if (dsplibStatus == DSPLIB_SUCCESS) {
      blkCopy2DInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

      dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle2, &nonInterleavedBufParams,
                                                       &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle2, &nonInterleavedBufParams,
                                           &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   /* Prepare streaming engine 0 to read output of fir phase 1           */
   se4Params         = __gen_SE_TEMPLATE_v1();
   se4Params.ICNT0   = outputSampleCount;
   se4Params.DIM1    = stateStride;
   se4Params.ICNT1   = numChannels;
   se4Params.ELETYPE = SE_ELETYPE;
   se4Params.VECLEN  = SE_VECLEN;
   se4Params.DIMFMT  = __SE_DIMFMT_2D;

   /* Prepare streaming engine 1 to read input data from phase 2         */
   se5Params         = __gen_SE_TEMPLATE_v1();
   se5Params.ICNT0   = intermediateSampleCount;
   se5Params.DIM1    = stateStride;
   se5Params.ICNT1   = numChannels;
   se5Params.DECIM   = __SE_DECIM_2;
   se5Params.ELETYPE = SE_ELETYPE;
   se5Params.VECLEN  = SE_VECLEN;
   se5Params.DIMFMT  = __SE_DIMFMT_2D;

   /* Prepare SA to store filtered samples                               */
   blkItrCount      = AUDIOLIB_ceilingDiv(outputSampleCount, eleCount);
   sa3Params        = __gen_SA_TEMPLATE_v1();
   sa3Params.ICNT0  = outputSampleCount;
   sa3Params.DIM1   = stateStride;
   sa3Params.ICNT1  = numChannels;
   sa3Params.VECLEN = SA_VECLEN;
   sa3Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks4 = blkItrCount * numChannels;

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

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = se4Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET) = se5Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET) = sa3Params;

   if (dsplibStatus != DSPLIB_SUCCESS) {
      status = AUDIOLIB_ERR_FAILURE;
   }

   return status;
}

/*
 * Initialization for 4x downsampling using block processing (circular buffer).
 * Used when inputSampleCount exceeds threshold. Sets up streaming engines and agents
 * for multi-channel block FIR filtering and output decimation.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_block_init_ci(AUDIOLIB_kernelHandle   handle,
                                                         AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                         AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                         AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_block_init_ci\n");
#endif

   AUDIOLIB_STATUS         status                  = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs            = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                  = pKerPrivArgs->bufPblock;
   int32_t                 numChannels             = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount        = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 intermediateSampleCount = inputSampleCount / 2;
   int32_t                 outputSampleCount       = pKerPrivArgs->outputSampleCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se4Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se5Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se6Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa3Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa4Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa5Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   /* In this C7x implementation we'll use the 2nd stage buffer as a linear buffer due to SA not having circular
    * addressing functionality. This deviates from the natural C implementation for 4x downsampling */
   pKerPrivArgs->stage2BuffStartIndex = 0;
   pKerPrivArgs->stage2BuffCurrIndex  = (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels;

   pKerPrivArgs->numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   int32_t icnt0Param             = AUDIOLIB_min(numChannels, eleCount);

   // Stage 1: 2x downsampling
   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch input samples with decimation  */
   /**********************************************************************/
   // Calculate the number of output blocks needed for stage 1
   pKerPrivArgs->numOutputBlocks1 =
       AUDIOLIB_ceilingDiv(intermediateSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   se0Params               = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0         = icnt0Param;
   se0Params.DIM1          = numChannels * 2; // Decimate by 2 for downsampling
   se0Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se0Params.DIM2          = numChannels * 2;
   se0Params.ICNT2         = (int32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS);
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
   se1Params.ICNT2         = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS;
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
   /* Prepare SA to fetch the filter coefficients for stage 1            */
   /**********************************************************************/
   sa0Params        = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0  = 1;
   sa0Params.DIM1   = 1;
   sa0Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS;
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
   sa1Params.DECDIM1_WIDTH = numChannels * intermediateSampleCount;
   sa1Params.DECDIM2       = __SA_DECDIM_DIM2;
   sa1Params.DECDIM2_WIDTH = numChannels;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;

   /**********************************************************************/
   /* Prepare streaming engine 0 to read output of fir phase 1           */
   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = numChannels;
   se2Params.DIM1    = numChannels;
   se2Params.ICNT1   = intermediateSampleCount;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to read input data from phase 2         */
   /**********************************************************************/
   se3Params         = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0   = numChannels;
   se3Params.DIM1    = numChannels * 2;
   se3Params.ICNT1   = intermediateSampleCount;
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
   sa2Params.ICNT1  = intermediateSampleCount;
   sa2Params.VECLEN = SA_VECLEN;
   sa2Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks2 = pKerPrivArgs->numChannelBlocks * intermediateSampleCount;

   // Stage 2: 2x downsampling on the output of stage 1

   /**********************************************************************/
   /* Prepare streaming engine 2 to fetch input samples with decimation   */
   /**********************************************************************/
   // Calculate the number of output blocks needed for stage 1
   pKerPrivArgs->numOutputBlocks3 = AUDIOLIB_ceilingDiv(outputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   se4Params               = __gen_SE_TEMPLATE_v1();
   se4Params.ICNT0         = icnt0Param;
   se4Params.DIM1          = numChannels * 2; // Decimate by 2 for downsampling
   se4Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se4Params.DIM2          = numChannels * 2;
   se4Params.ICNT2         = (int32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS);
   se4Params.DIM3          = numChannels * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK * 2;
   se4Params.ICNT3         = pKerPrivArgs->numOutputBlocks3;
   se4Params.DIM4          = eleCount;
   se4Params.ICNT4         = pKerPrivArgs->numChannelBlocks;
   se4Params.DECDIM1       = __SE_DECDIM_DIM3;
   se4Params.DECDIM1SD     = __SE_DECDIMSD_DIM1;
   se4Params.DECDIM1_WIDTH = intermediateSampleCount * numChannels;
   se4Params.DECDIM2       = __SE_DECDIM_DIM4;
   se4Params.DECDIM2_WIDTH = numChannels;
   se4Params.ELETYPE       = SE_ELETYPE;
   se4Params.VECLEN        = SE_VECLEN;
   se4Params.DIMFMT        = __SE_DIMFMT_5D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to fetch reverse samples.               */
   /**********************************************************************/
   se5Params               = __gen_SE_TEMPLATE_v1();
   se5Params.ICNT0         = icnt0Param;
   se5Params.DIM1          = numChannels * 2;
   se5Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se5Params.DIM2          = -numChannels * 2;
   se5Params.ICNT2         = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;
   se5Params.DIM3          = numChannels * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK * 2;
   se5Params.ICNT3         = pKerPrivArgs->numOutputBlocks3;
   se5Params.DIM4          = eleCount;
   se5Params.ICNT4         = pKerPrivArgs->numChannelBlocks;
   se5Params.DECDIM1       = __SE_DECDIM_DIM3;
   se5Params.DECDIM1SD     = __SE_DECDIMSD_DIM1;
   se5Params.DECDIM1_WIDTH = intermediateSampleCount * numChannels;
   se5Params.DECDIM2       = __SE_DECDIM_DIM4;
   se5Params.DECDIM2_WIDTH = numChannels;
   se5Params.ELETYPE       = SE_ELETYPE;
   se5Params.VECLEN        = SE_VECLEN;
   se5Params.DIMFMT        = __SE_DIMFMT_5D;

   /**********************************************************************/
   /* Prepare SA to fetch the filter coefficients for stage 2            */
   /**********************************************************************/
   sa3Params        = __gen_SA_TEMPLATE_v1();
   sa3Params.ICNT0  = 1;
   sa3Params.DIM1   = 1;
   sa3Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;
   sa3Params.DIM2   = 0;
   sa3Params.ICNT2  = pKerPrivArgs->numOutputBlocks3;
   sa3Params.DIM3   = 0;
   sa3Params.ICNT3  = pKerPrivArgs->numChannelBlocks;
   sa3Params.VECLEN = SA_VECLEN;
   sa3Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store stage 2 output (final output)         */
   /**********************************************************************/
   sa4Params               = __gen_SA_TEMPLATE_v1();
   sa4Params.ICNT0         = icnt0Param;
   sa4Params.DIM1          = numChannels;
   sa4Params.ICNT1         = pKerPrivArgs->numOutputBlocks3 * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   sa4Params.DIM2          = eleCount;
   sa4Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   sa4Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa4Params.DECDIM1_WIDTH = numChannels * outputSampleCount;
   sa4Params.DECDIM2       = __SA_DECDIM_DIM2;
   sa4Params.DECDIM2_WIDTH = numChannels;
   sa4Params.VECLEN        = SA_VECLEN;
   sa4Params.DIMFMT        = __SA_DIMFMT_3D;

   /**********************************************************************/
   /* Prepare streaming engine 0 to read output of fir phase 1 in stage 2*/
   /**********************************************************************/

   /*************************************************************************/
   /* Prepare streaming engine 1 to read input data from phase 2 in stage 2 */
   /*************************************************************************/

   /**********************************************************************/
   /* Prepare SA to store filtered samples in stage 2                    */
   /**********************************************************************/

   // reusing se2Params, se3Params, and sa2Params for phase integration

   pKerPrivArgs->numOutputBlocks4 = pKerPrivArgs->numChannelBlocks * outputSampleCount;

   /**********************************************************************/
   /* Prepare SE template for stage 2 block copy input                   */
   /**********************************************************************/
   se6Params         = __gen_SE_TEMPLATE_v1();
   se6Params.ICNT0   = (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels;
   se6Params.ELETYPE = SE_ELETYPE;
   se6Params.VECLEN  = SE_VECLEN;
   se6Params.DIMFMT  = __SE_DIMFMT_1D;

   /**********************************************************************/
   /* Prepare SA template for block copy output                          */
   /**********************************************************************/
   sa5Params        = __gen_SA_TEMPLATE_v1();
   sa5Params.ICNT0  = (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels;
   sa5Params.VECLEN = SA_VECLEN;
   sa5Params.DIMFMT = __SA_DIMFMT_1D;

   pKerPrivArgs->numOutputBlocks5 =
       AUDIOLIB_ceilingDiv((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels, eleCount);

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = se4Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET) = se5Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE6_PARAM_OFFSET) = se6Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET) = sa3Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA4_PARAM_OFFSET) = sa4Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA5_PARAM_OFFSET) = sa5Params;

   return status;
}

/*
 * Initialization for 4x downsampling using single sample processing (circular buffer).
 * Used when inputSampleCount is below threshold. Sets up streaming engines and agents
 * for per-sample FIR filtering and output decimation.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_ss_init_ci(AUDIOLIB_kernelHandle   handle,
                                                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                      AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                      AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_ss_init_ci\n");
#endif

   AUDIOLIB_STATUS         status                  = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs            = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                  = pKerPrivArgs->bufPblock;
   int32_t                 numChannels             = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount        = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 intermediateSampleCount = inputSampleCount / 2;
   int32_t                 outputSampleCount       = pKerPrivArgs->outputSampleCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se4Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se5Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se6Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa3Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa4Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   /* In this C7x implementation we'll use the 2nd stage buffer as a linear buffer due to SA not having circular
    * addressing functionality. This deviates from the natural C implementation for 4x downsampling */
   pKerPrivArgs->stage2BuffStartIndex = 0;
   pKerPrivArgs->stage2BuffCurrIndex  = (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels;

   pKerPrivArgs->numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   int32_t icnt0Param             = AUDIOLIB_min(numChannels, eleCount);

   // Stage 1: 2x downsampling
   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch input samples with decimation  */
   /**********************************************************************/
   pKerPrivArgs->numOutputBlocks1 = intermediateSampleCount * pKerPrivArgs->numChannelBlocks;

   se0Params               = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0         = icnt0Param;
   se0Params.DIM1          = numChannels * 2;
   se0Params.ICNT1         = (int32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS);
   se0Params.DIM2          = eleCount;
   se0Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se0Params.DIM3          = numChannels * 2;
   se0Params.ICNT3         = intermediateSampleCount;
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
   se1Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS;
   se1Params.DIM2          = eleCount;
   se1Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se1Params.DIM3          = numChannels * 2;
   se1Params.ICNT3         = intermediateSampleCount;
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
   /* Prepare SA to fetch the filter coefficients for stage 1            */
   /**********************************************************************/
   sa0Params        = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0  = 1;
   sa0Params.DIM1   = 1;
   sa0Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS;
   sa0Params.DIM2   = 0;
   sa0Params.ICNT2  = pKerPrivArgs->numChannelBlocks;
   sa0Params.DIM3   = 0;
   sa0Params.ICNT3  = intermediateSampleCount;
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store stage 1 output                        */
   /**********************************************************************/
   sa1Params               = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0         = icnt0Param;
   sa1Params.DIM1          = eleCount;
   sa1Params.ICNT1         = pKerPrivArgs->numChannelBlocks;
   sa1Params.DIM2          = numChannels;
   sa1Params.ICNT2         = intermediateSampleCount;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa1Params.DECDIM1_WIDTH = numChannels;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;

   /**********************************************************************/
   /* Prepare streaming engine 0 to read output of fir phase 1           */
   se2Params         = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0   = numChannels;
   se2Params.DIM1    = numChannels;
   se2Params.ICNT1   = intermediateSampleCount;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se2Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to read input data from phase 2         */
   /**********************************************************************/
   se3Params         = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0   = numChannels;
   se3Params.DIM1    = numChannels * 2;
   se3Params.ICNT1   = intermediateSampleCount;
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
   sa2Params.ICNT1  = intermediateSampleCount;
   sa2Params.VECLEN = SA_VECLEN;
   sa2Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->numOutputBlocks2 = pKerPrivArgs->numChannelBlocks * intermediateSampleCount;

   // Stage 2: 2x downsampling on the output of stage 1
   /**********************************************************************/
   /* Prepare streaming engine 2 to fetch input samples with decimation   */
   /**********************************************************************/
   se4Params               = __gen_SE_TEMPLATE_v1();
   se4Params.ICNT0         = icnt0Param;
   se4Params.DIM1          = numChannels * 2;
   se4Params.ICNT1         = (int32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS);
   se4Params.DIM2          = eleCount;
   se4Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se4Params.DIM3          = numChannels * 2;
   se4Params.ICNT3         = outputSampleCount;
   se4Params.DECDIM1       = __SE_DECDIM_DIM2;
   se4Params.DECDIM1_WIDTH = numChannels;
   se4Params.ELETYPE       = SE_ELETYPE;
   se4Params.VECLEN        = SE_VECLEN;
   se4Params.DIMFMT        = __SE_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare streaming engine 3 to fetch reverse samples for stage 2    */
   /**********************************************************************/
   se5Params               = __gen_SE_TEMPLATE_v1();
   se5Params.ICNT0         = icnt0Param;
   se5Params.DIM1          = -numChannels * 2;
   se5Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;
   se5Params.DIM2          = eleCount;
   se5Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se5Params.DIM3          = numChannels * 2;
   se5Params.ICNT3         = outputSampleCount;
   se5Params.DECDIM1       = __SE_DECDIM_DIM2;
   se5Params.DECDIM1_WIDTH = numChannels;
   se5Params.ELETYPE       = SE_ELETYPE;
   se5Params.VECLEN        = SE_VECLEN;
   se5Params.DIMFMT        = __SE_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA to fetch the filter coefficients for stage 2            */
   /**********************************************************************/
   sa3Params        = __gen_SA_TEMPLATE_v1();
   sa3Params.ICNT0  = 1;
   sa3Params.DIM1   = 1;
   sa3Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;
   sa3Params.DIM2   = 0;
   sa3Params.ICNT2  = pKerPrivArgs->numChannelBlocks;
   sa3Params.DIM3   = 0;
   sa3Params.ICNT3  = outputSampleCount;
   sa3Params.VECLEN = SA_VECLEN;
   sa3Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store stage 2 output (final output)         */
   /**********************************************************************/

   // reusing sa1Params to store stage 2 output

   /**********************************************************************/
   /* Prepare streaming engine 0 to read output of fir phase 1 in stage 2*/
   /**********************************************************************/

   /*************************************************************************/
   /* Prepare streaming engine 1 to read input data from phase 2 in stage 2 */
   /*************************************************************************/

   /**********************************************************************/
   /* Prepare SA to store filtered samples in stage 2                    */
   /**********************************************************************/

   // reusing se2Params, se3Params, and sa2Params for phase integration

   pKerPrivArgs->numOutputBlocks3 = pKerPrivArgs->numChannelBlocks * outputSampleCount;

   /**********************************************************************/
   /* Prepare SE template for stage 2 block copy input                   */
   /**********************************************************************/
   se6Params         = __gen_SE_TEMPLATE_v1();
   se6Params.ICNT0   = (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels;
   se6Params.ELETYPE = SE_ELETYPE;
   se6Params.VECLEN  = SE_VECLEN;
   se6Params.DIMFMT  = __SE_DIMFMT_1D;

   /**********************************************************************/
   /* Prepare SA template for block copy output                          */
   /**********************************************************************/
   sa4Params        = __gen_SA_TEMPLATE_v1();
   sa4Params.ICNT0  = (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels;
   sa4Params.VECLEN = SA_VECLEN;
   sa4Params.DIMFMT = __SA_DIMFMT_1D;

   pKerPrivArgs->numOutputBlocks4 =
       AUDIOLIB_ceilingDiv((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels, eleCount);

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = se4Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET) = se5Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE6_PARAM_OFFSET) = se6Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET) = sa3Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA4_PARAM_OFFSET) = sa4Params;

   return status;
}

/*
 * Initialization for 4x downsampling with non-interleaved data format.
 * Sets up DSPLIB kernels for block copy and FIR filtering for both stages.
 * Configures streaming engine and agent parameters for downsampling.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_non_interleaved_init_ci\n");
#endif

   AUDIOLIB_STATUS         status                  = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus            = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs            = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                  = pKerPrivArgs->bufPblock;
   int32_t                 numChannels             = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount        = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 intermediateSampleCount = inputSampleCount / 2;
   int32_t                 outputSampleCount       = pKerPrivArgs->outputSampleCount;
   int32_t                 stateStride             = pKerPrivArgs->inBufferTotalDimX;
   int32_t                 eleSize                 = AUDIOLIB_sizeof(pKerInitArgs->sampleDataType);

   typedef typename c7x::make_full_vector<dataType>::type vec;
   __SE_TEMPLATE_v1 se0Params, se1Params, se2Params, se3Params, se4Params, se5Params;
   __SA_TEMPLATE_v1 sa0Params, sa1Params, sa2Params, sa3Params;
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

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

   // --- Stage 1 ---
   int32_t icnt0Param1 = inputSampleCount + AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS - 1;
   se0Params           = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0     = icnt0Param1;
   se0Params.DIM1      = stateStride;
   se0Params.ICNT1     = numChannels;
   se0Params.DECIM     = __SE_DECIM_2;
   se0Params.ELETYPE   = SE_ELETYPE;
   se0Params.VECLEN    = SE_VECLEN;
   se0Params.DIMFMT    = __SE_DIMFMT_2D;

   int32_t blkItrCount1           = AUDIOLIB_ceilingDiv(icnt0Param1, eleCount * 2);
   sa0Params                      = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0                = icnt0Param1 / 2;
   sa0Params.DIM1                 = stateStride;
   sa0Params.ICNT1                = numChannels;
   sa0Params.VECLEN               = SA_VECLEN;
   sa0Params.DIMFMT               = __SA_DIMFMT_2D;
   pKerPrivArgs->numOutputBlocks1 = blkItrCount1 * numChannels;

   DSPLIB_fir_InitArgs firKerInitArgs1, firKerInitArgs2;
   firKerInitArgs1.funcStyle      = DSPLIB_FUNCTION_OPTIMIZED;
   firKerInitArgs1.dataSize       = icnt0Param1 / 2;
   firKerInitArgs1.batchSize      = numChannels;
   firKerInitArgs1.filterSize     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
   firKerInitArgs1.shift          = 0;
   firKerInitArgs1.enableNchCoefs = 0;
   firKerInitArgs1.enableMMA      = pKerInitArgs->enableMMA;
   firKerInitArgs1.MMA_SIZE       = pKerInitArgs->mmaSize;
   firKerInitArgs1.enableQ        = 1;
   firKerInitArgs1.Q              = 23;

   DSPLIB_bufParams2D_t firBufParamsIn1, firBufParamsOut1, firBufParamsFilter1;
   firBufParamsIn1.data_type = DSPLIB_FLOAT32;
   firBufParamsIn1.dim_x     = icnt0Param1 / 2;
   firBufParamsIn1.dim_y     = numChannels;
   firBufParamsIn1.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   firBufParamsOut1.data_type = DSPLIB_FLOAT32;
   firBufParamsOut1.dim_x     = intermediateSampleCount;
   firBufParamsOut1.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut1.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   firBufParamsFilter1.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter1.dim_x =
          AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter1.dim_y    = 1;
      firBufParamsFilter1.stride_y = firBufParamsFilter1.dim_x * eleSize;
   }
   else {
      firBufParamsFilter1.dim_x    = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter1.dim_y    = 1;
      firBufParamsFilter1.stride_y = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC * eleSize;
   }
   dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle1, &firBufParamsIn1, &firBufParamsFilter1,
                                              &firBufParamsOut1, &firKerInitArgs1);
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle1, &firBufParamsIn1, &firBufParamsFilter1,
                                     &firBufParamsOut1, &firKerInitArgs1);
   }

   se1Params         = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0   = intermediateSampleCount;
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

   int32_t blkItrCount2           = AUDIOLIB_ceilingDiv(intermediateSampleCount, eleCount);
   sa1Params                      = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0                = intermediateSampleCount;
   sa1Params.DIM1                 = stateStride;
   sa1Params.ICNT1                = numChannels;
   sa1Params.VECLEN               = SA_VECLEN;
   sa1Params.DIMFMT               = __SA_DIMFMT_2D;
   pKerPrivArgs->numOutputBlocks2 = blkItrCount2 * numChannels;

   // --- Stage 2 ---
   int32_t icnt0Param2 = intermediateSampleCount + AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
   se3Params           = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0     = icnt0Param2;
   se3Params.DIM1      = stateStride;
   se3Params.ICNT1     = numChannels;
   se3Params.DECIM     = __SE_DECIM_2;
   se3Params.ELETYPE   = SE_ELETYPE;
   se3Params.VECLEN    = SE_VECLEN;
   se3Params.DIMFMT    = __SE_DIMFMT_2D;

   int32_t blkItrCount3           = AUDIOLIB_ceilingDiv(icnt0Param2, eleCount * 2);
   sa2Params                      = __gen_SA_TEMPLATE_v1();
   sa2Params.ICNT0                = icnt0Param2 / 2;
   sa2Params.DIM1                 = stateStride;
   sa2Params.ICNT1                = numChannels;
   sa2Params.VECLEN               = SA_VECLEN;
   sa2Params.DIMFMT               = __SA_DIMFMT_2D;
   pKerPrivArgs->numOutputBlocks3 = blkItrCount3 * numChannels;

   firKerInitArgs2.funcStyle      = DSPLIB_FUNCTION_OPTIMIZED;
   firKerInitArgs2.dataSize       = icnt0Param2 / 2;
   firKerInitArgs2.batchSize      = numChannels;
   firKerInitArgs2.filterSize     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
   firKerInitArgs2.shift          = 0;
   firKerInitArgs2.enableNchCoefs = 0;
   firKerInitArgs2.enableMMA      = pKerInitArgs->enableMMA;
   firKerInitArgs2.MMA_SIZE       = pKerInitArgs->mmaSize;
   firKerInitArgs2.enableQ        = 1;
   firKerInitArgs2.Q              = 23;

   DSPLIB_bufParams2D_t firBufParamsIn2, firBufParamsOut2, firBufParamsFilter2;
   firBufParamsIn2.data_type = DSPLIB_FLOAT32;
   firBufParamsIn2.dim_x     = icnt0Param2 / 2;
   firBufParamsIn2.dim_y     = numChannels;
   firBufParamsIn2.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   firBufParamsOut2.data_type = DSPLIB_FLOAT32;
   firBufParamsOut2.dim_x     = outputSampleCount;
   firBufParamsOut2.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut2.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   firBufParamsFilter2.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter2.dim_x =
          AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter2.dim_y    = 1;
      firBufParamsFilter2.stride_y = firBufParamsFilter2.dim_x * eleSize;
   }
   else {
      firBufParamsFilter2.dim_x    = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter2.dim_y    = 1;
      firBufParamsFilter2.stride_y = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC * eleSize;
   }
   dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle2, &firBufParamsIn2, &firBufParamsFilter2,
                                              &firBufParamsOut2, &firKerInitArgs2);
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle2, &firBufParamsIn2, &firBufParamsFilter2,
                                     &firBufParamsOut2, &firKerInitArgs2);
   }

   se4Params         = __gen_SE_TEMPLATE_v1();
   se4Params.ICNT0   = outputSampleCount;
   se4Params.DIM1    = stateStride;
   se4Params.ICNT1   = numChannels;
   se4Params.ELETYPE = SE_ELETYPE;
   se4Params.VECLEN  = SE_VECLEN;
   se4Params.DIMFMT  = __SE_DIMFMT_2D;

   se5Params         = __gen_SE_TEMPLATE_v1();
   se5Params.ICNT0   = intermediateSampleCount;
   se5Params.DIM1    = stateStride;
   se5Params.ICNT1   = numChannels;
   se5Params.DECIM   = __SE_DECIM_2;
   se5Params.ELETYPE = SE_ELETYPE;
   se5Params.VECLEN  = SE_VECLEN;
   se5Params.DIMFMT  = __SE_DIMFMT_2D;

   int32_t blkItrCount4           = AUDIOLIB_ceilingDiv(outputSampleCount, eleCount);
   sa3Params                      = __gen_SA_TEMPLATE_v1();
   sa3Params.ICNT0                = outputSampleCount;
   sa3Params.DIM1                 = bufParamsOut->stride_y / eleSize;
   sa3Params.ICNT1                = numChannels;
   sa3Params.VECLEN               = SA_VECLEN;
   sa3Params.DIMFMT               = __SA_DIMFMT_2D;
   pKerPrivArgs->numOutputBlocks4 = blkItrCount4 * numChannels;

   // 3. Set up DSPLIB blkCopy2D handles for stage 1 and stage 2 history
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS - 1;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   blkCopy2DInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;
   dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle2, &nonInterleavedBufParams,
                                                    &nonInterleavedBufParams, &blkCopy2DInitArgs);
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle2, &nonInterleavedBufParams,
                                           &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle3, &nonInterleavedBufParams,
                                                    &nonInterleavedBufParams, &blkCopy2DInitArgs);
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle3, &nonInterleavedBufParams,
                                           &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = se4Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET) = se5Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET) = sa3Params;

   if (dsplibStatus != DSPLIB_SUCCESS) {
      status = AUDIOLIB_ERR_FAILURE;
   }

   return status;
}

/*******************************************************************************
 * EXECUTION FUNCTIONS
 ******************************************************************************/
/*
 * Execution for 4x downsampling with linear buffer format (interleaved data).
 * Performs deinterleave, decimation, FIR filtering, and interleaving for both stages.
 * Final output is re-interleaved.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
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
   dataType               *pFilterStage1           = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType               *pFilterStage2           = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 inputSampleCount        = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 intermediateSampleCount = inputSampleCount / 2;
   int32_t                 numOutputBlocks1        = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2        = pKerPrivArgs->numOutputBlocks2;
   int32_t                 numOutputBlocks3        = pKerPrivArgs->numOutputBlocks3;
   int32_t                 numOutputBlocks4        = pKerPrivArgs->numOutputBlocks4;
   int32_t                 vecCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se4Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se5Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa3Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET);

   /* Step 1: Transform interleaved input data to non-interleaved format using deinterleave kernel and store input
   samples to state buffer (after the filter history) */
   dsplibStatus = DSPLIB_deinterleave_exec(pKerPrivArgs->initArgs.deinterleaveHandle, pInLocal,
                                           &pStateLocal[pKerPrivArgs->stage1BuffCurrIndex]);

   /* Step 2: Decimate the non-interleaved input data for stage 1 downsampling */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->stage1BuffStartIndex], se0Params);
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
                                  pFilterStage1, &pStateLocal[pKerPrivArgs->scratch2BuffStartIndex]);

   /* Step 4: Complete anti-aliasing filtering by adding phase 2 filtering */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch2BuffStartIndex], se1Params);
   __SE1_OPEN(&pStateLocal[AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1], se2Params);
   __SA0_OPEN(sa1Params);

   for (vecCount = 0; vecCount < numOutputBlocks2; vecCount++) {
      vec vecFirPhase1 = c7x::strm_eng<0, vec>::get_adv();
      vec vecPhase2    = c7x::strm_eng<1, vec>::get_adv();

      vec vecDownsampled = vecFirPhase1 + vecPhase2 * 0.5f;

      __vpred opWr    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->stage2BuffCurrIndex]);
      __vstore_pred(opWr, opWrPtr, vecDownsampled);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   /* Step 5: Copy the last filterLength samples to the beginning of the state buffer for next time
      This creates the history for the next processing block for stage 1 filter */
   dsplibStatus =
       DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle1, &pStateLocal[inputSampleCount], pStateLocal);

   /* Step 6: Decimate the stage 2 input data for stage 2 downsampling */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->stage2BuffStartIndex], se3Params);
   __SA0_OPEN(sa2Params);

   for (vecCount = 0; vecCount < numOutputBlocks3; vecCount++) {
      vec vecDecimated = c7x::strm_eng<0, vec>::get_adv();

      __vpred opWr    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);
      __vstore_pred(opWr, opWrPtr, vecDecimated);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   /* Step 7: Apply anti-aliasing filter for stage 2 */
   // First, apply the phase 1 fir filter
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle2, &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex],
                                  pFilterStage2, &pStateLocal[pKerPrivArgs->scratch2BuffStartIndex]);

   /* Step 8: Complete anti-aliasing filtering by adding phase 2 filtering */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch2BuffStartIndex], se4Params);
   __SE1_OPEN(
       &pStateLocal[pKerPrivArgs->stage2BuffStartIndex + AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1],
       se5Params);
   __SA0_OPEN(sa3Params);

   for (vecCount = 0; vecCount < numOutputBlocks4; vecCount++) {
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

   /* Step 9: Transform non-interleaved output data back to interleaved format using interleave kernel */
   dsplibStatus = DSPLIB_interleave_exec(pKerPrivArgs->initArgs.interleaveHandle,
                                         &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex], pOutLocal);

   /* Step 10: Copy the last filterLength samples to the beginning of the state buffer for next time
      This creates the history for the next processing block for stage 2 filter */
   dsplibStatus = DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle2,
                                        &pStateLocal[pKerPrivArgs->stage2BuffStartIndex + intermediateSampleCount],
                                        &pStateLocal[pKerPrivArgs->stage2BuffStartIndex]);

   return status;
}

/*
 * Execution for 4x downsampling with non-interleaved data format.
 * Performs block copy, decimation, FIR filtering, and interleaving for both stages.
 * Output is written in non-interleaved format.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pState,
                                                                   void *restrict pFiltCoeffs,
                                                                   void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_non_interleaved_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                               = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus __attribute__((unused)) = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs                         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                               = pKerPrivArgs->bufPblock;
   dataType               *pInLocal                             = (dataType *) pIn;
   dataType               *pOutLocal                            = (dataType *) pOut;
   dataType               *pStateLocal                          = (dataType *) pState;
   dataType               *pFilterStage1           = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType               *pFilterStage2           = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 inputSampleCount        = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 intermediateSampleCount = inputSampleCount / 2;
   int32_t                 numOutputBlocks1        = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2        = pKerPrivArgs->numOutputBlocks2;
   int32_t                 numOutputBlocks3        = pKerPrivArgs->numOutputBlocks3;
   int32_t                 numOutputBlocks4        = pKerPrivArgs->numOutputBlocks4;
   int32_t                 vecCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se4Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se5Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa3Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET);

   // --- Stage 1 ---
   // Step 1: Copy input data to state buffer (after filter history)
   dsplibStatus = DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle1, pInLocal,
                                        &pStateLocal[pKerPrivArgs->stage1BuffCurrIndex]);

   // Step 2: Decimate by 2
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->stage1BuffStartIndex], se0Params);
   __SA0_OPEN(sa0Params);
   for (vecCount = 0; vecCount < numOutputBlocks1; vecCount++) {
      vec     vecDecimated = c7x::strm_eng<0, vec>::get_adv();
      __vpred opWr         = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr      = c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);
      __vstore_pred(opWr, opWrPtr, vecDecimated);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   // Step 3: FIR filter
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle1, &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex],
                                  pFilterStage1, &pStateLocal[pKerPrivArgs->scratch2BuffStartIndex]);

   // Step 4: Phase 2 filtering
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch2BuffStartIndex], se1Params);
   __SE1_OPEN(&pStateLocal[AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1], se2Params);
   __SA0_OPEN(sa1Params);
   for (vecCount = 0; vecCount < numOutputBlocks2; vecCount++) {
      vec     vecFirPhase1   = c7x::strm_eng<0, vec>::get_adv();
      vec     vecPhase2      = c7x::strm_eng<1, vec>::get_adv();
      vec     vecDownsampled = vecFirPhase1 + vecPhase2 * 0.5f;
      __vpred opWr           = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr        = c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->stage2BuffCurrIndex]);
      __vstore_pred(opWr, opWrPtr, vecDownsampled);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   // Step 5: Copy last filterLength samples to beginning of state buffer for next time (stage 1)
   dsplibStatus =
       DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle2, &pStateLocal[inputSampleCount], pStateLocal);

   // --- Stage 2 ---
   // Step 6: Decimate by 2
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->stage2BuffStartIndex], se3Params);
   __SA0_OPEN(sa2Params);
   for (vecCount = 0; vecCount < numOutputBlocks3; vecCount++) {
      vec     vecDecimated = c7x::strm_eng<0, vec>::get_adv();
      __vpred opWr         = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr      = c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);
      __vstore_pred(opWr, opWrPtr, vecDecimated);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   // Step 7: FIR filter
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle2, &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex],
                                  pFilterStage2, &pStateLocal[pKerPrivArgs->scratch2BuffStartIndex]);

   // Step 8: Phase 2 filtering (final output)
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch2BuffStartIndex], se4Params);
   __SE1_OPEN(
       &pStateLocal[pKerPrivArgs->stage2BuffStartIndex + AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1],
       se5Params);
   __SA0_OPEN(sa3Params);
   for (vecCount = 0; vecCount < numOutputBlocks4; vecCount++) {
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

   // Step 9: Copy last filterLength samples to beginning of state buffer for next time (stage 2)
   dsplibStatus = DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle3,
                                        &pStateLocal[pKerPrivArgs->stage2BuffStartIndex + intermediateSampleCount],
                                        &pStateLocal[pKerPrivArgs->stage2BuffStartIndex]);

   return status;
}

/*
 * Execution for 4x downsampling using block processing (circular buffer).
 * Processes input in blocks using streaming engines and agents for FIR filtering and output decimation.
 * Handles circular buffer index updates and history block copy.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_block_exec_ci(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pState,
                                                         void *restrict pFiltCoeffs,
                                                         void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_block_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                 = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs           = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                 = pKerPrivArgs->bufPblock;
   uint32_t               *stage1BuffStartIndex   = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t               *stage2BuffCurrIndex    = &pKerPrivArgs->stage2BuffCurrIndex;
   uint32_t                cirBuffAddressMask     = pKerPrivArgs->cirBuffAddressMask;
   uint32_t                inBufferTotalDimX      = pKerPrivArgs->inBufferTotalDimX;
   dataType               *pInLocal               = (dataType *) pIn;
   dataType               *pOutLocal              = (dataType *) pOut;
   dataType               *pFilterStage1          = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType               *pFilterStage2          = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 numChannels            = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount       = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 stage2InputSampleCount = inputSampleCount / 2;
   int32_t                 numOutputBlocks1       = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2       = pKerPrivArgs->numOutputBlocks2;
   int32_t                 numOutputBlocks3       = pKerPrivArgs->numOutputBlocks3;
   int32_t                 numOutputBlocks4       = pKerPrivArgs->numOutputBlocks4;
   int32_t                 numOutputBlocks5       = pKerPrivArgs->numOutputBlocks5;
   int32_t                 chCount, outputBlockCount, tapCount;

   // Intermediate buffer for stage 1 output (2x downsampled) which is also the input for stage 2
   dataType *pIntermediateLocal = pInLocal + inBufferTotalDimX; // Point to the second half of pIn buffer
   dataType *pStage2Input       = pIntermediateLocal + (*stage2BuffCurrIndex);

   // Use single sample processing with streaming engines
   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se4Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se5Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se6Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE6_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa3Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa4Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA4_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa5Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA5_PARAM_OFFSET);

   __SE0_OPEN(&pInLocal[*stage1BuffStartIndex], se0Params);
   __SE1_OPEN(&pInLocal[(*stage1BuffStartIndex +
                         ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS - 1) * numChannels)) &
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

         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS);
              tapCount++) {
            // Duplicate each filter coefficient
            dataType *VB1Dup    = c7x::strm_agen<0, dataType>::get_adv(pFilterStage1);
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
         vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc0);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc1);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc2);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc3);
      }
   }

   // Close streaming engines and agents
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   /* Complete anti-aliasing filtering by adding phase 2 filtering */
   __SE0_OPEN(pStage2Input, se2Params);
   __SE1_OPEN(&pInLocal[(*stage1BuffStartIndex +
                         ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1) * numChannels)) &
                        cirBuffAddressMask],
              se3Params);
   __SA2_OPEN(sa2Params);

   for (outputBlockCount = 0; outputBlockCount < numOutputBlocks2; outputBlockCount++) {
      vec vecFirPhase1 = c7x::strm_eng<0, vec>::get_adv();
      vec vecPhase2    = c7x::strm_eng<1, vec>::get_adv();

      vec vecDownsampled = vecFirPhase1 + vecPhase2 * 0.5f;

      __vpred opWr    = c7x::strm_agen<2, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<2, vec>::get_adv(pStage2Input);
      __vstore_pred(opWr, opWrPtr, vecDownsampled);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA2_CLOSE();

   __SE0_OPEN(pIntermediateLocal, se4Params);
   __SE1_OPEN(&pIntermediateLocal[(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels],
              se5Params);
   __SA0_OPEN(sa3Params);
   __SA1_OPEN(sa4Params);

   // Process each channel block
   for (chCount = 0; chCount < numChannels; chCount += eleCount) {
      // Process each output block
      for (outputBlockCount = 0; outputBlockCount < numOutputBlocks3; outputBlockCount++) {
         // Initialize accumulators for filtered outputs
         vec vecAcc0 = (vec) 0;
         vec vecAcc1 = (vec) 0;
         vec vecAcc2 = (vec) 0;
         vec vecAcc3 = (vec) 0;

         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS);
              tapCount++) {
            // Duplicate each filter coefficient
            dataType *VB1Dup    = c7x::strm_agen<0, dataType>::get_adv(pFilterStage2);
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

   // Close streaming engines and agents
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   /* Complete anti-aliasing filtering by adding phase 2 filtering for stage 2*/
   __SE0_OPEN(pOutLocal, se2Params);
   __SE1_OPEN(&pIntermediateLocal[(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels],
              se3Params);
   __SA2_OPEN(sa2Params);

   for (outputBlockCount = 0; outputBlockCount < numOutputBlocks4; outputBlockCount++) {
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

   // block copy to history buffer
   __SE0_OPEN(&pIntermediateLocal[stage2InputSampleCount * numChannels], se6Params);
   __SA0_OPEN(sa5Params);

   for (outputBlockCount = 0; outputBlockCount < numOutputBlocks5; outputBlockCount++) {
      vec     data    = c7x::strm_eng<0, vec>::get_adv();
      __vpred opWr    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<0, vec>::get_adv(pIntermediateLocal);
      __vstore_pred(opWr, opWrPtr, data);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   // Update circular buffer index for next call
   *stage1BuffStartIndex = (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

/*
 * Execution for 4x downsampling using single sample processing (circular buffer).
 * Processes each sample individually in two stages to achieve 4x downsampling.
 * Uses streaming engines and agents for FIR filtering and output decimation.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_ss_exec_ci(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pState,
                                                      void *restrict pFiltCoeffs,
                                                      void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_ss_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                 = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs           = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                 = pKerPrivArgs->bufPblock;
   uint32_t               *stage1BuffStartIndex   = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t               *stage2BuffCurrIndex    = &pKerPrivArgs->stage2BuffCurrIndex;
   uint32_t                cirBuffAddressMask     = pKerPrivArgs->cirBuffAddressMask;
   uint32_t                inBufferTotalDimX      = pKerPrivArgs->inBufferTotalDimX;
   dataType               *pInLocal               = (dataType *) pIn;
   dataType               *pOutLocal              = (dataType *) pOut;
   dataType               *pFilterStage1          = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType               *pFilterStage2          = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 numChannels            = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount       = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 stage2InputSampleCount = inputSampleCount / 2;
   int32_t                 outputSampleCount      = pKerPrivArgs->outputSampleCount;
   int32_t                 numOutputBlocks1       = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2       = pKerPrivArgs->numOutputBlocks2;
   int32_t                 numOutputBlocks3       = pKerPrivArgs->numOutputBlocks3;
   int32_t                 numOutputBlocks4       = pKerPrivArgs->numOutputBlocks4;
   int32_t                 chCount, outputCount, tapCount;

   // Intermediate buffer for stage 1 output (2x downsampled) which is also the input for stage 2
   dataType *pIntermediateLocal = pInLocal + inBufferTotalDimX; // Point to the second half of pIn buffer
   dataType *pStage2Input       = pIntermediateLocal + (*stage2BuffCurrIndex);

   // Use single sample processing with streaming engines
   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se4Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se5Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se6Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE6_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa3Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa4Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA4_PARAM_OFFSET);

   __SE0_OPEN(&pInLocal[*stage1BuffStartIndex], se0Params);
   __SE1_OPEN(&pInLocal[(*stage1BuffStartIndex +
                         ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS - 1) * numChannels)) &
                        cirBuffAddressMask],
              se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

#pragma MUST_ITERATE(2, , 1)
   for (outputCount = 0; outputCount < numOutputBlocks1; outputCount++) {
      vec vecAcc = (vec) 0;

      /* ---- UR0 ---------------------------------------------------------- */
      /* Load next coefficient */
      vec vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      /* Load forward input sample */
      vec vecInputF = c7x::strm_eng<0, vec>::get_adv();
      /* Load reverse input sample */
      vec vecInputR = c7x::strm_eng<1, vec>::get_adv();
      /* MAC */
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR1 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR2 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR3 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR4 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR5 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR6 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR7 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR8 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR9 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      // Write first output to final output buffer (filtered sample)
      __vpred opWrPred = c7x::strm_agen<1, vec>::get_vpred();
      vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
      __vstore_pred(opWrPred, opWrPtr, vecAcc);
   }

   // Close streaming engines and agents
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   /* Complete anti-aliasing filtering by adding phase 2 filtering */
   __SE0_OPEN(pStage2Input, se2Params);
   __SE1_OPEN(&pInLocal[(*stage1BuffStartIndex +
                         ((AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1) * numChannels)) &
                        cirBuffAddressMask],
              se3Params);
   __SA2_OPEN(sa2Params);

   for (outputCount = 0; outputCount < numOutputBlocks2; outputCount++) {
      vec vecFirPhase1 = c7x::strm_eng<0, vec>::get_adv();
      vec vecPhase2    = c7x::strm_eng<1, vec>::get_adv();

      vec vecDownsampled = vecFirPhase1 + vecPhase2 * 0.5f;

      __vpred opWr    = c7x::strm_agen<2, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<2, vec>::get_adv(pStage2Input);
      __vstore_pred(opWr, opWrPtr, vecDownsampled);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA2_CLOSE();

   __SE0_OPEN(pIntermediateLocal, se4Params);
   __SE1_OPEN(&pIntermediateLocal[(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1) * numChannels],
              se5Params);
   __SA0_OPEN(sa3Params);
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
            vec vecCoeffs0 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
            /* Load forward input sample */
            vec vecInputF0 = c7x::strm_eng<0, vec>::get_adv();
            /* Load reverse input sample */
            vec vecInputR0 = c7x::strm_eng<1, vec>::get_adv();
            /* MAC */
            vecAcc0 += vecCoeffs0 * (vecInputF0 + vecInputR0);

            /* ---- UR1 ---------------------------------------------------------- */
            vec vecCoeffs1 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
            vec vecInputF1 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInputR1 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc1 += vecCoeffs1 * (vecInputF1 + vecInputR1);

            /* ---- UR2 ---------------------------------------------------------- */
            vec vecCoeffs2 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
            vec vecInputF2 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInputR2 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc2 += vecCoeffs2 * (vecInputF2 + vecInputR2);

            /* ---- UR3 ---------------------------------------------------------- */
            vec vecCoeffs3 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
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

   /* Complete anti-aliasing filtering by adding phase 2 filtering for stage 2*/
   __SE0_OPEN(pOutLocal, se2Params);
   __SE1_OPEN(&pIntermediateLocal[(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels],
              se3Params);
   __SA2_OPEN(sa2Params);

   for (outputCount = 0; outputCount < numOutputBlocks3; outputCount++) {
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

   // block copy to history buffer
   __SE0_OPEN(&pIntermediateLocal[stage2InputSampleCount * numChannels], se6Params);
   __SA0_OPEN(sa4Params);

   for (outputCount = 0; outputCount < numOutputBlocks4; outputCount++) {
      vec     data    = c7x::strm_eng<0, vec>::get_adv();
      __vpred opWr    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<0, vec>::get_adv(pIntermediateLocal);
      __vstore_pred(opWr, opWrPtr, data);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   // Update circular buffer index for next call
   *stage1BuffStartIndex = (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

/*******************************************************************************
 * TEMPLATE INSTANTIATIONS
 ******************************************************************************/

// Initialization function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_linear_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_block_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                         AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                         AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                         AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_ss_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                      AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                      AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS
AUDIOLIB_ssrc_downsample4x_non_interleaved_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

// Execution function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_linear_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                          void *restrict pIn,
                                                                          void *restrict pState,
                                                                          void *restrict pFiltCoeffs,
                                                                          void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_non_interleaved_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                   void *restrict pIn,
                                                                                   void *restrict pState,
                                                                                   void *restrict pFiltCoeffs,
                                                                                   void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_block_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                         void *restrict pIn,
                                                                         void *restrict pState,
                                                                         void *restrict pFiltCoeffs,
                                                                         void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_ss_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                      void *restrict pIn,
                                                                      void *restrict pState,
                                                                      void *restrict pFiltCoeffs,
                                                                      void *restrict pOut);
