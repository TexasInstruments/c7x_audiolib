// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_asymmetricLimiter_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_asymmetricLimiter_PrivArgs *pKerPrivArgs = (AUDIOLIB_asymmetricLimiter_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_asymmetricLimiter_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t dim_x     = pKerPrivArgs->dim_x;
   uint32_t dim_y     = pKerPrivArgs->dim_y;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;
   float    asymmetricLimiterLower = pKerPrivArgs->setArgs.asymmetricLimiterLower;
   float    asymmetricLimiterUpper = pKerPrivArgs->setArgs.asymmetricLimiterUpper;

   for (uint32_t m = 0; m < dim_y; m++) {
      for (uint32_t n = 0; n < dim_x; n++) {
         pOutLocal[m * strideOut + n] =
             (pInLocal[m * strideIn + n] < asymmetricLimiterLower) ? asymmetricLimiterLower : pInLocal[m * strideIn + n];
         pOutLocal[m * strideOut + n] =
             (pOutLocal[m * strideOut + n] > asymmetricLimiterUpper) ? asymmetricLimiterUpper : pOutLocal[m * strideOut + n];
      }
   }

   return status;
}

/* explicit instantiation for the different data type versions */
template AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
