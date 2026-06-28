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
 * Main initialization function for 4x upsampling using C7x intrinsics.
 * Selects the appropriate processing mode (linear, block, or single sample)
 * based on buffer format and input sample count for interleaved data format.
 * For non-interleaved data format, uses linear buffer based processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   uint8_t                 bufferFormat     = pKerPrivArgs->initArgs.bufferFormat;
   uint8_t                 dataFormat       = pKerPrivArgs->initArgs.dataFormat;

   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      // Linear buffer: use linear upsampling implementation
      if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
         status = AUDIOLIB_ssrc_upsample4x_linear_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (status == AUDIOLIB_SUCCESS) {
            pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample4x_linear_exec_ci<float>;
         }
      }
      else {
         // Circular buffer: select block or single sample mode based on input size
         if (inputSampleCount > (int32_t) AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT) {
            status = AUDIOLIB_ssrc_upsample4x_block_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample4x_block_exec_ci<float>;
            }
         }
         else {
            status = AUDIOLIB_ssrc_upsample4x_ss_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample4x_ss_exec_ci<float>;
            }
         }
      }
   }
   else {
      // Non-interleaved data: use non-interleaved upsampling implementation
      status = AUDIOLIB_ssrc_upsample4x_non_interleaved_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      if (status == AUDIOLIB_SUCCESS) {
         pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample4x_non_interleaved_exec_ci<float>;
      }
   }

   return status;
}

/*******************************************************************************
 * SPECIALIZED INITIALIZATION FUNCTIONS
 ******************************************************************************/
/*
 * Initialization for 4x upsampling with linear buffer format (interleaved data).
 * Sets up DSPLIB kernels for deinterleave, FIR filtering, block copy, and interleave.
 * Configures streaming engine and agent parameters for both upsampling stages.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_linear_init_ci\n");
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

   // Initialize FIR kernels for both stages
   // Stage 1 FIR initialization
   DSPLIB_fir_InitArgs firKerInitArgs;
   firKerInitArgs.funcStyle      = DSPLIB_FUNCTION_OPTIMIZED;
   firKerInitArgs.dataSize       = inputSampleCount + AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
   firKerInitArgs.batchSize      = numChannels;
   firKerInitArgs.filterSize     = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
   firKerInitArgs.shift          = 0;
   firKerInitArgs.enableNchCoefs = 0;
   firKerInitArgs.enableMMA      = pKerInitArgs->enableMMA;
   firKerInitArgs.MMA_SIZE       = pKerInitArgs->mmaSize;
   firKerInitArgs.enableQ        = 1;
   firKerInitArgs.Q              = 23;

   /* Initialize FIR input buffer parameters for stage 1 */
   firBufParamsIn.data_type = DSPLIB_FLOAT32;
   firBufParamsIn.dim_x     = inputSampleCount + AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
   firBufParamsIn.dim_y     = numChannels;
   firBufParamsIn.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR output buffer parameters for stage 1 */
   firBufParamsOut.data_type = DSPLIB_FLOAT32;
   firBufParamsOut.dim_x     = pKerPrivArgs->bufParamsState.dim_x;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR filter buffer parameters for stage 1 */
   firBufParamsFilter.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter.dim_x =
          AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = firBufParamsFilter.dim_x * eleSize;
   }
   else {
      firBufParamsFilter.dim_x    = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC * eleSize;
   }

   /* Check FIR initialization parameters for stage 1 */
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter,
                                                 &firBufParamsOut, &firKerInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter, &firBufParamsOut,
                                     &firKerInitArgs);
   }

   // Stage 2 FIR initialization
   firKerInitArgs.funcStyle      = DSPLIB_FUNCTION_OPTIMIZED;
   firKerInitArgs.dataSize       = inputSampleCount * 2 + AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1;
   firKerInitArgs.batchSize      = numChannels;
   firKerInitArgs.filterSize     = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
   firKerInitArgs.shift          = 0;
   firKerInitArgs.enableNchCoefs = 0;
   firKerInitArgs.enableMMA      = pKerInitArgs->enableMMA;
   firKerInitArgs.MMA_SIZE       = pKerInitArgs->mmaSize;
   firKerInitArgs.enableQ        = 1;
   firKerInitArgs.Q              = 23;

   /* Initialize FIR input buffer parameters for stage 2 */
   firBufParamsIn.data_type = DSPLIB_FLOAT32;
   firBufParamsIn.dim_x     = inputSampleCount * 2 + AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1;
   firBufParamsIn.dim_y     = numChannels;
   firBufParamsIn.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR output buffer parameters for stage 2 */
   firBufParamsOut.data_type = DSPLIB_FLOAT32;
   firBufParamsOut.dim_x     = pKerPrivArgs->bufParamsState.dim_x * 2;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR filter buffer parameters for stage 2 */
   firBufParamsFilter.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter.dim_x =
          AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = firBufParamsFilter.dim_x * eleSize;
   }
   else {
      firBufParamsFilter.dim_x    = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC * eleSize;
   }

   /* Check FIR initialization parameters for stage 2 */
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle2, &firBufParamsIn, &firBufParamsFilter,
                                                 &firBufParamsOut, &firKerInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle2, &firBufParamsIn, &firBufParamsFilter, &firBufParamsOut,
                                     &firKerInitArgs);
   }

   // Set up non-interleaved intermediate buffer parameters for stage 1 block copy
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
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

   // Set up non-interleaved intermediate buffer parameters for stage 2 block copy
   DSPLIB_blkCopy2DInitArgs blkCopy2DInitArgs2;
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   if (dsplibStatus == DSPLIB_SUCCESS) {
      blkCopy2DInitArgs2.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

      dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle2, &nonInterleavedBufParams,
                                                       &nonInterleavedBufParams, &blkCopy2DInitArgs2);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle2, &nonInterleavedBufParams,
                                           &nonInterleavedBufParams, &blkCopy2DInitArgs2);
   }

   // Set up non-interleaved intermediate buffer parameters for final output
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = outputSampleCount; // 4x upsampling
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   // Set up interleaved output buffer parameters
   interleavedBufParams.data_type = DSPLIB_FLOAT32;
   interleavedBufParams.dim_x     = bufParamsOut->dim_x;
   interleavedBufParams.dim_y     = bufParamsOut->dim_y;
   interleavedBufParams.stride_y  = bufParamsOut->stride_y;

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
   /* SE and SA config for stage 1 interleaving                          */
   /**********************************************************************/
   se0Params         = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0   = inputSampleCount;
   se0Params.DIM1    = stateStride;
   se0Params.ICNT1   = numChannels;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_2D;

   int32_t icnt0Param  = AUDIOLIB_min(inputSampleCount * 2, eleCount);
   int32_t blkItrCount = AUDIOLIB_ceilingDiv(inputSampleCount, eleCount);

   sa0Params               = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0         = icnt0Param;
   sa0Params.DIM1          = icnt0Param;
   sa0Params.ICNT1         = blkItrCount * 2;
   sa0Params.DIM2          = stateStride;
   sa0Params.ICNT2         = numChannels;
   sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa0Params.DECDIM1_WIDTH = inputSampleCount * 2;
   sa0Params.VECLEN        = SA_VECLEN;
   sa0Params.DIMFMT        = __SA_DIMFMT_3D;

   pKerPrivArgs->numOutputBlocks1 = blkItrCount * numChannels;

   /**********************************************************************/
   /* SE and SA config for stage 2 interleaving                          */
   /**********************************************************************/
   se1Params         = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0   = inputSampleCount * 2;
   se1Params.DIM1    = stateStride;
   se1Params.ICNT1   = numChannels;
   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   se1Params.DIMFMT  = __SE_DIMFMT_2D;

   icnt0Param  = AUDIOLIB_min(outputSampleCount, eleCount);
   blkItrCount = AUDIOLIB_ceilingDiv(inputSampleCount * 2, eleCount);

   sa1Params               = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0         = icnt0Param;
   sa1Params.DIM1          = icnt0Param;
   sa1Params.ICNT1         = blkItrCount * 2;
   sa1Params.DIM2          = stateStride;
   sa1Params.ICNT2         = numChannels;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa1Params.DECDIM1_WIDTH = outputSampleCount;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;

   pKerPrivArgs->numOutputBlocks2 = blkItrCount * numChannels;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   if (dsplibStatus != DSPLIB_SUCCESS) {
      status = AUDIOLIB_ERR_FAILURE;
   }

   return status;
}

