// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_gainNCh_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNCh_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNCh_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNCh_exec_cn");

   dataType *restrict pInLocal   = (dataType *) pIn;
   dataType *restrict pGainLocal = (dataType *) pGain;
   dataType *restrict pOutLocal  = (dataType *) pOut;

   uint32_t dim_x     = pKerPrivArgs->dim_x;
   uint32_t dim_y     = pKerPrivArgs->dim_y;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   for (uint32_t i = 0; i < dim_y; i++) {
      dataType gainCh = pGainLocal[i];
      for (uint32_t j = 0; j < dim_x; j++) {
         pOutLocal[i * strideOut + j] = pInLocal[i * strideIn + j] * gainCh;
      }
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChInterLeave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                   void *restrict pIn,
                                                   void *restrict pGain,
                                                   void *restrict pOut)
{
   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNCh_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNCh_exec_cn");

   dataType *restrict pInLocal   = (dataType *) pIn;
   dataType *restrict pGainLocal = (dataType *) pGain;
   dataType *restrict pOutLocal  = (dataType *) pOut;

   uint32_t dim_x     = pKerPrivArgs->dim_x;
   uint32_t dim_y     = pKerPrivArgs->dim_y;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   for (uint32_t i = 0; i < dim_x; ++i) {
      dataType gainCh = pGainLocal[i];
      for (uint32_t j = 0; j < dim_y; ++j) {
         pOutLocal[j * strideOut + i] = pInLocal[j * strideIn + i] * gainCh;
      }
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_gainNCh_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pGain,
                                                         void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_gainNCh_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pGain,
                                                          void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_gainNChInterLeave_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pGain,
                                                                   void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_gainNChInterLeave_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                                    void *restrict pIn,
                                                                    void *restrict pGain,
                                                                    void *restrict pOut);
