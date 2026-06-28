// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_gain_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gain_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gain_PrivArgs *pKerPrivArgs = (AUDIOLIB_gain_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gain_exec_cn");

   dataType *restrict pInLocal   = (dataType *) pIn;
   dataType *restrict pGainLocal = (dataType *) pGain;
   dataType *restrict pOutLocal  = (dataType *) pOut;

   dataType gain      = *pGainLocal;
   uint32_t dim_x     = pKerPrivArgs->dim_x;
   uint32_t dim_y     = pKerPrivArgs->dim_y;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   for (uint32_t m = 0; m < dim_y; m++) {
      for (uint32_t n = 0; n < dim_x; n++) {
         pOutLocal[m * strideOut + n] = pInLocal[m * strideIn + n] * gain;
      }
   }

   return status;
}

// explicit insgaintiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_gain_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pGain,
                                                      void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_gain_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                       void *restrict pIn,
                                                       void *restrict pGain,
                                                       void *restrict pOut);
