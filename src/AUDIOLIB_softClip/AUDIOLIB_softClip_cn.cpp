// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_softClip_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_softClip_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS             status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_softClip_PrivArgs *pKerPrivArgs = (AUDIOLIB_softClip_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_softClip_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t dim_x     = pKerPrivArgs->dim_x;
   uint32_t dim_y     = pKerPrivArgs->dim_y;
   int32_t  inStride  = pKerPrivArgs->inStride;
   int32_t  outStride = pKerPrivArgs->outStride;
   float    threshold = pKerPrivArgs->threshold;
   float    endKnee   = pKerPrivArgs->endKnee;
   float    inp       = 0.0f;
   float    absInp    = 0.0f;
   float    out       = 0.0f;

   for (uint32_t m = 0; m < dim_y; m++) {
      for (uint32_t n = 0; n < dim_x; n++) {

         inp    = pInLocal[m * inStride + n];
         absInp = fabsf(inp);

         if (absInp < threshold) {
            pOutLocal[m * outStride + n] = pInLocal[m * inStride + n];
         }

         else {
            out = (absInp - threshold) / (absInp - 2.0f * threshold + endKnee);
            out = (endKnee - threshold) * out + threshold;

            if (inp < 0.0f) {
               pOutLocal[m * outStride + n] = -out;
            }

            else {
               pOutLocal[m * outStride + n] = out;
            }
         }
      }
   }
   return status;
}

// explicit insaddtiation for the different data type versions
template AUDIOLIB_STATUS
AUDIOLIB_softClip_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
