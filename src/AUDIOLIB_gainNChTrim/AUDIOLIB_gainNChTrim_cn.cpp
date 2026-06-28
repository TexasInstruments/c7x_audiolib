// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_gainNChTrim_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChTrim_exec_cn(AUDIOLIB_kernelHandle handle,
                                             void *restrict pIn,
                                             void *restrict pGain,
                                             void *restrict pMasterGain,
                                             void *restrict pOut)
{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrim_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrim_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrim_exec_cn");

   dataType *restrict pInLocal         = (dataType *) pIn;
   dataType *restrict pGainLocal       = (dataType *) pGain;
   dataType *restrict pMasterGainLocal = (dataType *) pMasterGain;
   dataType *restrict pOutLocal        = (dataType *) pOut;

   uint32_t dim_x      = pKerPrivArgs->dim_x;
   uint32_t dim_y      = pKerPrivArgs->dim_y;
   int32_t  strideIn   = pKerPrivArgs->strideIn;
   int32_t  strideOut  = pKerPrivArgs->strideOut;
   dataType masterGain = (dataType) (*pMasterGainLocal);

   for (uint32_t i = 0; i < dim_y; i++) {
      dataType gainCh = pGainLocal[i];
      for (uint32_t j = 0; j < dim_x; j++) {
         pOutLocal[i * strideOut + j] = pInLocal[i * strideIn + j] * gainCh * masterGain;
      }
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimInterLeave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                       void *restrict pIn,
                                                       void *restrict pGain,
                                                       void *restrict pMasterGain,
                                                       void *restrict pOut)
{
   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNChTrim_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNChTrim_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainNChTrim_exec_cn");

   dataType *restrict pInLocal         = (dataType *) pIn;
   dataType *restrict pGainLocal       = (dataType *) pGain;
   dataType *restrict pMasterGainLocal = (dataType *) pMasterGain;
   dataType *restrict pOutLocal        = (dataType *) pOut;

   uint32_t dim_x      = pKerPrivArgs->dim_x;
   uint32_t dim_y      = pKerPrivArgs->dim_y;
   int32_t  strideIn   = pKerPrivArgs->strideIn;
   int32_t  strideOut  = pKerPrivArgs->strideOut;
   dataType masterGain = (dataType) (*pMasterGainLocal);

   for (uint32_t i = 0; i < dim_x; ++i) {
      dataType gainCh = pGainLocal[i];
      for (uint32_t j = 0; j < dim_y; ++j) {
         pOutLocal[j * strideOut + i] = pInLocal[j * strideIn + i] * gainCh * masterGain;
      }
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_gainNChTrim_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn,
                                                             void *restrict pGain,
                                                             void *restrict pMasterGain,
                                                             void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_gainNChTrim_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                              void *restrict pIn,
                                                              void *restrict pGain,
                                                              void *restrict pMasterGain,
                                                              void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_gainNChTrimInterLeave_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                       void *restrict pIn,
                                                                       void *restrict pGain,
                                                                       void *restrict pMasterGain,
                                                                       void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_gainNChTrimInterLeave_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                                        void *restrict pIn,
                                                                        void *restrict pGain,
                                                                        void *restrict pMasterGain,
                                                                        void *restrict pOut);
