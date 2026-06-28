// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_delay_priv.h"
#include <algorithm>
#include <cstdint>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delay_interleave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pDelay,
                                                  void *restrict pOut,
                                                  void *restrict pScratch)
{
   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_delay_interleave_exec_cn");

   dataType *restrict pInLocal      = (dataType *) pIn;
   dataType *restrict pOutLocal     = (dataType *) pOut;
   dataType *restrict pDelayLocal   = (dataType *) pDelay;
   dataType *restrict pScratchLocal = (dataType *) pScratch;

   uint32_t mode                = pKerPrivArgs->mode;
   uint32_t delaySize           = pKerPrivArgs->delaySize;
   uint32_t numSamples          = pKerPrivArgs->numSamples;
   uint32_t numChannels         = pKerPrivArgs->numChannels;
   uint32_t delayBuffSize       = pKerPrivArgs->delayBuffSize;
   int32_t  strideInElements    = pKerPrivArgs->strideInElements;
   int32_t  strideOutElements   = pKerPrivArgs->strideOutElements;
   int32_t  strideDelayElements = pKerPrivArgs->strideDelayElements;

   uint32_t i, j;

   // Linear Delay Implementation
   if (mode == 0) {
      if (delaySize <= numSamples) {
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < delaySize; j++) {
               // Copy data from pDelay buffer to pOut buffer
               pScratchLocal[i * numSamples + j] = pDelayLocal[i * strideDelayElements + j];
               // Copy rest of the data from pIn buffer to pDelay buffer
               pDelayLocal[i * strideDelayElements + j] = pInLocal[strideInElements * (numSamples - delaySize + j) + i];
            }
         }

         // Copy data from pIn buffer to pOut buffer
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < (numSamples - delaySize); j++) {
               pScratchLocal[(i * numSamples) + delaySize + j] = pInLocal[j * strideInElements + i];
            }
         }
      }
      else {
         // Copy data from pDelay buffer to pOut buffer
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < numSamples; j++) {
               pScratchLocal[i * numSamples + j] = pDelayLocal[i * strideDelayElements + j];
            }
         }

         // Left Shift the remaining elements within pDelay buffer
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < (delaySize - numSamples); j++) {
               pDelayLocal[i * strideDelayElements + j] = pDelayLocal[i * strideDelayElements + numSamples + j];
            }
         }

         // Copy data from pIn buffer to pDelay buffer
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < numSamples; j++) {
               pDelayLocal[i * strideDelayElements + (delaySize - numSamples) + j] = pInLocal[j * strideInElements + i];
            }
         }
      }
   }
   // Circular Delay Implementation
   else if (mode == 1) {
      uint32_t readIdx  = pKerPrivArgs->readIdx;
      uint32_t writeIdx = pKerPrivArgs->writeIdx;

      uint32_t loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
      uint32_t loopWriteCnt2 = numSamples - loopWriteCnt1;

      uint32_t loopReadCnt1 = (delayBuffSize - readIdx) <= numSamples ? (delayBuffSize - readIdx) : numSamples;
      uint32_t loopReadCnt2 = numSamples - loopReadCnt1;

      // Write Input samples from pIn buffer to pDelay buffer
      for (i = 0; i < numChannels; i++) {
         for (j = 0; j < loopWriteCnt1; j++) {
            pDelayLocal[i * strideDelayElements + writeIdx + j] = pInLocal[j * strideInElements + i];
         }
      }
      writeIdx = (writeIdx + loopWriteCnt1) % delayBuffSize;

      if (loopWriteCnt2 > 0) {
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < loopWriteCnt2; j++) {
               pDelayLocal[i * strideDelayElements + j] = pInLocal[strideInElements * (loopWriteCnt1 + j) + i];
            }
         }
         writeIdx = loopWriteCnt2;
      }

      // Read samples from pDelay buffer and write to pOut buffer
      for (i = 0; i < numChannels; i++) {
         for (j = 0; j < loopReadCnt1; j++) {
            pScratchLocal[i * numSamples + j] = pDelayLocal[i * strideDelayElements + readIdx + j];
         }
      }
      readIdx = (readIdx + loopReadCnt1) % delayBuffSize;

      if (loopReadCnt2 > 0) {
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < loopReadCnt2; j++) {
               pScratchLocal[i * numSamples + loopReadCnt1 + j] = pDelayLocal[i * strideDelayElements + j];
            }
         }
         readIdx = loopReadCnt2;
      }

      pKerPrivArgs->readIdx  = readIdx;
      pKerPrivArgs->writeIdx = writeIdx;
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   for (i = 0; i < numSamples; i++) {
      for (j = 0; j < numChannels; j++) {
         pOutLocal[i * strideOutElements + j] = pScratchLocal[i + numSamples * j];
      }
   }

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_delay_interleave_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pDelay,
                                                                  void *restrict pOut,
                                                                  void *restrict pScratch);

