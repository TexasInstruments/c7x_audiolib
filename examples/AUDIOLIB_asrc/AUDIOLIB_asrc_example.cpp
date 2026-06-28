// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <cmath>
#include <cstdlib>
#ifdef C7X
#include <dsplib.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#if defined(_HOST_BUILD) || defined(ARM_A53)
#include <malloc.h>
#endif
#ifdef ARM_A53
#include "../generated/ti_board_config.h"
#include "../generated/ti_board_open_close.h"
#include "../generated/ti_drivers_config.h"
#include "../generated/ti_drivers_open_close.h"
#include <cstring>
#endif

#ifdef C7X
#define AUDIOLIB_DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif
#ifdef ARM_A53
#define AUDIOLIB_DEBUG_PRINT(fmt, ...) DebugP_log(fmt, ##__VA_ARGS__)
#endif

#define AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64 4
#define MATRIX_TRANSPOSE_ROW_STRIDE(x, y) (((x + y - 1) / y) * y)

#ifdef C7X
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT))) const float audiolib_asrc_h44_1[] = {
#endif
#ifdef ARM_A53
    const float audiolib_asrc_h44_1[] = {
#endif
#include "../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_48kHz_rfmt.h"
    };

int main(void)
{
#ifdef ARM_A53
   int32_t systemstatus = SystemP_SUCCESS;
   /* System initialization */
   System_init();
   /* Board initialization */
   Board_init();
   /* Drivers initialization */
   Drivers_open();
   /* Open drivers */
   systemstatus = Board_driversOpen();
   DebugP_assert(systemstatus == SystemP_SUCCESS);
#endif
   // Parameters
   const uint32_t         fsin                   = 44100;
   const uint32_t         fsout                  = 48000;
   const uint8_t          numChannels            = 8;
   const uint32_t         maxSampleCountPerBlock = 16; // 16 for low latency, use 256 for best performance
   const uint32_t         frameModuloFactor      = 4;  // output sample count will be a multiple of this factor
   const uint32_t         sampleDataType         = AUDIOLIB_FLOAT32;
   const uint8_t          dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED; // Interleaved
   AUDIOLIB_asrc_InitArgs kerInitArgs            = {0};
#ifdef C7X
   DSPLIB_kernelHandle matTransHandle = NULL;
#endif

   // Buffer params
   AUDIOLIB_bufParams2D_t bufParamsIn = {sampleDataType, numChannels, maxSampleCountPerBlock,
                                         numChannels * sizeof(float)};
   uint32_t               outFrameLength =
       AUDIOLIB_asrc_getOutBufferLength(SAMPLE_RATE_44100, SAMPLE_RATE_48000, maxSampleCountPerBlock);
   AUDIOLIB_bufParams2D_t bufParamsOut = {sampleDataType, numChannels, outFrameLength, numChannels * sizeof(float)};

   // Allocate buffers
   int32_t eleCount = 0;
   if ((AUDIOLIB_sizeof(sampleDataType) == 4) || (AUDIOLIB_sizeof(sampleDataType) == 8)) {
#ifdef C7X
      eleCount = 2 * (__C7X_VEC_SIZE_BYTES__ / AUDIOLIB_sizeof(sampleDataType));
#endif
#ifdef ARM_A53
      eleCount = 2 * ((__ARM_A53_VEC_SIZE_BYTES__) / AUDIOLIB_sizeof(sampleDataType));
#endif
   }
   else {
#ifdef C7X
      eleCount = __C7X_VEC_SIZE_BYTES__ / AUDIOLIB_sizeof(sampleDataType);
#endif
#ifdef ARM_A53
      eleCount = (__ARM_A53_VEC_SIZE_BYTES__) / AUDIOLIB_sizeof(sampleDataType);
#endif
   }
   int32_t dim_y_padded = MATRIX_TRANSPOSE_ROW_STRIDE(maxSampleCountPerBlock, eleCount);

   uint32_t filterLength = AUDIOLIB_asrc_getFilterLength();
   uint32_t alignment_size;
   if (maxSampleCountPerBlock >= filterLength) {
      alignment_size =
          maxSampleCountPerBlock * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64 * AUDIOLIB_sizeof(sampleDataType);
   }
   else {
      alignment_size = filterLength * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64 * AUDIOLIB_sizeof(sampleDataType);
   }

   size_t nonInterleavedDataBufSize =
       AUDIOLIB_asrc_getNonInterleavedDataBufSize(numChannels, maxSampleCountPerBlock, sampleDataType, dataFormat);
   float *nonInterleavedData = (float *) memalign(alignment_size, nonInterleavedDataBufSize);
   float *in                 = (float *) memalign(AUDIOLIB_L2DATA_ALIGNMENT, bufParamsIn.stride_y * dim_y_padded);
   // Prepare input data (interleaved: each row is a sample, each column is a channel)
   float baseFreq = 996.0f; // Base frequency in Hz
   for (uint32_t s = 0; s < maxSampleCountPerBlock; ++s)
      for (uint32_t c = 0; c < numChannels; ++c)
         in[s * numChannels + c] = sinf(2.0f * 3.14159265f * baseFreq * s / fsin + c * 0.2f);

   float *out          = (float *) memalign(AUDIOLIB_L2DATA_ALIGNMENT, numChannels * outFrameLength * sizeof(float));
   float *filterRembuf = (float *) memalign(
       AUDIOLIB_L2DATA_ALIGNMENT, AUDIOLIB_asrc_getFilterRembufSize(numChannels, sampleDataType, frameModuloFactor));
   float *filtCoeffs = (float *) memalign(AUDIOLIB_L2DATA_ALIGNMENT, AUDIOLIB_asrc_getFilterCoeffSize(sampleDataType));
   memcpy(filtCoeffs, audiolib_asrc_h44_1, AUDIOLIB_asrc_getFilterCoeffSize(sampleDataType));

   int32_t               handleSize = AUDIOLIB_asrc_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);
#ifdef C7X
   int32_t matTransHandleSize = DSPLIB_matTrans_getHandleSize(NULL);
   matTransHandle             = malloc(matTransHandleSize);
#endif
   // Kernel handle
   kerInitArgs.funcStyle              = 1; // AUDIOLIB_FUNCTION_OPTIMIZED
   kerInitArgs.maxSampleCountPerBlock = maxSampleCountPerBlock;
   kerInitArgs.sampleDataType         = sampleDataType;
   kerInitArgs.inputSampleRate        = SAMPLE_RATE_44100;
   kerInitArgs.outputSampleRate       = SAMPLE_RATE_48000;
   kerInitArgs.numChannels            = numChannels;
   kerInitArgs.dataFormat             = dataFormat;
   kerInitArgs.frameModuloFactor      = frameModuloFactor;
#ifdef C7X
   kerInitArgs.matTransHandle = matTransHandle;
#endif

   // Init kernel
   AUDIOLIB_STATUS status = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   if (status != 0) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("init failed\r\n"));
   }
   // Reset state
   status = AUDIOLIB_asrc_set(handle, AUDIOLIB_ASRC_MODE_RESET, (double) fsout / fsin, nonInterleavedData, in);
   if (status != 0) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("reset failed\r\n"));
   }

   // Set ratio
   status = AUDIOLIB_asrc_set(handle, AUDIOLIB_ASRC_MODE_SET, (double) fsout / fsin, nonInterleavedData, in);
   if (status != 0) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("set failed\r\n"));
   }

   // Prepare exec args
   AUDIOLIB_asrc_ExecInArgs  execInArgs  = {maxSampleCountPerBlock};
   AUDIOLIB_asrc_ExecOutArgs execOutArgs = {0};

   // Execute
   status =
       AUDIOLIB_asrc_exec(handle, in, nonInterleavedData, filtCoeffs, filterRembuf, out, &execInArgs, &execOutArgs);
   if (status != 0) {
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("exec failed\r\n"));
   }

   // Print results
   AUDIOLIB_DEBUG_PRINT(const_cast<char *>("ASRC output (first channel):\r\n"));
   for (uint32_t i = 0; i < execOutArgs.outputSampleCount; ++i)
      AUDIOLIB_DEBUG_PRINT(const_cast<char *>("%10g\r\n"), out[i]);

   // Free
   free(out);
   free(filterRembuf);
   free(filtCoeffs);
   free(nonInterleavedData);
   free(handle);

#ifdef ARM_A53
   /* Close drivers */
   Board_driversClose();
   /* Close drivers */
   Drivers_close();
   /* Board Deinitialization */
   Board_deinit();
   /* System Deinitialization */
   System_deinit();
#endif
   return status == AUDIOLIB_SUCCESS ? 0 : 1;
}