/*
 * Initialization for 4x upsampling with non-interleaved data format.
 * Sets up DSPLIB kernels for block copy and FIR filtering for both stages.
 * Configures streaming engine and agent parameters for upsampling.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_non_interleaved_init_ci\n");
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
   DSPLIB_bufParams2D_t nonInterleavedBufParams, srcBufParams;
   DSPLIB_bufParams2D_t firBufParamsIn, firBufParamsOut, firBufParamsFilter;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

   // Set up non-interleaved intermediate buffer parameters
   srcBufParams.data_type = DSPLIB_FLOAT32;
   srcBufParams.dim_x     = bufParamsIn->dim_x;
   srcBufParams.dim_y     = bufParamsIn->dim_y;
   srcBufParams.stride_y  = bufParamsIn->stride_y;

   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = pKerPrivArgs->bufParamsState.dim_x;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   // Initialize DSPLIB blkCopy2D for input copying to state buffer
   DSPLIB_blkCopy2DInitArgs blkCopy2DInitArgs;
   if (dsplibStatus == DSPLIB_SUCCESS) {
      blkCopy2DInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

      dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle1, &srcBufParams,
                                                       &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle1, &srcBufParams, &nonInterleavedBufParams,
                                           &blkCopy2DInitArgs);
   }

   // Initialize FIR kernels for both stages
   // Stage 1 FIR initialization
   DSPLIB_fir_InitArgs firKerInitArgs;
   firKerInitArgs.funcStyle      = DSPLIB_FUNCTION_OPTIMIZED;
   firKerInitArgs.dataSize       = inputSampleCount + AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
   firKerInitArgs.batchSize      = numChannels;
   firKerInitArgs.filterSize     = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
   firKerInitArgs.shift          = 0;
   firKerInitArgs.enableNchCoefs = 0;
   firKerInitArgs.enableMMA      = pKerInitArgs->enableMMA;
   firKerInitArgs.MMA_SIZE       = pKerInitArgs->mmaSize;
   firKerInitArgs.enableQ        = 1;
   firKerInitArgs.Q              = 23;

   /* Initialize FIR input buffer parameters for stage 1 */
   firBufParamsIn.data_type = DSPLIB_FLOAT32;
   firBufParamsIn.dim_x     = inputSampleCount + AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
   firBufParamsIn.dim_y     = numChannels;
   firBufParamsIn.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR output buffer parameters for stage 1 */
   firBufParamsOut.data_type = DSPLIB_FLOAT32;
   firBufParamsOut.dim_x     = pKerPrivArgs->bufParamsState.dim_x;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR filter buffer parameters for stage 1 */
   firBufParamsFilter.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter.dim_x =
          AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = firBufParamsFilter.dim_x * eleSize;
   }
   else {
      firBufParamsFilter.dim_x    = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC * eleSize;
   }

   /* Check FIR initialization parameters for stage 1 */
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter,
                                                 &firBufParamsOut, &firKerInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter, &firBufParamsOut,
                                     &firKerInitArgs);
   }

   // Stage 2 FIR initialization
   firKerInitArgs.funcStyle      = DSPLIB_FUNCTION_OPTIMIZED;
   firKerInitArgs.dataSize       = inputSampleCount * 2 + AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1;
   firKerInitArgs.batchSize      = numChannels;
   firKerInitArgs.filterSize     = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
   firKerInitArgs.shift          = 0;
   firKerInitArgs.enableNchCoefs = 0;
   firKerInitArgs.enableMMA      = pKerInitArgs->enableMMA;
   firKerInitArgs.MMA_SIZE       = pKerInitArgs->mmaSize;
   firKerInitArgs.enableQ        = 1;
   firKerInitArgs.Q              = 23;

   /* Initialize FIR input buffer parameters for stage 2 */
   firBufParamsIn.data_type = DSPLIB_FLOAT32;
   firBufParamsIn.dim_x     = inputSampleCount * 2 + AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1;
   firBufParamsIn.dim_y     = numChannels;
   firBufParamsIn.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR output buffer parameters for stage 2 */
   firBufParamsOut.data_type = DSPLIB_FLOAT32;
   firBufParamsOut.dim_x     = pKerPrivArgs->bufParamsState.dim_x * 2;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   /* Initialize FIR filter buffer parameters for stage 2 */
   firBufParamsFilter.data_type = DSPLIB_FLOAT32;
   if (pKerInitArgs->enableMMA) {
      firBufParamsFilter.dim_x =
          AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC + (pKerInitArgs->mmaSize * 3) - 1;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = firBufParamsFilter.dim_x * eleSize;
   }
   else {
      firBufParamsFilter.dim_x    = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
      firBufParamsFilter.dim_y    = 1;
      firBufParamsFilter.stride_y = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC * eleSize;
   }

   /* Check FIR initialization parameters for stage 2 */
   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle2, &firBufParamsIn, &firBufParamsFilter,
                                                 &firBufParamsOut, &firKerInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle2, &firBufParamsIn, &firBufParamsFilter, &firBufParamsOut,
                                     &firKerInitArgs);
   }

   // Set up non-interleaved intermediate buffer parameters for stage 1 block copy
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
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

   // Set up non-interleaved intermediate buffer parameters for stage 2 block copy
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   if (dsplibStatus == DSPLIB_SUCCESS) {
      blkCopy2DInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

      dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle3, &nonInterleavedBufParams,
                                                       &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle3, &nonInterleavedBufParams,
                                           &nonInterleavedBufParams, &blkCopy2DInitArgs);
   }

   /**********************************************************************/
   /* SE and SA config for stage 1 interleaving                          */
   /**********************************************************************/
   se0Params         = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0   = inputSampleCount;
   se0Params.DIM1    = stateStride;
   se0Params.ICNT1   = numChannels;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_2D;

   int32_t icnt0Param  = AUDIOLIB_min(inputSampleCount * 2, eleCount);
   int32_t blkItrCount = AUDIOLIB_ceilingDiv(inputSampleCount, eleCount);

   sa0Params               = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0         = icnt0Param;
   sa0Params.DIM1          = icnt0Param;
   sa0Params.ICNT1         = blkItrCount * 2;
   sa0Params.DIM2          = stateStride;
   sa0Params.ICNT2         = numChannels;
   sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa0Params.DECDIM1_WIDTH = inputSampleCount * 2;
   sa0Params.VECLEN        = SA_VECLEN;
   sa0Params.DIMFMT        = __SA_DIMFMT_3D;

   pKerPrivArgs->numOutputBlocks1 = blkItrCount * numChannels;

   /**********************************************************************/
   /* SE and SA config for stage 2 interleaving                          */
   /**********************************************************************/
   se1Params         = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0   = inputSampleCount * 2;
   se1Params.DIM1    = stateStride;
   se1Params.ICNT1   = numChannels;
   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   se1Params.DIMFMT  = __SE_DIMFMT_2D;

   icnt0Param  = AUDIOLIB_min(outputSampleCount, eleCount);
   blkItrCount = AUDIOLIB_ceilingDiv(inputSampleCount * 2, eleCount);

   sa1Params               = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0         = icnt0Param;
   sa1Params.DIM1          = icnt0Param;
   sa1Params.ICNT1         = blkItrCount * 2;
   sa1Params.DIM2          = bufParamsOut->stride_y / eleSize;
   sa1Params.ICNT2         = numChannels;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa1Params.DECDIM1_WIDTH = outputSampleCount;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;

   pKerPrivArgs->numOutputBlocks2 = blkItrCount * numChannels;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   if (dsplibStatus != DSPLIB_SUCCESS) {
      status = AUDIOLIB_ERR_FAILURE;
   }

   return status;
}

