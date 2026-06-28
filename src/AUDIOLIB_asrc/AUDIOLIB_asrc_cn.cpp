// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_asrc_priv.h"
#include <math.h>
#ifdef ARM_A53
#include <string.h>
#endif

#define AUDIOLIB_ASRC_MIN(x, y) (((x) < (y)) ? (x) : (y))

AUDIOLIB_STATUS AUDIOLIB_asrc_init_cn(AUDIOLIB_kernelHandle   handle,
                                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      AUDIOLIB_asrc_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_exec_cn_non_interleaved(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pNonInterleavedData,
                                                      void *restrict pFiltCoeffs,
                                                      void *restrict pFilterRembuf,
                                                      void *restrict pOut,
                                                      const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                      AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_exec_cn_non_interleaved\n");
#endif

   AUDIOLIB_STATUS         status             = AUDIOLIB_SUCCESS;
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs       = (AUDIOLIB_asrc_PrivArgs *) handle;
   dataType               *pInLocal           = (dataType *) pIn;
   dataType               *pOutLocal          = (dataType *) pOut;
   dataType               *pFilterRembufLocal = (dataType *) pFilterRembuf;
   const dataType         *pFiltCoeffsLocal   = (dataType *) pFiltCoeffs;
   const uint8_t           numChannels        = pKerPrivArgs->initArgs.numChannels;
   dataType                hq;                                                   /* interpolated coeffs */
   uint32_t               *cirBuffStartIndex = &pKerPrivArgs->cirBuffStartIndex; /* index into circular buffer */
   uint32_t                inputSampleCount  = pKerInArgs->inputSampleCount;
   int32_t                 preCompOutputCountInt;
   int32_t                 i, j, k, n, outSampleIdx;

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

#ifdef C7X
#pragma MUST_ITERATE(1, , 1)
#endif
   for (outSampleIdx = 0; outSampleIdx < preCompOutputCountInt; outSampleIdx++) {

      k                 = (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_PHASE_MASK) >>
                      AUDIOLIB_ASRC_PHASE_SHIFT); /* upsampler phase */
      const dataType dt = (dataType) (((uint32_t) tauInt & AUDIOLIB_ASRC_FRACTION_MASK)) * AUDIOLIB_ASRC_FRACTION_SCALE;
      const dataType dt2 = dt * dt;
      const dataType dt3 = dt * dt2;
      /* 3rd-order Lagrange interpolation coefficients */
      /* for use with 0 <= dt < 1 */
      // clang-format off
      const dataType q30 =     (-1.f/3) * dt + 0.5f * dt2 + (-1.f/6) * dt3;
      const dataType q31 = 1.f  - 0.5f  * dt -        dt2 +   0.5f   * dt3;
      const dataType q32 =                dt + 0.5f * dt2 -   0.5f   * dt3;
      const dataType q33 =     (-1.f/6) * dt              +  (1.f/6) * dt3;
      // clang-format on

#ifdef C7X
#pragma MUST_ITERATE(1, , 1)
#endif
      for (i = 0; i < pKerPrivArgs->initArgs.numChannels; i++) {
         *(pOutLocal + (i * pKerPrivArgs->outBufferDimX)) = 0.0f;
      }

      k *= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;

#ifdef C7X
#pragma MUST_ITERATE(64, 64, )
#endif
      for (i = 0; i < (int32_t) AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS; i++) {
         // compute coefficients
         // clang-format off
         hq = q30 * pFiltCoeffsLocal[k] + 
              q31 * pFiltCoeffsLocal[k + 1 * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS] +
              q32 * pFiltCoeffsLocal[k + 2 * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS] +
              q33 * pFiltCoeffsLocal[k + 3 * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS];
         // clang-format on
         k++;

         // compute sums
         j = (*cirBuffStartIndex + nearestInputSampleIndex + i) & (pKerPrivArgs->inBufferTotalDimX - 1);

#ifdef C7X
#pragma MUST_ITERATE(1, , 1)
#endif
         for (n = 0; n < pKerPrivArgs->initArgs.numChannels; n++) {
            *(pOutLocal + (n * pKerPrivArgs->outBufferDimX)) +=
                hq * pInLocal[j + (n * pKerPrivArgs->inBufferTotalDimX)];
         }
      }

#ifdef C7X
#pragma MUST_ITERATE(1, , 1)
#endif
      for (i = 0; i < pKerPrivArgs->initArgs.numChannels; i++) {
         *(pOutLocal + (i * pKerPrivArgs->outBufferDimX)) *= (dataType) AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES;
      }
      pOutLocal++;

      tauInt -= (int32_t) (AUDIOLIB_ASRC_FIXEDPOINT_Q28 * inputSampleIncrement);
      tauInt += rhoInt;
      inputSampleIncrement =
          (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
      nearestInputSampleIndex += inputSampleIncrement;
   }

   *cirBuffStartIndex = (*cirBuffStartIndex + inputSampleCount) & (pKerPrivArgs->inBufferTotalDimX - 1);

   // ASRC frame length must be a multiple of frameModuloFactor
   pKerOutArgs->outputSampleCount = preCompOutputCountInt + pKerPrivArgs->remBufCount;
   pKerPrivArgs->remBufCount      = pKerOutArgs->outputSampleCount % pKerPrivArgs->initArgs.frameModuloFactor;
   pKerOutArgs->outputSampleCount = pKerOutArgs->outputSampleCount - pKerPrivArgs->remBufCount;

   for (i = 0; i < numChannels; i++) {
      memcpy(pFilterRembufLocal + i * pKerPrivArgs->bufParamsFilterRembuf.dim_x,
             (dataType *) pOut + pKerOutArgs->outputSampleCount + i * pKerPrivArgs->outBufferDimX,
             pKerPrivArgs->remBufCount * AUDIOLIB_sizeof(pKerPrivArgs->bufParamsFilterRembuf.data_type));
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asrc_exec_cn_interleaved(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pNonInterleavedData,
                                                  void *restrict pFiltCoeffs,
                                                  void *restrict pFilterRembuf,
                                                  void *restrict pOut,
                                                  const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                  AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_asrc_exec_cn_interleaved\n");
#endif

   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
#ifdef C7X
   DSPLIB_STATUS dsplib_status __attribute__((unused)) = DSPLIB_SUCCESS;
#endif
   AUDIOLIB_asrc_PrivArgs *pKerPrivArgs             = (AUDIOLIB_asrc_PrivArgs *) handle;
   dataType               *pOutLocal                = ((dataType *) pOut);
   dataType               *pNonInterleavedDataLocal = (dataType *) pNonInterleavedData;
   dataType               *pFilterRembufLocal       = (dataType *) pFilterRembuf;
   const dataType         *pFiltCoeffsLocal         = (dataType *) pFiltCoeffs;
   const uint8_t           numChannels              = pKerPrivArgs->initArgs.numChannels;
   dataType                hq;                                                   /* interpolated coeffs */
   uint32_t               *cirBuffStartIndex = &pKerPrivArgs->cirBuffStartIndex; /* index into circular buffer */
   uint32_t                inputSampleCount  = pKerInArgs->inputSampleCount;
   int32_t                 preCompOutputCountInt;
   int32_t                 i, j, k, n, outSampleIdx;

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

   memcpy(pOutLocal, pFilterRembufLocal,
          numChannels * pKerPrivArgs->remBufCount * AUDIOLIB_sizeof(pKerPrivArgs->bufParamsFilterRembuf.data_type));

   pOutLocal += pKerPrivArgs->remBufCount * (int32_t) numChannels;

   const int32_t rhoInt  = (int32_t) ROUND((double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 *
                                           (double) (rho)); /* output time incr. w.r.t. input sample */
   double        tauTemp = (double) AUDIOLIB_ASRC_FIXEDPOINT_Q28 * (double) (tau);
   int32_t       tauInt  = (int32_t) ROUND(tauTemp); /* output time w.r.t. input sample */
   uint32_t      inputSampleIncrement =
       (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
   uint32_t nearestInputSampleIndex = inputSampleIncrement;
#ifdef C7X
   dsplib_status = DSPLIB_matTrans_exec(
       pKerPrivArgs->initArgs.matTransHandle, pIn,
       (void *) (&pNonInterleavedDataLocal[(*cirBuffStartIndex +
                                            (uint32_t) (AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS - 1)) &
                                           (pKerPrivArgs->inBufferTotalDimX - 1)]));
#endif

#ifdef ARM_A53
   int32_t   dataSize = sizeof(dataType);
   dataType *pInLocal = (dataType *) pIn;
   dataType *pTemp =
       &pNonInterleavedDataLocal[(*cirBuffStartIndex + (uint32_t) (AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS - 1)) &
                                 (pKerPrivArgs->inBufferTotalDimX - 1)];
   for (i = 0; i < (int32_t) pKerPrivArgs->widthInNonInterleavedData; i++) {
      for (j = 0; j < (int32_t) pKerPrivArgs->heightInNonInterleavedData; j++) {
         pTemp[(i * pKerPrivArgs->strideOutNonInterleavedData / dataSize) + j] =
             pInLocal[i + ((pKerPrivArgs->strideInNonInterleavedData / dataSize) * j)];
      }
   }
#endif
#ifdef C7X
#pragma MUST_ITERATE(1, , 1)
#endif
   for (outSampleIdx = 0; outSampleIdx < preCompOutputCountInt; outSampleIdx++) {

      k                 = (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_PHASE_MASK) >>
                      AUDIOLIB_ASRC_PHASE_SHIFT); /* upsampler phase */
      const dataType dt = (dataType) (((uint32_t) tauInt & AUDIOLIB_ASRC_FRACTION_MASK)) * AUDIOLIB_ASRC_FRACTION_SCALE;
      const dataType dt2 = dt * dt;
      const dataType dt3 = dt * dt2;
      /* 3rd-order Lagrange interpolation coefficients */
      /* for use with 0 <= dt < 1 */
      // clang-format off
         const dataType q30 =     (-1.f/3) * dt + 0.5f * dt2 + (-1.f/6) * dt3;
         const dataType q31 = 1.f  - 0.5f  * dt -        dt2 +   0.5f   * dt3;
         const dataType q32 =                dt + 0.5f * dt2 -   0.5f   * dt3;
         const dataType q33 =     (-1.f/6) * dt              +  (1.f/6) * dt3;
      // clang-format on

#ifdef C7X
#pragma MUST_ITERATE(1, , 1)
#endif
      for (i = 0; i < pKerPrivArgs->initArgs.numChannels; i++) {
         *(pOutLocal + i) = 0.0f;
      }

      k *= AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS;

#ifdef C7X
#pragma MUST_ITERATE(64, 64, )
#endif
      for (i = 0; i < (int32_t) AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS; i++) {
         // compute coefficients
         // clang-format off
            hq = q30 * pFiltCoeffsLocal[k] + 
                 q31 * pFiltCoeffsLocal[k + 1 * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS] +
                 q32 * pFiltCoeffsLocal[k + 2 * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS] +
                 q33 * pFiltCoeffsLocal[k + 3 * AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS];
         // clang-format on
         k++;

         // compute sums
         j = (*cirBuffStartIndex + nearestInputSampleIndex + i) & (pKerPrivArgs->inBufferTotalDimX - 1);

#ifdef C7X
#pragma MUST_ITERATE(1, , 1)
#endif
         for (n = 0; n < pKerPrivArgs->initArgs.numChannels; n++) {
            *(pOutLocal + n) += hq * pNonInterleavedDataLocal[j + (n * pKerPrivArgs->inBufferTotalDimX)];
         }
      }

#ifdef C7X
#pragma MUST_ITERATE(1, , 1)
#endif
      for (i = 0; i < pKerPrivArgs->initArgs.numChannels; i++) {
         *(pOutLocal + i) *= (dataType) AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES;
      }
      pOutLocal += pKerPrivArgs->initArgs.numChannels;

      tauInt -= (int32_t) (AUDIOLIB_ASRC_FIXEDPOINT_Q28 * inputSampleIncrement);
      tauInt += rhoInt;
      inputSampleIncrement =
          (uint32_t) (((uint32_t) tauInt & AUDIOLIB_ASRC_INPUT_INCREMENT_MASK) >> AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT);
      nearestInputSampleIndex += inputSampleIncrement;
   }

   *cirBuffStartIndex = (*cirBuffStartIndex + inputSampleCount) & (pKerPrivArgs->inBufferTotalDimX - 1);

   // ASRC frame length must be a multiple of frameModuloFactor
   pKerOutArgs->outputSampleCount = preCompOutputCountInt + pKerPrivArgs->remBufCount;
   pKerPrivArgs->remBufCount      = pKerOutArgs->outputSampleCount % pKerPrivArgs->initArgs.frameModuloFactor;
   pKerOutArgs->outputSampleCount = pKerOutArgs->outputSampleCount - pKerPrivArgs->remBufCount;

   memcpy(pFilterRembufLocal, (dataType *) pOut + pKerOutArgs->outputSampleCount * numChannels,
          numChannels * pKerPrivArgs->remBufCount * AUDIOLIB_sizeof(pKerPrivArgs->bufParamsFilterRembuf.data_type));

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_asrc_exec_cn_non_interleaved<float>(AUDIOLIB_kernelHandle handle,
                                                                      void *restrict pIn,
                                                                      void *restrict pNonInterleavedData,
                                                                      void *restrict pFiltCoeffs,
                                                                      void *restrict pFilterRembuf,
                                                                      void *restrict pOut,
                                                                      const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                                      AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

template AUDIOLIB_STATUS AUDIOLIB_asrc_exec_cn_interleaved<float>(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pNonInterleavedData,
                                                                  void *restrict pFiltCoeffs,
                                                                  void *restrict pFilterRembuf,
                                                                  void *restrict pOut,
                                                                  const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                                  AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);
