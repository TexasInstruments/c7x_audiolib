// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_round_priv.h"
#include <cstdint>
#include <cmath>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_round_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_round_PrivArgs *pKerPrivArgs = (AUDIOLIB_round_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_round_exec_cn");

   dataType *restrict pInLocal   = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples   = pKerPrivArgs->samples;
   uint32_t channels  = pKerPrivArgs->channels;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   for (uint32_t i = 0; i < channels; i++) {
      for (uint32_t j = 0; j < samples; j++) {
         pOutLocal[i * strideOut + j] = std::rint(pInLocal[i * strideIn + j]);
      }
   }

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting AUDIOLIB_round_exec_cn natural C implementation");

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_round_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_round_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
