// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_tableInterpolate_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_exec_cn(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn0,
                                                  void *restrict pIn1,
                                                  void *restrict pOut)

{
   AUDIOLIB_STATUS                     status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableInterpolate_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableInterpolate_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_tableInterpolate_exec_cn");
   dataType *restrict pInLocalSrc   = (dataType *) pIn0;
   dataType *restrict pInLocalTable = (dataType *) pIn1;
   dataType *restrict pOutLocalOut  = (dataType *) pOut;

   uint32_t srcSamples   = pKerPrivArgs->srcSamples;
   uint32_t tableSamples = pKerPrivArgs->tableSamples;
   float    minVal       = pKerPrivArgs->minVal;
   float    divisor      = pKerPrivArgs->divisor;
   float    fract        = 0;
   for (uint32_t sample = 0; sample < srcSamples; sample++) {
      dataType sampleSrc = pInLocalSrc[sample];
      uint32_t index     = 0;

      sampleSrc -= minVal;
      sampleSrc *= divisor;

      if (sampleSrc <= 0) {
         pOutLocalOut[sample] = pInLocalTable[0];
      }
      else if (sampleSrc >= (tableSamples - 1)) {

         pOutLocalOut[sample] = pInLocalTable[tableSamples - 1];
      }
      else {
         index                = floorf(sampleSrc);
         fract                = sampleSrc - index;
         pOutLocalOut[sample] = pInLocalTable[index] * (1 - fract) + pInLocalTable[index + 1] * (fract);
      }
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn0,
                                                                  void *restrict pIn1,
                                                                  void *restrict pOut);
