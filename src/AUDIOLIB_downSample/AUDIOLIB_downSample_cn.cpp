// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0


#include "AUDIOLIB_downSample_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_downSample_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_downSample_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t dim_x           = pKerPrivArgs->dim_x;
   uint32_t dim_y           = pKerPrivArgs->dim_y;
   uint32_t strideIn        = pKerPrivArgs->strideIn;
   uint32_t strideOut       = pKerPrivArgs->strideOut;
   uint32_t downSampleRatio = pKerPrivArgs->downSampleRatio;
   uint32_t phase           = pKerPrivArgs->phase;
   /* planar: keep every downSampleRatio-th sample of each channel row,
    * starting at column `phase`. */
   for (uint32_t i = 0U; i < dim_y; i++) {
      uint32_t iOut = 0U;
      for (uint32_t j = phase; j < dim_x; j += downSampleRatio) {
         pOutLocal[i * strideOut + iOut] = pInLocal[i * strideIn + j];
         iOut++;
      }
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleInterleave_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_downSampleInterleave_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t dim_x           = pKerPrivArgs->dim_x;
   uint32_t dim_y           = pKerPrivArgs->dim_y;
   uint32_t strideIn        = pKerPrivArgs->strideIn;
   uint32_t strideOut       = pKerPrivArgs->strideOut;
   uint32_t downSampleRatio = pKerPrivArgs->downSampleRatio;
   uint32_t phase           = pKerPrivArgs->phase;
   /* interleaved: keep every downSampleRatio-th sample-row, starting at row
    * `phase`; each kept row carries all dim_x channels. */
   for (uint32_t i = 0U; i < dim_x; ++i) {
      uint32_t iOut = 0U;
      for (uint32_t j = phase; j < dim_y; j += downSampleRatio) {
         pOutLocal[iOut * strideOut + i] = pInLocal[j * strideIn + i];
         iOut++;
      }
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_downSample_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_downSample_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_downSampleInterleave_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_downSampleInterleave_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
