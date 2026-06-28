// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_ssrc_priv.h"
#include <string.h>

#define AUDIOLIB_SSRC_MATRIX_TRANSPOSE_ROW_STRIDE(x, y) (((x + y - 1) / y) * y)

int32_t AUDIOLIB_ssrc_getHandleSize(AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_ssrc_PrivArgs);
   return privBufSize;
}

int32_t AUDIOLIB_ssrc_getFilterCoeffSize(ssrc_sample_rate_t            inputSampleRate,
                                         ssrc_sample_rate_t            outputSampleRate,
                                         AUDIOLIB_ssrc_buffer_format_t bufferFormat,
#ifdef C7X
                                         uint8_t  enableMMA,
                                         uint32_t mmaSize,
#endif
                                         uint32_t sampleDataType)
{
   int32_t  filterCoeffSizeInBytes;
   int32_t  eleSize         = AUDIOLIB_sizeof(sampleDataType);
   int32_t  alignedEleCount = AUDIOLIB_L2DATA_ALIGNMENT / eleSize;
   uint32_t filterLength1   = AUDIOLIB_ssrc_getFilterLength(inputSampleRate, outputSampleRate, 1U);
   uint32_t filterLength2   = AUDIOLIB_ssrc_getFilterLength(inputSampleRate, outputSampleRate, 2U);
   uint32_t filterLength    = (AUDIOLIB_ceilingDiv(filterLength1, alignedEleCount) * alignedEleCount) +
                           (AUDIOLIB_ceilingDiv(filterLength2, alignedEleCount) * alignedEleCount);

#ifdef C7X
   if (AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR == bufferFormat || enableMMA == 0U) {
      filterCoeffSizeInBytes = filterLength * eleSize;
   }
   else {
      filterCoeffSizeInBytes = (filterLength + (3 * mmaSize - 1) * 2) * eleSize;
   }
#endif
#ifdef ARM_A53
   filterCoeffSizeInBytes = filterLength * eleSize;
#endif

   return filterCoeffSizeInBytes;
}

