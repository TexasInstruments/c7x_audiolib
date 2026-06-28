// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_ssrc_priv.h"
#include <time.h>

/* N/A */

/*******************************************************************************
 * INITIALIZATION FUNCTIONS
 ******************************************************************************/
/*
 * Main initialization function for 2x upsampling using C7x intrinsics.
 * Selects the appropriate processing mode (linear, block, or single sample)
 * based on buffer format and input sample count for interleaved data format.
 * For non-interleaved data format, uses linear buffer based processing.
 * This function also handles the initialization of the execution function pointers.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   uint8_t                 bufferFormat     = pKerPrivArgs->initArgs.bufferFormat;
   uint8_t                 dataFormat       = pKerPrivArgs->initArgs.dataFormat;

   // Check data format first
   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      // For interleaved data format, check buffer format
      if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
         // For linear buffer format, use the specialized implementation
         status = AUDIOLIB_ssrc_upsample2x_linear_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (status == AUDIOLIB_SUCCESS) {
            pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample2x_linear_exec_ci<float>;
         }
      }
      else {
         // For circular buffer format, choose between block and single sample processing
         if (inputSampleCount > (int32_t) AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT) {
            // For larger sample counts, use block processing
            status = AUDIOLIB_ssrc_upsample2x_block_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample2x_block_exec_ci<float>;
            }
         }
         else {
            // For smaller sample counts, use single sample processing
            status = AUDIOLIB_ssrc_upsample2x_ss_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
            if (status == AUDIOLIB_SUCCESS) {
               pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample2x_ss_exec_ci<float>;
            }
         }
      }
   }
   else {
      // For non-interleaved data format, use the specialized implementation
      status = AUDIOLIB_ssrc_upsample2x_non_interleaved_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      if (status == AUDIOLIB_SUCCESS) {
         pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample2x_non_interleaved_exec_ci<float>;
      }
   }

   return status;
}

/*******************************************************************************
 * SPECIALIZED INITIALIZATION FUNCTIONS
 ******************************************************************************/

