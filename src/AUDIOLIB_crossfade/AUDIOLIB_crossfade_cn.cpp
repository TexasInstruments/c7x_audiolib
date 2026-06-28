// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_crossfade_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_cn(AUDIOLIB_kernelHandle handle,
                                           void *restrict pIn0,
                                           void *restrict pIn1,
                                           void *restrict pIn2,
                                           void *restrict pIn3,
                                           void *restrict pOut)

{
   AUDIOLIB_STATUS              status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_crossfade_exec_cn");
   dataType *restrict pInLocalPrev = (dataType *) pIn0;
   dataType *restrict pInLocalNext = (dataType *) pIn1;
   dataType *restrict pGainCos     = (dataType *) pIn2;
   dataType *restrict pGainSine    = (dataType *) pIn3;
   dataType *restrict pOutLocalOut = (dataType *) pOut;

   uint32_t samples           = pKerPrivArgs->samples;
   uint32_t channels          = pKerPrivArgs->channels;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;

   for (uint32_t i = 0; i < channels; i++) {
      for (uint32_t j = 0; j < samples; j++) {
         pOutLocalOut[i * strideOutElements + j] = pInLocalPrev[i * strideInElements + j] * pGainCos[j] +
                                                   pInLocalNext[i * strideInElements + j] * pGainSine[j];
      }
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pIn0,
                                                     void *restrict pIn1,
                                                     void *restrict pIn2,
                                                     void *restrict pIn3,
                                                     void *restrict pOut)

{
   AUDIOLIB_STATUS              status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_crossfade_PrivArgs *pKerPrivArgs = (AUDIOLIB_crossfade_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_crossfade_exec_cn");
   dataType *restrict pInLocalPrev = (dataType *) pIn0;
   dataType *restrict pInLocalNext = (dataType *) pIn1;
   dataType *restrict pGainCos     = (dataType *) pIn2;
   dataType *restrict pGainSine    = (dataType *) pIn3;
   dataType *restrict pOutLocalOut = (dataType *) pOut;

   uint32_t samples           = pKerPrivArgs->samples;
   uint32_t channels          = pKerPrivArgs->channels;
   uint32_t strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements = pKerPrivArgs->strideOutElements;

   dataType sineGain, cosGain;

   for (uint32_t i = 0; i < samples; i++) {
      cosGain  = pGainCos[i];
      sineGain = pGainSine[i];
      for (uint32_t j = 0; j < channels; j++) {
         pOutLocalOut[i * strideOutElements + j] =
             pInLocalPrev[i * strideInElements + j] * cosGain + pInLocalNext[i * strideInElements + j] * sineGain;
      }
   }

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                           void *restrict pIn0,
                                                           void *restrict pIn1,
                                                           void *restrict pIn2,
                                                           void *restrict pIn3,
                                                           void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                            void *restrict pIn0,
                                                            void *restrict pIn1,
                                                            void *restrict pIn2,
                                                            void *restrict pIn3,
                                                            void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                     void *restrict pIn0,
                                                                     void *restrict pIn1,
                                                                     void *restrict pIn2,
                                                                     void *restrict pIn3,
                                                                     void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                                      void *restrict pIn0,
                                                                      void *restrict pIn1,
                                                                      void *restrict pIn2,
                                                                      void *restrict pIn3,
                                                                      void *restrict pOut);

// end of file
