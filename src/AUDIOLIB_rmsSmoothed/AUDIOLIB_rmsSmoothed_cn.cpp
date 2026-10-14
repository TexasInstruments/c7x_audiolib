// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_rmsSmoothed_priv.h"
#include <cmath>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_rmsSmoothed_PrivArgs *pKerPrivArgs = (AUDIOLIB_rmsSmoothed_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_rmsSmoothed_exec_cn");
   dataType *restrict pInLocalSrc  = (dataType *) pIn;
   dataType *restrict pOutLocalOut = (dataType *) pOut;

   uint32_t blockSize        = pKerPrivArgs->blockSize;
   uint32_t numChannels      = pKerPrivArgs->numChannels;
   uint32_t strideInElements = pKerPrivArgs->strideInElements;

   dataType energy = 0;
   for (uint32_t ch = 0; ch < numChannels; ch++) {
      dataType *pInChannel = pInLocalSrc + ch * strideInElements;
      for (uint32_t sample = 0; sample < blockSize; sample++) {
         energy += pInChannel[sample] * pInChannel[sample];
      }
   }
   energy = energy * (dataType) pKerPrivArgs->invNumSamples;

   dataType smoothingCoefficient = (dataType) pKerPrivArgs->smoothingCoefficient;
   dataType y = (smoothingCoefficient * pKerPrivArgs->smoothedEnergy) + ((1 - smoothingCoefficient) * energy);

   pKerPrivArgs->smoothedEnergy = y;
   pOutLocalOut[0]              = std::sqrt(y);

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