/*
 * Initialization for 2x upsampling with non-interleaved linear buffer format.
 * Sets up DSPLIB kernels for block copy and FIR filtering.
 * Configures streaming engine and agent parameters for upsampling.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_non_interleaved_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus     = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock           = pKerPrivArgs->bufPblock;
   int32_t                 numChannels      = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 stateStride      = pKerPrivArgs->inBufferTotalDimX;
   int32_t                 eleSize          = AUDIOLIB_sizeof(pKerInitArgs->sampleDataType);

   // Create buffer parameters for FIR
   DSPLIB_bufParams2D_t firBufParamsIn, firBufParamsOut, firBufParamsFilter;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

   /* Initialize FIR kernel parameters */
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
   /* Initialize FIR input buffer parameters */
   firBufParamsIn.data_type = DSPLIB_FLOAT32;
   firBufParamsIn.dim_x     = inputSampleCount + AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
   firBufParamsIn.dim_y     = numChannels;
   firBufParamsIn.stride_y  = pKerPrivArgs->bufParamsState.stride_y;
   /* Initialize FIR output buffer parameters */
   firBufParamsOut.data_type = DSPLIB_FLOAT32;
   firBufParamsOut.dim_x     = pKerPrivArgs->bufParamsState.dim_x;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;
   /* Initialize FIR filter buffer parameters */
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

   /* Check FIR initialization parameters */
   dsplibStatus = DSPLIB_fir_init_checkParams(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter,
                                              &firBufParamsOut, &firKerInitArgs);

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus = DSPLIB_fir_init(pKerInitArgs->firHandle1, &firBufParamsIn, &firBufParamsFilter, &firBufParamsOut,
                                     &firKerInitArgs);
   }

   // Set up buffer parameters for copying input data to state buffer
   DSPLIB_bufParams2D_t srcBufParams, dstBufParams;

   // Set up source buffer parameters
   srcBufParams.data_type = DSPLIB_FLOAT32;
   srcBufParams.dim_x     = bufParamsIn->dim_x;
   srcBufParams.dim_y     = bufParamsIn->dim_y;
   srcBufParams.stride_y  = bufParamsIn->stride_y;

   // Set up destination buffer parameters
   dstBufParams.data_type = DSPLIB_FLOAT32;
   dstBufParams.dim_x     = inputSampleCount;
   dstBufParams.dim_y     = pKerPrivArgs->initArgs.numChannels;
   dstBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   // Initialize DSPLIB blkCopy2D for copying input data to state buffer
   DSPLIB_blkCopy2DInitArgs blkCopy2DInitArgs;
   if (dsplibStatus == DSPLIB_SUCCESS) {
      blkCopy2DInitArgs.funcStyle = DSPLIB_FUNCTION_OPTIMIZED;

      dsplibStatus = DSPLIB_blkCopy2D_init_checkParams(pKerInitArgs->blkCopy2DHandle1, &srcBufParams, &dstBufParams,
                                                       &blkCopy2DInitArgs);
   }

   if (dsplibStatus == DSPLIB_SUCCESS) {
      dsplibStatus =
          DSPLIB_blkCopy2D_init(pKerInitArgs->blkCopy2DHandle1, &srcBufParams, &dstBufParams, &blkCopy2DInitArgs);
   }

   // Set up non-interleaved intermediate buffer parameters for 2d block copy
   DSPLIB_bufParams2D_t nonInterleavedBufParams;
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

   // No need for blkCopy2DHandle3 as we write directly to output buffer in exec function

   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch samples.                       */
   /**********************************************************************/
   se0Params         = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0   = inputSampleCount;
   se0Params.DIM1    = stateStride;
   se0Params.ICNT1   = numChannels;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare SA to store samples                                        */
   /**********************************************************************/
   int32_t icnt0Param  = AUDIOLIB_min(inputSampleCount * 2, eleCount);
   int32_t blkItrCount = AUDIOLIB_ceilingDiv(inputSampleCount, eleCount);

   sa0Params               = __gen_SA_TEMPLATE_v1();
   sa0Params.ICNT0         = icnt0Param;
   sa0Params.DIM1          = icnt0Param;
   sa0Params.ICNT1         = blkItrCount * 2;
   sa0Params.DIM2          = bufParamsOut->stride_y / eleSize;
   sa0Params.ICNT2         = numChannels;
   sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa0Params.DECDIM1_WIDTH = inputSampleCount * 2;
   sa0Params.VECLEN        = SA_VECLEN;
   sa0Params.DIMFMT        = __SA_DIMFMT_3D;

   pKerPrivArgs->numOutputBlocks1 = blkItrCount * numChannels;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   if (dsplibStatus != DSPLIB_SUCCESS) {
      status = AUDIOLIB_ERR_FAILURE;
   }

   return status;
}

/*
 * Initialization for 2x upsampling with linear buffer format (interleaved data).
 * Sets up DSPLIB kernels for deinterleave, FIR filtering, block copy, and interleave.
 * Configures streaming engine and agent parameters for upsampling.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_linear_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   DSPLIB_STATUS           dsplibStatus     = DSPLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock           = pKerPrivArgs->bufPblock;
   int32_t                 numChannels      = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 stateStride      = pKerPrivArgs->inBufferTotalDimX;
   int32_t                 eleSize          = AUDIOLIB_sizeof(pKerInitArgs->sampleDataType);

   // Create buffer parameters for non-interleaved input and output
   DSPLIB_bufParams2D_t interleavedBufParams, nonInterleavedBufParams;
   DSPLIB_bufParams2D_t firBufParamsIn, firBufParamsOut, firBufParamsFilter;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
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

   /* Initialize FIR kernel parameters */
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
   /* Initialize FIR input buffer parameters */
   firBufParamsIn.data_type = DSPLIB_FLOAT32;
   firBufParamsIn.dim_x     = inputSampleCount + AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
   firBufParamsIn.dim_y     = numChannels;
   firBufParamsIn.stride_y  = pKerPrivArgs->bufParamsState.stride_y;
   /* Initialize FIR output buffer parameters */
   firBufParamsOut.data_type = DSPLIB_FLOAT32;
   firBufParamsOut.dim_x     = pKerPrivArgs->bufParamsState.dim_x;
   firBufParamsOut.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   firBufParamsOut.stride_y  = pKerPrivArgs->bufParamsState.stride_y;
   /* Initialize FIR filter buffer parameters */
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

   // Set up non-interleaved intermediate buffer parameters
   nonInterleavedBufParams.data_type = DSPLIB_FLOAT32;
   nonInterleavedBufParams.dim_x     = pKerPrivArgs->bufParamsState.dim_x * 2;
   nonInterleavedBufParams.dim_y     = pKerPrivArgs->bufParamsState.dim_y;
   nonInterleavedBufParams.stride_y  = pKerPrivArgs->bufParamsState.stride_y;

   // Set up interleaved input buffer parameters
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
   /* Prepare streaming engine 0 to fetch samples.                       */
   /**********************************************************************/
   se0Params         = __gen_SE_TEMPLATE_v1();
   se0Params.ICNT0   = inputSampleCount;
   se0Params.DIM1    = stateStride;
   se0Params.ICNT1   = numChannels;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_2D;

   /**********************************************************************/
   /* Prepare SA to store samples                                        */
   /**********************************************************************/
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

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   if (dsplibStatus != DSPLIB_SUCCESS) {
      status = AUDIOLIB_ERR_FAILURE;
   }

   return status;
}