/*
 * Initialization for 4x upsampling using block processing (circular buffer).
 * Used when inputSampleCount exceeds threshold. Sets up streaming engines and agents
 * for multi-channel block FIR filtering and output interleaving.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_block_init_ci(AUDIOLIB_kernelHandle   handle,
                                                       AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                       AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                       AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_block_init_ci\n");
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
   __SE_TEMPLATE_v1 se4Params; // =__gen_SE_TEMPLATE_v1();
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
    * addressing functionality. This deviates from the natural C implementation for 4x upsampling */
   pKerPrivArgs->stage2BuffStartIndex = 0;
   pKerPrivArgs->stage2BuffCurrIndex  = (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels;

   pKerPrivArgs->numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   int32_t icnt0Param             = AUDIOLIB_min(numChannels, eleCount);

   // Stage 1: 2x upsampling
   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(inputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch forward samples for stage 1     */
   /**********************************************************************/
   se0Params               = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0         = icnt0Param;
   se0Params.DIM1          = numChannels;
   se0Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se0Params.DIM2          = numChannels;
   se0Params.ICNT2         = (int32_t) (AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS + 1); // +1 for the center tap
   se0Params.DIM3          = numChannels * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
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
   /* Prepare streaming engine 1 to fetch reverse samples for stage 1     */
   /**********************************************************************/
   se1Params               = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0         = icnt0Param;
   se1Params.DIM1          = numChannels;
   se1Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se1Params.DIM2          = -numChannels;
   se1Params.ICNT2         = (int32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS;
   se1Params.DIM3          = numChannels * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
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
   /* Prepare SA to fetch the filter coefficients for stage 1             */
   /**********************************************************************/
   sa0Params        = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0  = 1;
   sa0Params.DIM1   = 1;
   sa0Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS;
   sa0Params.DIM2   = 0;
   sa0Params.ICNT2  = pKerPrivArgs->numOutputBlocks1;
   sa0Params.DIM3   = 0;
   sa0Params.ICNT3  = pKerPrivArgs->numChannelBlocks;
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store stage 1 output                        */
   /**********************************************************************/
   sa1Params       = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0 = icnt0Param;
   sa1Params.DIM1  = numChannels;
   sa1Params.ICNT1 =
       pKerPrivArgs->numOutputBlocks1 * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK * 2; // 2 for including center tap
   sa1Params.DIM2          = eleCount;
   sa1Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa1Params.DECDIM1_WIDTH = numChannels * inputSampleCount * 2; // 2x upsampled output
   sa1Params.DECDIM2       = __SA_DECDIM_DIM2;
   sa1Params.DECDIM2_WIDTH = numChannels;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;

   // Stage 2: 2x upsampling on the output of stage 1
   int32_t stage2InputSampleCount = inputSampleCount * 2; // Output of stage 1
   pKerPrivArgs->numOutputBlocks2 = AUDIOLIB_ceilingDiv(stage2InputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch forward samples for stage 2    */
   /**********************************************************************/
   se2Params               = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0         = icnt0Param;
   se2Params.DIM1          = numChannels;
   se2Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se2Params.DIM2          = numChannels;
   se2Params.ICNT2         = (int32_t) (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS + 1); // +1 for the center tap
   se2Params.DIM3          = numChannels * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se2Params.ICNT3         = pKerPrivArgs->numOutputBlocks2;
   se2Params.DIM4          = eleCount;
   se2Params.ICNT4         = pKerPrivArgs->numChannelBlocks;
   se2Params.DECDIM1       = __SE_DECDIM_DIM3;
   se2Params.DECDIM1SD     = __SE_DECDIMSD_DIM1;
   se2Params.DECDIM1_WIDTH = stage2InputSampleCount * numChannels;
   se2Params.DECDIM2       = __SE_DECDIM_DIM4;
   se2Params.DECDIM2_WIDTH = numChannels;
   se2Params.ELETYPE       = SE_ELETYPE;
   se2Params.VECLEN        = SE_VECLEN;
   se2Params.DIMFMT        = __SE_DIMFMT_5D;

   /**********************************************************************/
   /* Prepare streaming engine 1 to fetch reverse samples for stage 2    */
   /**********************************************************************/
   se3Params               = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0         = icnt0Param;
   se3Params.DIM1          = numChannels;
   se3Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se3Params.DIM2          = -numChannels;
   se3Params.ICNT2         = (int32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS;
   se3Params.DIM3          = numChannels * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK;
   se3Params.ICNT3         = pKerPrivArgs->numOutputBlocks2;
   se3Params.DIM4          = eleCount;
   se3Params.ICNT4         = pKerPrivArgs->numChannelBlocks;
   se3Params.DECDIM1       = __SE_DECDIM_DIM3;
   se3Params.DECDIM1SD     = __SE_DECDIMSD_DIM1;
   se3Params.DECDIM1_WIDTH = stage2InputSampleCount * numChannels;
   se3Params.DECDIM2       = __SE_DECDIM_DIM4;
   se3Params.DECDIM2_WIDTH = numChannels;
   se3Params.ELETYPE       = SE_ELETYPE;
   se3Params.VECLEN        = SE_VECLEN;
   se3Params.DIMFMT        = __SE_DIMFMT_5D;

   /**********************************************************************/
   /* Prepare SA to fetch the filter coefficients for stage 2             */
   /**********************************************************************/
   sa2Params        = __gen_SA_TEMPLATE_v1();
   sa2Params.ICNT0  = 1;
   sa2Params.DIM1   = 1;
   sa2Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS;
   sa2Params.DIM2   = 0;
   sa2Params.ICNT2  = pKerPrivArgs->numOutputBlocks2;
   sa2Params.DIM3   = 0;
   sa2Params.ICNT3  = pKerPrivArgs->numChannelBlocks;
   sa2Params.VECLEN = SA_VECLEN;
   sa2Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store stage 2 output                         */
   /**********************************************************************/
   sa3Params       = __gen_SA_TEMPLATE_v1();
   sa3Params.ICNT0 = icnt0Param;
   sa3Params.DIM1  = numChannels;
   sa3Params.ICNT1 =
       pKerPrivArgs->numOutputBlocks2 * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK * 2; // 2 for including center tap
   sa3Params.DIM2          = eleCount;
   sa3Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   sa3Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa3Params.DECDIM1_WIDTH = numChannels * outputSampleCount; // 4x upsampled output
   sa3Params.DECDIM2       = __SA_DECDIM_DIM2;
   sa3Params.DECDIM2_WIDTH = numChannels;
   sa3Params.VECLEN        = SA_VECLEN;
   sa3Params.DIMFMT        = __SA_DIMFMT_3D;

   /**********************************************************************/
   /* Prepare SE template for block copy input                           */
   /**********************************************************************/
   se4Params         = __gen_SE_TEMPLATE_v1();
   se4Params.ICNT0   = (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels;
   se4Params.ELETYPE = SE_ELETYPE;
   se4Params.VECLEN  = SE_VECLEN;
   se4Params.DIMFMT  = __SE_DIMFMT_1D;

   /**********************************************************************/
   /* Prepare SA template for block copy output                          */
   /**********************************************************************/
   sa4Params        = __gen_SA_TEMPLATE_v1();
   sa4Params.ICNT0  = (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels;
   sa4Params.VECLEN = SA_VECLEN;
   sa4Params.DIMFMT = __SA_DIMFMT_1D;

   pKerPrivArgs->numOutputBlocks3 =
       AUDIOLIB_ceilingDiv((AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels, eleCount);

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = se4Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET) = sa3Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA4_PARAM_OFFSET) = sa4Params;

   return status;
}

/*
 * Initialization for 4x upsampling using single sample processing (circular buffer).
 * Used when inputSampleCount is below threshold. Sets up streaming engines and agents
 * for per-sample FIR filtering and output interleaving.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_ss_init_ci(AUDIOLIB_kernelHandle   handle,
                                                    AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                    AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                    AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_ss_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock           = pKerPrivArgs->bufPblock;
   int32_t                 numChannels      = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se4Params; // =__gen_SE_TEMPLATE_v1();
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
    * addressing functionality. This deviates from the natural C implementation for 4x upsampling */
   pKerPrivArgs->stage2BuffStartIndex = 0;
   pKerPrivArgs->stage2BuffCurrIndex  = (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels;

   pKerPrivArgs->numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   int32_t icnt0Param             = AUDIOLIB_min(numChannels, eleCount);

   // Stage 1: 2x upsampling
   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch forward samples for stage 1     */
   /**********************************************************************/
   se0Params               = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0         = icnt0Param;
   se0Params.DIM1          = numChannels;
   se0Params.ICNT1         = (int32_t) (AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS + 1); // +1 for the center tap
   se0Params.DIM2          = eleCount;
   se0Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se0Params.DIM3          = numChannels;
   se0Params.ICNT3         = inputSampleCount;
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
   /* Prepare streaming engine 1 to fetch reverse samples for stage 1     */
   /**********************************************************************/
   se1Params               = __gen_SE_TEMPLATE_v1();
   se1Params.ICNT0         = icnt0Param;
   se1Params.DIM1          = -numChannels;
   se1Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS;
   se1Params.DIM2          = eleCount;
   se1Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se1Params.DIM3          = numChannels;
   se1Params.ICNT3         = inputSampleCount;
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
   /* Prepare SA to fetch the filter coefficients for stage 1             */
   /**********************************************************************/
   sa0Params        = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0  = 1;
   sa0Params.DIM1   = 1;
   sa0Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS;
   sa0Params.DIM2   = 0;
   sa0Params.ICNT2  = pKerPrivArgs->numChannelBlocks;
   sa0Params.DIM3   = 0;
   sa0Params.ICNT3  = inputSampleCount;
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store stage 1 output                         */
   /**********************************************************************/
   sa1Params               = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0         = icnt0Param;
   sa1Params.DIM1          = numChannels;
   sa1Params.ICNT1         = 2; // 2 for including center tap
   sa1Params.DIM2          = eleCount;
   sa1Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   sa1Params.DIM3          = numChannels * 2;
   sa1Params.ICNT3         = inputSampleCount;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM2;
   sa1Params.DECDIM1_WIDTH = numChannels;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_4D;

   // Stage 2: 2x upsampling on the output of stage 1
   int32_t stage2InputSampleCount = inputSampleCount * 2; // Output of stage 1

   /**********************************************************************/
   /* Prepare streaming engine 2 to fetch forward samples for stage 2     */
   /**********************************************************************/
   se2Params               = __gen_SE_TEMPLATE_v1();
   se2Params.ICNT0         = icnt0Param;
   se2Params.DIM1          = numChannels;
   se2Params.ICNT1         = (int32_t) (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS + 1); // +1 for the center tap
   se2Params.DIM2          = eleCount;
   se2Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se2Params.DIM3          = numChannels;
   se2Params.ICNT3         = stage2InputSampleCount;
   se2Params.DECDIM1       = __SE_DECDIM_DIM2;
   se2Params.DECDIM1_WIDTH = numChannels;
   se2Params.ELETYPE       = SE_ELETYPE;
   se2Params.VECLEN        = SE_VECLEN;
   se2Params.DIMFMT        = __SE_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare streaming engine 3 to fetch reverse samples for stage 2     */
   /**********************************************************************/
   se3Params               = __gen_SE_TEMPLATE_v1();
   se3Params.ICNT0         = icnt0Param;
   se3Params.DIM1          = -numChannels;
   se3Params.ICNT1         = (int32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS;
   se3Params.DIM2          = eleCount;
   se3Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   se3Params.DIM3          = numChannels;
   se3Params.ICNT3         = stage2InputSampleCount;
   se3Params.DECDIM1       = __SE_DECDIM_DIM2;
   se3Params.DECDIM1_WIDTH = numChannels;
   se3Params.ELETYPE       = SE_ELETYPE;
   se3Params.VECLEN        = SE_VECLEN;
   se3Params.DIMFMT        = __SE_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA to fetch the filter coefficients for stage 2             */
   /**********************************************************************/
   sa2Params        = __gen_SA_TEMPLATE_v1();
   sa2Params.ICNT0  = 1;
   sa2Params.DIM1   = 1;
   sa2Params.ICNT1  = (int32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS;
   sa2Params.DIM2   = 0;
   sa2Params.ICNT2  = pKerPrivArgs->numChannelBlocks;
   sa2Params.DIM3   = 0;
   sa2Params.ICNT3  = stage2InputSampleCount;
   sa2Params.VECLEN = SA_VECLEN;
   sa2Params.DIMFMT = __SA_DIMFMT_4D;

   /**********************************************************************/
   /* Prepare SA template to store stage 2 output (final output)          */
   /**********************************************************************/
   sa3Params               = __gen_SA_TEMPLATE_v1();
   sa3Params.ICNT0         = icnt0Param;
   sa3Params.DIM1          = numChannels;
   sa3Params.ICNT1         = 2; // 2 for including center tap
   sa3Params.DIM2          = eleCount;
   sa3Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   sa3Params.DIM3          = numChannels * 2;
   sa3Params.ICNT3         = stage2InputSampleCount;
   sa3Params.DECDIM1       = __SA_DECDIM_DIM2;
   sa3Params.DECDIM1_WIDTH = numChannels;
   sa3Params.VECLEN        = SA_VECLEN;
   sa3Params.DIMFMT        = __SA_DIMFMT_4D;

   pKerPrivArgs->numOutputBlocks2 = stage2InputSampleCount * pKerPrivArgs->numChannelBlocks;

   /**********************************************************************/
   /* Prepare SE template for block copy input                           */
   /**********************************************************************/
   se4Params         = __gen_SE_TEMPLATE_v1();
   se4Params.ICNT0   = (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels;
   se4Params.ELETYPE = SE_ELETYPE;
   se4Params.VECLEN  = SE_VECLEN;
   se4Params.DIMFMT  = __SE_DIMFMT_1D;

   /**********************************************************************/
   /* Prepare SA template for block copy output                          */
   /**********************************************************************/
   sa4Params        = __gen_SA_TEMPLATE_v1();
   sa4Params.ICNT0  = (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels;
   sa4Params.VECLEN = SA_VECLEN;
   sa4Params.DIMFMT = __SA_DIMFMT_1D;

   pKerPrivArgs->numOutputBlocks3 =
       AUDIOLIB_ceilingDiv((AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels, eleCount);

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = se4Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET) = sa3Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA4_PARAM_OFFSET) = sa4Params;

   return status;
}

/*******************************************************************************
 * EXECUTION FUNCTIONS
 ******************************************************************************/

/*
 * Execution for 4x upsampling with linear buffer format (interleaved data).
 * Performs deinterleave, FIR filtering, interleaving, and block copy for both stages.
 * Final output is re-interleaved.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pState,
                                                        void *restrict pFiltCoeffs,
                                                        void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_linear_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                               = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus __attribute__((unused)) = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs                         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                               = pKerPrivArgs->bufPblock;
   dataType               *pInLocal                             = (dataType *) pIn;
   dataType               *pOutLocal                            = (dataType *) pOut;
   dataType               *pStateLocal                          = (dataType *) pState;
   dataType               *pFilterStage1    = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType               *pFilterStage2    = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 numOutputBlocks1 = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2 = pKerPrivArgs->numOutputBlocks2;
   int32_t                 vecCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   /* Step 1: Transform interleaved input data to non-interleaved format using deinterleave kernel and store input
   samples to state buffer (after the filter history) */
   dsplibStatus = DSPLIB_deinterleave_exec(pKerPrivArgs->initArgs.deinterleaveHandle, pInLocal,
                                           &pStateLocal[pKerPrivArgs->stage1BuffCurrIndex]);

   /* Step 2: Run FIR on all input samples for stage 1 (gives interpolated sample) */
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle1, pStateLocal, pFilterStage1,
                                  &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);

   /* Step 3: Interleave the fir output samples with original input samples for stage 1 */
   /* Read filtered samples */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex], se0Params);
   /* Read original samples */
   __SE1_OPEN(&pStateLocal[AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS], se0Params);
   __SA0_OPEN(sa0Params);

   // Intermediate buffer for stage 1 output (2x upsampled) which is also the input for stage 2 filtering
   dataType *pStage2CurrInput = &pStateLocal[pKerPrivArgs->stage2BuffCurrIndex];

   for (vecCount = 0; vecCount < numOutputBlocks1; vecCount++) {
      vec vecFir  = c7x::strm_eng<0, vec>::get_adv();
      vec vecOrig = c7x::strm_eng<1, vec>::get_adv();

      __vpred        opWrPred0 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr0  = reinterpret_cast<c7x::uint_vec *>(c7x::strm_agen<0, vec>::get_adv(pStage2CurrInput));
      __vstore_pred_interleave_low_low(opWrPred0, opWrPtr0, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));

      __vpred        opWrPred1 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr1  = reinterpret_cast<c7x::uint_vec *>(c7x::strm_agen<0, vec>::get_adv(pStage2CurrInput));
      __vstore_pred_interleave_high_high(opWrPred1, opWrPtr1, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   /* Step 4: Copy the last filterLength samples to the beginning of the state buffer for next time
      This creates the history for the next processing block of stage 1 */
   dsplibStatus =
       DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle1, &pStateLocal[inputSampleCount], pStateLocal);

   /* Step 5: Process stage 2 - 2x upsampling on the stage 1 output */
   // For stage 2, we're processing the output of stage 1, which is 2x the input size
   // Point to the stage 2 input buffer including history
   dataType *pStage2Input = &pStateLocal[pKerPrivArgs->stage2BuffStartIndex];
   // Point to the stage 2 output buffer
   dataType *pStage2Output = &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex];

   // Run FIR for stage 2
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle2, pStage2Input, pFilterStage2, pStage2Output);

   /* Step 6: Interleave the stage 2 FIR output with original stage 2 input samples */
   // Final output buffer for stage 2 (after interleaving)
   dataType *pStage2FinalOutput = &pStateLocal[pKerPrivArgs->scratch2BuffStartIndex];

   // Open streaming engines for stage 2 interleaving
   __SE0_OPEN(pStage2Output, se1Params);
   __SE1_OPEN(pStage2Input + AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS, se1Params);
   __SA0_OPEN(sa1Params);

   // Interleave stage 2 FIR output with original samples
   for (vecCount = 0; vecCount < numOutputBlocks2; vecCount++) {
      vec vecFir  = c7x::strm_eng<0, vec>::get_adv();
      vec vecOrig = c7x::strm_eng<1, vec>::get_adv();

      __vpred        opWrPred0 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr0  = reinterpret_cast<c7x::uint_vec *>(c7x::strm_agen<0, vec>::get_adv(pStage2FinalOutput));
      __vstore_pred_interleave_low_low(opWrPred0, opWrPtr0, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));

      __vpred        opWrPred1 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr1  = reinterpret_cast<c7x::uint_vec *>(c7x::strm_agen<0, vec>::get_adv(pStage2FinalOutput));
      __vstore_pred_interleave_high_high(opWrPred1, opWrPtr1, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   /* Step 7: Copy the last filterLength samples to the beginning of the state buffer for stage 2 */
   // Create buffer parameters for stage 2 history copy
   int32_t stage2InputSampleCount = inputSampleCount * 2;

   dsplibStatus = DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle2, &pStage2Input[stage2InputSampleCount],
                                        pStage2Input);

   /* Step 8: Transform final non-interleaved output data back to interleaved format */
   dsplibStatus = DSPLIB_interleave_exec(pKerPrivArgs->initArgs.interleaveHandle, pStage2FinalOutput, pOutLocal);

   return status;
}

/*
 * Execution for 4x upsampling with non-interleaved data format.
 * Performs block copy, FIR filtering, and interleaving for both stages.
 * Output is written in non-interleaved format.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                 void *restrict pIn,
                                                                 void *restrict pState,
                                                                 void *restrict pFiltCoeffs,
                                                                 void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_non_interleaved_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                               = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus __attribute__((unused)) = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs                         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                               = pKerPrivArgs->bufPblock;
   dataType               *pInLocal                             = (dataType *) pIn;
   dataType               *pOutLocal                            = (dataType *) pOut;
   dataType               *pStateLocal                          = (dataType *) pState;
   dataType               *pFilterStage1    = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType               *pFilterStage2    = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 numOutputBlocks1 = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2 = pKerPrivArgs->numOutputBlocks2;
   int32_t                 vecCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   /* Step 1: Copy input data to state buffer (after the filter history) */
   dsplibStatus = DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle1, pInLocal,
                                        &pStateLocal[pKerPrivArgs->stage1BuffCurrIndex]);

   /* Step 2: Run FIR on all input samples for stage 1 (gives interpolated sample) */
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle1, pStateLocal, pFilterStage1,
                                  &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);

   /* Step 3: Interleave the fir output samples with original input samples for stage 1 */
   /* Read filtered samples */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex], se0Params);
   /* Read original samples */
   __SE1_OPEN(&pStateLocal[AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS], se0Params);
   __SA0_OPEN(sa0Params);

   // Intermediate buffer for stage 1 output (2x upsampled) which is also the input for stage 2 filtering
   dataType *pStage2CurrInput = &pStateLocal[pKerPrivArgs->stage2BuffCurrIndex];

   for (vecCount = 0; vecCount < numOutputBlocks1; vecCount++) {
      vec vecFir  = c7x::strm_eng<0, vec>::get_adv();
      vec vecOrig = c7x::strm_eng<1, vec>::get_adv();

      __vpred        opWrPred0 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr0  = reinterpret_cast<c7x::uint_vec *>(c7x::strm_agen<0, vec>::get_adv(pStage2CurrInput));
      __vstore_pred_interleave_low_low(opWrPred0, opWrPtr0, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));

      __vpred        opWrPred1 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr1  = reinterpret_cast<c7x::uint_vec *>(c7x::strm_agen<0, vec>::get_adv(pStage2CurrInput));
      __vstore_pred_interleave_high_high(opWrPred1, opWrPtr1, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   /* Step 4: Copy the last filterLength samples to the beginning of the state buffer for next time
      This creates the history for the next processing block of stage 1 */
   dsplibStatus =
       DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle2, &pStateLocal[inputSampleCount], pStateLocal);

   /* Step 5: Process stage 2 - 2x upsampling on the stage 1 output */
   // For stage 2, we're processing the output of stage 1, which is 2x the input size
   // Point to the stage 2 input buffer including history
   dataType *pStage2Input = &pStateLocal[pKerPrivArgs->stage2BuffStartIndex];
   // Point to the stage 2 output buffer
   dataType *pStage2Output = &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex];

   // Run FIR for stage 2
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle2, pStage2Input, pFilterStage2, pStage2Output);

   /* Step 6: Interleave the stage 2 FIR output with original stage 2 input samples */
   // Open streaming engines for stage 2 interleaving
   __SE0_OPEN(pStage2Output, se1Params);
   __SE1_OPEN(pStage2Input + AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS, se1Params);
   __SA0_OPEN(sa1Params);

   // Interleave stage 2 FIR output with original samples
   for (vecCount = 0; vecCount < numOutputBlocks2; vecCount++) {
      vec vecFir  = c7x::strm_eng<0, vec>::get_adv();
      vec vecOrig = c7x::strm_eng<1, vec>::get_adv();

      __vpred        opWrPred0 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr0  = reinterpret_cast<c7x::uint_vec *>(c7x::strm_agen<0, vec>::get_adv(pOutLocal));
      __vstore_pred_interleave_low_low(opWrPred0, opWrPtr0, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));

      __vpred        opWrPred1 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr1  = reinterpret_cast<c7x::uint_vec *>(c7x::strm_agen<0, vec>::get_adv(pOutLocal));
      __vstore_pred_interleave_high_high(opWrPred1, opWrPtr1, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   /* Step 7: Copy the last filterLength samples to the beginning of the state buffer for stage 2 */
   // Create buffer parameters for stage 2 history copy
   int32_t stage2InputSampleCount = inputSampleCount * 2;

   dsplibStatus = DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle3, &pStage2Input[stage2InputSampleCount],
                                        pStage2Input);

   return status;
}

