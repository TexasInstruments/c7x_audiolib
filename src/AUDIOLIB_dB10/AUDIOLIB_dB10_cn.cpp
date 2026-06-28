// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_dB10_priv.h"
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_dB10_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;

   uint32_t dim_x       = pKerPrivArgs->dim_x;
   uint32_t dim_y       = pKerPrivArgs->dim_y;
   int32_t  inStride    = pKerPrivArgs->inStride;
   int32_t  outStride   = pKerPrivArgs->outStride;
   uint32_t dataTypeVal = pKerPrivArgs->dataType;

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_dB10_exec_cn\n");
#endif

   dataType *pInLocal  = (dataType *) pIn;
   dataType *pOutLocal = (dataType *) pOut;

   for (uint32_t m = 0; m < dim_y; m++) {
      for (uint32_t n = 0; n < dim_x; n++) {

         pOutLocal[m * outStride + n] = 10 * log10(pInLocal[m * inStride + n]);

         if (dataTypeVal == AUDIOLIB_FLOAT32) {
            if (pInLocal[m * inStride + n] > FLT_MAX_DB10) {
               pOutLocal[m * outStride + n] = 3082.546875;
            }
            else if (pInLocal[m * inStride + n] <= 0) {
               pOutLocal[m * outStride + n] = 42865786880.0;
            }
         }
         if (dataTypeVal == AUDIOLIB_FLOAT64) {
            if (pInLocal[m * inStride + n] > DBL_MAX_DB10) {
               pOutLocal[m * outStride + n] = 3082.546875;
            }
            else if (pInLocal[m * inStride + n] <= 0) {
               pOutLocal[m * outStride + n] = 1.843542739450977e+20;
            }
         }
      }
   }

   return (status);
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS
AUDIOLIB_dB10_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_dB10_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
