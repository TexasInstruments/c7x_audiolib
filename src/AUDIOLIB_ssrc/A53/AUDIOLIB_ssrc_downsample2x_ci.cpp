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
 * Main initialization function for 2x downsampling using A53 NEON intrinsics.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_downsample2x_init_ci\r\n"));
#endif

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;
   uint8_t                 bufferFormat = pKerPrivArgs->initArgs.bufferFormat;
   uint8_t                 dataFormat   = pKerPrivArgs->initArgs.dataFormat;

   // Check data format first
   if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      // Check if we're using linear buffer format
      if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
         // For linear buffer format, use the specialized implementation
         status = AUDIOLIB_ssrc_downsample2x_linear_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         if (status == AUDIOLIB_SUCCESS) {
            pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample2x_linear_exec_ci<float>;
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

/*
 * Initialization for 2x downsampling using A53 NEON intrinsics and linear processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_downsample2x_linear_init_ci\r\n"));
#endif

   AUDIOLIB_STATUS         status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs      = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 outputSampleCount = pKerPrivArgs->outputSampleCount;

   // Calculate the number of output blocks needed
   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(outputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_downsample2x_non_interleaved_init_ci\r\n"));
#endif

   AUDIOLIB_STATUS         status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs      = (AUDIOLIB_ssrc_PrivArgs *) handle;
   int32_t                 numChannels       = pKerPrivArgs->initArgs.numChannels;
   int32_t                 inputSampleCount  = pKerPrivArgs->initArgs.inputSampleCount;
   int32_t                 outputSampleCount = pKerPrivArgs->outputSampleCount;

   // Filter constants
   const int32_t historySize     = static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS) - 1;
   const int32_t totalInputCount = inputSampleCount + historySize;
   const int32_t decimatedSampleCount = totalInputCount / 2;

   // Calculate buffer indices
   pKerPrivArgs->stage1BuffCurrIndex = historySize * numChannels;

   // Scratch1: starts after stage1 buffer, with 2-sample alignment
   int32_t stage1BufferSize             = totalInputCount * numChannels;
   pKerPrivArgs->scratch1BuffStartIndex = ((stage1BufferSize + 1) / 2) * 2;

   // Scratch2: starts after scratch1, with 2-sample alignment
   int32_t scratch1BufferSize           = decimatedSampleCount * numChannels;
   pKerPrivArgs->scratch2BuffStartIndex = pKerPrivArgs->scratch1BuffStartIndex + ((scratch1BufferSize + 1) / 2) * 2;

   pKerPrivArgs->numOutputBlocks1 = AUDIOLIB_ceilingDiv(outputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

   return status;
}
/*
 * Execution for 2x downsampling using A53 NEON intrinsics and linear  processing.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pState,
                                                          void *restrict pFiltCoeffs,
                                                          void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_downsample2x_linear_exec_ci\r\n"));
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

   /* Downsampling Filter parameters */
   const int32_t symTaps     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS; // 191
   const int32_t numTaps     = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS;          // 48
   const int32_t symTapsDiv2 = symTaps >> 1;                                           // 95

   /* History Management */
   const int32_t historyLen = (symTaps - 1) * numChannels; // 190 * numChannels
   const int32_t totalInput = inputSampleCount * numChannels;

   /* 1. APPEND: Add new input data after existing history in pState */
   // pHistory layout: [ OLD HISTORY (historyLen) ] + [ NEW INPUT (totalInput) ]
   memcpy(pHistory + historyLen, pNewInput, totalInput * sizeof(dataType));

   /* Vector constants */
   const int32_t eleCount  = 4;
   const int32_t decStride = numChannels * 2; // Decimation by 2: skip every other sample
   const int32_t nc3       = numChannels * 3;
   int32_t       vecLimit  = (numChannels / eleCount) * eleCount;

   /* LOOP 1: VECTORIZED (4 channels at a time) - OPTIMIZED */
   for (int32_t ch = 0; ch < vecLimit; ch += eleCount) {
      for (int32_t outB = 0; outB < numOutputBlocks; outB++) {

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         // For downsampling: output samples from input positions 0, 2, 4, 6, 8, ...
         uint32_t blockOffset = (outB * eleCount) * decStride + ch;

         // Base pointers for forward and reverse sides - now using pHistory
         dataType *ptrF_Base = &pHistory[blockOffset];
         dataType *ptrR_Base = &pHistory[blockOffset + (symTaps - 1) * numChannels]; // 190*nc ahead

         // Forward lead pointer starts at position for 4th sample (sample 3)
         dataType *ptrF_Lead = ptrF_Base + (3 * decStride);

         // Reverse lead pointer starts at base reverse position
         dataType *ptrR_Lead = ptrR_Base;

         // Initial forward loads (samples 0, 1, 2 at decimated positions)
         float32x4_t f0 = vld1q_f32(ptrF_Base);
         float32x4_t f1 = vld1q_f32(ptrF_Base + decStride);
         float32x4_t f2 = vld1q_f32(ptrF_Base + (decStride << 1));

         // Initial reverse loads (offset by decStride for samples 1, 2, 3)
         float32x4_t r0 = vld1q_f32(ptrR_Base + decStride);
         float32x4_t r1 = vld1q_f32(ptrR_Base + (decStride << 1));
         float32x4_t r2 = vld1q_f32(ptrR_Base + decStride * 3);

         /* Loop: Process symmetric FIR with decimation */
         for (int32_t tap = 0; tap < numTaps; tap++) {
            float32x4_t coeff = vld1q_dup_f32(&pFilt[tap]);
            float32x4_t f_new = vld1q_f32(ptrF_Lead);
            float32x4_t r_new = vld1q_f32(ptrR_Lead);

            acc0 = vmlaq_f32(acc0, coeff, vaddq_f32(f0, r_new));
            acc1 = vmlaq_f32(acc1, coeff, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff, vaddq_f32(f_new, r2));

            // Shift forward registers
            f0 = f1;
            f1 = f2;
            f2 = f_new;

            // Shift reverse registers
            r2 = r1;
            r1 = r0;
            r0 = r_new;

            ptrF_Lead += decStride;
            ptrR_Lead -= decStride; // Reverse goes BACKWARD
         }

         // Center tap processing (coefficient = 0.5)
         int32_t     centerOffset = symTapsDiv2 * numChannels;
         float32x4_t center0      = vld1q_f32(&pHistory[blockOffset + centerOffset]);
         float32x4_t center1      = vld1q_f32(&pHistory[blockOffset + decStride + centerOffset]);
         float32x4_t center2      = vld1q_f32(&pHistory[blockOffset + (decStride << 1) + centerOffset]);
         float32x4_t center3      = vld1q_f32(&pHistory[blockOffset + decStride * 3 + centerOffset]);

         acc0 = vmlaq_n_f32(acc0, center0, 0.5f);
         acc1 = vmlaq_n_f32(acc1, center1, 0.5f);
         acc2 = vmlaq_n_f32(acc2, center2, 0.5f);
         acc3 = vmlaq_n_f32(acc3, center3, 0.5f);

         // Output Handling
         float *pOutBase = &pOutLocal[(outB * eleCount) * numChannels + ch];

         vst1q_f32(pOutBase, acc0);
         vst1q_f32(pOutBase + numChannels, acc1);
         vst1q_f32(pOutBase + (numChannels << 1), acc2);
         vst1q_f32(pOutBase + nc3, acc3);
      }
   }

   /* LOOP 2: SCALAR REMAINDER */
   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      for (int32_t outB = 0; outB < numOutputBlocks; outB++) {
         float    acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;
         uint32_t blockOffset = (outB * eleCount) * decStride + ch;

         dataType *ptrF = &pHistory[blockOffset];
         dataType *ptrR = &pHistory[blockOffset + (symTaps - 1) * numChannels];

         dataType *ptrF_L = ptrF + (3 * decStride);
         dataType *ptrR_L = ptrR;

         float f0 = ptrF[0];
         float f1 = ptrF[decStride];
         float f2 = ptrF[decStride << 1];

         float r0 = ptrR[decStride];
         float r1 = ptrR[decStride << 1];
         float r2 = ptrR[decStride * 3];

         for (int32_t tap = 0; tap < numTaps; tap++) {
            float coeff = pFilt[tap];
            float fn    = *ptrF_L;
            float rn    = *ptrR_L;

            acc0 += coeff * (f0 + rn);
            acc1 += coeff * (f1 + r0);
            acc2 += coeff * (f2 + r1);
            acc3 += coeff * (fn + r2);

            f0 = f1;
            f1 = f2;
            f2 = fn;

            r2 = r1;
            r1 = r0;
            r0 = rn;

            ptrF_L += decStride;
            ptrR_L -= decStride;
         }

         // Center tap processing
         int32_t centerOffset = symTapsDiv2 * numChannels;
         float   center0      = pHistory[blockOffset + centerOffset];
         float   center1      = pHistory[blockOffset + decStride + centerOffset];
         float   center2      = pHistory[blockOffset + (decStride << 1) + centerOffset];
         float   center3      = pHistory[blockOffset + decStride * 3 + centerOffset];

         acc0 += 0.5f * center0;
         acc1 += 0.5f * center1;
         acc2 += 0.5f * center2;
         acc3 += 0.5f * center3;

         float *pOutBase = &pOutLocal[(outB * eleCount) * numChannels + ch];

         pOutBase[0]                = acc0;
         pOutBase[numChannels]      = acc1;
         pOutBase[numChannels << 1] = acc2;
         pOutBase[nc3]              = acc3;
      }
   }

   /* 2. UPDATE PERSISTENT HISTORY: Shift tail to beginning */
   memmove(pHistory, pHistory + totalInput, historyLen * sizeof(dataType));

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pState,
                                                                   void *restrict pFiltCoeffs,
                                                                   void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   DebugP_log(const_cast<char *>("Enter AUDIOLIB_ssrc_downsample2x_non_interleaved_exec_ci\r\n"));