/*
 * Execution for 4x upsampling using block processing (circular buffer).
 * Processes input in blocks using streaming engines and agents for FIR filtering and output interleaving.
 * Handles circular buffer index updates and history block copy.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_block_exec_ci(AUDIOLIB_kernelHandle handle,
                                                       void *restrict pIn,
                                                       void *restrict pState,
                                                       void *restrict pFiltCoeffs,
                                                       void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_block_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                 = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs           = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                 = pKerPrivArgs->bufPblock;
   uint32_t               *stage1BuffStartIndex   = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t               *stage2BuffStartIndex   = &pKerPrivArgs->stage2BuffStartIndex;
   uint32_t               *stage2BuffCurrIndex    = &pKerPrivArgs->stage2BuffCurrIndex;
   uint32_t                cirBuffAddressMask     = pKerPrivArgs->cirBuffAddressMask;
   uint32_t                inBufferTotalDimX      = pKerPrivArgs->inBufferTotalDimX;
   dataType               *pInLocal               = (dataType *) pIn;
   dataType               *pOutLocal              = (dataType *) pOut;
   dataType               *pFilterStage1          = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType               *pFilterStage2          = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 numChannels            = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount       = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 stage2InputSampleCount = inputSampleCount * 2;
   int32_t                 numOutputBlocks1       = pKerPrivArgs->numOutputBlocks1;
   int32_t                 numOutputBlocks2       = pKerPrivArgs->numOutputBlocks2;
   int32_t                 numOutputBlocks3       = pKerPrivArgs->numOutputBlocks3;
   int32_t                 chCount, outputBlockCount, tapCount;
   // For interleaved mode, we use the second half of the pIn buffer as the intermediate buffer
   // This buffer is treated as a linear buffer
   dataType *pIntermediateLocal = pInLocal + inBufferTotalDimX; // Point to the second half of pIn buffer
   dataType *pStage2Input       = pIntermediateLocal + (*stage2BuffCurrIndex);

   // Use Block FIR direct interleaved mode with streaming engines
   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se4Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa3Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa4Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA4_PARAM_OFFSET);

   __SE0_OPEN(&pInLocal[*stage1BuffStartIndex], se0Params);
   __SE1_OPEN(
       &pInLocal[(*stage1BuffStartIndex + ((AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1) * numChannels)) &
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

         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS); tapCount++) {
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

         // Write first output (filtered sample)
         __vpred opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc0);

         // Get the center tap input sample for the second output
         vec vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         // Write second output (original sample)
         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc1);

         vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         opWrPred     = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr      = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc2);

         vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         opWrPred     = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr      = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc3);

         vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         opWrPred     = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr      = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);
      }
   }

   // Close streaming engines and agents
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   // Update circular buffer index for next call
   *stage1BuffStartIndex = (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   /* Stage 2 processing for upsample 4x*/
   __SE0_OPEN(&pIntermediateLocal[*stage2BuffStartIndex], se2Params);
   __SE1_OPEN(&pIntermediateLocal[(*stage2BuffStartIndex +
                                   ((AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels)) &
                                  cirBuffAddressMask],
              se3Params);
   __SA0_OPEN(sa2Params);
   __SA1_OPEN(sa3Params);

   // Process each channel block
   for (chCount = 0; chCount < numChannels; chCount += eleCount) {
      // Process each output block
      for (outputBlockCount = 0; outputBlockCount < numOutputBlocks2; outputBlockCount++) {
         // Initialize accumulators for filtered outputs
         vec vecAcc0 = (vec) 0;
         vec vecAcc1 = (vec) 0;
         vec vecAcc2 = (vec) 0;
         vec vecAcc3 = (vec) 0;

         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS); tapCount++) {
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

         // Write first output (filtered sample)
         __vpred opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc0);

         // Get the center tap input sample for the second output
         vec vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         // Write second output (original sample)
         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc1);

         vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         opWrPred     = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr      = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc2);

         vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         opWrPred     = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr      = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);

         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc3);

         vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         opWrPred     = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr      = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);
      }
   }

   // Close streaming engines and agents
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   // block copy to history buffer
   __SE0_OPEN(&pIntermediateLocal[stage2InputSampleCount * numChannels], se4Params);
   __SA0_OPEN(sa4Params);

   for (outputBlockCount = 0; outputBlockCount < numOutputBlocks3; outputBlockCount++) {
      vec     data    = c7x::strm_eng<0, vec>::get_adv();
      __vpred opWr    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<0, vec>::get_adv(pIntermediateLocal);
      __vstore_pred(opWr, opWrPtr, data);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

/*
 * Execution for 4x upsampling using single sample processing (circular buffer).
 * Processes each sample individually in two stages to achieve 4x upsampling.
 * Uses streaming engines and agents for FIR filtering and output interleaving.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_ss_exec_ci(AUDIOLIB_kernelHandle handle,
                                                    void *restrict pIn,
                                                    void *restrict pState,
                                                    void *restrict pFiltCoeffs,
                                                    void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_ss_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status                 = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs           = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock                 = pKerPrivArgs->bufPblock;
   uint32_t               *stage1BuffStartIndex   = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t               *stage2BuffStartIndex   = &pKerPrivArgs->stage2BuffStartIndex;
   uint32_t               *stage2BuffCurrIndex    = &pKerPrivArgs->stage2BuffCurrIndex;
   uint32_t                cirBuffAddressMask     = pKerPrivArgs->cirBuffAddressMask;
   uint32_t                inBufferTotalDimX      = pKerPrivArgs->inBufferTotalDimX;
   dataType               *pInLocal               = (dataType *) pIn;
   dataType               *pOutLocal              = (dataType *) pOut;
   dataType               *pFilterStage1          = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType               *pFilterStage2          = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 numChannels            = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount       = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 stage2InputSampleCount = inputSampleCount * 2;
   int32_t                 numOutputBlocks2       = pKerPrivArgs->numOutputBlocks2;
   int32_t                 numOutputBlocks3       = pKerPrivArgs->numOutputBlocks3;
   int32_t                 chCount, outputCount, tapCount;

   // Intermediate buffer for stage 1 output (2x upsampled) which is also the input for stage 2
   dataType *pIntermediateLocal = pInLocal + inBufferTotalDimX; // Point to the second half of pIn buffer
   dataType *pStage2Input       = pIntermediateLocal + (*stage2BuffCurrIndex);

   // Use single sample processing with streaming engines
   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   // Stage 1: 2x upsampling
   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se4Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa3Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa4Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA4_PARAM_OFFSET);

   __SE0_OPEN(&pInLocal[*stage1BuffStartIndex], se0Params);
   __SE1_OPEN(
       &pInLocal[(*stage1BuffStartIndex + ((AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1) * numChannels)) &
                 cirBuffAddressMask],
       se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   for (outputCount = 0; outputCount < inputSampleCount; outputCount++) {
      // Process each channel block
      for (chCount = 0; chCount < numChannels; chCount += eleCount) {
         /* Reset accumulators for stage 1 */
         vec vecAcc0 = (vec) 0;
         vec vecAcc1 = (vec) 0;
         vec vecAcc2 = (vec) 0;
         vec vecAcc3 = (vec) 0;

         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS /
                                                            AUDIOLIB_SSRC_SS_UNROLL_FACTOR);
              tapCount++) {
            /* ---- UR0 ---------------------------------------------------------- */
            /* Load next coefficient */
            vec vecCoeffs0 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
            /* Load forward input sample */
            vec vecInputF0 = c7x::strm_eng<0, vec>::get_adv();
            /* Load reverse input sample */
            vec vecInputR0 = c7x::strm_eng<1, vec>::get_adv();
            /* MAC */
            vecAcc0 += vecCoeffs0 * (vecInputF0 + vecInputR0);

            /* ---- UR1 ---------------------------------------------------------- */
            vec vecCoeffs1 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
            vec vecInputF1 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInputR1 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc1 += vecCoeffs1 * (vecInputF1 + vecInputR1);

            /* ---- UR2 ---------------------------------------------------------- */
            vec vecCoeffs2 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
            vec vecInputF2 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInputR2 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc2 += vecCoeffs2 * (vecInputF2 + vecInputR2);

            /* ---- UR3 ---------------------------------------------------------- */
            vec vecCoeffs3 = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage1));
            vec vecInputF3 = c7x::strm_eng<0, vec>::get_adv();
            vec vecInputR3 = c7x::strm_eng<1, vec>::get_adv();
            vecAcc3 += vecCoeffs3 * (vecInputF3 + vecInputR3);
         }

         // Combine partial sums for stage 1
         vecAcc0 = vecAcc0 + vecAcc1;
         vecAcc2 = vecAcc2 + vecAcc3;
         vecAcc0 = vecAcc0 + vecAcc2;

         // Write first output to intermediate buffer (filtered sample)
         __vpred opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecAcc0);

         // Get the center tap input sample for the second output
         vec vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         // Write second output to intermediate buffer (original sample)
         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pStage2Input);
         __vstore_pred(opWrPred, opWrPtr, vecCenterTap);
      }
   }

   // Close streaming engines and agents for stage 1
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   // Update circular buffer index for next call
   *stage1BuffStartIndex = (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   // Stage 2: 2x upsampling on the output of stage 1
   __SE0_OPEN(&pIntermediateLocal[*stage2BuffStartIndex], se2Params);
   __SE1_OPEN(&pIntermediateLocal[*stage2BuffStartIndex +
                                  ((AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1) * numChannels)],
              se3Params);
   __SA0_OPEN(sa2Params);
   __SA1_OPEN(sa3Params);

#pragma MUST_ITERATE(2, , 1)
   for (outputCount = 0; outputCount < numOutputBlocks2; outputCount++) {
      vec vecAcc = (vec) 0;

      /* ---- UR0 ---------------------------------------------------------- */
      /* Load next coefficient */
      vec vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      /* Load forward input sample */
      vec vecInputF = c7x::strm_eng<0, vec>::get_adv();
      /* Load reverse input sample */
      vec vecInputR = c7x::strm_eng<1, vec>::get_adv();
      /* MAC */
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR1 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR2 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR3 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR4 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR5 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR6 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR7 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR8 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      /* ---- UR9 ---------------------------------------------------------- */
      vecCoeffs = __vload_dup(c7x::strm_agen<0, dataType>::get_adv(pFilterStage2));
      vecInputF = c7x::strm_eng<0, vec>::get_adv();
      vecInputR = c7x::strm_eng<1, vec>::get_adv();
      vecAcc += vecCoeffs * (vecInputF + vecInputR);

      // Write first output to final output buffer (filtered sample)
      __vpred opWrPred = c7x::strm_agen<1, vec>::get_vpred();
      vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
      __vstore_pred(opWrPred, opWrPtr, vecAcc);

      // Get the center tap input sample for the second output
      vec vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
      // Write second output to final output buffer (original sample)
      opWrPred = c7x::strm_agen<1, vec>::get_vpred();
      opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
      __vstore_pred(opWrPred, opWrPtr, vecCenterTap);
   }

   // Close streaming engines and agents for stage 2
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   // block copy to history buffer
   __SE0_OPEN(&pIntermediateLocal[stage2InputSampleCount * numChannels], se4Params);
   __SA0_OPEN(sa4Params);

   for (outputCount = 0; outputCount < numOutputBlocks3; outputCount++) {
      vec     data    = c7x::strm_eng<0, vec>::get_adv();
      __vpred opWr    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *opWrPtr = c7x::strm_agen<0, vec>::get_adv(pIntermediateLocal);
      __vstore_pred(opWr, opWrPtr, data);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

/*******************************************************************************
 * TEMPLATE INSTANTIATIONS
 ******************************************************************************/

// Initialization function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_linear_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_block_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                       AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                       AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                       AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_ss_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                    AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                    AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                    AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_non_interleaved_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

// Execution function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_linear_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                        void *restrict pIn,
                                                                        void *restrict pState,
                                                                        void *restrict pFiltCoeffs,
                                                                        void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_block_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                       void *restrict pIn,
                                                                       void *restrict pState,
                                                                       void *restrict pFiltCoeffs,
                                                                       void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_ss_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                    void *restrict pIn,
                                                                    void *restrict pState,
                                                                    void *restrict pFiltCoeffs,
                                                                    void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_non_interleaved_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                 void *restrict pIn,
                                                                                 void *restrict pState,
                                                                                 void *restrict pFiltCoeffs,
                                                                                 void *restrict pOut);
