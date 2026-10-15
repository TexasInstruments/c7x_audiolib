// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_tableLookup_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableLookup_exec_cn(AUDIOLIB_kernelHandle handle,
                                             void *restrict pIn0,
                                             void *restrict pIn1,
                                             void *restrict pOut)

{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_tableLookup_exec_cn");
   dataType *restrict pInLocalSrc   = (dataType *) pIn0;
   dataType *restrict pInLocalTable = (dataType *) pIn1;
   dataType *restrict pOutLocalOut  = (dataType *) pOut;

   uint32_t srcSamples   = pKerPrivArgs->srcSamples;
   uint32_t tableSamples = pKerPrivArgs->tableSamples;
   float    minVal       = pKerPrivArgs->minVal;
   float    divisor      = pKerPrivArgs->divisor;
   for (uint32_t sample = 0; sample < srcSamples; sample++) {
      dataType sampleSrc = pInLocalSrc[sample];
      uint32_t index     = 0;

      sampleSrc -= minVal;
      sampleSrc *= divisor;
      sampleSrc = sampleSrc + 0.5f;

      if (sampleSrc <= 0) {
         pOutLocalOut[sample] = pInLocalTable[0];
      }
      else if (sampleSrc >= (tableSamples - 1)) {

         pOutLocalOut[sample] = pInLocalTable[tableSamples - 1];
      }
      else {
         index                = floorf(sampleSrc);
         pOutLocalOut[sample] = pInLocalTable[index];
      }
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_tableLookup_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn0,
                                                             void *restrict pIn1,
                                                             void *restrict pOut);

template <typename dataTypeIndex, typename dataTypeSample>
AUDIOLIB_STATUS AUDIOLIB_tableLookupInt_exec_cn(AUDIOLIB_kernelHandle handle,
                                                void *restrict pIn0,
                                                void *restrict pIn1,
                                                void *restrict pOut)

{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_tableLookupInt_exec_cn");
   dataTypeIndex *restrict pInLocalSrc    = (dataTypeIndex *) pIn0;
   dataTypeSample *restrict pInLocalTable = (dataTypeSample *) pIn1;
   dataTypeSample *restrict pOutLocalOut  = (dataTypeSample *) pOut;

   uint32_t srcSamples   = pKerPrivArgs->srcSamples;
   int32_t  tableSamples = (int32_t) pKerPrivArgs->tableSamples;
   for (uint32_t sample = 0; sample < srcSamples; sample++) {
      dataTypeIndex sampleSrc = pInLocalSrc[sample];

      if (sampleSrc <= 0) {
         pOutLocalOut[sample] = pInLocalTable[0];
      }
      else if (sampleSrc >= (tableSamples - 1)) {
         pOutLocalOut[sample] = pInLocalTable[tableSamples - 1];
      }
      else {
         pOutLocalOut[sample] = pInLocalTable[sampleSrc];
      }
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_tableLookupInt_exec_cn<int32_t, float>(AUDIOLIB_kernelHandle handle,
                                                                         void *restrict pIn0,
                                                                         void *restrict pIn1,
                                                                         void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_tableLookupInt_exec_cn<int32_t, int32_t>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn0,
                                                                           void *restrict pIn1,
                                                                           void *restrict pOut);
