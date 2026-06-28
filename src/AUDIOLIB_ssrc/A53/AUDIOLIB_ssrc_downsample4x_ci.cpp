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
 * Main initialization function for 4x downsampling using A53 NEON intrinsics.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_downsample4x_init_ci\r\n"));
#endif

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                 bufferFormat = pKerPrivArgs->initArgs.bufferFormat;
   uint8_t                 dataFormat   = pKerPrivArgs->initArgs.dataFormat;

   // Check data format first
   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      // Check if using circular buffer format
      if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
         // For linear buffer format, use the specialized implementation
         status = AUDIOLIB_ssrc_downsample4x_linear_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (status == AUDIOLIB_SUCCESS) {
            pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample4x_linear_exec_ci<float>;
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

/*
 * Initialization for 4x downsampling using linear processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_downsample4x_linear_init_ci\r\n"));
#endif

   AUDIOLIB_STATUS         status                  = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs            = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 inputSampleCount        = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 intermediateSampleCount = inputSampleCount / 2;
   int32_t                 outputSampleCount       = pKerPrivArgs->outputSampleCount;

   // Calculate the number of output blocks needed for stage 1
   pKerPrivArgs->numOutputBlocks1 =
       AUDIOLIB_ceilingDiv(intermediateSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   // Calculate the number of output blocks needed for stage 1
   pKerPrivArgs->numOutputBlocks2 = AUDIOLIB_ceilingDiv(outputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_downsample4x_non_interleaved_init_ci\n");
#endif
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   return status;
}

/*
 * Execution for 4x downsampling using A53 intrinsics and linear processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pState,
                                                          void *restrict pFiltCoeffs,
                                                          void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_downsample4x_linear_exec_ci\r\n"));
#endif
   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   dataType               *pNewInput        = (dataType *) pIn;
   dataType               *pOutLocal        = (dataType *) pOut;
   const dataType         *pAll             = (dataType *) pFiltCoeffs;
   const dataType         *pFiltStage1      = pAll + pKerPrivArgs->stage1FiltCoeffsOffset;
   const dataType         *pFiltStage2      = pAll + pKerPrivArgs->stage2FiltCoeffsOffset;
   int32_t                 numChannels      = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 stage1Samples    = inputSampleCount / 2;
   int32_t                 stage2Samples    = stage1Samples / 2;

   /* Partition single state buffer into two history sections */
   const int32_t taps1Sym    = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS; // 39
   const int32_t taps2Sym    = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS; // 191
   const int32_t historyLen1 = (taps1Sym - 1) * numChannels;                           // 38 * numChannels
   const int32_t historyLen2 = (taps2Sym - 1) * numChannels;                           // 190 * numChannels

   /* Filter parameters */
   const int32_t taps1 = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS; // 10
   const int32_t taps2 = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS; // 48

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
   memcpy(pScratch1, pNewInput, totalInput * sizeof(dataType));

   /* Stage 1 output will go directly to pScratch2 (after history2) */
   dataType *pIntermediate = pScratch2;

   /* =========================================================================
    * STAGE 1: 2× DOWNSAMPLING (INPUT → INTERMEDIATE) - ARM NEON OPTIMIZED
    * Process 4 output samples per iteration with 4 accumulators
    * ========================================================================= */
   const int32_t decStride1 = numChannels * 2; // Decimation by 2
   const int32_t nc3        = numChannels * 3;
   int32_t       vecLimit   = (numChannels / 4) * 4;

   /* VECTORIZED PATH: Process 4 channels at a time */
   for (int32_t ch = 0; ch < vecLimit; ch += 4) {
      for (int32_t outB = 0; outB < (stage1Samples / 4); outB++) {

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         // Base offset for this output block
         uint32_t blockOffset = (outB * 4) * decStride1 + ch;

         // Base pointers for forward and reverse (now pointing to pStage1DataBuffer)
         dataType *ptrF_Base = &pStage1DataBuffer[blockOffset];
         dataType *ptrR_Base = &pStage1DataBuffer[blockOffset + (taps1Sym - 1) * numChannels];

         // Lead pointers
         dataType *ptrF_Lead = ptrF_Base + (3 * decStride1);
         dataType *ptrR_Lead = ptrR_Base;

         // Initial forward loads
         float32x4_t f0 = vld1q_f32(ptrF_Base);
         float32x4_t f1 = vld1q_f32(ptrF_Base + decStride1);
         float32x4_t f2 = vld1q_f32(ptrF_Base + (decStride1 << 1));

         // Initial reverse loads
         float32x4_t r0 = vld1q_f32(ptrR_Base + decStride1);
         float32x4_t r1 = vld1q_f32(ptrR_Base + (decStride1 << 1));
         float32x4_t r2 = vld1q_f32(ptrR_Base + decStride1 * 3);

         /* Loop unrolled by 2 */
         int32_t tap;
         for (tap = 0; tap < (taps1 & ~1); tap += 2) {
            // === TAP 0 ===
            float32x4_t coeff0 = vld1q_dup_f32(&pFiltStage1[tap]);
            float32x4_t f_new0 = vld1q_f32(ptrF_Lead);
            float32x4_t r_new0 = vld1q_f32(ptrR_Lead);

            acc0 = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r_new0));
            acc1 = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r2));

            // Shift forward registers
            float32x4_t f0_tmp = f1;
            float32x4_t f1_tmp = f2;
            float32x4_t f2_tmp = f_new0;

            ptrF_Lead += decStride1;
            ptrR_Lead -= decStride1;

            // === TAP 1 ===
            float32x4_t coeff1 = vld1q_dup_f32(&pFiltStage1[tap + 1]);
            float32x4_t f_new1 = vld1q_f32(ptrF_Lead);
            float32x4_t r_new1 = vld1q_f32(ptrR_Lead);

            acc0 = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_tmp, r_new1));
            acc1 = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_tmp, r_new0));
            acc2 = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_tmp, r0));
            acc3 = vmlaq_f32(acc3, coeff1, vaddq_f32(f_new1, r1));

            // Update for next iteration
            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = f_new1;

            r2 = r0;
            r1 = r_new0;
            r0 = r_new1;

            ptrF_Lead += decStride1;
            ptrR_Lead -= decStride1;
         }

         // Handle remaining tap if taps1 is odd
         if (tap < taps1) {
            float32x4_t coeff = vld1q_dup_f32(&pFiltStage1[tap]);
            float32x4_t f_new = vld1q_f32(ptrF_Lead);
            float32x4_t r_new = vld1q_f32(ptrR_Lead);

            acc0 = vmlaq_f32(acc0, coeff, vaddq_f32(f0, r_new));
            acc1 = vmlaq_f32(acc1, coeff, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff, vaddq_f32(f_new, r2));
         }

         // Center tap processing
         int32_t     centerOffset = ((taps1Sym - 1) >> 1) * numChannels;
         float32x4_t center0      = vld1q_f32(&pStage1DataBuffer[blockOffset + centerOffset]);
         float32x4_t center1      = vld1q_f32(&pStage1DataBuffer[blockOffset + decStride1 + centerOffset]);
         float32x4_t center2      = vld1q_f32(&pStage1DataBuffer[blockOffset + (decStride1 << 1) + centerOffset]);
         float32x4_t center3      = vld1q_f32(&pStage1DataBuffer[blockOffset + decStride1 * 3 + centerOffset]);

         acc0 = vmlaq_n_f32(acc0, center0, 0.5f);
         acc1 = vmlaq_n_f32(acc1, center1, 0.5f);
         acc2 = vmlaq_n_f32(acc2, center2, 0.5f);
         acc3 = vmlaq_n_f32(acc3, center3, 0.5f);

         // Store results
         float *pOutBase = &pIntermediate[(outB * 4) * numChannels + ch];
         vst1q_f32(pOutBase, acc0);
         vst1q_f32(pOutBase + numChannels, acc1);
         vst1q_f32(pOutBase + (numChannels << 1), acc2);
         vst1q_f32(pOutBase + nc3, acc3);
      }
   }

   /* SCALAR REMAINDER PATH */
   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      for (int32_t outB = 0; outB < (stage1Samples / 4); outB++) {
         float    acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;
         uint32_t blockOffset = (outB * 4) * decStride1 + ch;

         dataType *ptrF = &pStage1DataBuffer[blockOffset];
         dataType *ptrR = &pStage1DataBuffer[blockOffset + (taps1Sym - 1) * numChannels];

         dataType *ptrF_L = ptrF + (3 * decStride1);
         dataType *ptrR_L = ptrR;

         float f0 = ptrF[0];
         float f1 = ptrF[decStride1];
         float f2 = ptrF[decStride1 << 1];

         float r0 = ptrR[decStride1];
         float r1 = ptrR[decStride1 << 1];
         float r2 = ptrR[decStride1 * 3];

         // Loop unrolled by 2
         int32_t tap;
         for (tap = 0; tap < (taps1 & ~1); tap += 2) {
            // TAP 0
            float coeff0 = pFiltStage1[tap];
            float fn0    = *ptrF_L;
            float rn0    = *ptrR_L;

            acc0 += coeff0 * (f0 + rn0);
            acc1 += coeff0 * (f1 + r0);
            acc2 += coeff0 * (f2 + r1);
            acc3 += coeff0 * (fn0 + r2);

            float f0_tmp = f1;
            float f1_tmp = f2;
            float f2_tmp = fn0;

            ptrF_L += decStride1;
            ptrR_L -= decStride1;

            // TAP 1
            float coeff1 = pFiltStage1[tap + 1];
            float fn1    = *ptrF_L;
            float rn1    = *ptrR_L;

            acc0 += coeff1 * (f0_tmp + rn1);
            acc1 += coeff1 * (f1_tmp + rn0);
            acc2 += coeff1 * (f2_tmp + r0);
            acc3 += coeff1 * (fn1 + r1);

            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = fn1;

            r2 = r0;
            r1 = rn0;
            r0 = rn1;

            ptrF_L += decStride1;
            ptrR_L -= decStride1;
         }

         // Handle remaining tap if odd
         if (tap < taps1) {
            float coeff = pFiltStage1[tap];
            float fn    = *ptrF_L;
            float rn    = *ptrR_L;

            acc0 += coeff * (f0 + rn);
            acc1 += coeff * (f1 + r0);
            acc2 += coeff * (f2 + r1);
            acc3 += coeff * (fn + r2);
         }

         // Center tap
         int32_t centerOffset = ((taps1Sym - 1) >> 1) * numChannels;
         float   center0      = pStage1DataBuffer[blockOffset + centerOffset];
         float   center1      = pStage1DataBuffer[blockOffset + decStride1 + centerOffset];
         float   center2      = pStage1DataBuffer[blockOffset + (decStride1 << 1) + centerOffset];
         float   center3      = pStage1DataBuffer[blockOffset + decStride1 * 3 + centerOffset];

         acc0 += 0.5f * center0;
         acc1 += 0.5f * center1;
         acc2 += 0.5f * center2;
         acc3 += 0.5f * center3;

         float *pOutBase            = &pIntermediate[(outB * 4) * numChannels + ch];
         pOutBase[0]                = acc0;
         pOutBase[numChannels]      = acc1;
         pOutBase[numChannels << 1] = acc2;
         pOutBase[nc3]              = acc3;
      }
   }

   /* Handle remaining samples (if stage1Samples not divisible by 4) */
   int32_t remainingSamples = stage1Samples % 4;
   if (remainingSamples > 0) {
      int32_t startSample = (stage1Samples / 4) * 4;
      for (int32_t i = startSample; i < stage1Samples; i++) {
         for (int32_t j = 0; j < numChannels; j++) {
            dataType outTemp    = 0;
            int32_t  dataOffset = i * 2 * numChannels + j;

            for (int32_t k = 0; k < taps1; k++) {
               int32_t idx1 = dataOffset + k * 2 * numChannels;
               int32_t idx2 = dataOffset + ((taps1Sym - 1) - (k * 2)) * numChannels;
               outTemp += (pStage1DataBuffer[idx1] + pStage1DataBuffer[idx2]) * pFiltStage1[k];
            }

            int32_t centerIdx = dataOffset + ((taps1Sym - 1) >> 1) * numChannels;
            outTemp += pStage1DataBuffer[centerIdx] * 0.5f;
            pIntermediate[i * numChannels + j] = outTemp;
         }
      }
   }

   /* =========================================================================
    * STAGE 2: 2× DOWNSAMPLING (INTERMEDIATE → OUTPUT) - ARM NEON OPTIMIZED
    * Process 4 output samples per iteration with 4 accumulators
    * ========================================================================= */
   const int32_t decStride2 = numChannels * 2; // Decimation by 2

   /* VECTORIZED PATH: Process 4 channels at a time */
   for (int32_t ch = 0; ch < vecLimit; ch += 4) {
      for (int32_t outB = 0; outB < (stage2Samples / 4); outB++) {

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         // Base offset for this output block
         uint32_t blockOffset = (outB * 4) * decStride2 + ch;

         // Base pointers for forward and reverse (now pointing to pStage2DataBuffer)
         dataType *ptrF_Base = &pStage2DataBuffer[blockOffset];
         dataType *ptrR_Base = &pStage2DataBuffer[blockOffset + (taps2Sym - 1) * numChannels];

         // Lead pointers
         dataType *ptrF_Lead = ptrF_Base + (3 * decStride2);
         dataType *ptrR_Lead = ptrR_Base;

         // Initial forward loads
         float32x4_t f0 = vld1q_f32(ptrF_Base);
         float32x4_t f1 = vld1q_f32(ptrF_Base + decStride2);
         float32x4_t f2 = vld1q_f32(ptrF_Base + (decStride2 << 1));

         // Initial reverse loads
         float32x4_t r0 = vld1q_f32(ptrR_Base + decStride2);
         float32x4_t r1 = vld1q_f32(ptrR_Base + (decStride2 << 1));
         float32x4_t r2 = vld1q_f32(ptrR_Base + decStride2 * 3);

         /* Loop unrolled by 2 */
         int32_t tap;
         for (tap = 0; tap < (taps2 & ~1); tap += 2) {
            // === TAP 0 ===
            float32x4_t coeff0 = vld1q_dup_f32(&pFiltStage2[tap]);
            float32x4_t f_new0 = vld1q_f32(ptrF_Lead);
            float32x4_t r_new0 = vld1q_f32(ptrR_Lead);

            acc0 = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r_new0));
            acc1 = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r2));

            // Shift forward registers
            float32x4_t f0_tmp = f1;
            float32x4_t f1_tmp = f2;
            float32x4_t f2_tmp = f_new0;

            ptrF_Lead += decStride2;
            ptrR_Lead -= decStride2;

            // === TAP 1 ===
            float32x4_t coeff1 = vld1q_dup_f32(&pFiltStage2[tap + 1]);
            float32x4_t f_new1 = vld1q_f32(ptrF_Lead);
            float32x4_t r_new1 = vld1q_f32(ptrR_Lead);

            acc0 = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_tmp, r_new1));
            acc1 = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_tmp, r_new0));
            acc2 = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_tmp, r0));
            acc3 = vmlaq_f32(acc3, coeff1, vaddq_f32(f_new1, r1));

            // Update for next iteration
            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = f_new1;

            r2 = r0;
            r1 = r_new0;
            r0 = r_new1;

            ptrF_Lead += decStride2;
            ptrR_Lead -= decStride2;
         }

         // Center tap processing
         int32_t     centerOffset = ((taps2Sym - 1) >> 1) * numChannels;
         float32x4_t center0      = vld1q_f32(&pStage2DataBuffer[blockOffset + centerOffset]);
         float32x4_t center1      = vld1q_f32(&pStage2DataBuffer[blockOffset + decStride2 + centerOffset]);
         float32x4_t center2      = vld1q_f32(&pStage2DataBuffer[blockOffset + (decStride2 << 1) + centerOffset]);
         float32x4_t center3      = vld1q_f32(&pStage2DataBuffer[blockOffset + decStride2 * 3 + centerOffset]);

         acc0 = vmlaq_n_f32(acc0, center0, 0.5f);
         acc1 = vmlaq_n_f32(acc1, center1, 0.5f);
         acc2 = vmlaq_n_f32(acc2, center2, 0.5f);
         acc3 = vmlaq_n_f32(acc3, center3, 0.5f);

         // Store results
         float *pOutBase = &pOutLocal[(outB * 4) * numChannels + ch];
         vst1q_f32(pOutBase, acc0);
         vst1q_f32(pOutBase + numChannels, acc1);
         vst1q_f32(pOutBase + (numChannels << 1), acc2);
         vst1q_f32(pOutBase + nc3, acc3);
      }
   }

   /* SCALAR REMAINDER PATH */
   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      for (int32_t outB = 0; outB < (stage2Samples / 4); outB++) {
         float    acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;
         uint32_t blockOffset = (outB * 4) * decStride2 + ch;

         dataType *ptrF = &pStage2DataBuffer[blockOffset];
         dataType *ptrR = &pStage2DataBuffer[blockOffset + (taps2Sym - 1) * numChannels];

         dataType *ptrF_L = ptrF + (3 * decStride2);
         dataType *ptrR_L = ptrR;

         float f0 = ptrF[0];
         float f1 = ptrF[decStride2];
         float f2 = ptrF[decStride2 << 1];

         float r0 = ptrR[decStride2];
         float r1 = ptrR[decStride2 << 1];
         float r2 = ptrR[decStride2 * 3];

         // Loop unrolled by 2
         int32_t tap;
         for (tap = 0; tap < (taps2 & ~1); tap += 2) {
            // TAP 0
            float coeff0 = pFiltStage2[tap];
            float fn0    = *ptrF_L;
            float rn0    = *ptrR_L;

            acc0 += coeff0 * (f0 + rn0);
            acc1 += coeff0 * (f1 + r0);
            acc2 += coeff0 * (f2 + r1);
            acc3 += coeff0 * (fn0 + r2);

            float f0_tmp = f1;
            float f1_tmp = f2;
            float f2_tmp = fn0;

            ptrF_L += decStride2;
            ptrR_L -= decStride2;

            // TAP 1
            float coeff1 = pFiltStage2[tap + 1];
            float fn1    = *ptrF_L;
            float rn1    = *ptrR_L;

            acc0 += coeff1 * (f0_tmp + rn1);
            acc1 += coeff1 * (f1_tmp + rn0);
            acc2 += coeff1 * (f2_tmp + r0);
            acc3 += coeff1 * (fn1 + r1);

            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = fn1;

            r2 = r0;
            r1 = rn0;
            r0 = rn1;

            ptrF_L += decStride2;
            ptrR_L -= decStride2;
         }

         // Center tap
         int32_t centerOffset = ((taps2Sym - 1) >> 1) * numChannels;
         float   center0      = pStage2DataBuffer[blockOffset + centerOffset];
         float   center1      = pStage2DataBuffer[blockOffset + decStride2 + centerOffset];
         float   center2      = pStage2DataBuffer[blockOffset + (decStride2 << 1) + centerOffset];
         float   center3      = pStage2DataBuffer[blockOffset + decStride2 * 3 + centerOffset];

         acc0 += 0.5f * center0;
         acc1 += 0.5f * center1;
         acc2 += 0.5f * center2;
         acc3 += 0.5f * center3;

         float *pOutBase            = &pOutLocal[(outB * 4) * numChannels + ch];
         pOutBase[0]                = acc0;
         pOutBase[numChannels]      = acc1;
         pOutBase[numChannels << 1] = acc2;
         pOutBase[nc3]              = acc3;
      }
   }

   /* Handle remaining samples (if stage2Samples not divisible by 4) */
   remainingSamples = stage2Samples % 4;
   if (remainingSamples > 0) {
      int32_t startSample = (stage2Samples / 4) * 4;
      for (int32_t i = startSample; i < stage2Samples; i++) {
         for (int32_t j = 0; j < numChannels; j++) {
            dataType outTemp    = 0;
            int32_t  dataOffset = i * 2 * numChannels + j;

            for (int32_t k = 0; k < taps2; k++) {
               int32_t idx1 = dataOffset + k * 2 * numChannels;
               int32_t idx2 = dataOffset + ((taps2Sym - 1) - (k * 2)) * numChannels;
               outTemp += (pStage2DataBuffer[idx1] + pStage2DataBuffer[idx2]) * pFiltStage2[k];
            }

            int32_t centerIdx = dataOffset + ((taps2Sym - 1) >> 1) * numChannels;
            outTemp += pStage2DataBuffer[centerIdx] * 0.5f;
            pOutLocal[i * numChannels + j] = outTemp;
         }
      }
   }

   /* Update history buffers - save the last historyLen samples */
   memcpy(pHistory1, pScratch1 + totalInput - historyLen1, historyLen1 * sizeof(dataType));
   memcpy(pHistory2, pScratch2 + totalStage1 - historyLen2, historyLen2 * sizeof(dataType));

   return status;
}

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
   // =============================================================================
   // SSRC Downsample 4x — Non-Interleaved ARM NEON Version
   //
   // Two-stage cascaded downsampling: 2x → 2x = 4x total
   // Based on the working 2x non-interleaved (Document 5) and 4x linear (Document 6)
   // =============================================================================

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;

   dataType *pHistory  = (dataType *) pState;
   dataType *pNewInput = (dataType *) pIn;
   dataType *pOutLocal = (dataType *) pOut;

   const dataType *pAll    = (const dataType *) pFiltCoeffs;
   const dataType *pFiltS1 = pAll + pKerPrivArgs->stage1FiltCoeffsOffset;
   const dataType *pFiltS2 = pAll + pKerPrivArgs->stage2FiltCoeffsOffset;

   const int32_t numChannels      = pKerPrivArgs->initArgs.numChannels;
   const int32_t inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;

   const int32_t taps1Sym = static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS);
   const int32_t taps1    = static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS);
   const int32_t taps2Sym = static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS);
   const int32_t taps2    = static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS);

   const int32_t hist1 = taps1Sym - 1;
   const int32_t hist2 = taps2Sym - 1;

   const int32_t cTap1 = hist1 >> 1;
   const int32_t cTap2 = hist2 >> 1;

   const int32_t stage1Samples = inputSampleCount / 2;
   const int32_t stage2Samples = stage1Samples / 2;

   const int32_t completeBlocks1   = stage1Samples / 4;
   const int32_t remainingSamples1 = stage1Samples % 4;
   const int32_t completeBlocks2   = stage2Samples / 4;
   const int32_t remainingSamples2 = stage2Samples % 4;

   const int32_t chStride1  = hist1 + inputSampleCount;
   const int32_t chStride2  = hist2 + stage1Samples;
   dataType     *pHist1Base = pHistory;
   dataType     *pHist2Base = pHistory + numChannels * chStride1;

   const int32_t decStride = 2;
   const int32_t eleCount  = 4;
   const int32_t vecLimit  = (numChannels / eleCount) * eleCount;

   for (int32_t ch = 0; ch < numChannels; ch++) {
      dataType       *pDst = pHist1Base + ch * chStride1 + hist1;
      const dataType *pSrc = pNewInput + ch * inputSampleCount;
      memcpy(pDst, pSrc, inputSampleCount * sizeof(dataType));
   }

   for (int32_t ch = 0; ch < vecLimit; ch += eleCount) {
      dataType *pS1Ch0 = pHist1Base + (ch + 0) * chStride1;
      dataType *pS1Ch1 = pHist1Base + (ch + 1) * chStride1;
      dataType *pS1Ch2 = pHist1Base + (ch + 2) * chStride1;
      dataType *pS1Ch3 = pHist1Base + (ch + 3) * chStride1;

      dataType *pS2Out0 = pHist2Base + (ch + 0) * chStride2 + hist2;
      dataType *pS2Out1 = pHist2Base + (ch + 1) * chStride2 + hist2;
      dataType *pS2Out2 = pHist2Base + (ch + 2) * chStride2 + hist2;
      dataType *pS2Out3 = pHist2Base + (ch + 3) * chStride2 + hist2;

      for (int32_t outB = 0; outB < completeBlocks1; outB++) {
         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         const int32_t blockOffset = outB * eleCount * decStride;

         dataType *pF0_Base = pS1Ch0 + blockOffset;
         dataType *pF1_Base = pS1Ch1 + blockOffset;
         dataType *pF2_Base = pS1Ch2 + blockOffset;
         dataType *pF3_Base = pS1Ch3 + blockOffset;

         dataType *pR0_Base = pF0_Base + hist1;
         dataType *pR1_Base = pF1_Base + hist1;
         dataType *pR2_Base = pF2_Base + hist1;
         dataType *pR3_Base = pF3_Base + hist1;

         dataType *pF0_Lead = pF0_Base + (3 * decStride);
         dataType *pF1_Lead = pF1_Base + (3 * decStride);
         dataType *pF2_Lead = pF2_Base + (3 * decStride);
         dataType *pF3_Lead = pF3_Base + (3 * decStride);

         dataType *pR0_Lead = pR0_Base;
         dataType *pR1_Lead = pR1_Base;
         dataType *pR2_Lead = pR2_Base;
         dataType *pR3_Lead = pR3_Base;

         float32x4_t f0 = (float32x4_t){pF0_Base[0], pF1_Base[0], pF2_Base[0], pF3_Base[0]};
         float32x4_t f1 =
             (float32x4_t){pF0_Base[decStride], pF1_Base[decStride], pF2_Base[decStride], pF3_Base[decStride]};
         float32x4_t f2 = (float32x4_t){pF0_Base[decStride << 1], pF1_Base[decStride << 1], pF2_Base[decStride << 1],
                                        pF3_Base[decStride << 1]};

         float32x4_t r0 =
             (float32x4_t){pR0_Base[decStride], pR1_Base[decStride], pR2_Base[decStride], pR3_Base[decStride]};
         float32x4_t r1 = (float32x4_t){pR0_Base[decStride << 1], pR1_Base[decStride << 1], pR2_Base[decStride << 1],
                                        pR3_Base[decStride << 1]};
         float32x4_t r2 = (float32x4_t){pR0_Base[decStride * 3], pR1_Base[decStride * 3], pR2_Base[decStride * 3],
                                        pR3_Base[decStride * 3]};

         for (int32_t tap = 0; tap < taps1; tap += 2) {
            float32x4_t coeff0 = vld1q_dup_f32(&pFiltS1[tap]);
            float32x4_t f_new0 = (float32x4_t){*pF0_Lead, *pF1_Lead, *pF2_Lead, *pF3_Lead};
            float32x4_t r_new0 = (float32x4_t){*pR0_Lead, *pR1_Lead, *pR2_Lead, *pR3_Lead};

            acc0 = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r_new0));
            acc1 = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r2));

            float32x4_t f0_t = f1, f1_t = f2, f2_t = f_new0;
            float32x4_t r0_t = r_new0, r1_t = r0, r2_t = r1;
            (void) r2_t;

            pF0_Lead += decStride;
            pF1_Lead += decStride;
            pF2_Lead += decStride;
            pF3_Lead += decStride;
            pR0_Lead -= decStride;
            pR1_Lead -= decStride;
            pR2_Lead -= decStride;
            pR3_Lead -= decStride;

            float32x4_t coeff1 = vld1q_dup_f32(&pFiltS1[tap + 1]);
            float32x4_t f_new1 = (float32x4_t){*pF0_Lead, *pF1_Lead, *pF2_Lead, *pF3_Lead};
            float32x4_t r_new1 = (float32x4_t){*pR0_Lead, *pR1_Lead, *pR2_Lead, *pR3_Lead};

            acc0 = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_t, r_new1));
            acc1 = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_t, r0_t));
            acc2 = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_t, r1_t));
            acc3 = vmlaq_f32(acc3, coeff1, vaddq_f32(f_new1, r2_t));

            f0 = f1_t;
            f1 = f2_t;
            f2 = f_new1;
            r0 = r_new1;
            r1 = r0_t;
            r2 = r1_t;

            pF0_Lead += decStride;
            pF1_Lead += decStride;
            pF2_Lead += decStride;
            pF3_Lead += decStride;
            pR0_Lead -= decStride;
            pR1_Lead -= decStride;
            pR2_Lead -= decStride;
            pR3_Lead -= decStride;
         }

         const int32_t cOff1 = blockOffset + cTap1;
         acc0 = vmlaq_n_f32(acc0, (float32x4_t){pS1Ch0[cOff1], pS1Ch1[cOff1], pS1Ch2[cOff1], pS1Ch3[cOff1]}, 0.5f);
         acc1 = vmlaq_n_f32(acc1,
                            (float32x4_t){pS1Ch0[cOff1 + decStride], pS1Ch1[cOff1 + decStride],
                                          pS1Ch2[cOff1 + decStride], pS1Ch3[cOff1 + decStride]},
                            0.5f);
         acc2 = vmlaq_n_f32(acc2,
                            (float32x4_t){pS1Ch0[cOff1 + decStride * 2], pS1Ch1[cOff1 + decStride * 2],
                                          pS1Ch2[cOff1 + decStride * 2], pS1Ch3[cOff1 + decStride * 2]},
                            0.5f);
         acc3 = vmlaq_n_f32(acc3,
                            (float32x4_t){pS1Ch0[cOff1 + decStride * 3], pS1Ch1[cOff1 + decStride * 3],
                                          pS1Ch2[cOff1 + decStride * 3], pS1Ch3[cOff1 + decStride * 3]},
                            0.5f);

         const int32_t outBase = outB * eleCount;
         pS2Out0[outBase + 0]  = vgetq_lane_f32(acc0, 0);
         pS2Out1[outBase + 0]  = vgetq_lane_f32(acc0, 1);
         pS2Out2[outBase + 0]  = vgetq_lane_f32(acc0, 2);
         pS2Out3[outBase + 0]  = vgetq_lane_f32(acc0, 3);

         pS2Out0[outBase + 1] = vgetq_lane_f32(acc1, 0);
         pS2Out1[outBase + 1] = vgetq_lane_f32(acc1, 1);
         pS2Out2[outBase + 1] = vgetq_lane_f32(acc1, 2);
         pS2Out3[outBase + 1] = vgetq_lane_f32(acc1, 3);

         pS2Out0[outBase + 2] = vgetq_lane_f32(acc2, 0);
         pS2Out1[outBase + 2] = vgetq_lane_f32(acc2, 1);
         pS2Out2[outBase + 2] = vgetq_lane_f32(acc2, 2);
         pS2Out3[outBase + 2] = vgetq_lane_f32(acc2, 3);

         pS2Out0[outBase + 3] = vgetq_lane_f32(acc3, 0);
         pS2Out1[outBase + 3] = vgetq_lane_f32(acc3, 1);
         pS2Out2[outBase + 3] = vgetq_lane_f32(acc3, 2);
         pS2Out3[outBase + 3] = vgetq_lane_f32(acc3, 3);
      }

      for (int32_t rem = 0; rem < remainingSamples1; rem++) {
         const int32_t iRem    = completeBlocks1 * eleCount + rem;
         const int32_t baseOff = iRem * decStride;

         dataType *pF0_B = pS1Ch0 + baseOff;
         dataType *pR0_B = pF0_B + hist1;
         dataType *pF1_B = pS1Ch1 + baseOff;
         dataType *pR1_B = pF1_B + hist1;
         dataType *pF2_B = pS1Ch2 + baseOff;
         dataType *pR2_B = pF2_B + hist1;
         dataType *pF3_B = pS1Ch3 + baseOff;
         dataType *pR3_B = pF3_B + hist1;

         dataType *pFL0 = pF0_B, *pRL0 = pR0_B;
         dataType *pFL1 = pF1_B, *pRL1 = pR1_B;
         dataType *pFL2 = pF2_B, *pRL2 = pR2_B;
         dataType *pFL3 = pF3_B, *pRL3 = pR3_B;

         float32x4_t acc_a = vdupq_n_f32(0.0f);
         float32x4_t acc_b = vdupq_n_f32(0.0f);

         for (int32_t tap = 0; tap < taps1; tap += 2) {
            float32x4_t fwd_a = (float32x4_t){*pFL0, *pFL1, *pFL2, *pFL3};
            float32x4_t rev_a = (float32x4_t){*pRL0, *pRL1, *pRL2, *pRL3};
            acc_a             = vmlaq_f32(acc_a, vdupq_n_f32(pFiltS1[tap]), vaddq_f32(fwd_a, rev_a));
            pFL0 += decStride;
            pFL1 += decStride;
            pFL2 += decStride;
            pFL3 += decStride;
            pRL0 -= decStride;
            pRL1 -= decStride;
            pRL2 -= decStride;
            pRL3 -= decStride;

            float32x4_t fwd_b = (float32x4_t){*pFL0, *pFL1, *pFL2, *pFL3};
            float32x4_t rev_b = (float32x4_t){*pRL0, *pRL1, *pRL2, *pRL3};
            acc_b             = vmlaq_f32(acc_b, vdupq_n_f32(pFiltS1[tap + 1]), vaddq_f32(fwd_b, rev_b));
            pFL0 += decStride;
            pFL1 += decStride;
            pFL2 += decStride;
            pFL3 += decStride;
            pRL0 -= decStride;
            pRL1 -= decStride;
            pRL2 -= decStride;
            pRL3 -= decStride;
         }

         float32x4_t   result = vaddq_f32(acc_a, acc_b);
         const int32_t cOff1  = baseOff + cTap1;
         result = vmlaq_n_f32(result, (float32x4_t){pS1Ch0[cOff1], pS1Ch1[cOff1], pS1Ch2[cOff1], pS1Ch3[cOff1]}, 0.5f);

         pS2Out0[iRem] = vgetq_lane_f32(result, 0);
         pS2Out1[iRem] = vgetq_lane_f32(result, 1);
         pS2Out2[iRem] = vgetq_lane_f32(result, 2);
         pS2Out3[iRem] = vgetq_lane_f32(result, 3);
      }
   }

   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      dataType *pS1Ch  = pHist1Base + ch * chStride1;
      dataType *pS2Out = pHist2Base + ch * chStride2 + hist2;

      for (int32_t outB = 0; outB < completeBlocks1; outB++) {
         float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;

         const int32_t blockOffset = outB * eleCount * decStride;
         dataType     *pF_Base     = pS1Ch + blockOffset;
         dataType     *pR_Base     = pF_Base + hist1;
         dataType     *pF_Lead     = pF_Base + (3 * decStride);
         dataType     *pR_Lead     = pR_Base;

         float f0 = pF_Base[0];
         float f1 = pF_Base[decStride];
         float f2 = pF_Base[decStride << 1];
         float r0 = pR_Base[decStride];
         float r1 = pR_Base[decStride << 1];
         float r2 = pR_Base[decStride * 3];

         for (int32_t tap = 0; tap < taps1; tap += 2) {
            float coeff0 = pFiltS1[tap];
            float f_new0 = *pF_Lead;
            float r_new0 = *pR_Lead;

            acc0 += coeff0 * (f0 + r_new0);
            acc1 += coeff0 * (f1 + r0);
            acc2 += coeff0 * (f2 + r1);
            acc3 += coeff0 * (f_new0 + r2);

            float f0_t = f1, f1_t = f2, f2_t = f_new0;
            float r0_t = r_new0, r1_t = r0, r2_t = r1;
            (void) r2_t;

            pF_Lead += decStride;
            pR_Lead -= decStride;

            float coeff1 = pFiltS1[tap + 1];
            float f_new1 = *pF_Lead;
            float r_new1 = *pR_Lead;

            acc0 += coeff1 * (f0_t + r_new1);
            acc1 += coeff1 * (f1_t + r0_t);
            acc2 += coeff1 * (f2_t + r1_t);
            acc3 += coeff1 * (f_new1 + r2_t);

            f0 = f1_t;
            f1 = f2_t;
            f2 = f_new1;
            r0 = r_new1;
            r1 = r0_t;
            r2 = r1_t;

            pF_Lead += decStride;
            pR_Lead -= decStride;
         }

         const int32_t cOff1 = blockOffset + cTap1;
         acc0 += 0.5f * pS1Ch[cOff1];
         acc1 += 0.5f * pS1Ch[cOff1 + decStride];
         acc2 += 0.5f * pS1Ch[cOff1 + decStride * 2];
         acc3 += 0.5f * pS1Ch[cOff1 + decStride * 3];

         const int32_t outBase = outB * eleCount;
         pS2Out[outBase + 0]   = acc0;
         pS2Out[outBase + 1]   = acc1;
         pS2Out[outBase + 2]   = acc2;
         pS2Out[outBase + 3]   = acc3;
      }

      for (int32_t rem = 0; rem < remainingSamples1; rem++) {
         const int32_t iRem    = completeBlocks1 * eleCount + rem;
         const int32_t baseOff = iRem * decStride;
         dataType     *pF_B    = pS1Ch + baseOff;
         dataType     *pR_B    = pF_B + hist1;
         dataType     *pFL     = pF_B;
         dataType     *pRL     = pR_B;

         float acc_a = 0.0f, acc_b = 0.0f;
         for (int32_t tap = 0; tap < taps1; tap += 2) {
            acc_a += pFiltS1[tap] * (*pFL + *pRL);
            pFL += decStride;
            pRL -= decStride;
            acc_b += pFiltS1[tap + 1] * (*pFL + *pRL);
            pFL += decStride;
            pRL -= decStride;
         }
         pS2Out[iRem] = acc_a + acc_b + 0.5f * pS1Ch[baseOff + cTap1];
      }
   }

   for (int32_t ch = 0; ch < numChannels; ch++) {
      dataType *pBase = pHist1Base + ch * chStride1;
      memmove(pBase, pBase + inputSampleCount, hist1 * sizeof(dataType));
   }

   for (int32_t ch = 0; ch < vecLimit; ch += eleCount) {
      dataType *pS2Ch0 = pHist2Base + (ch + 0) * chStride2;
      dataType *pS2Ch1 = pHist2Base + (ch + 1) * chStride2;
      dataType *pS2Ch2 = pHist2Base + (ch + 2) * chStride2;
      dataType *pS2Ch3 = pHist2Base + (ch + 3) * chStride2;

      dataType *pOut0 = pOutLocal + (ch + 0) * stage2Samples;
      dataType *pOut1 = pOutLocal + (ch + 1) * stage2Samples;
      dataType *pOut2 = pOutLocal + (ch + 2) * stage2Samples;
      dataType *pOut3 = pOutLocal + (ch + 3) * stage2Samples;

      for (int32_t outB = 0; outB < completeBlocks2; outB++) {
         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         const int32_t blockOffset = outB * eleCount * decStride;

         dataType *pF0_Base = pS2Ch0 + blockOffset;
         dataType *pF1_Base = pS2Ch1 + blockOffset;
         dataType *pF2_Base = pS2Ch2 + blockOffset;
         dataType *pF3_Base = pS2Ch3 + blockOffset;

         dataType *pR0_Base = pF0_Base + hist2;
         dataType *pR1_Base = pF1_Base + hist2;
         dataType *pR2_Base = pF2_Base + hist2;
         dataType *pR3_Base = pF3_Base + hist2;

         dataType *pF0_Lead = pF0_Base + (3 * decStride);
         dataType *pF1_Lead = pF1_Base + (3 * decStride);
         dataType *pF2_Lead = pF2_Base + (3 * decStride);
         dataType *pF3_Lead = pF3_Base + (3 * decStride);

         dataType *pR0_Lead = pR0_Base;
         dataType *pR1_Lead = pR1_Base;
         dataType *pR2_Lead = pR2_Base;
         dataType *pR3_Lead = pR3_Base;

         float32x4_t f0 = (float32x4_t){pF0_Base[0], pF1_Base[0], pF2_Base[0], pF3_Base[0]};
         float32x4_t f1 =
             (float32x4_t){pF0_Base[decStride], pF1_Base[decStride], pF2_Base[decStride], pF3_Base[decStride]};
         float32x4_t f2 = (float32x4_t){pF0_Base[decStride << 1], pF1_Base[decStride << 1], pF2_Base[decStride << 1],
                                        pF3_Base[decStride << 1]};

         float32x4_t r0 =
             (float32x4_t){pR0_Base[decStride], pR1_Base[decStride], pR2_Base[decStride], pR3_Base[decStride]};
         float32x4_t r1 = (float32x4_t){pR0_Base[decStride << 1], pR1_Base[decStride << 1], pR2_Base[decStride << 1],
                                        pR3_Base[decStride << 1]};
         float32x4_t r2 = (float32x4_t){pR0_Base[decStride * 3], pR1_Base[decStride * 3], pR2_Base[decStride * 3],
                                        pR3_Base[decStride * 3]};

         for (int32_t tap = 0; tap < taps2; tap += 2) {
            float32x4_t coeff0 = vld1q_dup_f32(&pFiltS2[tap]);
            float32x4_t f_new0 = (float32x4_t){*pF0_Lead, *pF1_Lead, *pF2_Lead, *pF3_Lead};
            float32x4_t r_new0 = (float32x4_t){*pR0_Lead, *pR1_Lead, *pR2_Lead, *pR3_Lead};

            acc0 = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r_new0));
            acc1 = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r2));

            float32x4_t f0_t = f1, f1_t = f2, f2_t = f_new0;
            float32x4_t r0_t = r_new0, r1_t = r0, r2_t = r1;
            (void) r2_t;

            pF0_Lead += decStride;
            pF1_Lead += decStride;
            pF2_Lead += decStride;
            pF3_Lead += decStride;
            pR0_Lead -= decStride;
            pR1_Lead -= decStride;
            pR2_Lead -= decStride;
            pR3_Lead -= decStride;

            float32x4_t coeff1 = vld1q_dup_f32(&pFiltS2[tap + 1]);
            float32x4_t f_new1 = (float32x4_t){*pF0_Lead, *pF1_Lead, *pF2_Lead, *pF3_Lead};
            float32x4_t r_new1 = (float32x4_t){*pR0_Lead, *pR1_Lead, *pR2_Lead, *pR3_Lead};

            acc0 = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_t, r_new1));
            acc1 = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_t, r0_t));
            acc2 = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_t, r1_t));
            acc3 = vmlaq_f32(acc3, coeff1, vaddq_f32(f_new1, r2_t));

            f0 = f1_t;
            f1 = f2_t;
            f2 = f_new1;
            r0 = r_new1;
            r1 = r0_t;
            r2 = r1_t;

            pF0_Lead += decStride;
            pF1_Lead += decStride;
            pF2_Lead += decStride;
            pF3_Lead += decStride;
            pR0_Lead -= decStride;
            pR1_Lead -= decStride;
            pR2_Lead -= decStride;
            pR3_Lead -= decStride;
         }

         const int32_t cOff2 = blockOffset + cTap2;
         acc0 = vmlaq_n_f32(acc0, (float32x4_t){pS2Ch0[cOff2], pS2Ch1[cOff2], pS2Ch2[cOff2], pS2Ch3[cOff2]}, 0.5f);
         acc1 = vmlaq_n_f32(acc1,
                            (float32x4_t){pS2Ch0[cOff2 + decStride], pS2Ch1[cOff2 + decStride],
                                          pS2Ch2[cOff2 + decStride], pS2Ch3[cOff2 + decStride]},
                            0.5f);
         acc2 = vmlaq_n_f32(acc2,
                            (float32x4_t){pS2Ch0[cOff2 + decStride * 2], pS2Ch1[cOff2 + decStride * 2],
                                          pS2Ch2[cOff2 + decStride * 2], pS2Ch3[cOff2 + decStride * 2]},
                            0.5f);
         acc3 = vmlaq_n_f32(acc3,
                            (float32x4_t){pS2Ch0[cOff2 + decStride * 3], pS2Ch1[cOff2 + decStride * 3],
                                          pS2Ch2[cOff2 + decStride * 3], pS2Ch3[cOff2 + decStride * 3]},
                            0.5f);

         const int32_t outBase = outB * eleCount;
         pOut0[outBase + 0]    = vgetq_lane_f32(acc0, 0);
         pOut1[outBase + 0]    = vgetq_lane_f32(acc0, 1);
         pOut2[outBase + 0]    = vgetq_lane_f32(acc0, 2);
         pOut3[outBase + 0]    = vgetq_lane_f32(acc0, 3);

         pOut0[outBase + 1] = vgetq_lane_f32(acc1, 0);
         pOut1[outBase + 1] = vgetq_lane_f32(acc1, 1);
         pOut2[outBase + 1] = vgetq_lane_f32(acc1, 2);
         pOut3[outBase + 1] = vgetq_lane_f32(acc1, 3);

         pOut0[outBase + 2] = vgetq_lane_f32(acc2, 0);
         pOut1[outBase + 2] = vgetq_lane_f32(acc2, 1);
         pOut2[outBase + 2] = vgetq_lane_f32(acc2, 2);
         pOut3[outBase + 2] = vgetq_lane_f32(acc2, 3);

         pOut0[outBase + 3] = vgetq_lane_f32(acc3, 0);
         pOut1[outBase + 3] = vgetq_lane_f32(acc3, 1);
         pOut2[outBase + 3] = vgetq_lane_f32(acc3, 2);
         pOut3[outBase + 3] = vgetq_lane_f32(acc3, 3);
      }

      for (int32_t rem = 0; rem < remainingSamples2; rem++) {
         const int32_t iRem    = completeBlocks2 * eleCount + rem;
         const int32_t baseOff = iRem * decStride;

         dataType *pF0_B = pS2Ch0 + baseOff;
         dataType *pR0_B = pF0_B + hist2;
         dataType *pF1_B = pS2Ch1 + baseOff;
         dataType *pR1_B = pF1_B + hist2;
         dataType *pF2_B = pS2Ch2 + baseOff;
         dataType *pR2_B = pF2_B + hist2;
         dataType *pF3_B = pS2Ch3 + baseOff;
         dataType *pR3_B = pF3_B + hist2;

         dataType *pFL0 = pF0_B, *pRL0 = pR0_B;
         dataType *pFL1 = pF1_B, *pRL1 = pR1_B;
         dataType *pFL2 = pF2_B, *pRL2 = pR2_B;
         dataType *pFL3 = pF3_B, *pRL3 = pR3_B;

         float32x4_t acc_a = vdupq_n_f32(0.0f);
         float32x4_t acc_b = vdupq_n_f32(0.0f);

         for (int32_t tap = 0; tap < taps2; tap += 2) {
            float32x4_t fwd_a = (float32x4_t){*pFL0, *pFL1, *pFL2, *pFL3};
            float32x4_t rev_a = (float32x4_t){*pRL0, *pRL1, *pRL2, *pRL3};
            acc_a             = vmlaq_f32(acc_a, vdupq_n_f32(pFiltS2[tap]), vaddq_f32(fwd_a, rev_a));
            pFL0 += decStride;
            pFL1 += decStride;
            pFL2 += decStride;
            pFL3 += decStride;
            pRL0 -= decStride;
            pRL1 -= decStride;
            pRL2 -= decStride;
            pRL3 -= decStride;

            float32x4_t fwd_b = (float32x4_t){*pFL0, *pFL1, *pFL2, *pFL3};
            float32x4_t rev_b = (float32x4_t){*pRL0, *pRL1, *pRL2, *pRL3};
            acc_b             = vmlaq_f32(acc_b, vdupq_n_f32(pFiltS2[tap + 1]), vaddq_f32(fwd_b, rev_b));
            pFL0 += decStride;
            pFL1 += decStride;
            pFL2 += decStride;
            pFL3 += decStride;
            pRL0 -= decStride;
            pRL1 -= decStride;
            pRL2 -= decStride;
            pRL3 -= decStride;
         }

         float32x4_t   result = vaddq_f32(acc_a, acc_b);
         const int32_t cOff2  = baseOff + cTap2;
         result = vmlaq_n_f32(result, (float32x4_t){pS2Ch0[cOff2], pS2Ch1[cOff2], pS2Ch2[cOff2], pS2Ch3[cOff2]}, 0.5f);

         pOut0[iRem] = vgetq_lane_f32(result, 0);
         pOut1[iRem] = vgetq_lane_f32(result, 1);
         pOut2[iRem] = vgetq_lane_f32(result, 2);
         pOut3[iRem] = vgetq_lane_f32(result, 3);
      }
   }

   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      dataType *pS2Ch  = pHist2Base + ch * chStride2;
      dataType *pChOut = pOutLocal + ch * stage2Samples;

      for (int32_t outB = 0; outB < completeBlocks2; outB++) {
         float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;

         const int32_t blockOffset = outB * eleCount * decStride;
         dataType     *pF_Base     = pS2Ch + blockOffset;
         dataType     *pR_Base     = pF_Base + hist2;
         dataType     *pF_Lead     = pF_Base + (3 * decStride);
         dataType     *pR_Lead     = pR_Base;

         float f0 = pF_Base[0];
         float f1 = pF_Base[decStride];
         float f2 = pF_Base[decStride << 1];
         float r0 = pR_Base[decStride];
         float r1 = pR_Base[decStride << 1];
         float r2 = pR_Base[decStride * 3];

         for (int32_t tap = 0; tap < taps2; tap += 2) {
            float coeff0 = pFiltS2[tap];
            float f_new0 = *pF_Lead;
            float r_new0 = *pR_Lead;

            acc0 += coeff0 * (f0 + r_new0);
            acc1 += coeff0 * (f1 + r0);
            acc2 += coeff0 * (f2 + r1);
            acc3 += coeff0 * (f_new0 + r2);

            float f0_t = f1, f1_t = f2, f2_t = f_new0;
            float r0_t = r_new0, r1_t = r0, r2_t = r1;
            (void) r2_t;

            pF_Lead += decStride;
            pR_Lead -= decStride;

            float coeff1 = pFiltS2[tap + 1];
            float f_new1 = *pF_Lead;
            float r_new1 = *pR_Lead;

            acc0 += coeff1 * (f0_t + r_new1);
            acc1 += coeff1 * (f1_t + r0_t);
            acc2 += coeff1 * (f2_t + r1_t);
            acc3 += coeff1 * (f_new1 + r2_t);

            f0 = f1_t;
            f1 = f2_t;
            f2 = f_new1;
            r0 = r_new1;
            r1 = r0_t;
            r2 = r1_t;

            pF_Lead += decStride;
            pR_Lead -= decStride;
         }

         const int32_t cOff2 = blockOffset + cTap2;
         acc0 += 0.5f * pS2Ch[cOff2];
         acc1 += 0.5f * pS2Ch[cOff2 + decStride];
         acc2 += 0.5f * pS2Ch[cOff2 + decStride * 2];
         acc3 += 0.5f * pS2Ch[cOff2 + decStride * 3];

         const int32_t outBase = outB * eleCount;
         pChOut[outBase + 0]   = acc0;
         pChOut[outBase + 1]   = acc1;
         pChOut[outBase + 2]   = acc2;
         pChOut[outBase + 3]   = acc3;
      }

      for (int32_t rem = 0; rem < remainingSamples2; rem++) {
         const int32_t iRem    = completeBlocks2 * eleCount + rem;
         const int32_t baseOff = iRem * decStride;
         dataType     *pF_B    = pS2Ch + baseOff;
         dataType     *pR_B    = pF_B + hist2;
         dataType     *pFL     = pF_B;
         dataType     *pRL     = pR_B;

         float acc_a = 0.0f, acc_b = 0.0f;
         for (int32_t tap = 0; tap < taps2; tap += 2) {
            acc_a += pFiltS2[tap] * (*pFL + *pRL);
            pFL += decStride;
            pRL -= decStride;
            acc_b += pFiltS2[tap + 1] * (*pFL + *pRL);
            pFL += decStride;
            pRL -= decStride;
         }
         pChOut[iRem] = acc_a + acc_b + 0.5f * pS2Ch[baseOff + cTap2];
      }
   }

   for (int32_t ch = 0; ch < numChannels; ch++) {
      dataType *pBase = pHist2Base + ch * chStride2;
      memmove(pBase, pBase + stage1Samples, hist2 * sizeof(dataType));
   }

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
