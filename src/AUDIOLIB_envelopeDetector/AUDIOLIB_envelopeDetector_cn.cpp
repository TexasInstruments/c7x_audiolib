// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_envelopeDetector_priv.h"
#include <cmath>
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_envelopeDetector_PrivArgs *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t numSamples        = pKerPrivArgs->numSamples;
   uint32_t numChannels       = pKerPrivArgs->numChannels;
   int32_t  strideInElements  = pKerPrivArgs->strideInElements;
   int32_t  strideOutElements = pKerPrivArgs->strideOutElements;

   float *pEnvelopeStates    = pKerPrivArgs->envelopeStates;
   float  attackCoefficient  = pKerPrivArgs->attackCoefficient;
   float  releaseCoefficient = pKerPrivArgs->releaseCoefficient;

   for (uint32_t channel = 0; channel < numChannels; channel++) {
      dataType envelope = (dataType) pEnvelopeStates[channel];
      for (uint32_t sample = 0; sample < numSamples; sample++) {
         dataType inputSample    = pInLocal[channel * strideInElements + sample];
         dataType inputMagnitude = (inputSample >= 0) ? inputSample : -inputSample;
         float    coefficient    = (inputMagnitude > envelope) ? attackCoefficient : releaseCoefficient;
         envelope                = (dataType) ((1.0f - coefficient) * envelope + coefficient * inputMagnitude);
         pOutLocal[channel * strideOutElements + sample] = envelope;
      }
      pEnvelopeStates[channel] = (float) envelope;
   }

   return AUDIOLIB_SUCCESS;
}

/* explicit instantiation for the different data type versions */
template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