/*
 * Initialization for 2x upsampling using block processing (circular buffer).
 * Used when inputSampleCount exceeds threshold. Sets up streaming engines and agents
 * for multi-channel block FIR filtering and output interleaving.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_block_init_ci(AUDIOLIB_kernelHandle   handle,
                                                       AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                       AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                       AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_block_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock           = pKerPrivArgs->bufPblock;
   int32_t                 numChannels      = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   int32_t          eleCount   = c7x::element_count_of<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   pKerPrivArgs->numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   int32_t icnt0Param             = AUDIOLIB_min(numChannels, eleCount);

   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(inputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   /**********************************************************************/
   /* Prepare streaming engine 0 to fetch forward samples.               */
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
   /* Prepare streaming engine 1 to fetch reverse samples.               */
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
   /* Prepare SA to fetch the filter coefficients                        */
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
   /* Prepare SA template to store output                                */
   /**********************************************************************/
   sa1Params       = __gen_SA_TEMPLATE_v1();
   sa1Params.ICNT0 = icnt0Param;
   sa1Params.DIM1  = numChannels;
   sa1Params.ICNT1 =
       pKerPrivArgs->numOutputBlocks1 * AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK * 2; // 2 for including center tap
   sa1Params.DIM2          = eleCount;
   sa1Params.ICNT2         = pKerPrivArgs->numChannelBlocks;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa1Params.DECDIM1_WIDTH = numChannels * pKerPrivArgs->outputSampleCount;
   sa1Params.DECDIM2       = __SA_DECDIM_DIM2;
   sa1Params.DECDIM2_WIDTH = numChannels;
   sa1Params.VECLEN        = SA_VECLEN;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   return status;
}

/*
 * Initialization for 2x upsampling using single sample processing (circular buffer).
 * Used when inputSampleCount is below threshold. Sets up streaming engines and agents
 * for per-sample FIR filtering and output interleaving.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_ss_init_ci(AUDIOLIB_kernelHandle   handle,
                                                    AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                    AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                    AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_ss_init_ci\n");
#endif

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                *pBlock           = pKerPrivArgs->bufPblock;
   int32_t                 numChannels      = (int32_t) pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params; // =__gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params; // =__gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params; // =__gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params; // =__gen_SA_TEMPLATE_v1();
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
   /* Prepare streaming engine 0 to fetch forward samples.               */
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
   /* Prepare streaming engine 1 to fetch reverse samples.               */
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
   /* Prepare SA to fetch the filter coefficients                        */
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
   /* Prepare SA template to store output                                */
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

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   return status;
}

/*******************************************************************************
 * EXECUTION FUNCTIONS
 ******************************************************************************/

