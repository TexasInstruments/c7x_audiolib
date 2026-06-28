// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_ssrc_priv.h"
#include <string.h>

/* N/A */

/*******************************************************************************
 * INITIALIZATION FUNCTIONS
 ******************************************************************************/
/*
 * Main initialization function for 4x upsampling using A53 NEON intrinsics.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_upsample4x_init_ci\r\n"));
#endif

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                 bufferFormat = pKerPrivArgs->initArgs.bufferFormat;
   uint8_t                 dataFormat   = pKerPrivArgs->initArgs.dataFormat;

   // Check data format first
   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
         status = AUDIOLIB_ssrc_upsample4x_linear_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (status == AUDIOLIB_SUCCESS) {
            pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample4x_linear_exec_ci<float>;
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

/*
 * Initialization for 4x upsampling using linear processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_upsample4x_linear_init_ci\r\n"));
#endif
   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   // Stage 1: 2x upsampling
   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(inputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   int32_t stage2InputSampleCount = inputSampleCount * 2; // Output of stage 1
   // Stage 2: 2x upsampling on the output of stage 1
   pKerPrivArgs->numOutputBlocks2 = AUDIOLIB_ceilingDiv(stage2InputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample4x_non_interleaved_init_ci\n");
#endif
   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   // Stage 1: 2x upsampling
   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(inputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   int32_t stage2InputSampleCount = inputSampleCount * 2; // Output of stage 1
   // Stage 2: 2x upsampling on the output of stage 1
   pKerPrivArgs->numOutputBlocks2 = AUDIOLIB_ceilingDiv(stage2InputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   return status;
}

/*
 * Execution for 4x upsampling using linear processing .
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pState,
                                                        void *restrict pFiltCoeffs,
                                                        void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_upsample4x_linear_exec_ci\r\n"));
#endif
   AUDIOLIB_STATUS         status             = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs       = (AUDIOLIB_ssrc_PrivArgs *) handle;
   dataType               *pInLocal           = (dataType *) pIn;
   dataType               *pOutLocal          = (dataType *) pOut;
   const dataType         *pFilterCoeffsLocal = (dataType *) pFiltCoeffs;
   const dataType         *pFilterStage1      = pFilterCoeffsLocal + pKerPrivArgs->stage1FiltCoeffsOffset;
   const dataType         *pFilterStage2      = pFilterCoeffsLocal + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 numChannels        = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount   = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 stage1Samples      = inputSampleCount * 2;
   int32_t                 numOutputBlocks1   = pKerPrivArgs->numOutputBlocks1; // From init
   int32_t                 numOutputBlocks2   = pKerPrivArgs->numOutputBlocks2; // From init

   /* Upsampling Filter parameters */
   const int32_t taps1       = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS;
   const int32_t taps1Sym    = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
   const int32_t historyLen1 = (taps1Sym - 1) * numChannels;

   const int32_t taps2       = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS;
   const int32_t taps2Sym    = AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
   const int32_t historyLen2 = (taps2Sym - 1) * numChannels;

   /* Buffer sizes */
   const int32_t totalInput  = inputSampleCount * numChannels;
   const int32_t totalStage1 = stage1Samples * numChannels;

   /* Use the pre-allocated state buffer matching getStateBufferParams layout */
   dataType *pStateBuffer = (dataType *) pState;

   /* Stage 1: data buffer contains history1 + input space */
   int32_t   stage1DataBufferSize = historyLen1 + totalInput;
   dataType *pStage1DataBuffer    = pStateBuffer;
   dataType *pHistory1            = pStage1DataBuffer; // First historyLen1 elements

   /* Stage 2: data buffer contains history2 + stage1 output space */
   dataType *pStage2DataBuffer = pStateBuffer + stage1DataBufferSize;
   dataType *pHistory2         = pStage2DataBuffer; // First historyLen2 elements

   /* Scratch buffers point to positions after history in each stage buffer */
   dataType *pScratch1 = pStage1DataBuffer + historyLen1; // Points to where new input goes
   dataType *pScratch2 = pStage2DataBuffer + historyLen2; // Points to where stage1 output goes

   /* Copy new input to scratch1 position (after history in stage1 buffer) */
   memcpy(pScratch1, pInLocal, totalInput * sizeof(dataType));

   /* Stage 1 output will go directly to pScratch2 (after history2) */
   dataType *pIntermediate = pScratch2;

   const int32_t nc2      = numChannels * 2;
   const int32_t nc3      = numChannels * 3;
   const int32_t eleCount = 4;

   /* Vectorizable channel count */
   int32_t vecLimit = (numChannels / eleCount) * eleCount;

   /* ---------------------------------------------------------
    * STAGE 1 : 2× UPSAMPLING (INPUT → INTERMEDIATE)
    * --------------------------------------------------------- */

   /* LOOP 1: VECTORIZED (4 channels at a time) using ARM NEON Intrinsics */
   for (int32_t ch = 0; ch < vecLimit; ch += eleCount) {
      for (int32_t outB = 0; outB < numOutputBlocks1; ++outB) {
         /*
          * Initialize four accumulators (acc0, acc1, acc2, acc3) to zero
          */
         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         /* Calculate block offset - indexes into pStage1DataBuffer which contains [HISTORY][INPUT] */
         uint32_t blockOffset = (outB * eleCount) * numChannels + ch;

         /* Base pointers for forward and reverse */
         dataType *ptrF_Base = &pStage1DataBuffer[blockOffset];
         dataType *ptrR_Base = &pStage1DataBuffer[blockOffset + (taps1Sym - 1) * numChannels];

         /* Lead pointers */
         dataType *ptrF_Lead = ptrF_Base + nc3;
         dataType *ptrR_Lead = ptrR_Base;

         /* Load initial forward data */
         float32x4_t f0 = vld1q_f32(ptrF_Base);
         float32x4_t f1 = vld1q_f32(ptrF_Base + numChannels);
         float32x4_t f2 = vld1q_f32(ptrF_Base + (numChannels * 2));

         /* Load initial reverse data */
         float32x4_t r0 = vld1q_f32(ptrR_Base + numChannels);
         float32x4_t r1 = vld1q_f32(ptrR_Base + (numChannels * 2));
         float32x4_t r2 = vld1q_f32(ptrR_Base + (numChannels * 3));

         /* Main filter loop */
         for (int32_t tap = 0; tap < taps1; tap++) {
            /* Load coefficients for the current tap */
            float32x4_t coeff = vld1q_dup_f32(&pFilterStage1[tap]);

            /* Load input data for the current tap */
            float32x4_t f_new = vld1q_f32(ptrF_Lead);
            float32x4_t r_new = vld1q_f32(ptrR_Lead);

            /* Accumulate the output values for the current tap */
            acc0 = vmlaq_f32(acc0, coeff, vaddq_f32(f0, r_new));
            acc1 = vmlaq_f32(acc1, coeff, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff, vaddq_f32(f_new, r2));

            /* Update the data registers */
            f0 = f1;
            f1 = f2;
            f2 = f_new;
            r2 = r1;
            r1 = r0;
            r0 = r_new;

            ptrF_Lead += numChannels;
            ptrR_Lead -= numChannels;
         }

         /* Output Handling - interleave filtered and center tap samples */
         const int32_t symTapsDiv2 = taps1Sym >> 1;
         uint32_t      dstIdx      = (outB * eleCount) * nc2 + ch;
         uint32_t      cIdx        = blockOffset + symTapsDiv2 * numChannels;

         /* Store filtered output and center tap for each of 4 blocks */
         vst1q_f32(&pIntermediate[dstIdx], acc0);
         vst1q_f32(&pIntermediate[dstIdx + numChannels], vld1q_f32(&pStage1DataBuffer[cIdx]));

         cIdx += numChannels;
         vst1q_f32(&pIntermediate[dstIdx + nc2], acc1);
         vst1q_f32(&pIntermediate[dstIdx + nc2 + numChannels], vld1q_f32(&pStage1DataBuffer[cIdx]));

         cIdx += numChannels;
         vst1q_f32(&pIntermediate[dstIdx + (nc2 << 1)], acc2);
         vst1q_f32(&pIntermediate[dstIdx + (nc2 << 1) + numChannels], vld1q_f32(&pStage1DataBuffer[cIdx]));

         cIdx += numChannels;
         vst1q_f32(&pIntermediate[dstIdx + nc2 * 3], acc3);
         vst1q_f32(&pIntermediate[dstIdx + nc2 * 3 + numChannels], vld1q_f32(&pStage1DataBuffer[cIdx]));
      }
   }

   /*
    * LOOP 2: SCALAR REMAINDER (Handles remaining odd channels 1-by-1)
    */
   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      for (int32_t outB = 0; outB < numOutputBlocks1; outB++) {
         /* Initialize accumulator variables */
         float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;

         /* Calculate the block offset */
         uint32_t blockOffset = (outB * eleCount) * numChannels + ch;

         /* Base pointers */
         dataType *ptrF_Base = &pStage1DataBuffer[blockOffset];
         dataType *ptrR_Base = &pStage1DataBuffer[blockOffset + (taps1Sym - 1) * numChannels];

         /* Lead pointers */
         dataType *ptrF_Lead = ptrF_Base + nc3;
         dataType *ptrR_Lead = ptrR_Base;

         /* Load the input data for the current output block */
         float f0 = ptrF_Base[0];
         float f1 = ptrF_Base[numChannels];
         float f2 = ptrF_Base[numChannels * 2];

         float r0 = ptrR_Base[numChannels];
         float r1 = ptrR_Base[numChannels * 2];
         float r2 = ptrR_Base[numChannels * 3];

         /* Compute the output values for the current output block */
         for (int32_t tap = 0; tap < taps1; tap++) {
            float coeff = pFilterStage1[tap];

            float f_new = ptrF_Lead[0];
            float r_new = ptrR_Lead[0];

            // Math is simple scalar multiply-add
            acc0 += coeff * (f0 + r_new);
            acc1 += coeff * (f1 + r0);
            acc2 += coeff * (f2 + r1);
            acc3 += coeff * (f_new + r2);

            // Shift History
            f0 = f1;
            f1 = f2;
            f2 = f_new;
            r2 = r1;
            r1 = r0;
            r0 = r_new;

            ptrF_Lead += numChannels;
            ptrR_Lead -= numChannels;
         }

         /* Output Handling (Scalar) */
         const int32_t symTapsDiv2 = taps1Sym >> 1;
         uint32_t      cIdx        = blockOffset + symTapsDiv2 * numChannels;
         uint32_t      dstIdx      = (outB * eleCount) * nc2 + ch;

         // Block 0
         pIntermediate[dstIdx]               = acc0;
         pIntermediate[dstIdx + numChannels] = pStage1DataBuffer[cIdx];

         // Block 1
         cIdx += numChannels;
         pIntermediate[dstIdx + nc2]               = acc1;
         pIntermediate[dstIdx + nc2 + numChannels] = pStage1DataBuffer[cIdx];

         // Block 2
         cIdx += numChannels;
         pIntermediate[dstIdx + (nc2 << 1)]               = acc2;
         pIntermediate[dstIdx + (nc2 << 1) + numChannels] = pStage1DataBuffer[cIdx];

         // Block 3
         cIdx += numChannels;
         pIntermediate[dstIdx + nc2 * 3]               = acc3;
         pIntermediate[dstIdx + nc2 * 3 + numChannels] = pStage1DataBuffer[cIdx];
      }
   }

   /* =========================================================
    * STAGE 2 : 2× UPSAMPLING (INTERMEDIATE → OUTPUT)
    * ========================================================= */

   for (int32_t ch = 0; ch < vecLimit; ch += eleCount) {
      for (int32_t outB = 0; outB < numOutputBlocks2; ++outB) {
         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         uint32_t blockOffset = (outB * eleCount) * numChannels + ch;

         /* Base pointers for forward and reverse */
         dataType *ptrF_Base = &pStage2DataBuffer[blockOffset];
         dataType *ptrR_Base = &pStage2DataBuffer[blockOffset + (taps2Sym - 1) * numChannels];

         /* Lead pointers */
         dataType *ptrF_Lead = ptrF_Base + nc3;
         dataType *ptrR_Lead = ptrR_Base;

         float32x4_t f0 = vld1q_f32(ptrF_Base);
         float32x4_t f1 = vld1q_f32(ptrF_Base + numChannels);
         float32x4_t f2 = vld1q_f32(ptrF_Base + (numChannels * 2));

         float32x4_t r0 = vld1q_f32(ptrR_Base + numChannels);
         float32x4_t r1 = vld1q_f32(ptrR_Base + (numChannels * 2));
         float32x4_t r2 = vld1q_f32(ptrR_Base + (numChannels * 3));

         /* Main filter loop */
         for (int32_t tap = 0; tap < taps2; tap++) {
            float32x4_t coeff = vld1q_dup_f32(&pFilterStage2[tap]);

            float32x4_t f_new = vld1q_f32(ptrF_Lead);
            float32x4_t r_new = vld1q_f32(ptrR_Lead);

            acc0 = vmlaq_f32(acc0, coeff, vaddq_f32(f0, r_new));
            acc1 = vmlaq_f32(acc1, coeff, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff, vaddq_f32(f_new, r2));

            f0 = f1;
            f1 = f2;
            f2 = f_new;
            r2 = r1;
            r1 = r0;
            r0 = r_new;

            ptrF_Lead += numChannels;
            ptrR_Lead -= numChannels;
         }

         /* Output to final output buffer */
         float        *pOutBase    = &pOutLocal[(outB * eleCount) * nc2 + ch];
         const int32_t symTapsDiv2 = taps2Sym >> 1;
         uint32_t      cIdx        = blockOffset + symTapsDiv2 * numChannels;

         vst1q_f32(pOutBase, acc0);
         vst1q_f32(pOutBase + numChannels, vld1q_f32(&pStage2DataBuffer[cIdx]));

         cIdx += numChannels;
         vst1q_f32(pOutBase + nc2, acc1);
         vst1q_f32(pOutBase + nc2 + numChannels, vld1q_f32(&pStage2DataBuffer[cIdx]));

         cIdx += numChannels;
         vst1q_f32(pOutBase + (nc2 << 1), acc2);
         vst1q_f32(pOutBase + (nc2 << 1) + numChannels, vld1q_f32(&pStage2DataBuffer[cIdx]));

         cIdx += numChannels;
         vst1q_f32(pOutBase + nc2 * 3, acc3);
         vst1q_f32(pOutBase + nc2 * 3 + numChannels, vld1q_f32(&pStage2DataBuffer[cIdx]));
      }
   }

   // =========================================================================
   // LOOP 2: SCALAR REMAINDER (Handles remaining odd channels 1-by-1)
   // =========================================================================
   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      for (int32_t outB = 0; outB < numOutputBlocks2; outB++) {
         /* Initialize Accumulators */
         float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;

         /* Setup Base Indices */
         uint32_t blockOffset = (outB * eleCount) * numChannels + ch;

         /* Base pointers */
         dataType *ptrF_Base = &pStage2DataBuffer[blockOffset];
         dataType *ptrR_Base = &pStage2DataBuffer[blockOffset + (taps2Sym - 1) * numChannels];

         /* Lead pointers */
         dataType *ptrF_Lead = ptrF_Base + nc3;
         dataType *ptrR_Lead = ptrR_Base;

         /* Load Initial Scalars */
         float f0 = ptrF_Base[0];
         float f1 = ptrF_Base[numChannels];
         float f2 = ptrF_Base[numChannels * 2];

         float r0 = ptrR_Base[numChannels];
         float r1 = ptrR_Base[numChannels * 2];
         float r2 = ptrR_Base[numChannels * 3];

         /* Main Filter Loop (Scalar) */
         for (int32_t tap = 0; tap < taps2; tap++) {
            float coeff = pFilterStage2[tap];

            float f_new = ptrF_Lead[0];
            float r_new = ptrR_Lead[0];

            // Math is simple scalar multiply-add
            acc0 += coeff * (f0 + r_new);
            acc1 += coeff * (f1 + r0);
            acc2 += coeff * (f2 + r1);
            acc3 += coeff * (f_new + r2);

            // Shift History
            f0 = f1;
            f1 = f2;
            f2 = f_new;
            r2 = r1;
            r1 = r0;
            r0 = r_new;

            ptrF_Lead += numChannels;
            ptrR_Lead -= numChannels;
         }

         /* Output Handling (Scalar) */
         float *pOutBase = &pOutLocal[(outB * eleCount) * nc2 + ch];

         const int32_t symTapsDiv2 = taps2Sym >> 1;
         uint32_t      cIdx        = blockOffset + symTapsDiv2 * numChannels;

         // Block 0
         pOutBase[0]           = acc0;
         pOutBase[numChannels] = pStage2DataBuffer[cIdx];

         // Block 1
         cIdx += numChannels;
         pOutBase[nc2]               = acc1;
         pOutBase[nc2 + numChannels] = pStage2DataBuffer[cIdx];

         // Block 2
         cIdx += numChannels;
         pOutBase[nc2 * 2]               = acc2;
         pOutBase[nc2 * 2 + numChannels] = pStage2DataBuffer[cIdx];

         // Block 3
         cIdx += numChannels;
         pOutBase[nc2 * 3]               = acc3;
         pOutBase[nc2 * 3 + numChannels] = pStage2DataBuffer[cIdx];
      }
   }

   /* Update history buffers - save the last historyLen samples */
   memcpy(pHistory1, pScratch1 + totalInput - historyLen1, historyLen1 * sizeof(dataType));
   memcpy(pHistory2, pScratch2 + totalStage1 - historyLen2, historyLen2 * sizeof(dataType));

   return status;
}

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
   // =============================================================================
   // SSRC 4x Upsampling Kernel — Two Cascaded 2x Stages (ARM NEON)
   // =============================================================================

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;
   dataType               *pNewInput    = (dataType *) pIn;
   dataType               *pStateTyped  = (dataType *) pState;
   dataType               *pOutLocal    = (dataType *) pOut;
   // Filter coefficient arrays are stored back-to-back; offsets select each stage
   dataType *pFilterStage1    = (dataType *) pFiltCoeffs + pKerPrivArgs->stage1FiltCoeffsOffset;
   dataType *pFilterStage2    = (dataType *) pFiltCoeffs + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t   numChannels      = pKerPrivArgs->initArgs.numChannels;
   int32_t   inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;

   // Filter constants - Stage 1
   const int32_t filterTapsStage1        = static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS);
   const int32_t halfTapsStage1          = static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC);
   const int32_t historyNumSamplesStage1 = halfTapsStage1 - 1;

   // Filter constants - Stage 2
   const int32_t filterTapsStage2        = static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS);
   const int32_t halfTapsStage2          = static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC);
   const int32_t historyNumSamplesStage2 = halfTapsStage2 - 1;

   const int32_t stage1OutputSampleCount = inputSampleCount * 2;
   const int32_t stage2OutputSampleCount = stage1OutputSampleCount * 2;

   // ============================================================================
   // State buffer layout (similar to 2x approach):
   // [stage1Buff: history + input]
   // [stage2Buff: history + stage1 output]
   // [scratch1Buff: FIR output for stage 1]
   // [scratch2Buff: FIR output for stage 2 - can reuse scratch1]
   // ============================================================================

   // Calculate buffer indices (per channel, non-interleaved) - matching 2x pattern
   int32_t stage1BuffStartIndex = 0;

   // Stage 2 buffer starts after Stage 1 buffer
   int32_t stage2BuffStartIndex = numChannels * (historyNumSamplesStage1 + inputSampleCount);

   // Scratch buffer starts after Stage 2 buffer
   int32_t scratch1BuffStartIndex =
       stage2BuffStartIndex + numChannels * (historyNumSamplesStage2 + stage1OutputSampleCount);

   dataType *pStage1Buff   = &pStateTyped[stage1BuffStartIndex];
   dataType *pStage2Buff   = &pStateTyped[stage2BuffStartIndex];
   dataType *pScratch1Buff = &pStateTyped[scratch1BuffStartIndex];
   dataType *pScratch2Buff = pScratch1Buff; // Reuse scratch1 for stage 2

   // NEON processing constants
   const int32_t eleCount          = 4;
   const int32_t numCompleteBlocks = inputSampleCount / eleCount;
   const int32_t remainingSamples  = inputSampleCount % eleCount;
   const int32_t veclimit          = (numChannels / 4) * 4;

   // ============================================================================
   // STAGE 1: First 2x Upsampling
   // ============================================================================

   // Step 1: Copy input data to stage 1 state buffer (after the filter history)
   for (int32_t ch = 0; ch < numChannels; ch++) {
      memcpy(&pStage1Buff[ch * (historyNumSamplesStage1 + inputSampleCount) + historyNumSamplesStage1],
             &pNewInput[ch * inputSampleCount], inputSampleCount * sizeof(dataType));
   }

   // Step 2: Run FIR on all input samples for stage 1

   // PATH A: NEON - Process 4 channels simultaneously for Stage 1
   for (int32_t chGroup = 0; chGroup < veclimit; chGroup += 4) {

      dataType *pCh0Timeline = &pStage1Buff[(chGroup + 0) * (historyNumSamplesStage1 + inputSampleCount)];
      dataType *pCh1Timeline = &pStage1Buff[(chGroup + 1) * (historyNumSamplesStage1 + inputSampleCount)];
      dataType *pCh2Timeline = &pStage1Buff[(chGroup + 2) * (historyNumSamplesStage1 + inputSampleCount)];
      dataType *pCh3Timeline = &pStage1Buff[(chGroup + 3) * (historyNumSamplesStage1 + inputSampleCount)];

      dataType *pCh0Scratch = &pScratch1Buff[(chGroup + 0) * inputSampleCount];
      dataType *pCh1Scratch = &pScratch1Buff[(chGroup + 1) * inputSampleCount];
      dataType *pCh2Scratch = &pScratch1Buff[(chGroup + 2) * inputSampleCount];
      dataType *pCh3Scratch = &pScratch1Buff[(chGroup + 3) * inputSampleCount];

      // Process complete blocks of 4 samples
      for (int32_t blk = 0; blk < numCompleteBlocks; blk++) {

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         int32_t s0 = blk * 4 + 0;
         int32_t s1 = blk * 4 + 1;
         int32_t s2 = blk * 4 + 2;
         int32_t s3 = blk * 4 + 3;

         // Preload forward samples (sliding window optimization)
         float32x4_t f0 = {pCh0Timeline[s0], pCh1Timeline[s0], pCh2Timeline[s0], pCh3Timeline[s0]};
         float32x4_t f1 = {pCh0Timeline[s1], pCh1Timeline[s1], pCh2Timeline[s1], pCh3Timeline[s1]};
         float32x4_t f2 = {pCh0Timeline[s2], pCh1Timeline[s2], pCh2Timeline[s2], pCh3Timeline[s2]};

         int32_t fwdIdx = s0 + 3;

         // Symmetric FIR loop (2x unrolled)
         for (int32_t tap = 0; tap < filterTapsStage1; tap += 2) {

            // TAP 0
            float32x4_t coeff0 = vld1q_dup_f32(&pFilterStage1[tap]);
            float32x4_t f_new0 = {pCh0Timeline[fwdIdx], pCh1Timeline[fwdIdx], pCh2Timeline[fwdIdx],
                                  pCh3Timeline[fwdIdx]};

            int32_t     revIdx0 = s0 + (halfTapsStage1 - 1 - tap);
            float32x4_t r0_0    = {pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                   pCh3Timeline[revIdx0]};
            acc0                = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r0_0));

            revIdx0          = s1 + (halfTapsStage1 - 1 - tap);
            float32x4_t r1_0 = {pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                pCh3Timeline[revIdx0]};
            acc1             = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r1_0));

            revIdx0          = s2 + (halfTapsStage1 - 1 - tap);
            float32x4_t r2_0 = {pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                pCh3Timeline[revIdx0]};
            acc2             = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r2_0));

            revIdx0          = s3 + (halfTapsStage1 - 1 - tap);
            float32x4_t r3_0 = {pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                pCh3Timeline[revIdx0]};
            acc3             = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r3_0));

            float32x4_t f0_tmp = f1;
            float32x4_t f1_tmp = f2;
            float32x4_t f2_tmp = f_new0;
            fwdIdx++;

            // TAP 1
            float32x4_t coeff1 = vld1q_dup_f32(&pFilterStage1[tap + 1]);
            float32x4_t f_new1 = {pCh0Timeline[fwdIdx], pCh1Timeline[fwdIdx], pCh2Timeline[fwdIdx],
                                  pCh3Timeline[fwdIdx]};

            int32_t     revIdx1 = s0 + (halfTapsStage1 - 2 - tap);
            float32x4_t r0_1    = {pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                   pCh3Timeline[revIdx1]};
            acc0                = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_tmp, r0_1));

            revIdx1          = s1 + (halfTapsStage1 - 2 - tap);
            float32x4_t r1_1 = {pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                pCh3Timeline[revIdx1]};
            acc1             = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_tmp, r1_1));

            revIdx1          = s2 + (halfTapsStage1 - 2 - tap);
            float32x4_t r2_1 = {pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                pCh3Timeline[revIdx1]};
            acc2             = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_tmp, r2_1));

            revIdx1          = s3 + (halfTapsStage1 - 2 - tap);
            float32x4_t r3_1 = {pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                pCh3Timeline[revIdx1]};
            acc3             = vmlaq_f32(acc3, coeff1, vaddq_f32(f_new1, r3_1));

            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = f_new1;
            fwdIdx++;
         }

         // Store FIR results to scratch buffer
         pCh0Scratch[s0] = vgetq_lane_f32(acc0, 0);
         pCh1Scratch[s0] = vgetq_lane_f32(acc0, 1);
         pCh2Scratch[s0] = vgetq_lane_f32(acc0, 2);
         pCh3Scratch[s0] = vgetq_lane_f32(acc0, 3);

         pCh0Scratch[s1] = vgetq_lane_f32(acc1, 0);
         pCh1Scratch[s1] = vgetq_lane_f32(acc1, 1);
         pCh2Scratch[s1] = vgetq_lane_f32(acc1, 2);
         pCh3Scratch[s1] = vgetq_lane_f32(acc1, 3);

         pCh0Scratch[s2] = vgetq_lane_f32(acc2, 0);
         pCh1Scratch[s2] = vgetq_lane_f32(acc2, 1);
         pCh2Scratch[s2] = vgetq_lane_f32(acc2, 2);
         pCh3Scratch[s2] = vgetq_lane_f32(acc2, 3);

         pCh0Scratch[s3] = vgetq_lane_f32(acc3, 0);
         pCh1Scratch[s3] = vgetq_lane_f32(acc3, 1);
         pCh2Scratch[s3] = vgetq_lane_f32(acc3, 2);
         pCh3Scratch[s3] = vgetq_lane_f32(acc3, 3);
      }

      // Remaining samples (1-3) for Stage 1
      for (int32_t rem = 0; rem < remainingSamples; rem++) {
         int32_t sIdx = numCompleteBlocks * 4 + rem;

         float32x4_t acc_a = vdupq_n_f32(0.0f);
         float32x4_t acc_b = vdupq_n_f32(0.0f);

         for (int32_t tap = 0; tap < filterTapsStage1; tap += 2) {
            float32x4_t fwd0 = {pCh0Timeline[sIdx + tap], pCh1Timeline[sIdx + tap], pCh2Timeline[sIdx + tap],
                                pCh3Timeline[sIdx + tap]};
            float32x4_t rev0 = {
                pCh0Timeline[sIdx + halfTapsStage1 - 1 - tap], pCh1Timeline[sIdx + halfTapsStage1 - 1 - tap],
                pCh2Timeline[sIdx + halfTapsStage1 - 1 - tap], pCh3Timeline[sIdx + halfTapsStage1 - 1 - tap]};
            acc_a = vmlaq_f32(acc_a, vdupq_n_f32(pFilterStage1[tap]), vaddq_f32(fwd0, rev0));

            float32x4_t fwd1 = {pCh0Timeline[sIdx + tap + 1], pCh1Timeline[sIdx + tap + 1],
                                pCh2Timeline[sIdx + tap + 1], pCh3Timeline[sIdx + tap + 1]};
            float32x4_t rev1 = {
                pCh0Timeline[sIdx + halfTapsStage1 - 2 - tap], pCh1Timeline[sIdx + halfTapsStage1 - 2 - tap],
                pCh2Timeline[sIdx + halfTapsStage1 - 2 - tap], pCh3Timeline[sIdx + halfTapsStage1 - 2 - tap]};
            acc_b = vmlaq_f32(acc_b, vdupq_n_f32(pFilterStage1[tap + 1]), vaddq_f32(fwd1, rev1));
         }

         float32x4_t acc = vaddq_f32(acc_a, acc_b);

         pCh0Scratch[sIdx] = vgetq_lane_f32(acc, 0);
         pCh1Scratch[sIdx] = vgetq_lane_f32(acc, 1);
         pCh2Scratch[sIdx] = vgetq_lane_f32(acc, 2);
         pCh3Scratch[sIdx] = vgetq_lane_f32(acc, 3);
      }
   }

   // PATH B: Scalar tail channels for Stage 1
   for (int32_t ch = veclimit; ch < numChannels; ch++) {
      dataType *pChTimeline = &pStage1Buff[ch * (historyNumSamplesStage1 + inputSampleCount)];
      dataType *pChScratch  = &pScratch1Buff[ch * inputSampleCount];

      for (int32_t blk = 0; blk < numCompleteBlocks; blk++) {
         float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;

         int32_t s0 = blk * 4 + 0;
         int32_t s1 = blk * 4 + 1;
         int32_t s2 = blk * 4 + 2;
         int32_t s3 = blk * 4 + 3;

         float   f0     = pChTimeline[s0];
         float   f1     = pChTimeline[s1];
         float   f2     = pChTimeline[s2];
         int32_t fwdIdx = s0 + 3;

         for (int32_t tap = 0; tap < filterTapsStage1; tap += 2) {
            float coeff0 = pFilterStage1[tap];
            float f_new0 = pChTimeline[fwdIdx];

            acc0 += coeff0 * (f0 + pChTimeline[s0 + halfTapsStage1 - 1 - tap]);
            acc1 += coeff0 * (f1 + pChTimeline[s1 + halfTapsStage1 - 1 - tap]);
            acc2 += coeff0 * (f2 + pChTimeline[s2 + halfTapsStage1 - 1 - tap]);
            acc3 += coeff0 * (f_new0 + pChTimeline[s3 + halfTapsStage1 - 1 - tap]);

            float f0_tmp = f1, f1_tmp = f2, f2_tmp = f_new0;
            fwdIdx++;

            float coeff1 = pFilterStage1[tap + 1];
            float f_new1 = pChTimeline[fwdIdx];

            acc0 += coeff1 * (f0_tmp + pChTimeline[s0 + halfTapsStage1 - 2 - tap]);
            acc1 += coeff1 * (f1_tmp + pChTimeline[s1 + halfTapsStage1 - 2 - tap]);
            acc2 += coeff1 * (f2_tmp + pChTimeline[s2 + halfTapsStage1 - 2 - tap]);
            acc3 += coeff1 * (f_new1 + pChTimeline[s3 + halfTapsStage1 - 2 - tap]);

            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = f_new1;
            fwdIdx++;
         }

         pChScratch[s0] = acc0;
         pChScratch[s1] = acc1;
         pChScratch[s2] = acc2;
         pChScratch[s3] = acc3;
      }

      for (int32_t rem = 0; rem < remainingSamples; rem++) {
         int32_t sIdx  = numCompleteBlocks * 4 + rem;
         float   acc_a = 0.0f, acc_b = 0.0f;

         for (int32_t tap = 0; tap < filterTapsStage1; tap += 2) {
            acc_a += pFilterStage1[tap] * (pChTimeline[sIdx + tap] + pChTimeline[sIdx + halfTapsStage1 - 1 - tap]);
            acc_b +=
                pFilterStage1[tap + 1] * (pChTimeline[sIdx + tap + 1] + pChTimeline[sIdx + halfTapsStage1 - 2 - tap]);
         }

         pChScratch[sIdx] = acc_a + acc_b;
      }
   }

   // Step 3: Interleave Stage 1 FIR output with original samples into Stage 2 buffer
   // MATCHING 2X PATTERN EXACTLY
   for (int32_t ch = 0; ch < numChannels; ch++) {
      dataType *pChScratch  = &pScratch1Buff[ch * inputSampleCount];
      dataType *pChOriginal = &pStage1Buff[ch * (historyNumSamplesStage1 + inputSampleCount) + filterTapsStage1];
      dataType *pChStage2In =
          &pStage2Buff[ch * (historyNumSamplesStage2 + stage1OutputSampleCount) + historyNumSamplesStage2];

      // Interleave: [fir[0], orig[0], fir[1], orig[1], ...]
      for (int32_t i = 0; i < inputSampleCount; i++) {
         pChStage2In[i * 2 + 0] = pChScratch[i];  // Filtered sample (even phase)
         pChStage2In[i * 2 + 1] = pChOriginal[i]; // Original sample (odd phase)
      }
   }

   // Step 4: Copy last filterLength samples to beginning of stage 1 buffer for next time
   for (int32_t ch = 0; ch < numChannels; ch++) {
      memcpy(&pStage1Buff[ch * (historyNumSamplesStage1 + inputSampleCount)],
             &pStage1Buff[ch * (historyNumSamplesStage1 + inputSampleCount) + inputSampleCount],
             historyNumSamplesStage1 * sizeof(dataType));
   }

   // ============================================================================
   // STAGE 2: Second 2x Upsampling (on the 2x upsampled data from Stage 1)
   // ============================================================================

   const int32_t numCompleteBlocksStage2 = stage1OutputSampleCount / eleCount;
   const int32_t remainingSamplesStage2  = stage1OutputSampleCount % eleCount;

   // PATH A: NEON - Process 4 channels simultaneously for Stage 2
   for (int32_t chGroup = 0; chGroup < veclimit; chGroup += 4) {

      dataType *pCh0Timeline = &pStage2Buff[(chGroup + 0) * (historyNumSamplesStage2 + stage1OutputSampleCount)];
      dataType *pCh1Timeline = &pStage2Buff[(chGroup + 1) * (historyNumSamplesStage2 + stage1OutputSampleCount)];
      dataType *pCh2Timeline = &pStage2Buff[(chGroup + 2) * (historyNumSamplesStage2 + stage1OutputSampleCount)];
      dataType *pCh3Timeline = &pStage2Buff[(chGroup + 3) * (historyNumSamplesStage2 + stage1OutputSampleCount)];

      dataType *pCh0Scratch = &pScratch2Buff[(chGroup + 0) * stage1OutputSampleCount];
      dataType *pCh1Scratch = &pScratch2Buff[(chGroup + 1) * stage1OutputSampleCount];
      dataType *pCh2Scratch = &pScratch2Buff[(chGroup + 2) * stage1OutputSampleCount];
      dataType *pCh3Scratch = &pScratch2Buff[(chGroup + 3) * stage1OutputSampleCount];

      // Process complete blocks of 4 samples
      for (int32_t blk = 0; blk < numCompleteBlocksStage2; blk++) {

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         int32_t s0 = blk * 4 + 0;
         int32_t s1 = blk * 4 + 1;
         int32_t s2 = blk * 4 + 2;
         int32_t s3 = blk * 4 + 3;

         // Preload forward samples
         float32x4_t f0 = {pCh0Timeline[s0], pCh1Timeline[s0], pCh2Timeline[s0], pCh3Timeline[s0]};
         float32x4_t f1 = {pCh0Timeline[s1], pCh1Timeline[s1], pCh2Timeline[s1], pCh3Timeline[s1]};
         float32x4_t f2 = {pCh0Timeline[s2], pCh1Timeline[s2], pCh2Timeline[s2], pCh3Timeline[s2]};

         int32_t fwdIdx = s0 + 3;

         // Symmetric FIR loop (2x unrolled)
         for (int32_t tap = 0; tap < filterTapsStage2; tap += 2) {

            // TAP 0
            float32x4_t coeff0 = vld1q_dup_f32(&pFilterStage2[tap]);
            float32x4_t f_new0 = {pCh0Timeline[fwdIdx], pCh1Timeline[fwdIdx], pCh2Timeline[fwdIdx],
                                  pCh3Timeline[fwdIdx]};

            int32_t     revIdx0 = s0 + (halfTapsStage2 - 1 - tap);
            float32x4_t r0_0    = {pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                   pCh3Timeline[revIdx0]};
            acc0                = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r0_0));

            revIdx0          = s1 + (halfTapsStage2 - 1 - tap);
            float32x4_t r1_0 = {pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                pCh3Timeline[revIdx0]};
            acc1             = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r1_0));

            revIdx0          = s2 + (halfTapsStage2 - 1 - tap);
            float32x4_t r2_0 = {pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                pCh3Timeline[revIdx0]};
            acc2             = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r2_0));

            revIdx0          = s3 + (halfTapsStage2 - 1 - tap);
            float32x4_t r3_0 = {pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                pCh3Timeline[revIdx0]};
            acc3             = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r3_0));

            float32x4_t f0_tmp = f1;
            float32x4_t f1_tmp = f2;
            float32x4_t f2_tmp = f_new0;
            fwdIdx++;

            // TAP 1
            float32x4_t coeff1 = vld1q_dup_f32(&pFilterStage2[tap + 1]);
            float32x4_t f_new1 = {pCh0Timeline[fwdIdx], pCh1Timeline[fwdIdx], pCh2Timeline[fwdIdx],
                                  pCh3Timeline[fwdIdx]};

            int32_t     revIdx1 = s0 + (halfTapsStage2 - 2 - tap);
            float32x4_t r0_1    = {pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                   pCh3Timeline[revIdx1]};
            acc0                = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_tmp, r0_1));

            revIdx1          = s1 + (halfTapsStage2 - 2 - tap);
            float32x4_t r1_1 = {pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                pCh3Timeline[revIdx1]};
            acc1             = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_tmp, r1_1));

            revIdx1          = s2 + (halfTapsStage2 - 2 - tap);
            float32x4_t r2_1 = {pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                pCh3Timeline[revIdx1]};
            acc2             = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_tmp, r2_1));

            revIdx1          = s3 + (halfTapsStage2 - 2 - tap);
            float32x4_t r3_1 = {pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                pCh3Timeline[revIdx1]};
            acc3             = vmlaq_f32(acc3, coeff1, vaddq_f32(f_new1, r3_1));

            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = f_new1;
            fwdIdx++;
         }

         // Store FIR results to scratch buffer
         pCh0Scratch[s0] = vgetq_lane_f32(acc0, 0);
         pCh1Scratch[s0] = vgetq_lane_f32(acc0, 1);
         pCh2Scratch[s0] = vgetq_lane_f32(acc0, 2);
         pCh3Scratch[s0] = vgetq_lane_f32(acc0, 3);

         pCh0Scratch[s1] = vgetq_lane_f32(acc1, 0);
         pCh1Scratch[s1] = vgetq_lane_f32(acc1, 1);
         pCh2Scratch[s1] = vgetq_lane_f32(acc1, 2);
         pCh3Scratch[s1] = vgetq_lane_f32(acc1, 3);

         pCh0Scratch[s2] = vgetq_lane_f32(acc2, 0);
         pCh1Scratch[s2] = vgetq_lane_f32(acc2, 1);
         pCh2Scratch[s2] = vgetq_lane_f32(acc2, 2);
         pCh3Scratch[s2] = vgetq_lane_f32(acc2, 3);

         pCh0Scratch[s3] = vgetq_lane_f32(acc3, 0);
         pCh1Scratch[s3] = vgetq_lane_f32(acc3, 1);
         pCh2Scratch[s3] = vgetq_lane_f32(acc3, 2);
         pCh3Scratch[s3] = vgetq_lane_f32(acc3, 3);
      }

      // Remaining samples (1-3) for Stage 2
      for (int32_t rem = 0; rem < remainingSamplesStage2; rem++) {
         int32_t sIdx = numCompleteBlocksStage2 * 4 + rem;

         float32x4_t acc_a = vdupq_n_f32(0.0f);
         float32x4_t acc_b = vdupq_n_f32(0.0f);

         for (int32_t tap = 0; tap < filterTapsStage2; tap += 2) {
            float32x4_t fwd0 = {pCh0Timeline[sIdx + tap], pCh1Timeline[sIdx + tap], pCh2Timeline[sIdx + tap],
                                pCh3Timeline[sIdx + tap]};
            float32x4_t rev0 = {
                pCh0Timeline[sIdx + halfTapsStage2 - 1 - tap], pCh1Timeline[sIdx + halfTapsStage2 - 1 - tap],
                pCh2Timeline[sIdx + halfTapsStage2 - 1 - tap], pCh3Timeline[sIdx + halfTapsStage2 - 1 - tap]};
            acc_a = vmlaq_f32(acc_a, vdupq_n_f32(pFilterStage2[tap]), vaddq_f32(fwd0, rev0));

            float32x4_t fwd1 = {pCh0Timeline[sIdx + tap + 1], pCh1Timeline[sIdx + tap + 1],
                                pCh2Timeline[sIdx + tap + 1], pCh3Timeline[sIdx + tap + 1]};
            float32x4_t rev1 = {
                pCh0Timeline[sIdx + halfTapsStage2 - 2 - tap], pCh1Timeline[sIdx + halfTapsStage2 - 2 - tap],
                pCh2Timeline[sIdx + halfTapsStage2 - 2 - tap], pCh3Timeline[sIdx + halfTapsStage2 - 2 - tap]};
            acc_b = vmlaq_f32(acc_b, vdupq_n_f32(pFilterStage2[tap + 1]), vaddq_f32(fwd1, rev1));
         }

         float32x4_t acc = vaddq_f32(acc_a, acc_b);

         pCh0Scratch[sIdx] = vgetq_lane_f32(acc, 0);
         pCh1Scratch[sIdx] = vgetq_lane_f32(acc, 1);
         pCh2Scratch[sIdx] = vgetq_lane_f32(acc, 2);
         pCh3Scratch[sIdx] = vgetq_lane_f32(acc, 3);
      }
   }

   // PATH B: Scalar tail channels for Stage 2
   for (int32_t ch = veclimit; ch < numChannels; ch++) {
      dataType *pChTimeline = &pStage2Buff[ch * (historyNumSamplesStage2 + stage1OutputSampleCount)];
      dataType *pChScratch  = &pScratch2Buff[ch * stage1OutputSampleCount];

      for (int32_t blk = 0; blk < numCompleteBlocksStage2; blk++) {
         float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;

         int32_t s0 = blk * 4 + 0;
         int32_t s1 = blk * 4 + 1;
         int32_t s2 = blk * 4 + 2;
         int32_t s3 = blk * 4 + 3;

         float   f0     = pChTimeline[s0];
         float   f1     = pChTimeline[s1];
         float   f2     = pChTimeline[s2];
         int32_t fwdIdx = s0 + 3;

         for (int32_t tap = 0; tap < filterTapsStage2; tap += 2) {
            float coeff0 = pFilterStage2[tap];
            float f_new0 = pChTimeline[fwdIdx];

            acc0 += coeff0 * (f0 + pChTimeline[s0 + halfTapsStage2 - 1 - tap]);
            acc1 += coeff0 * (f1 + pChTimeline[s1 + halfTapsStage2 - 1 - tap]);
            acc2 += coeff0 * (f2 + pChTimeline[s2 + halfTapsStage2 - 1 - tap]);
            acc3 += coeff0 * (f_new0 + pChTimeline[s3 + halfTapsStage2 - 1 - tap]);

            float f0_tmp = f1, f1_tmp = f2, f2_tmp = f_new0;
            fwdIdx++;

            float coeff1 = pFilterStage2[tap + 1];
            float f_new1 = pChTimeline[fwdIdx];

            acc0 += coeff1 * (f0_tmp + pChTimeline[s0 + halfTapsStage2 - 2 - tap]);
            acc1 += coeff1 * (f1_tmp + pChTimeline[s1 + halfTapsStage2 - 2 - tap]);
            acc2 += coeff1 * (f2_tmp + pChTimeline[s2 + halfTapsStage2 - 2 - tap]);
            acc3 += coeff1 * (f_new1 + pChTimeline[s3 + halfTapsStage2 - 2 - tap]);

            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = f_new1;
            fwdIdx++;
         }

         pChScratch[s0] = acc0;
         pChScratch[s1] = acc1;
         pChScratch[s2] = acc2;
         pChScratch[s3] = acc3;
      }

      for (int32_t rem = 0; rem < remainingSamplesStage2; rem++) {
         int32_t sIdx  = numCompleteBlocksStage2 * 4 + rem;
         float   acc_a = 0.0f, acc_b = 0.0f;

         for (int32_t tap = 0; tap < filterTapsStage2; tap += 2) {
            acc_a += pFilterStage2[tap] * (pChTimeline[sIdx + tap] + pChTimeline[sIdx + halfTapsStage2 - 1 - tap]);
            acc_b +=
                pFilterStage2[tap + 1] * (pChTimeline[sIdx + tap + 1] + pChTimeline[sIdx + halfTapsStage2 - 2 - tap]);
         }

         pChScratch[sIdx] = acc_a + acc_b;
      }
   }

   // Step 6: Interleave Stage 2 FIR output with original Stage 2 input samples to final output
   for (int32_t ch = 0; ch < numChannels; ch++) {
      dataType *pChScratch  = &pScratch2Buff[ch * stage1OutputSampleCount];
      dataType *pChOriginal = &pStage2Buff[ch * (historyNumSamplesStage2 + stage1OutputSampleCount) + filterTapsStage2];
      dataType *pChOut      = &pOutLocal[ch * stage2OutputSampleCount];

      // Interleave: [fir[0], orig[0], fir[1], orig[1], ...]
      for (int32_t i = 0; i < stage1OutputSampleCount; i++) {
         pChOut[i * 2 + 0] = pChScratch[i];  // Filtered sample (even phase)
         pChOut[i * 2 + 1] = pChOriginal[i]; // Original sample (odd phase)
      }
   }

   // Step 7: Copy last filterLength samples to beginning of stage 2 buffer for next time
   for (int32_t ch = 0; ch < numChannels; ch++) {
      memcpy(&pStage2Buff[ch * (historyNumSamplesStage2 + stage1OutputSampleCount)],
             &pStage2Buff[ch * (historyNumSamplesStage2 + stage1OutputSampleCount) + stage1OutputSampleCount],
             historyNumSamplesStage2 * sizeof(dataType));
   }

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

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_non_interleaved_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                 void *restrict pIn,
                                                                                 void *restrict pState,
                                                                                 void *restrict pFiltCoeffs,
                                                                                 void *restrict pOut);
