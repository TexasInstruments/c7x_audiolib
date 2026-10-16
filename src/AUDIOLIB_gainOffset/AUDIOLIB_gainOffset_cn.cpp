// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_gainOffset_priv.h"

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainOffset_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainOffset_exec_cn");

   const dataType *restrict pInLocal = (const dataType *) pIn;
   dataType *restrict pOutLocal      = (dataType *) pOut;

   const dataType gain      = (dataType) pKerPrivArgs->gain;
   const dataType offset    = (dataType) pKerPrivArgs->offset;
   const uint32_t dimX      = pKerPrivArgs->dimX;
   const uint32_t dimY      = pKerPrivArgs->dimY;
   const uint32_t strideIn  = (uint32_t) pKerPrivArgs->strideIn;
   const uint32_t strideOut = (uint32_t) pKerPrivArgs->strideOut;

   for (uint32_t m = 0U; m < dimY; m++) {
      for (uint32_t n = 0U; n < dimX; n++) {
         pOutLocal[(m * strideOut) + n] = (pInLocal[(m * strideIn) + n] * gain) + offset;
      }
   }

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
