// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_floor_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floor_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_floor_PrivArgs *pKerPrivArgs = (AUDIOLIB_floor_PrivArgs *) handle;

   uint32_t dim_x     = pKerPrivArgs->dim_x;
   uint32_t dim_y     = pKerPrivArgs->dim_y;
   int32_t  inStride  = pKerPrivArgs->inStride;
   int32_t  outStride = pKerPrivArgs->outStride;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floor_exec_cn\n");

   dataType *pInLocal  = (dataType *) pIn;
   dataType *pOutLocal = (dataType *) pOut;

   for (uint32_t m = 0; m < dim_y; m++) {
      for (uint32_t n = 0; n < dim_x; n++) {
         pOutLocal[m * outStride + n] = floor(pInLocal[m * inStride + n]);
      }
   }

   return (status);
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS
AUDIOLIB_floor_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_floor_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