AUDIOLIB_STATUS AUDIOLIB_ssrc_copyFilterCoeffs(ssrc_sample_rate_t            inputSampleRate,
                                               ssrc_sample_rate_t            outputSampleRate,
                                               AUDIOLIB_ssrc_buffer_format_t bufferFormat,
#ifdef C7X
                                               uint8_t  enableMMA,
                                               uint32_t mmaSize,
#endif
                                               uint32_t     sampleDataType,
                                               const float *downsampleStage1,
                                               const float *downsampleStage2,
                                               const float *upsampleStage1,
                                               const float *upsampleStage2,
                                               int32_t      filterCoeffSizeInBytes,
                                               void        *pFiltCoeffs)
{
   AUDIOLIB_STATUS status           = AUDIOLIB_SUCCESS;
   int32_t         eleSize          = AUDIOLIB_sizeof(sampleDataType);
   int32_t         alignedEleCount  = AUDIOLIB_L2DATA_ALIGNMENT / eleSize;
   float          *pFiltCoeffsLocal = (float *) pFiltCoeffs;

   // Check for NULL pointer
   if (pFiltCoeffs == nullptr || downsampleStage1 == nullptr || downsampleStage2 == nullptr ||
       upsampleStage1 == nullptr || upsampleStage2 == nullptr) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (AUDIOLIB_SUCCESS == status) {
      // Zero out the destination buffer
      memset(pFiltCoeffs, 0, filterCoeffSizeInBytes);

      // Determine if it's upsampling or downsampling
      bool isUpsampling = AUDIOLIB_ssrc_isUpsampling(inputSampleRate, outputSampleRate);

      // Get filter lengths for stage 1 and stage 2
      uint32_t filterLengthStage1       = AUDIOLIB_ssrc_getFilterLength(inputSampleRate, outputSampleRate, 1U);
      uint32_t filterLengthStage2       = AUDIOLIB_ssrc_getFilterLength(inputSampleRate, outputSampleRate, 2U);
      uint32_t filterLengthStage1Padded = AUDIOLIB_ceilingDiv(filterLengthStage1, alignedEleCount) * alignedEleCount;

      // Copy the appropriate filter coefficients based on upsampling/downsampling
      if (isUpsampling) {
         // Copy upsampling filter coefficients
#ifdef C7X
         if (AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR == bufferFormat || enableMMA == 0U) {
#endif
#ifdef ARM_A53
            if (AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR == bufferFormat ||
                AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR == bufferFormat) {
#endif
               memcpy((void *) pFiltCoeffsLocal, (void *) upsampleStage1, filterLengthStage1 * eleSize);
               pFiltCoeffsLocal += filterLengthStage1Padded;
               // If there's a second stage, copy those coefficients too
               if (filterLengthStage2 > 0) {
                  memcpy((void *) pFiltCoeffsLocal, (void *) upsampleStage2, filterLengthStage2 * eleSize);
               }
            }
            else {
#ifdef C7X
               // For MMA-enabled linear buffer, align to MMA_SIZE boundary
               pFiltCoeffsLocal += 2 * mmaSize;
               memcpy((void *) pFiltCoeffsLocal, (void *) upsampleStage1, filterLengthStage1 * eleSize);
               pFiltCoeffsLocal += filterLengthStage1Padded + (mmaSize - 1);
               // If there's a second stage, copy those coefficients too
               if (filterLengthStage2 > 0) {
                  pFiltCoeffsLocal += 2 * mmaSize;
                  memcpy((void *) pFiltCoeffsLocal, (void *) upsampleStage2, filterLengthStage2 * eleSize);
               }
#endif
            }
         }
         else {
            if (filterLengthStage2 == 0) {
#ifdef C7X
               if (AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR == bufferFormat || enableMMA == 0U) {
#endif
#ifdef ARM_A53
                  if (AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR == bufferFormat ||
                      AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR == bufferFormat) {
#endif
                     // Copy downsampling filter coefficients
                     memcpy((void *) pFiltCoeffsLocal, (void *) downsampleStage2, filterLengthStage1 * eleSize);
                  }
                  else {
#ifdef C7X
                     // For MMA-enabled linear buffer, align to MMA_SIZE boundary
                     pFiltCoeffsLocal += 2 * mmaSize;
                     memcpy((void *) pFiltCoeffsLocal, (void *) downsampleStage2, filterLengthStage1 * eleSize);
#endif
                  }
               }
               else { // If it is 2 stages, copy both coefficient sets
#ifdef C7X
                  if (AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR == bufferFormat || enableMMA == 0U) {
#endif
#ifdef ARM_A53
                     if (AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR == bufferFormat ||
                         AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR == bufferFormat) {
#endif
                        // Copy downsampling filter coefficients
                        memcpy((void *) pFiltCoeffsLocal, (void *) downsampleStage1, filterLengthStage1 * eleSize);
                        pFiltCoeffsLocal += filterLengthStage1Padded;
                        memcpy((void *) pFiltCoeffsLocal, (void *) downsampleStage2, filterLengthStage2 * eleSize);
                     }
                     else {
#ifdef C7X
                        // For MMA-enabled linear buffer, align to MMA_SIZE boundary
                        pFiltCoeffsLocal += 2 * mmaSize;
                        memcpy((void *) pFiltCoeffsLocal, (void *) downsampleStage1, filterLengthStage1 * eleSize);
                        pFiltCoeffsLocal += filterLengthStage1Padded + (mmaSize - 1);
                        pFiltCoeffsLocal += 2 * mmaSize;
                        memcpy((void *) pFiltCoeffsLocal, (void *) downsampleStage2, filterLengthStage2 * eleSize);
#endif
                     }
                  }
               }
            }

            return status;
         }

         int32_t AUDIOLIB_ssrc_getOutBufferLength(ssrc_sample_rate_t inputSampleRate,
                                                  ssrc_sample_rate_t outputSampleRate, uint32_t inputSampleCount)
         {
            float   sampleRateRatio;
            int32_t outFrameLength;
            bool    isUpsampling;

#if AUDIOLIB_DEBUGPRINT
            printf("Enter AUDIOLIB_ssrc_getOutBufferLength\n");
#endif

            // Calculate the asynchronous sample rate conversion ratio
            sampleRateRatio = AUDIOLIB_ssrc_getSampleRateRatio(inputSampleRate, outputSampleRate);
            isUpsampling    = AUDIOLIB_ssrc_isUpsampling(inputSampleRate, outputSampleRate);

            // Calculate output frame length based on asynchronous sample rate conversion ratio
            if (isUpsampling) {
               // For upsampling, output samples = input samples * ratio
               outFrameLength = (int32_t) ceil(inputSampleCount * sampleRateRatio);
            }
            else {
               // For downsampling, output samples = input samples / ratio
               outFrameLength = (int32_t) (inputSampleCount / sampleRateRatio);
            }

            return outFrameLength;
         }

         float AUDIOLIB_ssrc_getSampleRateRatio(ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate)
         {
            // Convert enum values to actual sample rates
            uint32_t outRate = AUDIOLIB_ssrc_convertSampleRateToInt(outputSampleRate);
            uint32_t inRate  = AUDIOLIB_ssrc_convertSampleRateToInt(inputSampleRate);
            float    ratio;

            // Calculate ratio depending on upsampling or downsampling
            if (outRate > inRate) {
               // Upsampling
               ratio = ((float) outRate) / ((float) inRate);
            }
            else {
               // Downsampling
               ratio = ((float) inRate) / ((float) outRate);
            }

            return ratio;
         }

         bool AUDIOLIB_ssrc_isUpsampling(ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate)
         {
            // Convert enum values to actual sample rates
            uint32_t outRate = AUDIOLIB_ssrc_convertSampleRateToInt(outputSampleRate);
            uint32_t inRate  = AUDIOLIB_ssrc_convertSampleRateToInt(inputSampleRate);

            // Return 1 (true) if upsampling, 0 (false) otherwise
            return (outRate > inRate) ? 1 : 0;
         }

         AUDIOLIB_STATUS AUDIOLIB_ssrc_checkValidRatio(ssrc_sample_rate_t inputSampleRate,
                                                       ssrc_sample_rate_t outputSampleRate)
         {
            AUDIOLIB_STATUS status          = AUDIOLIB_SUCCESS;
            float           sampleRateRatio = AUDIOLIB_ssrc_getSampleRateRatio(inputSampleRate, outputSampleRate);

            // Check if ratio is 2 or 4
            if (sampleRateRatio != 2.0f && sampleRateRatio != 4.0f) {
               status = AUDIOLIB_ERR_INVALID_VALUE;
            }

            return status;
         }

         uint32_t AUDIOLIB_ssrc_getNextPowerOf2(uint32_t value)
         {
            uint32_t paddedValue = value;
            uint32_t result;

#if AUDIOLIB_DEBUGPRINT
            /* Enter function debug print */
            printf("Enter AUDIOLIB_ssrc_getNextPowerOf2\n");
#endif

            /* Handle edge case for value == 0 */
            if (paddedValue == 0) {
               result = 1;
            }
            else {
               /* Find the next power of 2 using bit manipulation */
               paddedValue--;
               paddedValue = (paddedValue | (paddedValue >> 1));
               paddedValue = (paddedValue | (paddedValue >> 2));
               paddedValue = (paddedValue | (paddedValue >> 4));
               paddedValue = (paddedValue | (paddedValue >> 8));
               paddedValue = (paddedValue | (paddedValue >> 16));
               paddedValue++;

               result = paddedValue;
            }

            return result;
         }

         uint32_t AUDIOLIB_ssrc_getFilterLength(ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate,
                                                uint32_t stage)
         {
            uint32_t filterLength;
            bool     isUpsampling    = AUDIOLIB_ssrc_isUpsampling(inputSampleRate, outputSampleRate);
            float    sampleRateRatio = AUDIOLIB_ssrc_getSampleRateRatio(inputSampleRate, outputSampleRate);
            uint32_t numStages       = (sampleRateRatio == 4.0f) ? 2 : 1;

            // return filter length as 0 if stage parameter is invalid
            if (stage > numStages || stage == 0) {
               filterLength = 0;
            }
            else {
               if (isUpsampling) {
                  // For upsampling
                  if (numStages == 1 || stage == 1) {
                     // For single stage or first stage of multi-stage upsampling
                     filterLength = (uint32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
                  }
                  else {
                     // For second stage of multi-stage upsampling
                     filterLength = (uint32_t) AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
                  }
               }
               else {
                  // For downsampling
                  if (numStages == 1) {
                     // For single stage downsampling
                     filterLength = (uint32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
                  }
                  else {
                     if (stage == 1) {
                        // For first stage of multi-stage downsampling
                        filterLength = (uint32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC;
                     }
                     else {
                        // For second stage of multi-stage downsampling
                        filterLength = (uint32_t) AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC;
                     }
                  }
               }
            }

            return filterLength;
         }

         uint32_t AUDIOLIB_ssrc_getSampleHistoryLength(ssrc_sample_rate_t inputSampleRate,
                                                       ssrc_sample_rate_t outputSampleRate, uint32_t stage)
         {
            uint32_t historyLength;
            bool     isUpsampling    = AUDIOLIB_ssrc_isUpsampling(inputSampleRate, outputSampleRate);
            float    sampleRateRatio = AUDIOLIB_ssrc_getSampleRateRatio(inputSampleRate, outputSampleRate);
            uint32_t numStages       = (sampleRateRatio == 4.0f) ? 2 : 1;

            // return filter length as 0 if stage parameter is invalid
            if (stage > numStages || stage == 0) {
               historyLength = 0;
            }
            else {
               if (isUpsampling) {
                  // For upsampling
                  if (numStages == 1 || stage == 1) {
                     // For single stage or first stage of multi-stage upsampling
                     historyLength = (uint32_t) (AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1);
                  }
                  else {
                     // For second stage of multi-stage upsampling
                     historyLength = (uint32_t) (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC - 1);
                  }
               }
               else {
                  // For downsampling
                  if (numStages == 1) {
                     // For single stage downsampling
                     historyLength = (uint32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1);
                  }
                  else {
                     if (stage == 1) {
                        // For first stage of multi-stage downsampling
                        historyLength = (uint32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS - 1);
                     }
                     else {
                        // For second stage of multi-stage downsampling
                        historyLength = (uint32_t) (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1);
                     }
                  }
               }
            }

            return historyLength;
         }
#ifdef C7X
         AUDIOLIB_ssrc_buffer_format_t AUDIOLIB_ssrc_optimalBufferFormat(
             ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate, uint32_t inputSampleCount,
             uint8_t numChannels, uint8_t dataFormat, uint8_t enableMMA)
         {
            float sampleRateRatio;
            bool  isUpsampling;

            AUDIOLIB_ssrc_buffer_format_t bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;

#if AUDIOLIB_DEBUGPRINT
            printf("Enter AUDIOLIB_ssrc_optimalBufferFormat\n");
#endif

            // Calculate the asynchronous sample rate conversion ratio
            sampleRateRatio = AUDIOLIB_ssrc_getSampleRateRatio(inputSampleRate, outputSampleRate);
            isUpsampling    = AUDIOLIB_ssrc_isUpsampling(inputSampleRate, outputSampleRate);

            if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
               if (enableMMA) {
#if defined(__C7X_MMA_2_256__)
                  if (isUpsampling) {
                     if (sampleRateRatio == 2.0f) {
                        if (inputSampleCount < 64U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                        }
                     }
                     else {
                        if (inputSampleCount < 32U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                        }
                     }
                  }
                  else {
                     if (sampleRateRatio == 2.0f) {
                        if (inputSampleCount < 96U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                        }
                     }
                     else {
                        if (inputSampleCount < 256U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                        }
                     }
                  }

#elif defined(__C7X_MMA_2_256F__)
                  if (isUpsampling) {
                     if (sampleRateRatio == 2.0f) {
                        if (inputSampleCount < 32U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                        }
                     }
                     else {
                        if (inputSampleCount < 32U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                        }
                     }
                  }
                  else {
                     if (sampleRateRatio == 2.0f) {
                        if (inputSampleCount < 64U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                        }
                     }
                     else {
                        if (inputSampleCount < 128U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                        }
                     }
                  }
#endif
               }
               else {
                  if (isUpsampling) {
                     if (sampleRateRatio == 2.0f) {
                        if (inputSampleCount < 64U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else if (inputSampleCount < 96U) {
                           if (numChannels == 1) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                        else if (inputSampleCount < 128U) {
                           if (numChannels <= 2) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                        else {
                           if (numChannels % 8 <= 4) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                     }
                     else {
                        if (inputSampleCount < 32U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else if (inputSampleCount < 64U) {
                           if (numChannels == 1) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                        else if (inputSampleCount < 128U) {
                           if (numChannels <= 2) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                        else {
                           if (numChannels % 8 <= 4) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                     }
                  }
                  else {
                     if (sampleRateRatio == 2.0f) {
                        if (inputSampleCount < 96U) {
                           bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                        }
                        else if (inputSampleCount < 192U) {
                           if (numChannels <= 1) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                        else if (inputSampleCount < 256U) {
                           if (numChannels <= 2) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                        else {
                           if (numChannels % 8 <= 4) {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
                           }
                           else {
                              bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                           }
                        }
                     }
                     else {
                        bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
                     }
                  }
               }
            }
            else {
               bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
            }

            return bufferFormat;
         }
#endif
         uint32_t AUDIOLIB_ssrc_convertSampleRateToInt(ssrc_sample_rate_t rate)
         {
            uint32_t result = 0;

            switch (rate) {
            case SSRC_SAMPLE_RATE_8000:
               result = 8000;
               break;
            case SSRC_SAMPLE_RATE_11025:
               result = 11025;
               break;
            case SSRC_SAMPLE_RATE_12000:
               result = 12000;
               break;
            case SSRC_SAMPLE_RATE_16000:
               result = 16000;
               break;
            case SSRC_SAMPLE_RATE_22050:
               result = 22050;
               break;
            case SSRC_SAMPLE_RATE_24000:
               result = 24000;
               break;
            case SSRC_SAMPLE_RATE_32000:
               result = 32000;
               break;
            case SSRC_SAMPLE_RATE_44100:
               result = 44100;
               break;
            case SSRC_SAMPLE_RATE_48000:
               result = 48000;
               break;
            case SSRC_SAMPLE_RATE_64000:
               result = 64000;
               break;
            case SSRC_SAMPLE_RATE_88200:
               result = 88200;
               break;
            case SSRC_SAMPLE_RATE_96000:
               result = 96000;
               break;
            case SSRC_SAMPLE_RATE_128000:
               result = 128000;
               break;
            case SSRC_SAMPLE_RATE_176400:
               result = 176400;
               break;
            case SSRC_SAMPLE_RATE_192000:
               result = 192000;
               break;
            default:
               result = 0;
               break;
            }

            return result;
         }

         AUDIOLIB_STATUS AUDIOLIB_ssrc_getCircularBufferParams(
             ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate, uint32_t inputSampleCount,
             uint8_t numChannels, uint32_t sampleDataType, AUDIOLIB_ssrc_bufferParams_t * pBufferParams)
         {
            AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
            // Calculate sample rate ratio and filter length using existing functions
            bool     isUpsampling    = AUDIOLIB_ssrc_isUpsampling(inputSampleRate, outputSampleRate);
            float    sampleRateRatio = AUDIOLIB_ssrc_getSampleRateRatio(inputSampleRate, outputSampleRate);
            uint32_t historyLength;

            // Check if the ratio between output and input sample rates is valid (only 2 or 4 allowed)
            status = AUDIOLIB_ssrc_checkValidRatio(inputSampleRate, outputSampleRate);

            if (status == AUDIOLIB_SUCCESS) {
               if (isUpsampling) {
                  historyLength = AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC - 1;
               }
               else {
                  historyLength = AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS - 1;
               }
               /* Minimum size required for a circular buffer for C7000 is 512 bytes,
               since the filter size is 96, we'll always meet that requirement because
               the next power of two multiple is 128 => 128 * 4 bytes*/
               // For interleaved format
               uint32_t inBufferTotalDimY =
                   historyLength + inputSampleCount * (uint32_t) AUDIOLIB_SSRC_PING_PONG_FACTOR;
               uint32_t inBufferTotalElementsInterleaved =
                   AUDIOLIB_ssrc_getNextPowerOf2(inBufferTotalDimY * numChannels);
               uint32_t inBufferTotalSizeInterleaved =
                   inBufferTotalElementsInterleaved * AUDIOLIB_sizeof(sampleDataType);

               // Set buffer size based on sample rate ratio
               if (sampleRateRatio == 2.0f) {
                  pBufferParams->bufferSize = inBufferTotalSizeInterleaved;
               }
               else {
                  pBufferParams->bufferSize = 2 * inBufferTotalSizeInterleaved;
               }
               pBufferParams->alignment = inBufferTotalSizeInterleaved;
               pBufferParams->stride    = numChannels * AUDIOLIB_sizeof(sampleDataType);
            }

            return status;
         }

         uint32_t AUDIOLIB_ssrc_getLinearInputBufferSize(
             ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate, uint32_t inputSampleCount,
             uint8_t numChannels, uint32_t sampleDataType, uint8_t dataFormat)
         {
            uint32_t inputBufferSize = 0;

            if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
#ifdef C7X
               // Calculate element count based on vector size and data type
               int32_t eleCount = 2 * (__C7X_VEC_SIZE_BYTES__ / AUDIOLIB_sizeof(sampleDataType));

               // Calculate padded row stride for C7x matrix transpose
               int32_t inputSampleCountPadded = AUDIOLIB_SSRC_MATRIX_TRANSPOSE_ROW_STRIDE(inputSampleCount, eleCount);

               inputBufferSize = inputSampleCountPadded * numChannels * AUDIOLIB_sizeof(sampleDataType);
#endif
#ifdef ARM_A53
               // A53 does not use C7x based matrix transpose, no padding needed
               inputBufferSize = inputSampleCount * numChannels * AUDIOLIB_sizeof(sampleDataType);
#endif
            }
            else {
               // For non-interleaved format
               inputBufferSize = numChannels * inputSampleCount * AUDIOLIB_sizeof(sampleDataType);
            }

            return inputBufferSize;
         }

         AUDIOLIB_STATUS AUDIOLIB_ssrc_getStateBufferParams(
             ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate, uint32_t inputSampleCount,
             uint8_t numChannels, uint32_t sampleDataType, uint8_t dataFormat,
             AUDIOLIB_ssrc_bufferParams_t * pBufferParams)
         {
            AUDIOLIB_STATUS status            = AUDIOLIB_SUCCESS;
            uint32_t        stateBufferSize   = 0;
            int32_t         numChannelsPadded = 0;
            int32_t         eleSize           = AUDIOLIB_sizeof(sampleDataType);
            bool            isUpsampling      = AUDIOLIB_ssrc_isUpsampling(inputSampleRate, outputSampleRate);
            float           sampleRateRatio   = AUDIOLIB_ssrc_getSampleRateRatio(inputSampleRate, outputSampleRate);
            uint32_t historyLength1  = AUDIOLIB_ssrc_getSampleHistoryLength(inputSampleRate, outputSampleRate, 1U);
            uint32_t historyLength2  = AUDIOLIB_ssrc_getSampleHistoryLength(inputSampleRate, outputSampleRate, 2U);
            uint32_t alignedEleCount = AUDIOLIB_L2DATA_ALIGNMENT / eleSize;
            uint32_t scratchBufferEleCount            = 0;
            uint32_t stage1DataBufferEleCount         = 0;
            uint32_t stage2DataBufferEleCount         = 0;
            uint32_t firSampleCount                   = 0;
            uint32_t inputVsFirInterleavedSampleCount = 0;
            uint32_t firDecimatedSampleCount          = 0;

            // Check if the ratio between output and input sample rates is valid (only 2 or 4 allowed)
            status = AUDIOLIB_ssrc_checkValidRatio(inputSampleRate, outputSampleRate);

            if (status == AUDIOLIB_SUCCESS) {
               if (dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  // Calculate element count based on vector size and data type
#ifdef C7X
                  int32_t eleCount = 2 * (__C7X_VEC_SIZE_BYTES__ / eleSize);
                  // Calculate padded row stride
                  numChannelsPadded = AUDIOLIB_SSRC_MATRIX_TRANSPOSE_ROW_STRIDE(numChannels, eleCount);
#endif
#ifdef ARM_A53
                  numChannelsPadded = numChannels;
#endif
               }
               else {
                  numChannelsPadded = numChannels;
               }

               // For linear buffer format, calculate the state buffer size
               if (sampleRateRatio == 2.0f) {
                  if (isUpsampling) {
                     firSampleCount = AUDIOLIB_ceilingDiv(inputSampleCount, alignedEleCount) * alignedEleCount;
                     inputVsFirInterleavedSampleCount =
                         AUDIOLIB_ceilingDiv(2 * inputSampleCount, alignedEleCount) * alignedEleCount;
                     scratchBufferEleCount = firSampleCount + inputVsFirInterleavedSampleCount;
                     stage1DataBufferEleCount =
                         AUDIOLIB_ceilingDiv(historyLength1 + inputSampleCount, alignedEleCount) * alignedEleCount;
                     stateBufferSize = (stage1DataBufferEleCount + scratchBufferEleCount) * numChannelsPadded * eleSize;
                  }
                  else {
                     firSampleCount = AUDIOLIB_ceilingDiv((historyLength1 + inputSampleCount) / 2, alignedEleCount) *
                                      alignedEleCount;
                     firDecimatedSampleCount =
                         AUDIOLIB_ceilingDiv(inputSampleCount / 2, alignedEleCount) * alignedEleCount;
                     scratchBufferEleCount = firSampleCount + firDecimatedSampleCount;
                     stage1DataBufferEleCount =
                         AUDIOLIB_ceilingDiv(historyLength1 + inputSampleCount, alignedEleCount) * alignedEleCount;
                     stateBufferSize = (stage1DataBufferEleCount + scratchBufferEleCount) * numChannelsPadded * eleSize;
                  }
               }
               else {
                  if (isUpsampling) {
                     firSampleCount = AUDIOLIB_ceilingDiv(2 * inputSampleCount, alignedEleCount) * alignedEleCount;
                     inputVsFirInterleavedSampleCount =
                         AUDIOLIB_ceilingDiv(4 * inputSampleCount, alignedEleCount) * alignedEleCount;
                     scratchBufferEleCount = firSampleCount + inputVsFirInterleavedSampleCount;
                     stage1DataBufferEleCount =
                         AUDIOLIB_ceilingDiv(historyLength1 + inputSampleCount, alignedEleCount) * alignedEleCount;
                     stage2DataBufferEleCount =
                         AUDIOLIB_ceilingDiv(historyLength2 + 2 * inputSampleCount, alignedEleCount) * alignedEleCount;
                     stateBufferSize = (stage1DataBufferEleCount + stage2DataBufferEleCount + scratchBufferEleCount) *
                                       numChannelsPadded * eleSize;
                  }
                  else {
                     firSampleCount = AUDIOLIB_ceilingDiv((historyLength2 + inputSampleCount) / 2, alignedEleCount) *
                                      alignedEleCount;
                     firDecimatedSampleCount =
                         AUDIOLIB_ceilingDiv(inputSampleCount / 2, alignedEleCount) * alignedEleCount;
                     scratchBufferEleCount = firSampleCount + firDecimatedSampleCount;
                     stage1DataBufferEleCount =
                         AUDIOLIB_ceilingDiv(historyLength1 + inputSampleCount, alignedEleCount) * alignedEleCount;
                     stage2DataBufferEleCount =
                         AUDIOLIB_ceilingDiv(historyLength2 + inputSampleCount / 2, alignedEleCount) * alignedEleCount;
                     stateBufferSize = (stage1DataBufferEleCount + stage2DataBufferEleCount + scratchBufferEleCount) *
                                       numChannelsPadded * eleSize;
                  }
               }

               pBufferParams->bufferSize = stateBufferSize;
               pBufferParams->alignment  = stateBufferSize / numChannelsPadded;
               pBufferParams->stride     = stateBufferSize / numChannelsPadded;
            }

            return status;
         }

         AUDIOLIB_STATUS
         AUDIOLIB_ssrc_init_checkParams(AUDIOLIB_kernelHandle handle, const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                        const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                        const AUDIOLIB_ssrc_InitArgs *pKerInitArgs)
         {
            AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
            int32_t         outFrameLength;

#if AUDIOLIB_DEBUGPRINT
            printf("Enter AUDIOLIB_ssrc_init_checkParams\n");
#endif

            // Check for NULL pointers
            if (handle == nullptr || bufParamsIn == nullptr || bufParamsOut == nullptr || pKerInitArgs == nullptr) {
               status = AUDIOLIB_ERR_NULL_POINTER;
            }

            if (status == AUDIOLIB_SUCCESS) {
               // Validate dataFormat
               if (pKerInitArgs->dataFormat != AUDIOLIB_DATA_FORMAT_INTERLEAVED &&
                   pKerInitArgs->dataFormat != AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED) {
                  status = AUDIOLIB_ERR_INVALID_VALUE;
               }
            }

#ifdef C7X
            if (status == AUDIOLIB_SUCCESS) {
               // Check for required handles based on data format
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  // For interleaved format with linear buffer, check deinterleave and interleave handles
                  if (pKerInitArgs->bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
                     if (pKerInitArgs->deinterleaveHandle == NULL || pKerInitArgs->interleaveHandle == NULL ||
                         pKerInitArgs->blkCopy2DHandle1 == NULL || pKerInitArgs->firHandle1 == NULL ||
                         pKerInitArgs->blkCopy2DHandle2 == NULL || pKerInitArgs->firHandle2 == NULL) {
                        status = AUDIOLIB_ERR_NULL_POINTER;
                     }
                  }
               }
               else {
                  if (pKerInitArgs->blkCopy2DHandle1 == NULL || pKerInitArgs->blkCopy2DHandle2 == NULL ||
                      pKerInitArgs->blkCopy2DHandle3 == NULL || pKerInitArgs->firHandle1 == NULL ||
                      pKerInitArgs->firHandle2 == NULL) {
                     status = AUDIOLIB_ERR_NULL_POINTER;
                  }
               }
            }
#endif

            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED) {
                  if (pKerInitArgs->bufferFormat != AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
                     status = AUDIOLIB_ERR_INVALID_VALUE;
                  }
               }
            }

#ifdef C7X
            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) {
                  if (pKerInitArgs->enableMMA == 1) {
                     status = AUDIOLIB_ERR_INVALID_VALUE;
                  }
               }
            }

            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) {
                  if (pKerInitArgs->enableMMA == 1 && pKerInitArgs->mmaSize != 8) {
                     status = AUDIOLIB_ERR_INVALID_VALUE;
                  }
               }
            }
#endif

            // Validate kernel initialization parameters
            if (status == AUDIOLIB_SUCCESS) {
               // Validate funcStyle
               if (pKerInitArgs->funcStyle < AUDIOLIB_FUNCTION_NATC ||
                   pKerInitArgs->funcStyle > AUDIOLIB_FUNCTION_OPTIMIZED) {
                  status = AUDIOLIB_ERR_INVALID_VALUE;
               }
            }

            if (status == AUDIOLIB_SUCCESS) {
               // Validate inputSampleCount (must be non-zero)
               if (pKerInitArgs->inputSampleCount == 0) {
                  status = AUDIOLIB_ERR_INVALID_VALUE;
               }
            }

            if (status == AUDIOLIB_SUCCESS) {
               // Validate sampleDataType (only FLOAT32 supported)
               if (pKerInitArgs->sampleDataType != AUDIOLIB_FLOAT32) {
                  status = AUDIOLIB_ERR_INVALID_TYPE;
               }
            }

            if (status == AUDIOLIB_SUCCESS) {
               // Validate sample rates
               if (pKerInitArgs->inputSampleRate < SSRC_SAMPLE_RATE_8000 ||
                   pKerInitArgs->outputSampleRate > SSRC_SAMPLE_RATE_192000) {
                  status = AUDIOLIB_ERR_INVALID_VALUE;
               }
            }

#ifdef C7X
            if (status == AUDIOLIB_SUCCESS) {
               // Validate enableMMA
               if (pKerInitArgs->enableMMA != 0 && pKerInitArgs->enableMMA != 1) {
                  status = AUDIOLIB_ERR_INVALID_VALUE;
               }
            }
#endif

            if (status == AUDIOLIB_SUCCESS) {
               // Check if the ratio between output and input sample rates is valid (only 2 or 4 allowed)
               status = AUDIOLIB_ssrc_checkValidRatio(pKerInitArgs->inputSampleRate, pKerInitArgs->outputSampleRate);
            }

            if (status == AUDIOLIB_SUCCESS) {
               // Validate numChannels
               if (pKerInitArgs->numChannels == 0 || pKerInitArgs->numChannels > AUDIOLIB_SSRC_MAX_NUM_CHANNELS) {
                  status = AUDIOLIB_ERR_INVALID_VALUE;
               }
            }

            // For downsampling, check if inputSampleCount is a multiple of the sample rate ratio
            if (status == AUDIOLIB_SUCCESS) {
               // Check if this is a downsampling operation
               bool isUpsampling =
                   AUDIOLIB_ssrc_isUpsampling(pKerInitArgs->inputSampleRate, pKerInitArgs->outputSampleRate);
               if (!isUpsampling) {
                  // Get the sample rate ratio for downsampling
                  float sampleRateRatio =
                      AUDIOLIB_ssrc_getSampleRateRatio(pKerInitArgs->inputSampleRate, pKerInitArgs->outputSampleRate);
                  uint32_t sampleRateRatioInt = (uint32_t) sampleRateRatio;

                  // Check if inputSampleCount is a multiple of the sample rate ratio
                  if ((pKerInitArgs->inputSampleCount % sampleRateRatioInt) != 0) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
            }

            // Calculate output frame length for dimension validation
            if (status == AUDIOLIB_SUCCESS) {
               outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(
                   pKerInitArgs->inputSampleRate, pKerInitArgs->outputSampleRate, pKerInitArgs->inputSampleCount);
            }

            // Validate buffer parameters
            if (status == AUDIOLIB_SUCCESS) {
               // Input buffer must be FLOAT32
               if (bufParamsIn->data_type != AUDIOLIB_FLOAT32) {
                  status = AUDIOLIB_ERR_INVALID_TYPE;
               }
            }

            if (status == AUDIOLIB_SUCCESS) {
               // Input and output buffers must have the same data type
               if (bufParamsIn->data_type != bufParamsOut->data_type) {
                  status = AUDIOLIB_ERR_INVALID_TYPE;
               }
            }

            // Validate buffer dimensions based on data format
            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  // For interleaved format:
                  // - dim_y = number of samples per channel (time dimension)
                  // - dim_x = number of channels (channel dimension)
                  if (bufParamsIn->dim_y != pKerInitArgs->inputSampleCount) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
               else {
                  // For non-interleaved format:
                  // - dim_y = number of channels
                  // - dim_x = number of samples per channel
                  if (bufParamsIn->dim_y != pKerInitArgs->numChannels) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
            }

            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  if (bufParamsIn->dim_x != pKerInitArgs->numChannels) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
               else {
                  if (bufParamsIn->dim_x != pKerInitArgs->inputSampleCount) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
            }

            // Validate stride values
            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  // For interleaved format, stride_y = bytes to move to next sample in time (all channels)
                  if (bufParamsIn->stride_y !=
                      (int32_t) (pKerInitArgs->numChannels * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType))) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
               else {
                  // For non-interleaved format, stride_y = bytes per row (one channel's buffer)
                  if (bufParamsIn->stride_y !=
                      (int32_t) (pKerInitArgs->inputSampleCount * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType))) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
            }

            // Validate output buffer dimensions
            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  if (bufParamsOut->dim_x != pKerInitArgs->numChannels) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
               else {
                  if (bufParamsOut->dim_x != (uint32_t) outFrameLength) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
            }

            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  if (bufParamsOut->dim_y != (uint32_t) outFrameLength) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
               else {
                  if (bufParamsOut->dim_y != pKerInitArgs->numChannels) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
            }

            // Validate output buffer stride
            if (status == AUDIOLIB_SUCCESS) {
               if (pKerInitArgs->dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  if (bufParamsOut->stride_y !=
                      pKerInitArgs->numChannels * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType)) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
               else {
                  if (bufParamsOut->stride_y != outFrameLength * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType)) {
                     status = AUDIOLIB_ERR_INVALID_DIMENSION;
                  }
               }
            }

            return status;
         }

         AUDIOLIB_STATUS
         AUDIOLIB_ssrc_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn,
                                        const void *restrict pState, const void *restrict pFiltCoeffs,
                                        const void *restrict pOut)
         {
            AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
            AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;

#if AUDIOLIB_DEBUGPRINT
            printf("Enter AUDIOLIB_ssrc_exec_checkParams\n");
#endif
            if ((handle == nullptr) || (pIn == nullptr) || (pOut == nullptr) || (pFiltCoeffs == nullptr) ||
                ((pKerPrivArgs->initArgs.bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) && (pState == nullptr))) {
               status = AUDIOLIB_ERR_NULL_POINTER;
            }

            return status;
         }

         AUDIOLIB_STATUS AUDIOLIB_ssrc_init(AUDIOLIB_kernelHandle handle, AUDIOLIB_bufParams2D_t * bufParamsIn,
                                            AUDIOLIB_bufParams2D_t * bufParamsOut,
                                            AUDIOLIB_ssrc_InitArgs * pKerInitArgs)
         {
            AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
            AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;
            bool                    isUpsampling;
            uint32_t                sampleRateRatio;

            // Create local variables for pKerInitArgs parameters
            ssrc_sample_rate_t inputSampleRate  = pKerInitArgs->inputSampleRate;
            ssrc_sample_rate_t outputSampleRate = pKerInitArgs->outputSampleRate;
            uint32_t           inputSampleCount = pKerInitArgs->inputSampleCount;
            uint8_t            numChannels      = pKerInitArgs->numChannels;
            uint32_t           sampleDataType   = pKerInitArgs->sampleDataType;
            uint8_t            dataFormat       = pKerInitArgs->dataFormat;
            uint8_t            bufferFormat     = pKerInitArgs->bufferFormat;
            uint8_t            funcStyle        = pKerInitArgs->funcStyle;
            uint32_t historyLengthStage1 = AUDIOLIB_ssrc_getSampleHistoryLength(inputSampleRate, outputSampleRate, 1U);
            uint32_t historyLengthStage2 = AUDIOLIB_ssrc_getSampleHistoryLength(inputSampleRate, outputSampleRate, 2U);
            // Get filter lengths for stage 1 and stage 2
            uint32_t filterLengthStage1 = AUDIOLIB_ssrc_getFilterLength(inputSampleRate, outputSampleRate, 1U);
#ifdef C7X
            uint8_t  enableMMA = pKerInitArgs->enableMMA;
            uint32_t mmaSize   = pKerInitArgs->mmaSize;
#endif
            int32_t  eleSize                     = AUDIOLIB_sizeof(sampleDataType);
            int32_t  alignedEleCount             = AUDIOLIB_L2DATA_ALIGNMENT / eleSize;
            uint32_t stage1DataBufferEleCount    = 0;
            uint32_t stage2DataBufferEleCount    = 0;
            uint32_t stage1ScratchBufferEleCount = 0;

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT Enter AUDIOLIB_ssrc_init\n");
#endif

            pKerPrivArgs->initArgs = *pKerInitArgs;

            pKerPrivArgs->stateSize = 0U;

            isUpsampling               = AUDIOLIB_ssrc_isUpsampling(inputSampleRate, outputSampleRate);
            float sampleRateRatioFloat = AUDIOLIB_ssrc_getSampleRateRatio(inputSampleRate, outputSampleRate);
            sampleRateRatio            = (uint32_t) sampleRateRatioFloat;

            // Store isUpsampling and sampleRateRatio in private args
            pKerPrivArgs->isUpsampling    = isUpsampling;
            pKerPrivArgs->sampleRateRatio = sampleRateRatio;

            // Assign filter coefficients based on sample rate ratio and whether upsampling or downsampling
            if (isUpsampling) {
               // Calculate and set outputSampleCount based on the sample rate ratio and input sample count
               // For upsampling, output samples = input samples * ratio
               pKerPrivArgs->outputSampleCount = (int32_t) ceil(inputSampleCount * sampleRateRatio);
            }
            else {
               // Calculate and set outputSampleCount based on the sample rate ratio and input sample count
               // For downsampling, output samples = input samples / ratio
               pKerPrivArgs->outputSampleCount = (int32_t) (inputSampleCount / sampleRateRatio);
            }

            pKerPrivArgs->bufParamsState.data_type = sampleDataType;
            pKerPrivArgs->bufParamsState.dim_x     = 0;
            pKerPrivArgs->bufParamsState.dim_y     = 0;
            pKerPrivArgs->bufParamsState.stride_y  = 0;

            pKerPrivArgs->stage1FiltCoeffsOffset = 0;
            pKerPrivArgs->stage2FiltCoeffsOffset =
                AUDIOLIB_ceilingDiv(filterLengthStage1, alignedEleCount) * alignedEleCount;

            if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) {
               AUDIOLIB_ssrc_bufferParams_t cbParams;
               status = AUDIOLIB_ssrc_getCircularBufferParams(inputSampleRate, outputSampleRate, inputSampleCount,
                                                              numChannels, sampleDataType, &cbParams);

               pKerPrivArgs->inBufferTotalDimX = cbParams.alignment / eleSize;
               pKerPrivArgs->stage1BuffStartIndex =
                   (int32_t) pKerPrivArgs->inBufferTotalDimX - (int32_t) historyLengthStage1 * bufParamsIn->dim_x;
               pKerPrivArgs->stage2BuffStartIndex =
                   (int32_t) pKerPrivArgs->inBufferTotalDimX - (int32_t) historyLengthStage2 * bufParamsIn->dim_x;

               pKerPrivArgs->cirBuffAddressMask     = pKerPrivArgs->inBufferTotalDimX - 1;
               pKerPrivArgs->inBufferTotalStrideY   = cbParams.alignment;
               pKerPrivArgs->scratch1BuffStartIndex = 0;
               pKerPrivArgs->scratch2BuffStartIndex = 0;
               pKerPrivArgs->stage1BuffCurrIndex    = 0;
               pKerPrivArgs->stage2BuffCurrIndex    = 0;
            }
            else { /* bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR */
               AUDIOLIB_ssrc_bufferParams_t lbParams;
               status = AUDIOLIB_ssrc_getStateBufferParams(inputSampleRate, outputSampleRate, inputSampleCount,
                                                           numChannels, sampleDataType, dataFormat, &lbParams);
               pKerPrivArgs->stateSize            = lbParams.bufferSize;
               pKerPrivArgs->inBufferTotalDimX    = lbParams.alignment / eleSize;
               pKerPrivArgs->inBufferTotalStrideY = lbParams.stride;
               pKerPrivArgs->stage1BuffCurrIndex  = historyLengthStage1;
               stage1DataBufferEleCount =
                   AUDIOLIB_ceilingDiv(historyLengthStage1 + inputSampleCount, alignedEleCount) * alignedEleCount;
               pKerPrivArgs->stage2BuffCurrIndex  = stage1DataBufferEleCount + historyLengthStage2;
               pKerPrivArgs->stage1BuffStartIndex = 0;
               pKerPrivArgs->stage2BuffStartIndex = stage1DataBufferEleCount;

               pKerPrivArgs->bufParamsState.dim_x    = inputSampleCount;
               pKerPrivArgs->bufParamsState.dim_y    = numChannels;
               pKerPrivArgs->bufParamsState.stride_y = lbParams.stride;

               if (sampleRateRatio == 2.0f) {
                  if (isUpsampling) {
                     stage1ScratchBufferEleCount =
                         AUDIOLIB_ceilingDiv(inputSampleCount, alignedEleCount) * alignedEleCount;
                  }
                  else {
                     stage1ScratchBufferEleCount =
                         AUDIOLIB_ceilingDiv((historyLengthStage1 + inputSampleCount) / 2, alignedEleCount) *
                         alignedEleCount;
                  }
                  pKerPrivArgs->scratch1BuffStartIndex = stage1DataBufferEleCount;
                  pKerPrivArgs->scratch2BuffStartIndex =
                      pKerPrivArgs->scratch1BuffStartIndex + stage1ScratchBufferEleCount;
               }
               else {
                  if (isUpsampling) {
                     stage2DataBufferEleCount =
                         AUDIOLIB_ceilingDiv(historyLengthStage2 + 2 * inputSampleCount, alignedEleCount) *
                         alignedEleCount;
                     pKerPrivArgs->scratch1BuffStartIndex = stage1DataBufferEleCount + stage2DataBufferEleCount;
                     stage1ScratchBufferEleCount =
                         AUDIOLIB_ceilingDiv(2 * inputSampleCount, alignedEleCount) * alignedEleCount;
                     pKerPrivArgs->scratch2BuffStartIndex =
                         pKerPrivArgs->scratch1BuffStartIndex + stage1ScratchBufferEleCount;
                  }
                  else {
                     stage2DataBufferEleCount =
                         AUDIOLIB_ceilingDiv(historyLengthStage2 + inputSampleCount / 2, alignedEleCount) *
                         alignedEleCount;
                     pKerPrivArgs->scratch1BuffStartIndex = stage1DataBufferEleCount + stage2DataBufferEleCount;
                     stage1ScratchBufferEleCount =
                         AUDIOLIB_ceilingDiv((historyLengthStage2 + inputSampleCount) / 2, alignedEleCount) *
                         alignedEleCount;
                     pKerPrivArgs->scratch2BuffStartIndex =
                         pKerPrivArgs->scratch1BuffStartIndex + stage1ScratchBufferEleCount;
                  }
               }

#ifdef C7X
               if (enableMMA) {
                  pKerPrivArgs->stage2FiltCoeffsOffset += 3 * mmaSize - 1;
               }
#endif
            }

            if (status == AUDIOLIB_SUCCESS) {
               if (funcStyle == AUDIOLIB_FUNCTION_NATC) {
                  status = AUDIOLIB_ssrc_init_cn(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
                  if (status == AUDIOLIB_SUCCESS) {
                     if (isUpsampling) {
                        // Set the appropriate function pointer based on the upsampling ratio
                        if (sampleRateRatio == 4) {
                           pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample4x_exec_cn<float>;
                        }
                        else {
                           pKerPrivArgs->execute = AUDIOLIB_ssrc_upsample2x_exec_cn<float>;
                        }
                     }
                     else {
                        // Set the appropriate function pointer based on the downsampling ratio
                        if (sampleRateRatio == 4) {
                           pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample4x_exec_cn<float>;
                        }
                        else {
                           pKerPrivArgs->execute = AUDIOLIB_ssrc_downsample2x_exec_cn<float>;
                        }
                     }
                  }
               }
               else {
                  // For optimized implementation, call the appropriate specialized init function directly
                  if (isUpsampling) {
                     if (sampleRateRatio == 4) {
                        status =
                            AUDIOLIB_ssrc_upsample4x_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
                     }
                     else {
                        // For 2x upsampling, call the main initialization function
                        status =
                            AUDIOLIB_ssrc_upsample2x_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
                     }
                  }
                  else {
                     if (sampleRateRatio == 4) {
                        status =
                            AUDIOLIB_ssrc_downsample4x_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
                     }
                     else {
                        status =
                            AUDIOLIB_ssrc_downsample2x_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
                     }
                  }
               }
            }

            return status;
         }

         AUDIOLIB_STATUS
         AUDIOLIB_ssrc_set(AUDIOLIB_kernelHandle handle, uint8_t mode, void *restrict pIn, void *restrict pState)
         {
            AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
            AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;
            ssrc_sample_rate_t      inputSampleRate;
            ssrc_sample_rate_t      outputSampleRate;
            uint32_t                numChannels;
            uint32_t                inBufferTotalDimX;
            uint32_t                sampleRateRatio;
            uint32_t                inputSampleCount;
            uint8_t                 bufferFormat;
            int32_t                 eleSize;
            uint32_t                historyLengthStage1;
            uint32_t                historyLengthStage2;
            int32_t                 historyStartOffset;
            int32_t                 i;

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT Enter AUDIOLIB_ssrc_set\n");
#endif

            if (handle == nullptr) {
               status = AUDIOLIB_ERR_NULL_POINTER;
            }
            else {
               inputSampleRate          = pKerPrivArgs->initArgs.inputSampleRate;
               outputSampleRate         = pKerPrivArgs->initArgs.outputSampleRate;
               numChannels              = pKerPrivArgs->initArgs.numChannels;
               inBufferTotalDimX        = pKerPrivArgs->inBufferTotalDimX;
               sampleRateRatio          = pKerPrivArgs->sampleRateRatio;
               inputSampleCount         = pKerPrivArgs->initArgs.inputSampleCount;
               bufferFormat             = pKerPrivArgs->initArgs.bufferFormat;
               eleSize                  = AUDIOLIB_sizeof(pKerPrivArgs->initArgs.sampleDataType);
               int32_t  alignedEleCount = AUDIOLIB_L2DATA_ALIGNMENT / eleSize;
               uint32_t stage1DataBufferEleCount;
               historyLengthStage1 = AUDIOLIB_ssrc_getSampleHistoryLength(inputSampleRate, outputSampleRate, 1U);
               historyLengthStage2 = AUDIOLIB_ssrc_getSampleHistoryLength(inputSampleRate, outputSampleRate, 2U);
               if (mode == AUDIOLIB_SSRC_MODE_RESET) {
                  if (pIn != nullptr) {
                     if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) {
                        // For circular buffer format in the non interleaved mode, there is  either one or two circular
                        // buffers with interleaved channel data Calculate the starting position for the filter history
                        // in the circular buffer
                        historyStartOffset = inBufferTotalDimX - historyLengthStage1 * numChannels;
                        // In interleaved mode, we need to clear the filter history for all channels
                        float *pInLocal = (float *) pIn + historyStartOffset;
                        // Clear stage 1 filter history
                        memset((void *) pInLocal, 0, historyLengthStage1 * numChannels * eleSize);

                        // If 4x conversion, also clear stage 2 filter history
                        if (sampleRateRatio == 4) {
                           historyStartOffset = inBufferTotalDimX * 2 - historyLengthStage2 * numChannels;
                           pInLocal           = (float *) pIn + historyStartOffset;
                           memset((void *) pInLocal, 0, historyLengthStage2 * numChannels * eleSize);
                           historyStartOffset = inBufferTotalDimX;
                           pInLocal           = (float *) pIn + historyStartOffset;
                           memset((void *) pInLocal, 0, historyLengthStage2 * numChannels * eleSize);
                        }
                     }
                     else {
                        // For linear buffer format
                        if (pState != nullptr) {
                           // For linear buffer format, we use a separate state buffer
                           // Clear stage 1 filter history
                           float *pStateLocal = (float *) pState;
                           for (i = 0; i < (int32_t) numChannels; i++) {
                              memset((void *) pStateLocal, 0, historyLengthStage1 * eleSize);
                              pStateLocal += inBufferTotalDimX;
                           }

                           // If 4x conversion, also clear stage 2 filter history
                           stage1DataBufferEleCount =
                               AUDIOLIB_ceilingDiv(historyLengthStage1 + inputSampleCount, alignedEleCount) *
                               alignedEleCount;
                           if (sampleRateRatio == 4) {
                              pStateLocal = (float *) pState + stage1DataBufferEleCount;
                              for (i = 0; i < (int32_t) numChannels; i++) {
                                 memset((void *) pStateLocal, 0, historyLengthStage2 * eleSize);
                                 pStateLocal += inBufferTotalDimX;
                              }
                           }
#ifdef ARM_A53
                           // Clear the state buffer
                           memset(pState, 0, pKerPrivArgs->stateSize);
#endif
                        }
                        else {
                           status = AUDIOLIB_ERR_NULL_POINTER; /* pState must not be NULL for linear buffer format */
                        }
                     }
                  }
                  else {
                     status = AUDIOLIB_ERR_NULL_POINTER; /* pIn must not be NULL for reset */
                  }
               }
               else {
                  status = AUDIOLIB_ERR_INVALID_VALUE;
               }
            }

            return status;
         }

         /** call exec_check before this */
         AUDIOLIB_STATUS
         AUDIOLIB_ssrc_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pState,
                            void *restrict pFiltCoeffs, void *restrict pOut)
         {
            AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
            AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs = (AUDIOLIB_ssrc_PrivArgs *) handle;

            status = pKerPrivArgs->execute(handle, pIn, pState, pFiltCoeffs, pOut);

            return status;
         }

         void AUDIOLIB_ssrc_perfEst(AUDIOLIB_kernelHandle handle, const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                    const AUDIOLIB_bufParams2D_t *bufParamsOut, uint64_t *archCycles,
                                    uint64_t *estCycles)
         {
            AUDIOLIB_ssrc_PrivArgs *pKerPrivArgs     = (AUDIOLIB_ssrc_PrivArgs *) handle;
            uint32_t                numChannels      = pKerPrivArgs->initArgs.numChannels;
            uint32_t                inputSampleCount = pKerPrivArgs->initArgs.inputSampleCount;
            uint32_t                sampleRateRatio  = pKerPrivArgs->sampleRateRatio;
            bool                    isUpsampling     = pKerPrivArgs->isUpsampling;
            uint32_t                outputSamples    = pKerPrivArgs->outputSampleCount;
            uint8_t                 bufferFormat     = pKerPrivArgs->initArgs.bufferFormat;
            uint32_t                sampleDataType   = pKerPrivArgs->initArgs.sampleDataType;
#ifdef C7X
            int32_t eleCount = __C7X_VEC_SIZE_BYTES__ / AUDIOLIB_sizeof(sampleDataType);
#endif
#ifdef ARM_A53
            int32_t eleCount = (__ARM_A53_VEC_SIZE_BYTES__) / AUDIOLIB_sizeof(sampleDataType);
#endif
            uint64_t computeCycles = 0;

            *archCycles = 1;
            *estCycles  = 1;

            /*
             * Block processing or single sample processing thresholds
             * Based on AUDIOLIB_ssrc_priv.h definitions
             */
            uint32_t switchThreshold = isUpsampling ? AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT
                                                    : AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT;

            /*
             * Determine execution mode based on buffer format and sample count
             * This matches the logic in the init_ci functions
             */
            bool useBlockExec = (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) &&
                                (inputSampleCount > switchThreshold);

            bool useLinearExec = (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR);

            if (useLinearExec) {
               *archCycles = 1;
               *estCycles  = 1;
            }
            else if (useBlockExec) {
               /*
                * Block execution mode performance estimation
                * Based on assembly analysis of block execution functions
                */

               if (isUpsampling) {
                  if (sampleRateRatio == 2) {
                     /*
                      * Upsample 2x block processing
                      * Assembly: "11 + min_trip_cnt * 8 = 203" (24 iterations per block)
                      */
                     const uint64_t blockSetupOverhead = 29 + 2 + 3 + 3 + 9;
                     const uint64_t prolog             = 9;
                     const uint64_t epilogAndProlog    = 22;
                     uint32_t       numChannelBlocks   = AUDIOLIB_ceilingDiv(numChannels, eleCount);
                     uint32_t       numOutputBlocks =
                         AUDIOLIB_ceilingDiv(inputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

                     /*
                      * Core FIR filtering cycles
                      * From assembly: 8 cycles per iteration, 24 iterations (filter taps)
                      */
                     const uint64_t firCyclesPerOutput = 4;

                     /* Per-channel FIR processing */
                     uint64_t firTotalCycles =
                         numChannelBlocks * numOutputBlocks *
                         (firCyclesPerOutput * AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS + epilogAndProlog);

                     computeCycles = blockSetupOverhead + prolog + firTotalCycles;
                  }
                  else {
                     /*
                      * Upsample 4x block processing (two-stage)
                      * Based on assembly analysis of AUDIOLIB_ssrc_upsample4x_block_exec_ci
                      */

                     /* Setup overhead for both stages */
                     const uint64_t blockSetupOverhead = 56;
                     const uint64_t stage1Prolog       = 9;
                     const uint64_t stage1EpilogProlog = 22;
                     const uint64_t stage2Prolog       = 7;
                     const uint64_t stage2EpilogProlog = 14;
                     const uint64_t interStageOverhead = 36;
                     const uint64_t finalStageOverhead = 24;

                     uint32_t numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
                     uint32_t numOutputBlocks1 =
                         AUDIOLIB_ceilingDiv(inputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);
                     uint32_t stage2InputSampleCount = inputSampleCount * 2;
                     uint32_t numOutputBlocks2 =
                         AUDIOLIB_ceilingDiv(stage2InputSampleCount, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

                     /*
                      * Stage 1: First 2x upsampling
                      * Assembly: "11 + min_trip_cnt * 8 = 203" (24 iterations, 96 filter taps)
                      */
                     const uint64_t stage1FirCyclesPerOutput = 4;
                     uint64_t       stage1Cycles =
                         numChannelBlocks * numOutputBlocks1 *
                         (stage1FirCyclesPerOutput * AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS + stage1EpilogProlog);

                     /*
                      * Stage 2: Second 2x upsampling
                      * Assembly: "11 + min_trip_cnt * 4 = 51" (10 iterations, 20 filter taps)
                      * Note: Stage 2 uses shorter filter and has ii=4 instead of ii=8
                      */
                     const uint64_t stage2FirCyclesPerOutput = 2;
                     uint64_t       stage2Cycles =
                         numChannelBlocks * numOutputBlocks2 *
                         (stage2FirCyclesPerOutput * AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS + stage2EpilogProlog);

                     /*
                      * Stage 3: Final output copy
                      * Assembly: "1 + trip_cnt * 1"
                      */
                     uint64_t stage3Cycles = 1 + outputSamples * numChannels;

                     computeCycles = blockSetupOverhead + stage1Prolog + stage1Cycles + interStageOverhead +
                                     stage2Prolog + stage2Cycles + finalStageOverhead + stage3Cycles;
                  }
               }
               else {
                  /* Downsampling block processing */
                  if (sampleRateRatio == 2) {
                     /*
                      * Downsample 2x block processing (two-phase anti-aliasing)
                      * Based on assembly analysis of AUDIOLIB_ssrc_downsample2x_block_exec_ci
                      */

                     /* Setup overhead for phase 1 and phase 2 */
                     const uint64_t blockSetupOverhead = 49;
                     const uint64_t phase1Prolog       = 9;
                     const uint64_t phase1EpilogProlog = 16;
                     const uint64_t interPhaseOverhead = 28;
                     const uint64_t phase2Overhead     = 7;

                     uint32_t numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);
                     uint32_t numOutputBlocks1 =
                         AUDIOLIB_ceilingDiv(outputSamples, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

                     /*
                      * Phase 1: Anti-aliasing FIR filter with decimation
                      * Assembly: "11 + min_trip_cnt * 8 = 203" (24 iterations for filter taps)
                      * Processes decimated input through symmetric FIR filter
                      */
                     const uint64_t phase1FirCyclesPerOutput = 4;
                     uint64_t       phase1Cycles             = numChannelBlocks * numOutputBlocks1 *
                                             (phase1FirCyclesPerOutput * AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS +
                                              phase1EpilogProlog);

                     /*
                      * Phase 2: Final downsampling with center tap addition
                      * Assembly: "7 + trip_cnt * 1" for combining FIR output with phase 2 samples
                      * Adds scaled center tap contribution (0.5x) to complete anti-aliasing
                      */
                     uint64_t phase2Cycles = numChannelBlocks * outputSamples * 1;

                     computeCycles = blockSetupOverhead + phase1Prolog + phase1Cycles + interPhaseOverhead +
                                     phase2Overhead + phase2Cycles;
                  }
                  else {
                     /*
                      * Downsample 4x block processing (two-stage with anti-aliasing)
                      * Based on assembly analysis of AUDIOLIB_ssrc_downsample4x_block_exec_ci
                      */

                     /* Setup overhead for both stages */
                     const uint64_t blockSetupOverhead = 58;
                     const uint64_t stage1Prolog       = 9;
                     const uint64_t stage1EpilogProlog = 16;
                     const uint64_t interStageOverhead = 42;
                     const uint64_t stage2Prolog       = 9;
                     const uint64_t stage2EpilogProlog = 16;
                     const uint64_t finalPhaseOverhead = 7;

                     uint32_t numChannelBlocks    = AUDIOLIB_ceilingDiv(numChannels, eleCount);
                     uint32_t stage1OutputSamples = inputSampleCount / 2;
                     uint32_t numOutputBlocks1 =
                         AUDIOLIB_ceilingDiv(stage1OutputSamples, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);
                     uint32_t numOutputBlocks2 =
                         AUDIOLIB_ceilingDiv(outputSamples, AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK);

                     /*
                      * Stage 1: First 2x downsampling with anti-aliasing
                      * Assembly: "11 + min_trip_cnt * 8 = 203" (24 iterations for filter taps)
                      */
                     const uint64_t stage1FirCyclesPerOutput = 4;
                     uint64_t       stage1Cycles             = numChannelBlocks * numOutputBlocks1 *
                                             (stage1FirCyclesPerOutput * AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS +
                                              stage1EpilogProlog);

                     /*
                      * Stage 2: Second 2x downsampling with anti-aliasing
                      * Assembly: "11 + min_trip_cnt * 8 = 203" (24 iterations for filter taps)
                      */
                     const uint64_t stage2FirCyclesPerOutput = 4;
                     uint64_t       stage2Cycles             = numChannelBlocks * numOutputBlocks2 *
                                             (stage2FirCyclesPerOutput * AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS +
                                              stage2EpilogProlog);

                     /*
                      * Phase 2 completion for both stages
                      * Combines filtered output with center tap contributions
                      */
                     uint64_t phase2TotalCycles = numChannelBlocks * (stage1OutputSamples + outputSamples) * 1;

                     computeCycles = blockSetupOverhead + stage1Prolog + stage1Cycles + interStageOverhead +
                                     stage2Prolog + stage2Cycles + finalPhaseOverhead + phase2TotalCycles;
                  }
               }
            }
            else {
               /*
                * Single sample execution mode performance estimation
                * Based on assembly analysis of single sample execution functions
                */

               if (isUpsampling) {
                  if (sampleRateRatio == 2) {
                     /*
                      * Upsample 2x single sample processing
                      * From assembly analysis: AUDIOLIB_ssrc_upsample2x_ss_exec_ci
                      */

                     /* Initial setup overhead before main processing loop */
                     const uint64_t ssSetupOverhead = 28;

                     /* Number of channel blocks based on vector element count */
                     uint32_t numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);

                     /*
                      * Per output sample FIR loop cycles
                      * Assembly: "13 + min_trip_cnt * 4 = 61"
                      * - 13 cycles prolog/epilog per FIR computation
                      * - 4 cycles per iteration
                      * - 12 iterations (AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS / AUDIOLIB_SSRC_SS_UNROLL_FACTOR)
                      */
                     const uint64_t firProlog             = 13;
                     const uint64_t firCyclesPerIteration = 4;
                     const uint64_t firIterations =
                         AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS / AUDIOLIB_SSRC_SS_UNROLL_FACTOR;
                     uint64_t firCyclesPerOutput = firProlog + (firCyclesPerIteration * firIterations);

                     /*
                      * Epilog processing per output sample
                      * Assembly shows: 21 cycles for combining accumulators and writing outputs
                      */
                     const uint64_t outputEpilog = 21;

                     /* Per-channel-block per-output-sample processing */
                     uint64_t totalFirCycles =
                         numChannelBlocks * inputSampleCount * (firCyclesPerOutput + outputEpilog);

                     /*
                      * Final cleanup
                      * Assembly: 9 cycles for SE/SA close and circular buffer index update
                      */
                     const uint64_t finalCleanup = 9;

                     computeCycles = ssSetupOverhead + totalFirCycles + finalCleanup;
                  }
                  else {
                     /*
                      * Upsample 4x single sample processing (two-stage)
                      * Similar structure to 2x but with two cascaded stages
                      */

                     /* Setup overhead for both stages */
                     const uint64_t ssSetupOverhead = 40;

                     uint32_t numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);

                     /*
                      * Stage 1: First 2x upsampling
                      * Assembly: "13 + min_trip_cnt * 4 = 61" (12 iterations)
                      */
                     const uint64_t stage1FirProlog             = 13;
                     const uint64_t stage1FirCyclesPerIteration = 4;
                     const uint64_t stage1FirIterations =
                         AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS / AUDIOLIB_SSRC_SS_UNROLL_FACTOR;
                     uint64_t stage1FirCyclesPerOutput =
                         stage1FirProlog + (stage1FirCyclesPerIteration * stage1FirIterations);
                     const uint64_t stage1OutputEpilog = 21;

                     uint64_t stage1Cycles =
                         numChannelBlocks * inputSampleCount * (stage1FirCyclesPerOutput + stage1OutputEpilog);

                     /*
                      * Stage 2: Second 2x upsampling on intermediate buffer
                      * Shorter filter, similar pattern but fewer iterations
                      * AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS = 10
                      */
                     const uint64_t stage2FirProlog             = 11;
                     const uint64_t stage2FirCyclesPerIteration = 4;
                     const uint64_t stage2FirIterations =
                         AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS / AUDIOLIB_SSRC_SS_UNROLL_FACTOR;
                     uint64_t stage2FirCyclesPerOutput =
                         stage2FirProlog + (stage2FirCyclesPerIteration * stage2FirIterations);
                     const uint64_t stage2OutputEpilog = 18;

                     /* Stage 2 processes 2x more samples (output of stage 1) */
                     uint64_t stage2Cycles =
                         numChannelBlocks * (inputSampleCount * 2) * (stage2FirCyclesPerOutput + stage2OutputEpilog);

                     /*
                      * Inter-stage buffer management
                      * Assembly: "31 + trip_cnt * 10" for buffer copy between stages
                      */
                     uint64_t interStageOverhead = numChannelBlocks * (31 + inputSampleCount * 10);

                     /* Final cleanup for both stages */
                     const uint64_t finalCleanup = 15;

                     computeCycles = ssSetupOverhead + stage1Cycles + interStageOverhead + stage2Cycles + finalCleanup;
                  }
               }
               else {
                  /* Downsampling single sample processing */
                  if (sampleRateRatio == 2) {
                     /*
                      * Downsample 2x single sample processing (two-phase anti-aliasing)
                      * From assembly analysis: AUDIOLIB_ssrc_downsample2x_ss_exec_ci
                      */

                     /* Initial setup overhead for both phases */
                     const uint64_t ssSetupOverhead = 32;

                     uint32_t numChannelBlocks = AUDIOLIB_ceilingDiv(numChannels, eleCount);

                     /*
                      * Phase 1: FIR anti-aliasing filter on decimated input
                      * Assembly: "13 + min_trip_cnt * 4 = 61"
                      * - 13 cycles prolog/epilog per FIR computation
                      * - 4 cycles per iteration
                      * - 12 iterations (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS / AUDIOLIB_SSRC_SS_UNROLL_FACTOR)
                      */
                     const uint64_t firProlog             = 13;
                     const uint64_t firCyclesPerIteration = 4;
                     const uint64_t firIterations =
                         AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS / AUDIOLIB_SSRC_SS_UNROLL_FACTOR;
                     uint64_t firCyclesPerOutput = firProlog + (firCyclesPerIteration * firIterations);

                     /*
                      * Epilog processing per output sample (accumulator combining and write)
                      * Assembly shows: 19 cycles for combining accumulators and writing outputs
                      */
                     const uint64_t phase1Epilog = 19;

                     /* Phase 1 FIR processing for all output samples */
                     uint64_t phase1Cycles = numChannelBlocks * outputSamples * (firCyclesPerOutput + phase1Epilog);

                     /*
                      * Phase 2: Add decimated center tap contribution (0.5x scaling)
                      * Assembly: "1 + trip_cnt * 2" for reading FIR output, reading phase 2, combining and writing
                      */
                     const uint64_t phase2Setup  = 12;
                     uint64_t       phase2Cycles = phase2Setup + (numChannelBlocks * outputSamples *
                                                            2); // 2 cycles per sample for read+combine+write

                     /*
                      * Final cleanup
                      * Assembly: 11 cycles for SE/SA close and circular buffer index update
                      */
                     const uint64_t finalCleanup = 11;

                     computeCycles = ssSetupOverhead + phase1Cycles + phase2Cycles + finalCleanup;
                  }
                  else {
                     /*
                      * Downsample 4x single sample processing (two-stage with anti-aliasing)
                      * Two cascaded 2x downsampling stages, each with two-phase anti-aliasing
                      */

                     /* Setup overhead for both stages */
                     const uint64_t ssSetupOverhead = 48;

                     uint32_t numChannelBlocks    = AUDIOLIB_ceilingDiv(numChannels, eleCount);
                     uint32_t stage1OutputSamples = inputSampleCount / 2;

                     /*
                      * Stage 1: First 2x downsampling with anti-aliasing
                      * Phase 1 FIR: "13 + min_trip_cnt * 4 = 61" (12 iterations)
                      */
                     const uint64_t stage1FirProlog             = 13;
                     const uint64_t stage1FirCyclesPerIteration = 4;
                     const uint64_t stage1FirIterations =
                         AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS / AUDIOLIB_SSRC_SS_UNROLL_FACTOR;
                     uint64_t stage1FirCyclesPerOutput =
                         stage1FirProlog + (stage1FirCyclesPerIteration * stage1FirIterations);
                     const uint64_t stage1Phase1Epilog = 19;
                     uint64_t       stage1Phase1Cycles =
                         numChannelBlocks * stage1OutputSamples * (stage1FirCyclesPerOutput + stage1Phase1Epilog);

                     /* Stage 1 Phase 2 completion */
                     const uint64_t stage1Phase2Setup = 12;
                     uint64_t stage1Phase2Cycles = stage1Phase2Setup + (numChannelBlocks * stage1OutputSamples * 2);

                     /*
                      * Stage 2: Second 2x downsampling with anti-aliasing
                      * Phase 1 FIR: "13 + min_trip_cnt * 4 = 61" (12 iterations)
                      */
                     const uint64_t stage2FirProlog             = 13;
                     const uint64_t stage2FirCyclesPerIteration = 4;
                     const uint64_t stage2FirIterations =
                         AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS / AUDIOLIB_SSRC_SS_UNROLL_FACTOR;
                     uint64_t stage2FirCyclesPerOutput =
                         stage2FirProlog + (stage2FirCyclesPerIteration * stage2FirIterations);
                     const uint64_t stage2Phase1Epilog = 19;
                     uint64_t       stage2Phase1Cycles =
                         numChannelBlocks * outputSamples * (stage2FirCyclesPerOutput + stage2Phase1Epilog);

                     /* Stage 2 Phase 2 completion */
                     const uint64_t stage2Phase2Setup  = 12;
                     uint64_t       stage2Phase2Cycles = stage2Phase2Setup + (numChannelBlocks * outputSamples * 2);

                     /*
                      * Inter-stage buffer management
                      * Assembly: "28 + trip_cnt * 8" for buffer management between stages
                      */
                     uint64_t interStageOverhead = numChannelBlocks * (28 + stage1OutputSamples * 8);

                     /* Final cleanup for both stages */
                     const uint64_t finalCleanup = 17;

                     computeCycles = ssSetupOverhead + stage1Phase1Cycles + stage1Phase2Cycles + interStageOverhead +
                                     stage2Phase1Cycles + stage2Phase2Cycles + finalCleanup;
                  }
               }
            }

            *archCycles = computeCycles;
            *estCycles  = computeCycles;
         }