#endif

   // =============================================================================
   // SSRC 2x Downsampling Kernel (ARM NEON)
   // =============================================================================

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;

   dataType       *pHistory  = (dataType *) pState;
   dataType       *pNewInput = (dataType *) pIn;
   dataType       *pOutLocal = (dataType *) pOut;
   const dataType *pFilt     = (const dataType *) pFiltCoeffs;

   const int32_t numChannels      = pKerPrivArgs->initArgs.numChannels;
   const int32_t inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;

   const int32_t symTaps     = static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS);
   const int32_t numTaps     = static_cast<int32_t>(AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS);
   const int32_t symTapsDiv2 = symTaps >> 1;

   // -------------------------------------------------------------------------
   // Buffer dimensions (per channel)
   // -------------------------------------------------------------------------
   const int32_t historyLen        = symTaps - 1;                   // 190
   const int32_t chStride          = historyLen + inputSampleCount; // per-channel state size
   const int32_t outputSampleCount = inputSampleCount / 2;          // 2× decimation

   const int32_t completeBlocks   = outputSampleCount / 4; // floor
   const int32_t remainingSamples = outputSampleCount % 4; // 0..3

   // Non-interleaved: samples of one channel are contiguous, stride = 1.
   // Decimation by 2 ↔ stepping by 2 within a channel's timeline.
   const int32_t decStride = 2;

   const int32_t eleCount = 4;
   const int32_t vecLimit = (numChannels / eleCount) * eleCount;

   // =========================================================================
   // Step 1 – Append new input after history for every channel.
   // =========================================================================
   for (int32_t ch = 0; ch < numChannels; ch++) {
      dataType       *pDst = &pHistory[ch * chStride + historyLen];
      const dataType *pSrc = &pNewInput[ch * inputSampleCount];
      memcpy(pDst, pSrc, inputSampleCount * sizeof(dataType));
   }

   // =========================================================================
   // Step 2 – Decimating symmetric FIR + centre tap.
   // =========================================================================

   /* =========================================================
       NEON, 4 channels × complete blocks of 4 outputs
      ========================================================= */
   for (int32_t ch = 0; ch < vecLimit; ch += eleCount) {
      // Base pointer to each channel's timeline in the state buffer
      dataType *pCh0 = &pHistory[(ch + 0) * chStride];
      dataType *pCh1 = &pHistory[(ch + 1) * chStride];
      dataType *pCh2 = &pHistory[(ch + 2) * chStride];
      dataType *pCh3 = &pHistory[(ch + 3) * chStride];

      // Non-interleaved output slice per channel
      dataType *pOut0 = &pOutLocal[(ch + 0) * outputSampleCount];
      dataType *pOut1 = &pOutLocal[(ch + 1) * outputSampleCount];
      dataType *pOut2 = &pOutLocal[(ch + 2) * outputSampleCount];
      dataType *pOut3 = &pOutLocal[(ch + 3) * outputSampleCount];

      for (int32_t outB = 0; outB < completeBlocks; outB++) {
         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         const int32_t blockOffset = outB * eleCount * decStride;

         dataType *pF0_Base = pCh0 + blockOffset;
         dataType *pF1_Base = pCh1 + blockOffset;
         dataType *pF2_Base = pCh2 + blockOffset;
         dataType *pF3_Base = pCh3 + blockOffset;

         dataType *pR0_Base = pF0_Base + (symTaps - 1);
         dataType *pR1_Base = pF1_Base + (symTaps - 1);
         dataType *pR2_Base = pF2_Base + (symTaps - 1);
         dataType *pR3_Base = pF3_Base + (symTaps - 1);

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

         for (int32_t tap = 0; tap < numTaps; tap += 2) {
            // ---- TAP (tap) -----------------------------------------------
            float32x4_t coeff0 = vld1q_dup_f32(&pFilt[tap]);

            float32x4_t f_new0 = (float32x4_t){*pF0_Lead, *pF1_Lead, *pF2_Lead, *pF3_Lead};
            float32x4_t r_new0 = (float32x4_t){*pR0_Lead, *pR1_Lead, *pR2_Lead, *pR3_Lead};

            acc0 = vmlaq_f32(acc0, coeff0, vaddq_f32(f0, r_new0));
            acc1 = vmlaq_f32(acc1, coeff0, vaddq_f32(f1, r0));
            acc2 = vmlaq_f32(acc2, coeff0, vaddq_f32(f2, r1));
            acc3 = vmlaq_f32(acc3, coeff0, vaddq_f32(f_new0, r2));

            // Save shifted state for second tap
            // Forward window after one shift:
            float32x4_t f0_t = f1, f1_t = f2, f2_t = f_new0;
            float32x4_t r0_t = r_new0, r1_t = r0, r2_t = r1;

            pF0_Lead += decStride;
            pF1_Lead += decStride;
            pF2_Lead += decStride;
            pF3_Lead += decStride;
            pR0_Lead -= decStride;
            pR1_Lead -= decStride;
            pR2_Lead -= decStride;
            pR3_Lead -= decStride;

            // ---- TAP (tap+1) ---------------------------------------------
            float32x4_t coeff1 = vld1q_dup_f32(&pFilt[tap + 1]);

            float32x4_t f_new1 = (float32x4_t){*pF0_Lead, *pF1_Lead, *pF2_Lead, *pF3_Lead};
            float32x4_t r_new1 = (float32x4_t){*pR0_Lead, *pR1_Lead, *pR2_Lead, *pR3_Lead};

            // Use post-one-shift state (f0_t..f2_t, r0_t..r2_t)
            acc0 = vmlaq_f32(acc0, coeff1, vaddq_f32(f0_t, r_new1));
            acc1 = vmlaq_f32(acc1, coeff1, vaddq_f32(f1_t, r0_t));
            acc2 = vmlaq_f32(acc2, coeff1, vaddq_f32(f2_t, r1_t));
            acc3 = vmlaq_f32(acc3, coeff1, vaddq_f32(f_new1, r2_t));

            // Complete the second shift for both forward and reverse windows
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

         // -----------------------------------------------------------------
         // Centre tap: coefficient = 0.5
         // -----------------------------------------------------------------
         const int32_t cOff = blockOffset + symTapsDiv2;

         acc0 = vmlaq_n_f32(acc0, (float32x4_t){pCh0[cOff], pCh1[cOff], pCh2[cOff], pCh3[cOff]}, 0.5f);
         acc1 = vmlaq_n_f32(acc1,
                            (float32x4_t){pCh0[cOff + decStride], pCh1[cOff + decStride], pCh2[cOff + decStride],
                                          pCh3[cOff + decStride]},
                            0.5f);
         acc2 = vmlaq_n_f32(acc2,
                            (float32x4_t){pCh0[cOff + decStride * 2], pCh1[cOff + decStride * 2],
                                          pCh2[cOff + decStride * 2], pCh3[cOff + decStride * 2]},
                            0.5f);
         acc3 = vmlaq_n_f32(acc3,
                            (float32x4_t){pCh0[cOff + decStride * 3], pCh1[cOff + decStride * 3],
                                          pCh2[cOff + decStride * 3], pCh3[cOff + decStride * 3]},
                            0.5f);

         const int32_t outBase = outB * eleCount;

         pOut0[outBase + 0] = vgetq_lane_f32(acc0, 0);
         pOut1[outBase + 0] = vgetq_lane_f32(acc0, 1);
         pOut2[outBase + 0] = vgetq_lane_f32(acc0, 2);
         pOut3[outBase + 0] = vgetq_lane_f32(acc0, 3);

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
      for (int32_t rem = 0; rem < remainingSamples; rem++) {
         const int32_t iRem    = completeBlocks * eleCount + rem;
         const int32_t baseOff = iRem * decStride; // per-channel offset

         // Forward and reverse window bases for this single output sample
         dataType *pF0_B = pCh0 + baseOff;
         dataType *pR0_B = pF0_B + (symTaps - 1);
         dataType *pF1_B = pCh1 + baseOff;
         dataType *pR1_B = pF1_B + (symTaps - 1);
         dataType *pF2_B = pCh2 + baseOff;
         dataType *pR2_B = pF2_B + (symTaps - 1);
         dataType *pF3_B = pCh3 + baseOff;
         dataType *pR3_B = pF3_B + (symTaps - 1);

         dataType *pFL0 = pF0_B, *pRL0 = pR0_B;
         dataType *pFL1 = pF1_B, *pRL1 = pR1_B;
         dataType *pFL2 = pF2_B, *pRL2 = pR2_B;
         dataType *pFL3 = pF3_B, *pRL3 = pR3_B;

         float32x4_t acc_a = vdupq_n_f32(0.0f); // even taps
         float32x4_t acc_b = vdupq_n_f32(0.0f); // odd taps

         for (int32_t tap = 0; tap < numTaps; tap += 2) {
            // TAP (tap) → acc_a
            float32x4_t fwd_a = (float32x4_t){*pFL0, *pFL1, *pFL2, *pFL3};
            float32x4_t rev_a = (float32x4_t){*pRL0, *pRL1, *pRL2, *pRL3};
            acc_a             = vmlaq_f32(acc_a, vdupq_n_f32(pFilt[tap]), vaddq_f32(fwd_a, rev_a));
            pFL0 += decStride;
            pFL1 += decStride;
            pFL2 += decStride;
            pFL3 += decStride;
            pRL0 -= decStride;
            pRL1 -= decStride;
            pRL2 -= decStride;
            pRL3 -= decStride;

            // TAP (tap+1) → acc_b
            float32x4_t fwd_b = (float32x4_t){*pFL0, *pFL1, *pFL2, *pFL3};
            float32x4_t rev_b = (float32x4_t){*pRL0, *pRL1, *pRL2, *pRL3};
            acc_b             = vmlaq_f32(acc_b, vdupq_n_f32(pFilt[tap + 1]), vaddq_f32(fwd_b, rev_b));
            pFL0 += decStride;
            pFL1 += decStride;
            pFL2 += decStride;
            pFL3 += decStride;
            pRL0 -= decStride;
            pRL1 -= decStride;
            pRL2 -= decStride;
            pRL3 -= decStride;
         }

         // Centre tap
         float32x4_t   result = vaddq_f32(acc_a, acc_b);
         const int32_t cOff   = baseOff + symTapsDiv2;
         result = vmlaq_n_f32(result, (float32x4_t){pCh0[cOff], pCh1[cOff], pCh2[cOff], pCh3[cOff]}, 0.5f);

         pOut0[iRem] = vgetq_lane_f32(result, 0);
         pOut1[iRem] = vgetq_lane_f32(result, 1);
         pOut2[iRem] = vgetq_lane_f32(result, 2);
         pOut3[iRem] = vgetq_lane_f32(result, 3);
      }
   }

   // =========================================================================
   // Step 3 – Scalar path: channels not covered by the 4-wide NEON loop
   // =========================================================================
   for (int32_t ch = vecLimit; ch < numChannels; ch++) {
      dataType *pCh    = &pHistory[ch * chStride];
      dataType *pChOut = &pOutLocal[ch * outputSampleCount];

      /* --- PATH C1: complete blocks of 4 output samples --- */
      for (int32_t outB = 0; outB < completeBlocks; outB++) {
         float acc0 = 0.0f, acc1 = 0.0f, acc2 = 0.0f, acc3 = 0.0f;

         const int32_t blockOffset = outB * eleCount * decStride;

         dataType *pF_Base = pCh + blockOffset;
         dataType *pR_Base = pF_Base + (symTaps - 1);
         dataType *pF_Lead = pF_Base + (3 * decStride);
         dataType *pR_Lead = pR_Base;

         float f0 = pF_Base[0];
         float f1 = pF_Base[decStride];
         float f2 = pF_Base[decStride << 1];

         float r0 = pR_Base[decStride];
         float r1 = pR_Base[decStride << 1];
         float r2 = pR_Base[decStride * 3];

         // 2×-unrolled tap loop (FIX BUG 3+4)
         for (int32_t tap = 0; tap < numTaps; tap += 2) {
            // TAP (tap)
            float coeff0 = pFilt[tap];
            float f_new0 = *pF_Lead;
            float r_new0 = *pR_Lead;

            acc0 += coeff0 * (f0 + r_new0);
            acc1 += coeff0 * (f1 + r0);
            acc2 += coeff0 * (f2 + r1);
            acc3 += coeff0 * (f_new0 + r2);

            // Save post-one-shift state
            float f0_t = f1, f1_t = f2, f2_t = f_new0;
            float r0_t = r_new0, r1_t = r0, r2_t = r1;
            (void) r2_t;

            pF_Lead += decStride;
            pR_Lead -= decStride;

            // TAP (tap+1)
            float coeff1 = pFilt[tap + 1];
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

         // Centre tap
         const int32_t cOff = blockOffset + symTapsDiv2;
         acc0 += 0.5f * pCh[cOff];
         acc1 += 0.5f * pCh[cOff + decStride];
         acc2 += 0.5f * pCh[cOff + decStride * 2];
         acc3 += 0.5f * pCh[cOff + decStride * 3];

         const int32_t outBase = outB * eleCount;
         pChOut[outBase + 0]   = acc0;
         pChOut[outBase + 1]   = acc1;
         pChOut[outBase + 2]   = acc2;
         pChOut[outBase + 3]   = acc3;
      }

      /* --- PATH C2: remaining 1-3 output samples (FIX BUG 1+2) --- */
      for (int32_t rem = 0; rem < remainingSamples; rem++) {
         const int32_t iRem    = completeBlocks * eleCount + rem;
         const int32_t baseOff = iRem * decStride;

         dataType *pF_B = pCh + baseOff;
         dataType *pR_B = pF_B + (symTaps - 1);
         dataType *pFL  = pF_B;
         dataType *pRL  = pR_B;

         float acc_a = 0.0f; // even taps
         float acc_b = 0.0f; // odd taps

         for (int32_t tap = 0; tap < numTaps; tap += 2) {
            acc_a += pFilt[tap] * (*pFL + *pRL);
            pFL += decStride;
            pRL -= decStride;
            acc_b += pFilt[tap + 1] * (*pFL + *pRL);
            pFL += decStride;
            pRL -= decStride;
         }

         // Centre tap
         pChOut[iRem] = acc_a + acc_b + 0.5f * pCh[baseOff + symTapsDiv2];
      }
   }

   // =========================================================================
   // Step 4 – Slide history forward: keep last historyLen samples per channel.
   // =========================================================================
   for (int32_t ch = 0; ch < numChannels; ch++) {
      dataType *pBase = &pHistory[ch * chStride];
      memmove(pBase, pBase + inputSampleCount, historyLen * sizeof(dataType));
   }

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

template AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_non_interleaved_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                   void *restrict pIn,
                                                                                   void *restrict pState,
                                                                                   void *restrict pFiltCoeffs,
                                                                                   void *restrict pOut);