/*
 * Execution for 2x upsampling with linear buffer format (interleaved data).
 * Performs deinterleave, FIR filtering, interleaving, and block copy.
 * Final output is re-interleaved.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pState,
                                                        void *restrict pFiltCoeffs,
                                                        void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_linear_exec_ci\n");
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
   int32_t                 numOutputBlocks                      = pKerPrivArgs->numOutputBlocks1;
   int32_t                 vecCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   /* Step 1: Transform interleaved input data to non-interleaved format using deinterleave kernel and store input
   samples to state buffer (after the filter history) */
   dsplibStatus = DSPLIB_deinterleave_exec(pKerPrivArgs->initArgs.deinterleaveHandle, pInLocal,
                                           &pStateLocal[pKerPrivArgs->stage1BuffCurrIndex]);

   /* Step 2: Run FIR on all input samples (gives interpolated sample) */
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle1, &pStateLocal[pKerPrivArgs->stage1BuffStartIndex],
                                  pFilterLocal, &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);
   /* Step 3: Interleave the fir output samples with original input samples */
   /* Read filtered samples */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex], se0Params);
   /* Read original samples */
   __SE1_OPEN(&pStateLocal[pKerPrivArgs->stage1BuffStartIndex + AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS],
              se0Params);
   __SA0_OPEN(sa0Params);

   for (vecCount = 0; vecCount < numOutputBlocks; vecCount++) {
      vec vecFir  = c7x::strm_eng<0, vec>::get_adv();
      vec vecOrig = c7x::strm_eng<1, vec>::get_adv();

      __vpred        opWrPred0 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr0  = reinterpret_cast<c7x::uint_vec *>(
          c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->scratch2BuffStartIndex]));
      __vstore_pred_interleave_low_low(opWrPred0, opWrPtr0, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));

      __vpred        opWrPred1 = c7x::strm_agen<0, vec>::get_vpred();
      c7x::uint_vec *opWrPtr1  = reinterpret_cast<c7x::uint_vec *>(
          c7x::strm_agen<0, vec>::get_adv(&pStateLocal[pKerPrivArgs->scratch2BuffStartIndex]));
      __vstore_pred_interleave_high_high(opWrPred1, opWrPtr1, c7x::as_uint_vec(vecFir), c7x::as_uint_vec(vecOrig));
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   /* Step 4: Transform non-interleaved output data back to interleaved format using interleave kernel */
   dsplibStatus = DSPLIB_interleave_exec(pKerPrivArgs->initArgs.interleaveHandle,
                                         &pStateLocal[pKerPrivArgs->scratch2BuffStartIndex], pOutLocal);

   /* Step 5: Copy the last filterLength samples to the beginning of the state buffer for next time
      This creates the history for the next processing block */
   dsplibStatus =
       DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle1, &pStateLocal[inputSampleCount], pStateLocal);

   return status;
}

/*
 * Execution for 2x upsampling using block processing (circular buffer).
 * Processes input in blocks using streaming engines and agents for FIR filtering and output interleaving.
 * Handles circular buffer index updates.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_block_exec_ci(AUDIOLIB_kernelHandle handle,
                                                       void *restrict pIn,
                                                       void *restrict pState,
                                                       void *restrict pFiltCoeffs,
                                                       void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_block_exec_ci\n");
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
   int32_t                 numOutputBlocks      = pKerPrivArgs->numOutputBlocks1;
   int32_t                 chCount, outputBlockCount, tapCount;

   // Use Block FIR direct interleaved mode with streaming engines
   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

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
      for (outputBlockCount = 0; outputBlockCount < numOutputBlocks; outputBlockCount++) {
         // Initialize accumulators for filtered outputs
         vec vecAcc0 = (vec) 0;
         vec vecAcc1 = (vec) 0;
         vec vecAcc2 = (vec) 0;
         vec vecAcc3 = (vec) 0;

         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS); tapCount++) {
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

   // Update circular buffer index for next call
   *stage1BuffStartIndex = (*stage1BuffStartIndex + inputSampleCount * numChannels) & cirBuffAddressMask;

   return status;
}

/*
 * Execution for 2x upsampling using single sample processing (circular buffer).
 * Processes each sample individually to achieve 2x upsampling.
 * Uses streaming engines and agents for FIR filtering and output interleaving.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_ss_exec_ci(AUDIOLIB_kernelHandle handle,
                                                    void *restrict pIn,
                                                    void *restrict pState,
                                                    void *restrict pFiltCoeffs,
                                                    void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_ss_exec_ci\n");
#endif

   AUDIOLIB_STATUS         status               = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs         = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint32_t               *stage1BuffStartIndex = &pKerPrivArgs->stage1BuffStartIndex;
   uint32_t                cirBuffAddressMask   = pKerPrivArgs->cirBuffAddressMask;
   dataType               *pInLocal             = (dataType *) pIn;
   dataType               *pOutLocal            = (dataType *) pOut;
   const dataType         *pFilterLocal         = (dataType *) pFiltCoeffs;
   int32_t                 numChannels          = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount     = pKerPrivArgs->initArgs.inputSampleCount;
   uint8_t                *pBlock               = pKerPrivArgs->bufPblock;
   int32_t                 chCount, outputCount, tapCount;

   // Use Block FIR direct interleaved mode with streaming engines
   typedef typename c7x::make_full_vector<dataType>::type vec;
   int32_t                                                eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

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
         /* Reset accumulators */
         vec vecAcc0 = (vec) 0;
         vec vecAcc1 = (vec) 0;
         vec vecAcc2 = (vec) 0;
         vec vecAcc3 = (vec) 0;

         for (tapCount = 0; tapCount < static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS /
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

         // Write output – use streaming engine 1 as the destination
         __vpred opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         vec    *opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
         __vstore_pred(opWrPred, opWrPtr, vecAcc0);

         // Get the center tap input sample for the second output
         vec vecCenterTap = c7x::strm_eng<0, vec>::get_adv();
         // Write second output (original sample)
         opWrPred = c7x::strm_agen<1, vec>::get_vpred();
         opWrPtr  = c7x::strm_agen<1, vec>::get_adv(pOutLocal);
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

   return status;
}

