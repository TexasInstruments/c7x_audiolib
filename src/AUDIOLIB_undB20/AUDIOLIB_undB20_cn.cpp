// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_undB20_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_undB20_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_undB20_PrivArgs *pKerPrivArgs = (AUDIOLIB_undB20_PrivArgs *) handle;

   uint32_t dim_x     = pKerPrivArgs->dim_x;
   uint32_t dim_y     = pKerPrivArgs->dim_y;
   int32_t  inStride  = pKerPrivArgs->inStride;
   int32_t  outStride = pKerPrivArgs->outStride;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_undB20_exec_cn\n");
#endif

   dataType *pInLocal  = (dataType *) pIn;
   dataType *pOutLocal = (dataType *) pOut;

   for (uint32_t m = 0; m < dim_y; m++) {
      for (uint32_t n = 0; n < dim_x; n++) {
         /*  exp(ln(10) * (x / 20)) */
         pOutLocal[m * outStride + n] = exp((2.302585092994046 * pInLocal[(m * inStride) + n]) / 20);

         if (pOutLocal[m * outStride + n] > 3.4028235e+38) {
            pOutLocal[m * outStride + n] = 3.4028235e+38;
         }
         if (pOutLocal[m * outStride + n] < 1.1754944e-38) {
            pOutLocal[m * outStride + n] = 0.0;
         }
      }
   }

   return (status);
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS
AUDIOLIB_undB20_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
