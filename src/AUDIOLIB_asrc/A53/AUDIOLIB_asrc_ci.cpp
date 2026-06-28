// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_asrc_priv.h"
#include <arm_neon.h>
#include <math.h>
#include <string.h>

#define AUDIOLIB_ASRC_MIN(x, y) (((x) < (y)) ? (x) : (y))

/*******************************************************************************
 * INITIALIZATION FUNCTIONS
 ******************************************************************************/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_non_interleaved(AUDIOLIB_kernelHandle         handle,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                      const AUDIOLIB_asrc_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_init_ci_non_interleaved\n");
#endif

   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_interleaved(AUDIOLIB_kernelHandle         handle,
                                                  const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                  const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                  const AUDIOLIB_asrc_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_init_ci_interleaved\n");
#endif

   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_non_interleaved<float>(AUDIOLIB_kernelHandle         handle,
                                                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                      const AUDIOLIB_asrc_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_interleaved<float>(AUDIOLIB_kernelHandle         handle,
                                                                  const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                  const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                  const AUDIOLIB_asrc_InitArgs *pKerInitArgs);

/*******************************************************************************
 * EXECUTION FUNCTIONS
 ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_non_interleaved(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pNonInterleavedData,
                                                      void *restrict pFiltCoeffs,
                                                      void *restrict pFilterRembuf,
                                                      void *restrict pOut,
                                                      const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                      AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_exec_ci_non_interleaved\n");
#endif

   AUDIOLIB_STATUS         status             = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs       = (AUDIOLIB_asrc_PrivArgs *) handle;
   dataType               *pInLocal           = (dataType *) pIn;
   dataType               *pOutLocal          = (dataType *) pOut;
   dataType               *pFilterRembufLocal = (dataType *) pFilterRembuf;
   const dataType         *pFiltCoeffsLocal   = (dataType *) pFiltCoeffs;
   dataType               *effective_in       = (dataType *) pNonInterleavedData;

   const uint8_t numChannels           = pKerPrivArgs->initArgs.numChannels;
   uint32_t      inputSampleCount      = pKerInArgs->inputSampleCount;
   int32_t       preCompOutputCountInt = 0;
   int32_t       i                     = 0;

   const int32_t  history_length    = pKerPrivArgs->history_length;
   const int32_t  effective_dim     = pKerPrivArgs->inBufferTotalDimX; // For internal buffer
   const uint32_t inputBufferStride = pKerPrivArgs->inBufferTotalDimX; // ← Use this for input!

   // Build effective input buffer: [history | new_input] for each channel
   for (i = 0; i < numChannels; i++) {
      memcpy(effective_in + i * effective_dim + history_length,
             pInLocal + i * inputBufferStride, // ← Use inputBufferStride here!
             inputSampleCount * sizeof(dataType));
   }

   // output / input time step
   /* maxOutputsPerInputRatio will set the maximum rate ratio change acceptable*/
   /** Accumulator increment value. This variable stores the accumulator increment value, which is calculated as
    * 1/asrcRatio.
    */
   const double rho = 1 / AUDIOLIB_ASRC_MIN(pKerPrivArgs->outputsPerInputRatio, pKerPrivArgs->maxOutputsPerInputRatio);

   /** Accumulator value. This variable stores the accumulator value used in the ARC2 core processing.
    */
   const double tau = rho * (1 - pKerPrivArgs->fracOutputsRemaining);

   // Use rho directly without storing it in pKerPrivArgs->rho
   double preCompOutputCnt = ((double) pKerInArgs->inputSampleCount / rho) + pKerPrivArgs->fracOutputsRemaining;

   /* This condition check has to be done to prevent the algorithm from interpolating the last output sample without an
    * full input sample.
    * Ex:- for 32 kHz -> 48 kHz example,
    *      Assume input sample count = 256
    *      so precomputer output sample count = 256 * 48/32 = 384 samples
    *      But if you try to generate 384 samples in the algorithm, it'll interpolate the last output sample without an
    *      input sample because it has run out of all 256 input samples. This 384th output sample perfectly aligns with
    *      the 257th input sample (if we have one!). So if this perfect alignment happens we need to reduce 1 output
    *      sample. Which means fracOutputsRemaining will be 1;
    */
   if (preCompOutputCnt == (int32_t) preCompOutputCnt) {
      preCompOutputCnt--;
      preCompOutputCountInt              = (int32_t) preCompOutputCnt;
      pKerPrivArgs->fracOutputsRemaining = 1.0;
   }
   else {
      preCompOutputCountInt = (int32_t) preCompOutputCnt;
      /* Update time for next invocation;
         effectively, tau is "reset" each call;
         "fracOutputsRemaining" is fraction of next output sample
         "available" at end of this input block */
      pKerPrivArgs->fracOutputsRemaining = preCompOutputCnt - (double) preCompOutputCountInt;
   }

   // Copy remainder buffer to output
   for (i = 0; i < numChannels; i++) {
      memcpy(pOutLocal + i * pKerPrivArgs->outBufferDimX,
             pFilterRembufLocal + i * pKerPrivArgs->bufParamsFilterRembuf.dim_x,
             pKerPrivArgs->remBufCount * AUDIOLIB_sizeof(pKerPrivArgs->bufParamsFilterRembuf.data_type));
   }

   pOutLocal += pKerPrivArgs->remBufCount;

   const int32_t rhoInt  = (int32_t) ROUND((double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 *
                                           (double) (rho)); /* output time incr. w.r.t. input sample */
   double        tauTemp = (double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 * (double) (tau);
   int32_t       tauInt  = (int32_t) ROUND(tauTemp); /* output time w.r.t. input sample */
   uint32_t      inputSampleIncrement =
       (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
   uint32_t nearestInputSampleIndex = inputSampleIncrement;

   const float    scale     = (float) AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES;
   const uint32_t outStride = pKerPrivArgs->outBufferDimX;

   // Main output sample loop
   for (int32_t outSampleIdx = 0; outSampleIdx < preCompOutputCountInt; outSampleIdx++) {
      uint32_t    k   = (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_PHASE_MASK) >> AUDIOLIB_ASRC_PHASE_SHIFT);
      const float dt  = (float) (((uint32_t) tauInt & AUDIOLIB_ASRC_FRACTION_MASK)) * AUDIOLIB_ASRC_FRACTION_SCALE;
      const float dt2 = dt * dt;
      const float dt3 = dt * dt2;

      /* 3rd-order Lagrange interpolation coefficients */
      /* for use with 0 <= dt < 1 */
      // clang-format off
      const float q30 = (-1.f / 3) * dt + 0.5f * dt2 + (-1.f / 6) * dt3;
      const float q31 = 1.f - 0.5f * dt - dt2 + 0.5f * dt3;
      const float q32 = dt + 0.5f * dt2 - 0.5f * dt3;
      const float q33 = (-1.f / 6) * dt + (1.f / 6) * dt3;
      // clang-format on

      // Broadcast coefficients to NEON vectors
      const float32x4_t qv0 = vdupq_n_f32(q30);
      const float32x4_t qv1 = vdupq_n_f32(q31);
      const float32x4_t qv2 = vdupq_n_f32(q32);
      const float32x4_t qv3 = vdupq_n_f32(q33);

      uint32_t kBase = k * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
      uint32_t baseJ = nearestInputSampleIndex;

      // Filter coefficient pointers
      const float *ptr0 = &pFiltCoeffsLocal[kBase];
      const float *ptr1 = ptr0 + AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
      const float *ptr2 = ptr1 + AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
      const float *ptr3 = ptr2 + AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;

      // ========================================================================
      // OPTIMIZATION: Pre-compute interpolated filter coefficients
      // Compute once, reuse for all channels
      // ========================================================================
      float32x4_t hq_cache[16]; // Cache for 16 blocks of 4 taps

      for (int32_t tap_block = 0; tap_block < 16; tap_block++) {
         const int32_t offset = tap_block * 4;

         float32x4_t c0 = vld1q_f32(ptr0 + offset);
         float32x4_t c1 = vld1q_f32(ptr1 + offset);
         float32x4_t c2 = vld1q_f32(ptr2 + offset);
         float32x4_t c3 = vld1q_f32(ptr3 + offset);

         // Interpolate and cache
         float32x4_t hq = vmulq_f32(c0, qv0);
         hq             = vmlaq_f32(hq, c1, qv1);
         hq             = vmlaq_f32(hq, c2, qv2);
         hq             = vmlaq_f32(hq, c3, qv3);

         hq_cache[tap_block] = hq;
      }

      int32_t ch = 0;

      // ========================================================================
      // Process 4 channels at a time (uses cached filter coefficients)
      // ========================================================================
      while (ch + 4 <= numChannels) {
         float *in_ch0 = effective_in + (ch + 0) * effective_dim;
         float *in_ch1 = effective_in + (ch + 1) * effective_dim;
         float *in_ch2 = effective_in + (ch + 2) * effective_dim;
         float *in_ch3 = effective_in + (ch + 3) * effective_dim;

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         // Process using pre-computed filter coefficients
         // Unroll 2x for better pipeline utilization
         for (int32_t tap_block = 0; tap_block < 16; tap_block += 2) {
            const int32_t offset0 = tap_block * 4;
            const int32_t offset1 = (tap_block + 1) * 4;

            // Block 0
            float32x4_t hq0   = hq_cache[tap_block];
            float32x4_t in0_0 = vld1q_f32(&in_ch0[baseJ + offset0]);
            float32x4_t in1_0 = vld1q_f32(&in_ch1[baseJ + offset0]);
            float32x4_t in2_0 = vld1q_f32(&in_ch2[baseJ + offset0]);
            float32x4_t in3_0 = vld1q_f32(&in_ch3[baseJ + offset0]);

            // Block 1
            float32x4_t hq1   = hq_cache[tap_block + 1];
            float32x4_t in0_1 = vld1q_f32(&in_ch0[baseJ + offset1]);
            float32x4_t in1_1 = vld1q_f32(&in_ch1[baseJ + offset1]);
            float32x4_t in2_1 = vld1q_f32(&in_ch2[baseJ + offset1]);
            float32x4_t in3_1 = vld1q_f32(&in_ch3[baseJ + offset1]);

            // Accumulate block 0 (can dual-issue with block 1)
            acc0 = vmlaq_f32(acc0, hq0, in0_0);
            acc1 = vmlaq_f32(acc1, hq0, in1_0);
            acc2 = vmlaq_f32(acc2, hq0, in2_0);
            acc3 = vmlaq_f32(acc3, hq0, in3_0);

            // Accumulate block 1
            acc0 = vmlaq_f32(acc0, hq1, in0_1);
            acc1 = vmlaq_f32(acc1, hq1, in1_1);
            acc2 = vmlaq_f32(acc2, hq1, in2_1);
            acc3 = vmlaq_f32(acc3, hq1, in3_1);
         }

         // Horizontal reduction (optimized)
         float32x2_t low0  = vget_low_f32(acc0);
         float32x2_t high0 = vget_high_f32(acc0);
         float32x2_t low1  = vget_low_f32(acc1);
         float32x2_t high1 = vget_high_f32(acc1);
         float32x2_t low2  = vget_low_f32(acc2);
         float32x2_t high2 = vget_high_f32(acc2);
         float32x2_t low3  = vget_low_f32(acc3);
         float32x2_t high3 = vget_high_f32(acc3);

         float32x2_t sum0 = vadd_f32(low0, high0);
         float32x2_t sum1 = vadd_f32(low1, high1);
         float32x2_t sum2 = vadd_f32(low2, high2);
         float32x2_t sum3 = vadd_f32(low3, high3);

         sum0 = vpadd_f32(sum0, sum1); // [sum0, sum1]
         sum2 = vpadd_f32(sum2, sum3); // [sum2, sum3]

         // Store using pointer arithmetic (faster than array indexing)
         *(pOutLocal + ch * outStride)       = vget_lane_f32(sum0, 0) * scale;
         *(pOutLocal + (ch + 1) * outStride) = vget_lane_f32(sum0, 1) * scale;
         *(pOutLocal + (ch + 2) * outStride) = vget_lane_f32(sum2, 0) * scale;
         *(pOutLocal + (ch + 3) * outStride) = vget_lane_f32(sum2, 1) * scale;

         ch += 4;
      }

      // ========================================================================
      // Process 3 remaining channels
      // ========================================================================
      if (ch + 3 <= numChannels) {
         float *in_ch0 = effective_in + (ch + 0) * effective_dim;
         float *in_ch1 = effective_in + (ch + 1) * effective_dim;
         float *in_ch2 = effective_in + (ch + 2) * effective_dim;

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);

         for (int32_t tap_block = 0; tap_block < 16; tap_block += 2) {
            const int32_t offset0 = tap_block * 4;
            const int32_t offset1 = (tap_block + 1) * 4;

            float32x4_t hq0 = hq_cache[tap_block];
            float32x4_t hq1 = hq_cache[tap_block + 1];

            float32x4_t in0_0 = vld1q_f32(&in_ch0[baseJ + offset0]);
            float32x4_t in1_0 = vld1q_f32(&in_ch1[baseJ + offset0]);
            float32x4_t in2_0 = vld1q_f32(&in_ch2[baseJ + offset0]);

            float32x4_t in0_1 = vld1q_f32(&in_ch0[baseJ + offset1]);
            float32x4_t in1_1 = vld1q_f32(&in_ch1[baseJ + offset1]);
            float32x4_t in2_1 = vld1q_f32(&in_ch2[baseJ + offset1]);

            acc0 = vmlaq_f32(acc0, hq0, in0_0);
            acc1 = vmlaq_f32(acc1, hq0, in1_0);
            acc2 = vmlaq_f32(acc2, hq0, in2_0);

            acc0 = vmlaq_f32(acc0, hq1, in0_1);
            acc1 = vmlaq_f32(acc1, hq1, in1_1);
            acc2 = vmlaq_f32(acc2, hq1, in2_1);
         }

         float32x2_t sum0 = vpadd_f32(vget_low_f32(acc0), vget_high_f32(acc0));
         float32x2_t sum1 = vpadd_f32(vget_low_f32(acc1), vget_high_f32(acc1));
         float32x2_t sum2 = vpadd_f32(vget_low_f32(acc2), vget_high_f32(acc2));

         sum0 = vpadd_f32(sum0, sum1);
         sum2 = vpadd_f32(sum2, sum2);

         *(pOutLocal + ch * outStride)       = vget_lane_f32(sum0, 0) * scale;
         *(pOutLocal + (ch + 1) * outStride) = vget_lane_f32(sum0, 1) * scale;
         *(pOutLocal + (ch + 2) * outStride) = vget_lane_f32(sum2, 0) * scale;

         ch += 3;
      }

      // ========================================================================
      // Process 2 remaining channels
      // ========================================================================
      if (ch + 2 <= numChannels) {
         float *in_ch0 = effective_in + (ch + 0) * effective_dim;
         float *in_ch1 = effective_in + (ch + 1) * effective_dim;

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);

         for (int32_t tap_block = 0; tap_block < 16; tap_block += 2) {
            const int32_t offset0 = tap_block * 4;
            const int32_t offset1 = (tap_block + 1) * 4;

            float32x4_t hq0 = hq_cache[tap_block];
            float32x4_t hq1 = hq_cache[tap_block + 1];

            float32x4_t in0_0 = vld1q_f32(&in_ch0[baseJ + offset0]);
            float32x4_t in1_0 = vld1q_f32(&in_ch1[baseJ + offset0]);

            float32x4_t in0_1 = vld1q_f32(&in_ch0[baseJ + offset1]);
            float32x4_t in1_1 = vld1q_f32(&in_ch1[baseJ + offset1]);

            acc0 = vmlaq_f32(acc0, hq0, in0_0);
            acc1 = vmlaq_f32(acc1, hq0, in1_0);

            acc0 = vmlaq_f32(acc0, hq1, in0_1);
            acc1 = vmlaq_f32(acc1, hq1, in1_1);
         }

         float32x2_t sum = vpadd_f32(vget_low_f32(acc0), vget_high_f32(acc0));
         sum             = vpadd_f32(sum, vpadd_f32(vget_low_f32(acc1), vget_high_f32(acc1)));

         *(pOutLocal + ch * outStride)       = vget_lane_f32(sum, 0) * scale;
         *(pOutLocal + (ch + 1) * outStride) = vget_lane_f32(sum, 1) * scale;

         ch += 2;
      }

      // ========================================================================
      // Process 1 remaining channel
      // ========================================================================
      if (ch < numChannels) {
         float *in_ch0 = effective_in + ch * effective_dim;

         float32x4_t acc0 = vdupq_n_f32(0.0f);

         for (int32_t tap_block = 0; tap_block < 16; tap_block += 2) {
            const int32_t offset0 = tap_block * 4;
            const int32_t offset1 = (tap_block + 1) * 4;

            float32x4_t hq0 = hq_cache[tap_block];
            float32x4_t hq1 = hq_cache[tap_block + 1];

            float32x4_t in0_0 = vld1q_f32(&in_ch0[baseJ + offset0]);
            float32x4_t in0_1 = vld1q_f32(&in_ch0[baseJ + offset1]);

            acc0 = vmlaq_f32(acc0, hq0, in0_0);
            acc0 = vmlaq_f32(acc0, hq1, in0_1);
         }

         float32x2_t sum = vadd_f32(vget_low_f32(acc0), vget_high_f32(acc0));
         sum             = vpadd_f32(sum, sum);

         *(pOutLocal + ch * outStride) = vget_lane_f32(sum, 0) * scale;
      }

      // Advance to next output sample
      pOutLocal++;

      // Update time tracking
      tauInt -= (int32_t) (AUDIOLIB_ASRC_FIXEDPOINT_Q28 * inputSampleIncrement);
      tauInt += rhoInt;
      inputSampleIncrement =
          (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
      nearestInputSampleIndex += inputSampleIncrement;
   }

   // Update history buffer for next frame
   for (i = 0; i < numChannels; i++) {
      memcpy(effective_in + i * effective_dim, effective_in + i * effective_dim + inputSampleCount,
             history_length * sizeof(dataType));
   }

   // Handle frame modulo factor
   pKerOutArgs->outputSampleCount = preCompOutputCountInt + pKerPrivArgs->remBufCount;
   pKerPrivArgs->remBufCount      = pKerOutArgs->outputSampleCount % pKerPrivArgs->initArgs.frameModuloFactor;
   pKerOutArgs->outputSampleCount = pKerOutArgs->outputSampleCount - pKerPrivArgs->remBufCount;

   // Save remainder samples for next frame
   for (i = 0; i < numChannels; i++) {
      memcpy(pFilterRembufLocal + i * pKerPrivArgs->bufParamsFilterRembuf.dim_x,
             (dataType *) pOut + pKerOutArgs->outputSampleCount + i * pKerPrivArgs->outBufferDimX,
             pKerPrivArgs->remBufCount * AUDIOLIB_sizeof(pKerPrivArgs->bufParamsFilterRembuf.data_type));
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_interleaved(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pNonInterleavedData,
                                                  void *restrict pFiltCoeffs,
                                                  void *restrict pFilterRembuf,
                                                  void *restrict pOut,
                                                  const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                  AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_exec_ci_interleaved\n");
#endif

   AUDIOLIB_STATUS         status                = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs          = (AUDIOLIB_asrc_PrivArgs *) handle;
   dataType               *pOutLocal             = (dataType *) pOut;
   dataType               *pFilterRembufLocal    = (dataType *) pFilterRembuf;
   const dataType         *pFiltCoeffsLocal      = (dataType *) pFiltCoeffs;
   dataType               *effective_in          = (dataType *) pNonInterleavedData;
   const uint8_t           numChannels           = pKerPrivArgs->initArgs.numChannels;
   uint32_t                inputSampleCount      = pKerInArgs->inputSampleCount;
   int32_t                 preCompOutputCountInt = 0;
   int32_t                 i                     = 0;

   const int32_t history_length = pKerPrivArgs->history_length; // set in init: taps - 1
   const int32_t effective_dim  = history_length + inputSampleCount;

   // Build effective input: history (already in pNonInterleavedData) + current input (de-interleave input)
   const dataType *pInLocal = (const dataType *) pIn;
   for (i = 0; i < numChannels; i++) {
      // De-interleave current input into channel i (after history section)
      dataType       *dst = effective_in + i * effective_dim + history_length;
      const dataType *src = pInLocal + i; // start at channel offset
      for (uint32_t s = 0; s < inputSampleCount; s++) {
         dst[s] = src[s * numChannels];
      }
   }
   // output / input time step
   /* maxOutputsPerInputRatio will set the maximum rate ratio change acceptable*/
   /** Accumulator increment value. This variable stores the accumulator increment value, which is calculated as
    * 1/asrcRatio.
    */
   const double rho = 1 / AUDIOLIB_ASRC_MIN(pKerPrivArgs->outputsPerInputRatio, pKerPrivArgs->maxOutputsPerInputRatio);
   const double tau = rho * (1.0 - pKerPrivArgs->fracOutputsRemaining);

   double preCompOutputCnt = ((double) inputSampleCount / rho) + pKerPrivArgs->fracOutputsRemaining;
   /* This condition check has to be done to prevent the algorithm from interpolating the last output sample without an
    * full input sample.
    * Ex:- for 32 kHz -> 48 kHz example,
    *      Assume input sample count = 256
    *      so precomputer output sample count = 256 * 48/32 = 384 samples
    *      But if you try to generate 384 samples in the algorithm, it'll interpolate the last output sample without an
    *      input sample because it has run out of all 256 input samples. This 384th output sample perfectly aligns with
    *      the 257th input sample (if we have one!). So if this perfect alignment happens we need to reduce 1 output
    *      sample. Which means fracOutputsRemaining will be 1;
    */
   if (preCompOutputCnt == (int32_t) preCompOutputCnt) {
      preCompOutputCnt--;
      preCompOutputCountInt              = (int32_t) preCompOutputCnt;
      pKerPrivArgs->fracOutputsRemaining = 1.0;
   }
   else {
      preCompOutputCountInt = (int32_t) preCompOutputCnt;
      /* Update time for next invocation;
         effectively, tau is "reset" each call;
         "fracOutputsRemaining" is fraction of next output sample
         "available" at end of this input block */
      pKerPrivArgs->fracOutputsRemaining = preCompOutputCnt - (double) preCompOutputCountInt;
   }

   memcpy(pOutLocal, pFilterRembufLocal, numChannels * pKerPrivArgs->remBufCount * sizeof(dataType));

   pOutLocal += pKerPrivArgs->remBufCount * (int32_t) numChannels;

   const int32_t rhoInt  = (int32_t) ROUND((double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 * (double) rho);
   double        tauTemp = (double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 * (double) tau;
   int32_t       tauInt  = (int32_t) ROUND(tauTemp);
   uint32_t      inputSampleIncrement =
       (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
   uint32_t nearestInputSampleIndex = inputSampleIncrement;

   // Precompute scaling factor
   const float scale = (float) AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES;

   // ────────────────────────────────────────────────────────────────
   // Main processing loop – ARM NEON optimized for A53
   // ────────────────────────────────────────────────────────────────
   for (int32_t outSampleIdx = 0; outSampleIdx < preCompOutputCountInt; outSampleIdx++) {

      uint32_t    k   = (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_PHASE_MASK) >> AUDIOLIB_ASRC_PHASE_SHIFT);
      const float dt  = (float) (((uint32_t) tauInt & AUDIOLIB_ASRC_FRACTION_MASK)) * AUDIOLIB_ASRC_FRACTION_SCALE;
      const float dt2 = dt * dt;
      const float dt3 = dt * dt2;

      /* 3rd-order Lagrange interpolation coefficients */
      /* for use with 0 <= dt < 1 */
      // clang-format off
      const float q30 = (-1.f / 3) * dt + 0.5f * dt2 + (-1.f / 6) * dt3;
      const float q31 = 1.f - 0.5f * dt - dt2 + 0.5f * dt3;
      const float q32 = dt + 0.5f * dt2 - 0.5f * dt3;
      const float q33 = (-1.f / 6) * dt + (1.f / 6) * dt3;
      // clang-format on

      /* Broadcast coefficients for vector operations */
      const float32x4_t qv0 = vdupq_n_f32(q30);
      const float32x4_t qv1 = vdupq_n_f32(q31);
      const float32x4_t qv2 = vdupq_n_f32(q32);
      const float32x4_t qv3 = vdupq_n_f32(q33);

      uint32_t kBase = k * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
      uint32_t baseJ = nearestInputSampleIndex;

      // Filter coefficient pointers
      const float *ptr0 = &pFiltCoeffsLocal[kBase];
      const float *ptr1 = ptr0 + AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
      const float *ptr2 = ptr1 + AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;
      const float *ptr3 = ptr2 + AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;

      // ========================================================================
      // OPTIMIZATION: Pre-compute interpolated filter coefficients
      // Compute once, reuse for all channels
      // ========================================================================
      float32x4_t hq_cache[16]; // Cache for 16 blocks of 4 taps

      for (int32_t tap_block = 0; tap_block < 16; tap_block++) {
         const int32_t offset = tap_block * 4;

         float32x4_t c0 = vld1q_f32(ptr0 + offset);
         float32x4_t c1 = vld1q_f32(ptr1 + offset);
         float32x4_t c2 = vld1q_f32(ptr2 + offset);
         float32x4_t c3 = vld1q_f32(ptr3 + offset);

         // Interpolate and cache
         float32x4_t hq = vmulq_f32(c0, qv0);
         hq             = vmlaq_f32(hq, c1, qv1);
         hq             = vmlaq_f32(hq, c2, qv2);
         hq             = vmlaq_f32(hq, c3, qv3);

         hq_cache[tap_block] = hq;
      }

      int32_t ch = 0;

      // ========================================================================
      // Process 4 channels at a time (uses cached filter coefficients)
      // ========================================================================
      while (ch + 4 <= numChannels) {
         float *in_ch0 = effective_in + (ch + 0) * effective_dim;
         float *in_ch1 = effective_in + (ch + 1) * effective_dim;
         float *in_ch2 = effective_in + (ch + 2) * effective_dim;
         float *in_ch3 = effective_in + (ch + 3) * effective_dim;

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);
         float32x4_t acc3 = vdupq_n_f32(0.0f);

         // Process using pre-computed filter coefficients
         // Unroll 2x for better pipeline utilization
         for (int32_t tap_block = 0; tap_block < 16; tap_block += 2) {
            const int32_t offset0 = tap_block * 4;
            const int32_t offset1 = (tap_block + 1) * 4;

            // Block 0
            float32x4_t hq0   = hq_cache[tap_block];
            float32x4_t in0_0 = vld1q_f32(&in_ch0[baseJ + offset0]);
            float32x4_t in1_0 = vld1q_f32(&in_ch1[baseJ + offset0]);
            float32x4_t in2_0 = vld1q_f32(&in_ch2[baseJ + offset0]);
            float32x4_t in3_0 = vld1q_f32(&in_ch3[baseJ + offset0]);

            // Block 1
            float32x4_t hq1   = hq_cache[tap_block + 1];
            float32x4_t in0_1 = vld1q_f32(&in_ch0[baseJ + offset1]);
            float32x4_t in1_1 = vld1q_f32(&in_ch1[baseJ + offset1]);
            float32x4_t in2_1 = vld1q_f32(&in_ch2[baseJ + offset1]);
            float32x4_t in3_1 = vld1q_f32(&in_ch3[baseJ + offset1]);

            // Accumulate block 0 (can dual-issue with block 1)
            acc0 = vmlaq_f32(acc0, hq0, in0_0);
            acc1 = vmlaq_f32(acc1, hq0, in1_0);
            acc2 = vmlaq_f32(acc2, hq0, in2_0);
            acc3 = vmlaq_f32(acc3, hq0, in3_0);

            // Accumulate block 1
            acc0 = vmlaq_f32(acc0, hq1, in0_1);
            acc1 = vmlaq_f32(acc1, hq1, in1_1);
            acc2 = vmlaq_f32(acc2, hq1, in2_1);
            acc3 = vmlaq_f32(acc3, hq1, in3_1);
         }

         // Horizontal reduction (optimized for interleaved output)
         float32x2_t low0  = vget_low_f32(acc0);
         float32x2_t high0 = vget_high_f32(acc0);
         float32x2_t low1  = vget_low_f32(acc1);
         float32x2_t high1 = vget_high_f32(acc1);
         float32x2_t low2  = vget_low_f32(acc2);
         float32x2_t high2 = vget_high_f32(acc2);
         float32x2_t low3  = vget_low_f32(acc3);
         float32x2_t high3 = vget_high_f32(acc3);

         float32x2_t sum0 = vadd_f32(low0, high0);
         float32x2_t sum1 = vadd_f32(low1, high1);
         float32x2_t sum2 = vadd_f32(low2, high2);
         float32x2_t sum3 = vadd_f32(low3, high3);

         sum0 = vpadd_f32(sum0, sum1); // [sum0, sum1]
         sum2 = vpadd_f32(sum2, sum3); // [sum2, sum3]

         // Store to interleaved output
         pOutLocal[ch + 0] = vget_lane_f32(sum0, 0) * scale;
         pOutLocal[ch + 1] = vget_lane_f32(sum0, 1) * scale;
         pOutLocal[ch + 2] = vget_lane_f32(sum2, 0) * scale;
         pOutLocal[ch + 3] = vget_lane_f32(sum2, 1) * scale;

         ch += 4;
      }

      // ========================================================================
      // Process 3 remaining channels
      // ========================================================================
      if (ch + 3 <= numChannels) {
         float *in_ch0 = effective_in + (ch + 0) * effective_dim;
         float *in_ch1 = effective_in + (ch + 1) * effective_dim;
         float *in_ch2 = effective_in + (ch + 2) * effective_dim;

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);
         float32x4_t acc2 = vdupq_n_f32(0.0f);

         for (int32_t tap_block = 0; tap_block < 16; tap_block += 2) {
            const int32_t offset0 = tap_block * 4;
            const int32_t offset1 = (tap_block + 1) * 4;

            float32x4_t hq0 = hq_cache[tap_block];
            float32x4_t hq1 = hq_cache[tap_block + 1];

            float32x4_t in0_0 = vld1q_f32(&in_ch0[baseJ + offset0]);
            float32x4_t in1_0 = vld1q_f32(&in_ch1[baseJ + offset0]);
            float32x4_t in2_0 = vld1q_f32(&in_ch2[baseJ + offset0]);

            float32x4_t in0_1 = vld1q_f32(&in_ch0[baseJ + offset1]);
            float32x4_t in1_1 = vld1q_f32(&in_ch1[baseJ + offset1]);
            float32x4_t in2_1 = vld1q_f32(&in_ch2[baseJ + offset1]);

            acc0 = vmlaq_f32(acc0, hq0, in0_0);
            acc1 = vmlaq_f32(acc1, hq0, in1_0);
            acc2 = vmlaq_f32(acc2, hq0, in2_0);

            acc0 = vmlaq_f32(acc0, hq1, in0_1);
            acc1 = vmlaq_f32(acc1, hq1, in1_1);
            acc2 = vmlaq_f32(acc2, hq1, in2_1);
         }

         float32x2_t sum0 = vpadd_f32(vget_low_f32(acc0), vget_high_f32(acc0));
         float32x2_t sum1 = vpadd_f32(vget_low_f32(acc1), vget_high_f32(acc1));
         float32x2_t sum2 = vpadd_f32(vget_low_f32(acc2), vget_high_f32(acc2));

         sum0 = vpadd_f32(sum0, sum1);
         sum2 = vpadd_f32(sum2, sum2);

         pOutLocal[ch + 0] = vget_lane_f32(sum0, 0) * scale;
         pOutLocal[ch + 1] = vget_lane_f32(sum0, 1) * scale;
         pOutLocal[ch + 2] = vget_lane_f32(sum2, 0) * scale;

         ch += 3;
      }

      // ========================================================================
      // Process 2 remaining channels
      // ========================================================================
      if (ch + 2 <= numChannels) {
         float *in_ch0 = effective_in + (ch + 0) * effective_dim;
         float *in_ch1 = effective_in + (ch + 1) * effective_dim;

         float32x4_t acc0 = vdupq_n_f32(0.0f);
         float32x4_t acc1 = vdupq_n_f32(0.0f);

         for (int32_t tap_block = 0; tap_block < 16; tap_block += 2) {
            const int32_t offset0 = tap_block * 4;
            const int32_t offset1 = (tap_block + 1) * 4;

            float32x4_t hq0 = hq_cache[tap_block];
            float32x4_t hq1 = hq_cache[tap_block + 1];

            float32x4_t in0_0 = vld1q_f32(&in_ch0[baseJ + offset0]);
            float32x4_t in1_0 = vld1q_f32(&in_ch1[baseJ + offset0]);

            float32x4_t in0_1 = vld1q_f32(&in_ch0[baseJ + offset1]);
            float32x4_t in1_1 = vld1q_f32(&in_ch1[baseJ + offset1]);

            acc0 = vmlaq_f32(acc0, hq0, in0_0);
            acc1 = vmlaq_f32(acc1, hq0, in1_0);

            acc0 = vmlaq_f32(acc0, hq1, in0_1);
            acc1 = vmlaq_f32(acc1, hq1, in1_1);
         }

         float32x2_t sum = vpadd_f32(vget_low_f32(acc0), vget_high_f32(acc0));
         sum             = vpadd_f32(sum, vpadd_f32(vget_low_f32(acc1), vget_high_f32(acc1)));

         pOutLocal[ch + 0] = vget_lane_f32(sum, 0) * scale;
         pOutLocal[ch + 1] = vget_lane_f32(sum, 1) * scale;

         ch += 2;
      }

      // ========================================================================
      // Process 1 remaining channel
      // ========================================================================
      if (ch < numChannels) {
         float *in_ch0 = effective_in + ch * effective_dim;

         float32x4_t acc0 = vdupq_n_f32(0.0f);

         for (int32_t tap_block = 0; tap_block < 16; tap_block += 2) {
            const int32_t offset0 = tap_block * 4;
            const int32_t offset1 = (tap_block + 1) * 4;

            float32x4_t hq0 = hq_cache[tap_block];
            float32x4_t hq1 = hq_cache[tap_block + 1];

            float32x4_t in0_0 = vld1q_f32(&in_ch0[baseJ + offset0]);
            float32x4_t in0_1 = vld1q_f32(&in_ch0[baseJ + offset1]);

            acc0 = vmlaq_f32(acc0, hq0, in0_0);
            acc0 = vmlaq_f32(acc0, hq1, in0_1);
         }

         float32x2_t sum = vadd_f32(vget_low_f32(acc0), vget_high_f32(acc0));
         sum             = vpadd_f32(sum, sum);

         pOutLocal[ch] = vget_lane_f32(sum, 0) * scale;
      }

      // Advance to next output sample (interleaved)
      pOutLocal += numChannels;

      // Update time tracking
      tauInt -= (int32_t) (AUDIOLIB_ASRC_FIXEDPOINT_Q28 * inputSampleIncrement);
      tauInt += rhoInt;
      inputSampleIncrement =
          (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
      nearestInputSampleIndex += inputSampleIncrement;
   }
   // ────────────────────────────────────────────────────────────────
   // Update history – save last history_length samples per channel
   // ────────────────────────────────────────────────────────────────
   for (i = 0; i < numChannels; i++) {
      memcpy(effective_in + i * effective_dim, effective_in + i * effective_dim + inputSampleCount,
             (size_t) history_length * sizeof(dataType));
   }

   // ────────────────────────────────────────────────────────────────
   // Frame length & remainder (interleaved output)
   // ────────────────────────────────────────────────────────────────
   pKerOutArgs->outputSampleCount = preCompOutputCountInt + pKerPrivArgs->remBufCount;
   pKerPrivArgs->remBufCount      = pKerOutArgs->outputSampleCount % pKerPrivArgs->initArgs.frameModuloFactor;
   pKerOutArgs->outputSampleCount = pKerOutArgs->outputSampleCount - pKerPrivArgs->remBufCount;

   memcpy(pFilterRembufLocal, (dataType *) pOut + pKerOutArgs->outputSampleCount * numChannels,
          numChannels * pKerPrivArgs->remBufCount * sizeof(dataType));

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_non_interleaved<float>(AUDIOLIB_kernelHandle handle,
                                                                      void *restrict pIn,
                                                                      void *restrict pNonInterleavedData,
                                                                      void *restrict pFiltCoeffs,
                                                                      void *restrict pFilterRembuf,
                                                                      void *restrict pOut,
                                                                      const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                                      AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

template AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_interleaved<float>(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pNonInterleavedData,
                                                                  void *restrict pFiltCoeffs,
                                                                  void *restrict pFilterRembuf,
                                                                  void *restrict pOut,
                                                                  const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                                  AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);
