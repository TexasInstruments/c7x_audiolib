// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_router_priv.h"
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_router_exec_cn(AUDIOLIB_kernelHandle handle,
                                        void *restrict pIn,
                                        void *restrict pOut,
                                        void *restrict pOutScratch)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_router_PrivArgs *pKerPrivArgs = (AUDIOLIB_router_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_router_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t  samples           = pKerPrivArgs->samples;
   uint32_t  strideOut         = pKerPrivArgs->strideOut;
   uint32_t  outputChannels    = pKerPrivArgs->outputChannels;
   uint32_t *outputOffsetArray = pKerPrivArgs->outputOffsetArray;
   int32_t  *muteArray         = pKerPrivArgs->muteArray;

   for (uint32_t outCh = 0; outCh < outputChannels; outCh++) {
      int32_t muteVAl = muteArray[outCh];
      for (uint32_t s = 0; s < samples; s++) {
         pOutLocal[outCh * strideOut + s] = pInLocal[outputOffsetArray[outCh] + s] * muteVAl;
      }
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_routerInterLeave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pOut,
                                                  void *restrict pOutScratch)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_router_PrivArgs *pKerPrivArgs = (AUDIOLIB_router_PrivArgs *) handle;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_router_exec_cn");
   uint32_t  samples           = pKerPrivArgs->samples;
   uint32_t  strideOut         = pKerPrivArgs->strideOut;
   uint32_t  strideIn          = pKerPrivArgs->strideIn;
   uint32_t  outputChannels    = pKerPrivArgs->outputChannels;
   uint32_t *outputOffsetArray = pKerPrivArgs->outputOffsetArray;
   int32_t  *muteArray         = pKerPrivArgs->muteArray;

   for (uint32_t s = 0; s < outputChannels; s++) {
      int32_t  muteVal = muteArray[s];
      uint32_t base    = outputOffsetArray[s];

      for (uint32_t outCh = 0; outCh < samples; outCh++) {
         dataType inVal = pInLocal[base + outCh * strideIn];

         pOutLocal[outCh * strideOut + s] = inVal * muteVal;
      }
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_router_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pOut,
                                                        void *restrict pOutScratch);

template AUDIOLIB_STATUS AUDIOLIB_router_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pOut,
                                                         void *restrict pOutScratch);

template AUDIOLIB_STATUS AUDIOLIB_routerInterLeave_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pOut,
                                                                  void *restrict pOutScratch);

template AUDIOLIB_STATUS AUDIOLIB_routerInterLeave_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pOut,
                                                                   void *restrict pOutScratch);
