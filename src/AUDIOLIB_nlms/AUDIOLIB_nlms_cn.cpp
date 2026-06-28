// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_nlms_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_nlms_exec_cn(AUDIOLIB_kernelHandle handle,
                                      void *restrict pIn,
                                      void *restrict pInRef,
                                      void *restrict pStateBuffer,
                                      void *restrict pScratchBuffer,
                                      void *restrict pCoefficients,
                                      void *restrict pOut)
{
   AUDIOLIB_STATUS         status = AUDIOLIB_SUCCESS;
   AUDIOLIB_nlms_PrivArgs *args   = (AUDIOLIB_nlms_PrivArgs *) handle;

   const uint32_t numChannels  = args->dim_y;
   const uint32_t totalSamples = args->totalSamples;
   const uint32_t filterLength = args->filterLength;
   const float    stepSize     = args->stepSize;
   const uint32_t circBuffMask = args->circBuffMask;

   const int32_t strideInElements    = args->strideInElements;
   const int32_t strideOutElements   = args->strideOutElements;
   const int32_t strideStateElements = args->strideStateElements;

   dataType *restrict inputSignal  = (dataType *) pIn;
   dataType *restrict refSignal    = (dataType *) pInRef;
   dataType *restrict stateBuffer  = (dataType *) pStateBuffer;
   dataType *restrict accumBuffer  = (dataType *) pScratchBuffer;
   dataType *restrict coeffs       = (dataType *) pCoefficients;
   dataType *restrict outputSignal = (dataType *) pOut;

   int32_t startReadIdx  = args->readIdx;
   int32_t startWriteIdx = args->writeIdx;

   int32_t nextReadIdx  = 0;
   int32_t nextWriteIdx = 0;

   for (uint32_t ch = 0; ch < numChannels; ch++) {
      dataType *restrict chCoeffs  = coeffs + ch * strideInElements;
      dataType *restrict chState   = stateBuffer + ch * strideStateElements;
      dataType *restrict chAccum   = accumBuffer + ch * strideInElements;
      dataType *restrict chInput   = inputSignal + ch * strideInElements;
      dataType *restrict chDesired = refSignal + ch * strideOutElements;
      dataType *restrict chOutput  = outputSignal + ch * strideOutElements;

      int32_t readIdx  = startReadIdx;
      int32_t writeIdx = startWriteIdx;

      for (uint32_t i = 0; i < filterLength; i++) {
         chAccum[i] = (dataType) 0.0f;
      }

      for (int sample = 0; sample < (int) totalSamples; sample++) {
         const dataType x_n = chInput[sample];

         chState[writeIdx] = x_n;

         dataType energy = (dataType) 0.0f;
         for (uint32_t i = 0; i < filterLength; i++) {
            int32_t  idx = (readIdx + i + 1) & circBuffMask;
            dataType val = chState[idx];
            energy += val * val;
         }

         dataType y_n = (dataType) 0.0f;

         for (uint32_t i = 0; i < filterLength; i++) {
            int32_t idx      = (readIdx + i + 1) & circBuffMask;
            int32_t coeffIdx = filterLength - 1 - i;
            y_n += chState[idx] * chCoeffs[coeffIdx];
         }

         chOutput[sample] = y_n;

         const dataType e_n = chDesired[sample] - y_n;

         dataType       energyClamped = (energy < (dataType) 1e-6f) ? (dataType) 1e-6f : energy;
         const dataType stepFactor =
             (dataType) 2.0f * (dataType) stepSize * e_n / (energyClamped + args->regularization);

         for (uint32_t i = 0; i < filterLength; i++) {
            int32_t idx = (readIdx + i + 1) & circBuffMask;
            chAccum[filterLength - 1 - i] += stepFactor * chState[idx];
         }

         readIdx  = (readIdx + 1) & circBuffMask;
         writeIdx = (writeIdx + 1) & circBuffMask;
      }

      for (uint32_t i = 0; i < filterLength; i++) {
         chCoeffs[i] += chAccum[i];
      }

      nextReadIdx  = readIdx;
      nextWriteIdx = writeIdx;
   }

   args->readIdx  = nextReadIdx;
   args->writeIdx = nextWriteIdx;

   return status;
}

// ======================== EXPLICIT TEMPLATE INSTANTIATIONS ========================
template AUDIOLIB_STATUS AUDIOLIB_nlms_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pInRef,
                                                      void *restrict pStateBuffer,
                                                      void *restrict pScratchBuffer,
                                                      void *restrict pCoefficients,
                                                      void *restrict pOut);