/*
 * Execution for 2x upsampling with non-interleaved linear buffer format.
 * Performs block copy, FIR filtering, and interleaving.
 * Output is written in non-interleaved format.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                 void *restrict pIn,
                                                                 void *restrict pState,
                                                                 void *restrict pFiltCoeffs,
                                                                 void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_non_interleaved_exec_ci\n");
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
   int32_t                 numOutputBlocks                      = pKerPrivArgs->numOutputBlocks1;
   int32_t                 vecCount;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   /* Step 1: Copy input data to state buffer (after the filter history) */
   // Since data is already non-interleaved, use DSPLIB_blkCopy2D_exec
   // Use blkCopy2D to copy input data to state buffer - parameters were already set in init function
   dsplibStatus = DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle1, pInLocal,
                                        &pStateLocal[pKerPrivArgs->stage1BuffCurrIndex]);

   /* Step 2: Run FIR on all input samples (gives interpolated sample) */
   dsplibStatus = DSPLIB_fir_exec(pKerPrivArgs->initArgs.firHandle1, &pStateLocal[pKerPrivArgs->stage1BuffStartIndex],
                                  pFilterLocal, &pStateLocal[pKerPrivArgs->scratch1BuffStartIndex]);

   /* Step 3: Interleave the fir output samples with original input samples directly to output buffer */
   /* Read filtered samples */
   __SE0_OPEN(&pStateLocal[pKerPrivArgs->scratch1BuffStartIndex], se0Params);
   /* Read original samples */
   __SE1_OPEN(&pStateLocal[pKerPrivArgs->stage1BuffStartIndex + AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS],
              se0Params);
   __SA0_OPEN(sa0Params);

   for (vecCount = 0; vecCount < numOutputBlocks; vecCount++) {
      vec vecFir  = c7x::strm_eng<0, vec>::get_adv();
      vec vecOrig = c7x::strm_eng<1, vec>::get_adv();

      // Write directly to output buffer instead of scratch buffer
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

   /* Step 5: Copy the last filterLength samples to the beginning of the state buffer for next time
      This creates the history for the next processing block */
   dsplibStatus =
       DSPLIB_blkCopy2D_exec(pKerPrivArgs->initArgs.blkCopy2DHandle2, &pStateLocal[inputSampleCount], pStateLocal);

   return status;
}

/*******************************************************************************
 * TEMPLATE INSTANTIATIONS
 ******************************************************************************/

// Initialization function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_block_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                       AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                       AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                       AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_ss_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                    AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                    AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                    AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

// Execution function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                        void *restrict pIn,
                                                                        void *restrict pState,
                                                                        void *restrict pFiltCoeffs,
                                                                        void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_block_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                       void *restrict pIn,
                                                                       void *restrict pState,
                                                                       void *restrict pFiltCoeffs,
                                                                       void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_ss_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                    void *restrict pIn,
                                                                    void *restrict pState,
                                                                    void *restrict pFiltCoeffs,
                                                                    void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                 void *restrict pIn,
                                                                                 void *restrict pState,
                                                                                 void *restrict pFiltCoeffs,
                                                                                 void *restrict pOut);
