// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/********************************************************************************
 *  @file     AUDIOLIB_upSample_cn.cpp
 *  @brief    Natural-C reference implementation of AUDIOLIB_upSample.
 *  @version  0.1 - 2025 : Initial Version
 ********************************************************************************/

#include "AUDIOLIB_upSample_priv.h"
#include <cstdint>

/*
 * Natural-C reference upSampler (zero insertion).  Mirrors the Python
 * reference test/AUDIOLIB_idat_gen/AUDIOLIB_upSample/upSample.py exactly:
 * zero the output, then copy each input sample to output sample index
 * phase + k*upFactor, for every channel.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut )
{
   AUDIOLIB_STATUS             status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_upSample_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t upFactor     = pKerPrivArgs->upFactor;
   uint32_t phase        = pKerPrivArgs->phase;
   uint32_t isInterleave = pKerPrivArgs->isInterleave;
   int32_t  strideIn     = pKerPrivArgs->strideIn;  // elements
   int32_t  strideOut    = pKerPrivArgs->strideOut; // elements

   if (isInterleave == 0U) {
      /* planar: rows = channels, columns = samples; insert along columns */
      uint32_t numChannels = pKerPrivArgs->dimYIn;
      uint32_t numSamples  = pKerPrivArgs->dimXIn;
      uint32_t outSamples  = upFactor * numSamples;

      for (uint32_t m = 0; m < numChannels; m++) {
         for (uint32_t n = 0; n < outSamples; n++) {
            pOutLocal[m * strideOut + n] = (dataType) 0;
         }
         for (uint32_t k = 0; k < numSamples; k++) {
            pOutLocal[m * strideOut + phase + k * upFactor] = pInLocal[m * strideIn + k];
         }
      }
   }
   else {
      /* interleaved: rows = samples, columns = channels; insert along rows */
      uint32_t numSamples  = pKerPrivArgs->dimYIn;
      uint32_t numChannels = pKerPrivArgs->dimXIn;
      uint32_t outSamples  = upFactor * numSamples;

      for (uint32_t r = 0; r < outSamples; r++) {
         for (uint32_t c = 0; c < numChannels; c++) {
            pOutLocal[r * strideOut + c] = (dataType) 0;
         }
      }
      for (uint32_t k = 0; k < numSamples; k++) {
         uint32_t outRow = phase + k * upFactor;
         for (uint32_t c = 0; c < numChannels; c++) {
            pOutLocal[outRow * strideOut + c] = pInLocal[k * strideIn + c];
         }
      }
   }

   return status;
}

/* explicit instantiation for the supported data types */
template AUDIOLIB_STATUS
AUDIOLIB_upSample_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut );

template AUDIOLIB_STATUS
AUDIOLIB_upSample_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample_cn.cpp                                   */
/* ======================================================================== */