template AUDIOLIB_STATUS AUDIOLIB_delay_interleave_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pDelay,
                                                                   void *restrict pOut,
                                                                   void *restrict pScratch);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delay_exec_cn(AUDIOLIB_kernelHandle handle,
                                       void *restrict pIn,
                                       void *restrict pDelay,
                                       void *restrict pOut,
                                       void *restrict pScratch)
{
   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_delay_exec_cn");

   dataType *restrict pInLocal    = (dataType *) pIn;
   dataType *restrict pOutLocal   = (dataType *) pOut;
   dataType *restrict pDelayLocal = (dataType *) pDelay;

   uint32_t mode                = pKerPrivArgs->mode;
   uint32_t delaySize           = pKerPrivArgs->delaySize;
   uint32_t numSamples          = pKerPrivArgs->numSamples;
   uint32_t numChannels         = pKerPrivArgs->numChannels;
   uint32_t delayBuffSize       = pKerPrivArgs->delayBuffSize;
   int32_t  strideInElements    = pKerPrivArgs->strideInElements;
   int32_t  strideOutElements   = pKerPrivArgs->strideOutElements;
   int32_t  strideDelayElements = pKerPrivArgs->strideDelayElements;

   uint32_t i, j;

   // Linear Delay Implementation
   if (mode == 0) {
      if (delaySize <= numSamples) {
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < delaySize; j++) {
               // Copy data from pDelay buffer to pOut buffer
               pOutLocal[i * strideOutElements + j] = pDelayLocal[i * strideDelayElements + j];
               // Copy rest of the data from pIn buffer to pDelay buffer
               pDelayLocal[i * strideDelayElements + j] = pInLocal[i * strideInElements + (numSamples - delaySize) + j];
            }
         }

         // Copy data from pIn buffer to pOut buffer
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < (numSamples - delaySize); j++) {
               pOutLocal[(i * strideOutElements) + delaySize + j] = pInLocal[i * strideInElements + j];
            }
         }
      }
      else {
         // Copy data from pDelay buffer to pOut buffer
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < numSamples; j++) {
               pOutLocal[i * strideOutElements + j] = pDelayLocal[i * strideDelayElements + j];
            }
         }

         // Left Shift the remaining elements within pDelay buffer
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < (delaySize - numSamples); j++) {
               pDelayLocal[i * strideDelayElements + j] = pDelayLocal[i * strideDelayElements + numSamples + j];
            }
         }

         // Copy data from pIn buffer to pDelay buffer
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < numSamples; j++) {
               pDelayLocal[i * strideDelayElements + (delaySize - numSamples) + j] = pInLocal[i * strideInElements + j];
            }
         }
      }
   }
   // Circular Delay Implementation
   else if (mode == 1) {
      uint32_t readIdx  = pKerPrivArgs->readIdx;
      uint32_t writeIdx = pKerPrivArgs->writeIdx;

      uint32_t loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
      uint32_t loopWriteCnt2 = numSamples - loopWriteCnt1;

      uint32_t loopReadCnt1 = (delayBuffSize - readIdx) <= numSamples ? (delayBuffSize - readIdx) : numSamples;
      uint32_t loopReadCnt2 = numSamples - loopReadCnt1;

      // Write Input samples from pIn buffer to pDelay buffer
      for (i = 0; i < numChannels; i++) {
         for (j = 0; j < loopWriteCnt1; j++) {
            pDelayLocal[i * strideDelayElements + writeIdx + j] = pInLocal[i * strideInElements + j];
         }
      }
      writeIdx = (writeIdx + loopWriteCnt1) % delayBuffSize;

      if (loopWriteCnt2 > 0) {
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < loopWriteCnt2; j++) {
               pDelayLocal[i * strideDelayElements + j] = pInLocal[i * strideInElements + loopWriteCnt1 + j];
            }
         }
         writeIdx = loopWriteCnt2;
      }

      // Read samples from pDelay buffer and write to pOut buffer
      for (i = 0; i < numChannels; i++) {
         for (j = 0; j < loopReadCnt1; j++) {
            pOutLocal[i * strideOutElements + j] = pDelayLocal[i * strideDelayElements + readIdx + j];
         }
      }
      readIdx = (readIdx + loopReadCnt1) % delayBuffSize;

      if (loopReadCnt2 > 0) {
         for (i = 0; i < numChannels; i++) {
            for (j = 0; j < loopReadCnt2; j++) {
               pOutLocal[i * strideOutElements + loopReadCnt1 + j] = pDelayLocal[i * strideDelayElements + j];
            }
         }
         readIdx = loopReadCnt2;
      }

      pKerPrivArgs->readIdx  = readIdx;
      pKerPrivArgs->writeIdx = writeIdx;
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS AUDIOLIB_delay_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                       void *restrict pIn,
                                                       void *restrict pDelay,
                                                       void *restrict pOut,
                                                       void *restrict pScratch);

template AUDIOLIB_STATUS AUDIOLIB_delay_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pDelay,
                                                        void *restrict pOut,
                                                        void *restrict pScratch);
