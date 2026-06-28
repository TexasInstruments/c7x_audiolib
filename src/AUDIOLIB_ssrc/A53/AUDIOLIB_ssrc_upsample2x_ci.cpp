// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_ssrc_priv.h"
#include <string.h>

/* N/A */

/*******************************************************************************
 * INITIALIZATION FUNCTION
 ******************************************************************************/
/*
 * Main initialization function for 2x upsampling using A53 NEON intrinsics.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_upsample2x_init_ci\r\n"));
#endif

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                 bufferFormat = pKerPrivArgs->initArgs.bufferFormat;
   uint8_t                 dataFormat   = pKerPrivArgs->initArgs.dataFormat;

   // Check data format first
   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
         // For linear buffer format, use the specialized implementation
         status = AUDIOLIB_ssrc_upsample2x_linear_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (status == AUDIOLIB_SUCCESS) {
            pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample2x_linear_exec_ci<float>;
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

/*
 * Initialization for 2x upsampling using A53 intrinsics and linear processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_upsample2x_linear_init_ci\r\n"));
#endif

   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   // Calculate the number of output blocks needed
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(inputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_ssrc_upsample2x_non_interleaved_init_ci\n");
#endif

   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   // Calculate the number of output blocks needed
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(inputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);
   return status;
}

/*
 * Execution for 2x upsampling using A53 NEON intrinsics and linear processing .
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pState,
                                                        void *restrict pFiltCoeffs,
                                                        void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_upsample2x_linear_exec_ci\r\n"));
#endif
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;
   dataType               *pNewInput    = (dataType *) pIn;
   dataType               *pHistory     = (dataType *) pState;
   dataType               *pOutLocal    = (dataType *) pOut;
   const dataType         *pFilt        = (dataType *) pFiltCoeffs;

   /* Extract Parameters */
   int32_t numChannels      = pKerPrivArgs->initArgs.numChannels;
   int32_t inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t numOutputBlocks  = pKerPrivArgs->numOutputBlocks1;

   /* Upsampling Filter parameters */
   const int32_t symTaps     = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
   const int32_t numTaps     = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS;
   const int32_t symTapsDiv2 = symTaps >> 1;

   /* History Management */
   const int32_t historyLen = (symTaps - 1) * numChannels;
   const int32_t totalInput = inputSampleCount * numChannels;

   /* 1. APPEND: Add new input data after existing history in pState */
   // pHistory layout: [ OLD HISTORY (historyLen) ] + [ NEW INPUT (totalInput) ]
   memcpy(pHistory + historyLen, pNewInput, totalInput * sizeof(dataType));

   /* Vector constants */
   const int32_t eleCount = 4;
   const int32_t nc2      = numChannels * 2;
   const int32_t nc3      = numChannels * 3;
   int32_t       vecLimit = (numChannels / eleCount) * eleCount;

   /* LOOP 1: VECTORIZED (4 channels at a time) - OPTIMIZED */
   for (int32_t ch = 0; ch < vecLimit; ch += eleCount) {
      for (int32_t outB = 0; outB < numOutputBlocks; outB++) {

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         uint32_t blockOffset = (outB * eleCount) * numChannels + ch;

         // Pre-calculate all base pointers - now using pHistory instead of pScratch
         dataType *ptrF_Base = &pHistory[blockOffset];
         dataType *ptrF_Lead = ptrF_Base + nc3;

         dataType *ptrR_Base0 = &pHistory[blockOffset];
         dataType *ptrR_Base1 = ptrR_Base0 + numChannels;
         dataType *ptrR_Base2 = ptrR_Base1 + numChannels;
         dataType *ptrR_Base3 = ptrR_Base2 + numChannels;

         // Initial forward loads
         float32x4_t f0 = vld1q_f32(ptrF_Base);
         float32x4_t f1 = vld1q_f32(ptrF_Base + numChannels);
         float32x4_t f2 = vld1q_f32(ptrF_Base + (numChannels << 1));

         /* OPTIMIZED: Loop unrolled by 2 for better instruction pipelining */
         int32_t tap;
         for (tap = 0; tap < (numTaps & ~1); tap += 2) {
            // === TAP 0 ===
            int32_t revOffset0 = (symTaps - 1 - tap) * numChannels;

            float32x4_t coeff0 = vld1q_dup_f32(&pFilt[tap]);
            float32x4_t f_new0 = vld1q_f32(ptrF_Lead);

            float32x4_t r0_0 = vld1q_f32(ptrR_Base0 + revOffset0);
            float32x4_t r1_0 = vld1q_f32(ptrR_Base1 + revOffset0);
            float32x4_t r2_0 = vld1q_f32(ptrR_Base2 + revOffset0);
            float32x4_t r3_0 = vld1q_f32(ptrR_Base3 + revOffset0);

            acc0 = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r0_0));
            acc1 = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r1_0));
            acc2 = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r2_0));
            acc3 = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r3_0));

            // Shift for next iteration
            float32x4_t f0_tmp = f1;
            float32x4_t f1_tmp = f2;
            float32x4_t f2_tmp = f_new0;
            ptrF_Lead += numChannels;

            // === TAP 1 ===
            int32_t revOffset1 = revOffset0 - numChannels; // Optimized: reuse calculation

            float32x4_t coeff1 = vld1q_dup_f32(&pFilt[tap + 1]);
            float32x4_t f_new1 = vld1q_f32(ptrF_Lead);

            float32x4_t r0_1 = vld1q_f32(ptrR_Base0 + revOffset1);
            float32x4_t r1_1 = vld1q_f32(ptrR_Base1 + revOffset1);
            float32x4_t r2_1 = vld1q_f32(ptrR_Base2 + revOffset1);
            float32x4_t r3_1 = vld1q_f32(ptrR_Base3 + revOffset1);

            acc0 = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_tmp, r0_1));
            acc1 = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_tmp, r1_1));
            acc2 = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_tmp, r2_1));
            acc3 = vmlaq_f32(acc3, coeff1, vaddq_f32(f_new1, r3_1));

            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = f_new1;
            ptrF_Lead += numChannels;
         }

         // Handle remaining tap if numTaps is odd
         if (tap < numTaps) {
            int32_t revOffset = (symTaps - 1 - tap) * numChannels;

            float32x4_t coeff = vld1q_dup_f32(&pFilt[tap]);
            float32x4_t f_new = vld1q_f32(ptrF_Lead);

            float32x4_t r0 = vld1q_f32(ptrR_Base0 + revOffset);
            float32x4_t r1 = vld1q_f32(ptrR_Base1 + revOffset);
            float32x4_t r2 = vld1q_f32(ptrR_Base2 + revOffset);
            float32x4_t r3 = vld1q_f32(ptrR_Base3 + revOffset);

            acc0 = vmlaq_f32(acc0, coeff, vaddq_f32(f0, r0));
            acc1 = vmlaq_f32(acc1, coeff, vaddq_f32(f1, r1));
            acc2 = vmlaq_f32(acc2, coeff, vaddq_f32(f2, r2));
            acc3 = vmlaq_f32(acc3, coeff, vaddq_f32(f_new, r3));
         }

         // Output Handling
         float    *pOutBase    = &pOutLocal[(outB * eleCount) * nc2 + ch];
         dataType *pCenterBase = &pHistory[blockOffset + symTapsDiv2 * numChannels];

         vst1q_f32(pOutBase, acc0);
         vst1q_f32(pOutBase + numChannels, vld1q_f32(pCenterBase));

         vst1q_f32(pOutBase + nc2, acc1);
         vst1q_f32(pOutBase + nc2 + numChannels, vld1q_f32(pCenterBase + numChannels));

         vst1q_f32(pOutBase + (nc2 << 1), acc2);
         vst1q_f32(pOutBase + (nc2 << 1) + numChannels, vld1q_f32(pCenterBase + (numChannels << 1)));

         vst1q_f32(pOutBase + nc2 * 3, acc3);
         vst1q_f32(pOutBase + nc2 * 3 + numChannels, vld1q_f32(pCenterBase + nc3));
      }
   }

   /* LOOP 2: SCALAR REMAINDER - OPTIMIZED */
   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      for (int32_t outB = 0; outB < numOutputBlocks; outB++) {
         float    acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;
         uint32_t blockOffset = (outB * eleCount) * numChannels + ch;

         dataType *ptrF   = &pHistory[blockOffset];
         dataType *ptrF_L = ptrF + nc3;

         // Pre-calculate reverse base pointers
         dataType *ptrR_Base0 = &pHistory[blockOffset];
         dataType *ptrR_Base1 = ptrR_Base0 + numChannels;
         dataType *ptrR_Base2 = ptrR_Base1 + numChannels;
         dataType *ptrR_Base3 = ptrR_Base2 + numChannels;

         float f0 = ptrF[0];
         float f1 = ptrF[numChannels];
         float f2 = ptrF[numChannels << 1];

         // Unroll by 2
         int32_t tap;
         for (tap = 0; tap < (numTaps & ~1); tap += 2) {
            // TAP 0
            float coeff0 = pFilt[tap];
            float fn0    = *ptrF_L;

            int32_t revOffset0 = (symTaps - 1 - tap) * numChannels;
            float   r0_0       = ptrR_Base0[revOffset0];
            float   r1_0       = ptrR_Base1[revOffset0];
            float   r2_0       = ptrR_Base2[revOffset0];
            float   r3_0       = ptrR_Base3[revOffset0];

            acc0 += coeff0 * (f0 + r0_0);
            acc1 += coeff0 * (f1 + r1_0);
            acc2 += coeff0 * (f2 + r2_0);
            acc3 += coeff0 * (fn0 + r3_0);

            float f0_tmp = f1;
            float f1_tmp = f2;
            float f2_tmp = fn0;
            ptrF_L += numChannels;

            // TAP 1
            float coeff1 = pFilt[tap + 1];
            float fn1    = *ptrF_L;

            int32_t revOffset1 = revOffset0 - numChannels;
            float   r0_1       = ptrR_Base0[revOffset1];
            float   r1_1       = ptrR_Base1[revOffset1];
            float   r2_1       = ptrR_Base2[revOffset1];
            float   r3_1       = ptrR_Base3[revOffset1];

            acc0 += coeff1 * (f0_tmp + r0_1);
            acc1 += coeff1 * (f1_tmp + r1_1);
            acc2 += coeff1 * (f2_tmp + r2_1);
            acc3 += coeff1 * (fn1 + r3_1);

            f0 = f1_tmp;
            f1 = f2_tmp;
            f2 = fn1;
            ptrF_L += numChannels;
         }

         // Handle remaining tap
         if (tap < numTaps) {
            float coeff = pFilt[tap];
            float fn    = *ptrF_L;

            int32_t revOffset = (symTaps - 1 - tap) * numChannels;
            float   r0        = ptrR_Base0[revOffset];
            float   r1        = ptrR_Base1[revOffset];
            float   r2        = ptrR_Base2[revOffset];
            float   r3        = ptrR_Base3[revOffset];

            acc0 += coeff * (f0 + r0);
            acc1 += coeff * (f1 + r1);
            acc2 += coeff * (f2 + r2);
            acc3 += coeff * (fn + r3);
         }

         float    *pOutBase = &pOutLocal[(outB * eleCount) * nc2 + ch];
         dataType *pC       = &pHistory[blockOffset + symTapsDiv2 * numChannels];

         pOutBase[0]           = acc0;
         pOutBase[numChannels] = *pC;

         pOutBase[nc2]               = acc1;
         pOutBase[nc2 + numChannels] = *(pC + numChannels);

         pOutBase[nc2 << 1]                 = acc2;
         pOutBase[(nc2 << 1) + numChannels] = *(pC + (numChannels << 1));

         pOutBase[nc2 * 3]               = acc3;
         pOutBase[nc2 * 3 + numChannels] = *(pC + nc3);
      }
   }

   /* 2. UPDATE PERSISTENT HISTORY: Shift tail to beginning */
   memmove(pHistory, pHistory + totalInput, historyLen * sizeof(dataType));

   return status;
}

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

   AUDIOLIB_STATUS         status           = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
   dataType               *pNewInput        = (dataType *) pIn;
   dataType               *pOutLocal        = (dataType *) pOut;
   const dataType         *pFilterLocal     = (dataType *) pFiltCoeffs;
   int32_t                 numChannels      = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;

   const int32_t filterTaps        = static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS);
   const int32_t halfTaps          = static_cast<int32_t>(AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC);
   const int32_t historyNumSamples = halfTaps - 1;
   const int32_t outputSampleCount = inputSampleCount * AUDIOLIB_SSRC_NUMBER_OF_FILTER_PHASES;

   // ============================================================================
   // State buffer layout (similar to C7x approach):
   // [stage1Buff: history + input] [scratch1Buff: FIR output]
   // ============================================================================
   dataType *pStateTyped = (dataType *) pState;

   // Calculate buffer indices (per channel, non-interleaved)
   int32_t stage1BuffStartIndex   = 0;
   int32_t scratch1BuffStartIndex = (historyNumSamples + inputSampleCount) * numChannels;

   dataType *pStage1Buff   = &pStateTyped[stage1BuffStartIndex];
   dataType *pScratch1Buff = &pStateTyped[scratch1BuffStartIndex];

   // ============================================================================
   // Step 1: Copy input data to state buffer (after the filter history)
   // Non-interleaved → Non-interleaved copy (channel-by-channel)
   // ============================================================================
   for (int32_t ch = 0; ch < numChannels; ch++) {
      memcpy(&pStage1Buff[ch * (historyNumSamples + inputSampleCount) + historyNumSamples],
             &pNewInput[ch * inputSampleCount], inputSampleCount * sizeof(dataType));
   }

   // ============================================================================
   // Step 2: Run FIR on all input samples (symmetric FIR optimization)
   // Process channel-by-channel, producing filtered samples to scratch buffer
   // ============================================================================
   const int32_t eleCount          = 4;
   const int32_t numCompleteBlocks = inputSampleCount / eleCount;
   const int32_t remainingSamples  = inputSampleCount % eleCount;
   const int32_t veclimit          = (numChannels / 4) * 4;

   // PATH A: NEON - Process 4 channels simultaneously
   for (int32_t chGroup = 0; chGroup < veclimit; chGroup += 4) {

      dataType *pCh0Timeline = &pStage1Buff[(chGroup + 0) * (historyNumSamples + inputSampleCount)];
      dataType *pCh1Timeline = &pStage1Buff[(chGroup + 1) * (historyNumSamples + inputSampleCount)];
      dataType *pCh2Timeline = &pStage1Buff[(chGroup + 2) * (historyNumSamples + inputSampleCount)];
      dataType *pCh3Timeline = &pStage1Buff[(chGroup + 3) * (historyNumSamples + inputSampleCount)];

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
         float32x4_t f0 = (float32x4_t){pCh0Timeline[s0], pCh1Timeline[s0], pCh2Timeline[s0], pCh3Timeline[s0]};
         float32x4_t f1 = (float32x4_t){pCh0Timeline[s1], pCh1Timeline[s1], pCh2Timeline[s1], pCh3Timeline[s1]};
         float32x4_t f2 = (float32x4_t){pCh0Timeline[s2], pCh1Timeline[s2], pCh2Timeline[s2], pCh3Timeline[s2]};

         int32_t fwdIdx = s0 + 3;

         // Symmetric FIR loop (2x unrolled)
         for (int32_t tap = 0; tap < filterTaps; tap += 2) {

            // TAP 0
            float32x4_t coeff0 = vld1q_dup_f32(&pFilterLocal[tap]);
            float32x4_t f_new0 =
                (float32x4_t){pCh0Timeline[fwdIdx], pCh1Timeline[fwdIdx], pCh2Timeline[fwdIdx], pCh3Timeline[fwdIdx]};

            int32_t     revIdx0 = s0 + (halfTaps - 1 - tap);
            float32x4_t r0_0    = (float32x4_t){pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                                pCh3Timeline[revIdx0]};
            acc0                = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r0_0));

            revIdx0          = s1 + (halfTaps - 1 - tap);
            float32x4_t r1_0 = (float32x4_t){pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                             pCh3Timeline[revIdx0]};
            acc1             = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r1_0));

            revIdx0          = s2 + (halfTaps - 1 - tap);
            float32x4_t r2_0 = (float32x4_t){pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                             pCh3Timeline[revIdx0]};
            acc2             = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r2_0));

            revIdx0          = s3 + (halfTaps - 1 - tap);
            float32x4_t r3_0 = (float32x4_t){pCh0Timeline[revIdx0], pCh1Timeline[revIdx0], pCh2Timeline[revIdx0],
                                             pCh3Timeline[revIdx0]};
            acc3             = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r3_0));

            float32x4_t f0_tmp = f1;
            float32x4_t f1_tmp = f2;
            float32x4_t f2_tmp = f_new0;
            fwdIdx++;

            // TAP 1
            float32x4_t coeff1 = vld1q_dup_f32(&pFilterLocal[tap + 1]);
            float32x4_t f_new1 =
                (float32x4_t){pCh0Timeline[fwdIdx], pCh1Timeline[fwdIdx], pCh2Timeline[fwdIdx], pCh3Timeline[fwdIdx]};

            int32_t     revIdx1 = s0 + (halfTaps - 2 - tap);
            float32x4_t r0_1    = (float32x4_t){pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                                pCh3Timeline[revIdx1]};
            acc0                = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_tmp, r0_1));

            revIdx1          = s1 + (halfTaps - 2 - tap);
            float32x4_t r1_1 = (float32x4_t){pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                             pCh3Timeline[revIdx1]};
            acc1             = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_tmp, r1_1));

            revIdx1          = s2 + (halfTaps - 2 - tap);
            float32x4_t r2_1 = (float32x4_t){pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
                                             pCh3Timeline[revIdx1]};
            acc2             = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_tmp, r2_1));

            revIdx1          = s3 + (halfTaps - 2 - tap);
            float32x4_t r3_1 = (float32x4_t){pCh0Timeline[revIdx1], pCh1Timeline[revIdx1], pCh2Timeline[revIdx1],
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

      // Remaining samples (1-3)
      for (int32_t rem = 0; rem < remainingSamples; rem++) {
         int32_t sIdx = numCompleteBlocks * 4 + rem;

         float32x4_t acc_a = vdupq_n_f32(0.0f);
         float32x4_t acc_b = vdupq_n_f32(0.0f);

         for (int32_t tap = 0; tap < filterTaps; tap += 2) {
            float32x4_t fwd0 = (float32x4_t){pCh0Timeline[sIdx + tap], pCh1Timeline[sIdx + tap],
                                             pCh2Timeline[sIdx + tap], pCh3Timeline[sIdx + tap]};
            float32x4_t rev0 =
                (float32x4_t){pCh0Timeline[sIdx + halfTaps - 1 - tap], pCh1Timeline[sIdx + halfTaps - 1 - tap],
                              pCh2Timeline[sIdx + halfTaps - 1 - tap], pCh3Timeline[sIdx + halfTaps - 1 - tap]};
            acc_a = vmlaq_f32(acc_a, vdupq_n_f32(pFilterLocal[tap]), vaddq_f32(fwd0, rev0));

            float32x4_t fwd1 = (float32x4_t){pCh0Timeline[sIdx + tap + 1], pCh1Timeline[sIdx + tap + 1],
                                             pCh2Timeline[sIdx + tap + 1], pCh3Timeline[sIdx + tap + 1]};
            float32x4_t rev1 =
                (float32x4_t){pCh0Timeline[sIdx + halfTaps - 2 - tap], pCh1Timeline[sIdx + halfTaps - 2 - tap],
                              pCh2Timeline[sIdx + halfTaps - 2 - tap], pCh3Timeline[sIdx + halfTaps - 2 - tap]};
            acc_b = vmlaq_f32(acc_b, vdupq_n_f32(pFilterLocal[tap + 1]), vaddq_f32(fwd1, rev1));
         }

         float32x4_t acc = vaddq_f32(acc_a, acc_b);

         pCh0Scratch[sIdx] = vgetq_lane_f32(acc, 0);
         pCh1Scratch[sIdx] = vgetq_lane_f32(acc, 1);
         pCh2Scratch[sIdx] = vgetq_lane_f32(acc, 2);
         pCh3Scratch[sIdx] = vgetq_lane_f32(acc, 3);
      }
   }

   // PATH C: Scalar tail channels
   for (int32_t ch = veclimit; ch < numChannels; ch++) {
      dataType *pChTimeline = &pStage1Buff[ch * (historyNumSamples + inputSampleCount)];
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

         for (int32_t tap = 0; tap < filterTaps; tap += 2) {
            float coeff0 = pFilterLocal[tap];
            float f_new0 = pChTimeline[fwdIdx];

            acc0 += coeff0 * (f0 + pChTimeline[s0 + halfTaps - 1 - tap]);
            acc1 += coeff0 * (f1 + pChTimeline[s1 + halfTaps - 1 - tap]);
            acc2 += coeff0 * (f2 + pChTimeline[s2 + halfTaps - 1 - tap]);
            acc3 += coeff0 * (f_new0 + pChTimeline[s3 + halfTaps - 1 - tap]);

            float f0_tmp = f1, f1_tmp = f2, f2_tmp = f_new0;
            fwdIdx++;

            float coeff1 = pFilterLocal[tap + 1];
            float f_new1 = pChTimeline[fwdIdx];

            acc0 += coeff1 * (f0_tmp + pChTimeline[s0 + halfTaps - 2 - tap]);
            acc1 += coeff1 * (f1_tmp + pChTimeline[s1 + halfTaps - 2 - tap]);
            acc2 += coeff1 * (f2_tmp + pChTimeline[s2 + halfTaps - 2 - tap]);
            acc3 += coeff1 * (f_new1 + pChTimeline[s3 + halfTaps - 2 - tap]);

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

         for (int32_t tap = 0; tap < filterTaps; tap += 2) {
            acc_a += pFilterLocal[tap] * (pChTimeline[sIdx + tap] + pChTimeline[sIdx + halfTaps - 1 - tap]);
            acc_b += pFilterLocal[tap + 1] * (pChTimeline[sIdx + tap + 1] + pChTimeline[sIdx + halfTaps - 2 - tap]);
         }

         pChScratch[sIdx] = acc_a + acc_b;
      }
   }

   // ============================================================================
   // Step 3: Interleave FIR output with original samples directly to output
   // This is the ARM NEON equivalent of C7x's vstore_interleave operations
   // ============================================================================
   for (int32_t ch = 0; ch < numChannels; ch++) {
      dataType *pChScratch  = &pScratch1Buff[ch * inputSampleCount];
      dataType *pChOriginal = &pStage1Buff[ch * (historyNumSamples + inputSampleCount) + filterTaps];
      dataType *pChOut      = &pOutLocal[ch * outputSampleCount];

      // Interleave: [fir[0], orig[0], fir[1], orig[1], ...]
      for (int32_t i = 0; i < inputSampleCount; i++) {
         pChOut[i * 2 + 0] = pChScratch[i];  // Filtered sample (even phase)
         pChOut[i * 2 + 1] = pChOriginal[i]; // Original sample (odd phase)
      }
   }

   // ============================================================================
   // Step 5: Copy last filterLength samples to beginning for next time
   // Update history for next processing block
   // ============================================================================
   for (int32_t ch = 0; ch < numChannels; ch++) {
      memcpy(&pStage1Buff[ch * (historyNumSamples + inputSampleCount)],
             &pStage1Buff[ch * (historyNumSamples + inputSampleCount) + inputSampleCount],
             historyNumSamples * sizeof(dataType));
   }

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

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_init_ci<float>(AUDIOLIB_kernelHandle   handle,
                                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

// Execution function template instantiations
template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                        void *restrict pIn,
                                                                        void *restrict pState,
                                                                        void *restrict pFiltCoeffs,
                                                                        void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                 void *restrict pIn,
                                                                                 void *restrict pState,
                                                                                 void *restrict pFiltCoeffs,
                                                                                 void *restrict pOut);
