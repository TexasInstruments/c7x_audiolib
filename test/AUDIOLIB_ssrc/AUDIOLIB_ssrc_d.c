// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#ifdef ARM_A53
#include <stdio.h>
#endif

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_ssrc_idat.h"

#ifdef ARM_A53
#include "../generated/ti_board_config.h"
#include "../generated/ti_board_open_close.h"
#include "../generated/ti_drivers_config.h"
#include "../generated/ti_drivers_open_close.h"
#endif

#ifdef C7X
#define AUDIOLIB_DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif
#ifdef ARM_A53
#define AUDIOLIB_DEBUG_PRINT(fmt, ...) DebugP_log(fmt, ##__VA_ARGS__)
#endif

#ifdef C7X
#ifdef _HOST_BUILD
#if defined(__C7504__) || defined(__C7524__)
int8_t l2auxBuffer[AUDIOLIB_L2_BUFFER_SIZE];
int8_t ddrBuffer[2048 * 1024];

const float audiolib_ssrc_downsampleStage1[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage1_downsample2x_rfmt.h"
};
const float audiolib_ssrc_downsampleStage2[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage2_downsample2x_rfmt.h"
};
const float audiolib_ssrc_upsampleStage1[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage1_upsample2x_rfmt.h"
};
const float audiolib_ssrc_upsampleStage2[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage2_upsample2x_rfmt.h"
};
#endif
#else
#if defined(__C7504__) || defined(__C7524__)
__attribute__((section(".l2sramaux"), aligned(64))) int8_t l2auxBuffer[AUDIOLIB_L2_BUFFER_SIZE];
__attribute__((section(".ddrData"), aligned(64))) int8_t   ddrBuffer[2048 * 1024];

__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_ssrc_downsampleStage1[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage1_downsample2x_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_ssrc_downsampleStage2[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage2_downsample2x_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_ssrc_upsampleStage1[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage1_upsample2x_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_ssrc_upsampleStage2[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage2_upsample2x_rfmt.h"
};
#else
__attribute__((section(".msmcData"), aligned(64))) int8_t msmcBuffer[AUDIOLIB_L3_RESULTS_BUFFER_SIZE];
__attribute__((section(".ddrData"), aligned(64))) int8_t  ddrBuffer[2048 * 1024];

#endif
#endif // WIN32
#endif

#ifdef ARM_A53
const float audiolib_ssrc_downsampleStage1[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage1_downsample2x_rfmt.h"
};

const float audiolib_ssrc_downsampleStage2[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage2_downsample2x_rfmt.h"
};

const float audiolib_ssrc_upsampleStage1[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage1_upsample2x_rfmt.h"
};
const float audiolib_ssrc_upsampleStage2[] = {
#include "../../src/AUDIOLIB_ssrc/filt_coeffs/halfband_filter_stage2_upsample2x_rfmt.h"
};

int8_t msmcBuffer[AUDIOLIB_L3_RESULTS_BUFFER_SIZE];
int8_t ddrBuffer[2048 * 1024];
#endif

/* Generate output file if defined! This is used for snr analysis*/
//#define ENABLE_GENERATE_OUTPUT_FILE

/* define used to allocate ping-pong buffers */
#define AUDIOLIB_SSRC_TEST_PING_PONG_FACTOR (2U)
/* Reducing the warm reps from the default AUDIOLIB_NUM_WARM_REPS */
#define AUDIOLIB_SSRC_NUM_WARM_REPS (10)

int16_t volatile volatileSum = 0; // use volatile to keep compiler from removing this operation

#ifdef ENABLE_GENERATE_OUTPUT_FILE
static void writeInOutDataToFile(float  *pOut,
                                 int32_t numChannels,
                                 int32_t sampleCount,
                                 int32_t testId,
                                 int32_t blockNum,
                                 int32_t isInterleaved,
                                 int32_t isInput)
{
   char  filename[256];
   FILE *fp;
   int   s, c;

   if (isInput) {
      sprintf(filename, "ssrc_testcase%d_input.csv", testId);
   }
   else {
      sprintf(filename, "ssrc_testcase%d_output.csv", testId);
   }

   if (blockNum == 0) {
      // First block - create new file
      fp = fopen(filename, "w");
   }
   else {
      // Append to existing file
      fp = fopen(filename, "a");
   }

   if (fp) {

      // Always write in interleaved format to the file
      for (s = 0; s < sampleCount; s++) {
         for (c = 0; c < numChannels; c++) {
            if (isInterleaved) {
               // For interleaved data: [ch0,ch1,ch0,ch1,...]
               fprintf(fp, "%.21g", pOut[s * numChannels + c]);
            }
            else {
               // For non-interleaved data: [ch0,ch0,...,ch1,ch1,...]
               fprintf(fp, "%.21g", pOut[c * sampleCount + s]);
            }
            // Print a comma only if it's not the last column
            if (c < numChannels - 1) {
               fprintf(fp, ", ");
            }
         }
         fprintf(fp, "\n");
      }

      fclose(fp);
   }
}
#endif

int AUDIOLIB_ssrc_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;
   int32_t                outFrameLength, filtCoeffSizeBytes;
   uint8_t                mode;
   float                 *pInData, *pOutData;
   float                 *pInDBuffer;
   int64_t                t_ssrc_cycles_opt, t_ssrc_cycles_warm, t_ssrc_cycles_warmwrb;
   int64_t                ssrc_cycles;
#ifdef C7X
   float processorUtilization;
#endif

   int32_t  block                = 0;
   int32_t  totalOutputSampleCnt = 0;
   uint32_t pInStartOffset;

   float fsin, fsout, sigFreq;

   uint32_t testNum;
   uint64_t archCycles = 0;
   uint64_t estCycles  = 0;
   uint32_t k;

   ssrc_testParams_t *prm;
   ssrc_testParams_t  currPrm;
   ssrc_getTestParams(&prm, &test_cases);

   AUDIOLIB_ssrc_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_ssrc_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   TI_profile_init("AUDIOLIB_ssrc");

   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_ssrc.csv", "w+");
   fprintf(
       fpOutputCSV,
       "Kernel, Test ID, Test type, Bit Width, Parameters, "
       "Input Data Format, Buffer Format, Enable MMA, Input Sample Count, Blocks, Input Signal Frequency (Hz), Input "
       "Sample Rate (Hz), Output Sample Rate (Hz), Num of Channels, Output Sample Count, Processor Utilization (MHz), "
       "Arch cycles, Estimated cycles, Estimated/Warm cycles, Pass/Fail, Cold Cycles, EVM cycles, Warm Cycles WRB\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      for (repCount = 0; repCount < numReps; repCount++) {
         int32_t                       status_ref_vs_opt             = TI_TEST_KERNEL_FAIL;
         int32_t                       status_ref_vs_nat             = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS               status_init                   = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS               status_opt                    = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS               status_nat                    = AUDIOLIB_SUCCESS;
         uint32_t                      inBufferCbSizeInterleaved     = 0;
         uint32_t                      inBufferCbElementsInterleaved = 0;
         AUDIOLIB_ssrc_buffer_format_t bufferFormat                  = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
         fsout   = (float) AUDIOLIB_ssrc_convertSampleRateToInt((ssrc_sample_rate_t) currPrm.outputSampleRate);
         fsin    = (float) AUDIOLIB_ssrc_convertSampleRateToInt((ssrc_sample_rate_t) currPrm.inputSampleRate);
         sigFreq = (float) currPrm.signalFrequency;
#ifdef C7X
         processorUtilization = 0.0;
#endif
         void *pIn = NULL, *pState __attribute__((unused)) = NULL;
         void *pFiltCoeffs;
         void *pOut, *pOutCn;
#ifdef C7X
         DSPLIB_kernelHandle deinterleaveHandle = NULL;
         DSPLIB_kernelHandle interleaveHandle   = NULL;
         DSPLIB_kernelHandle blkCopy2DHandle1   = NULL;
         DSPLIB_kernelHandle blkCopy2DHandle2   = NULL;
         DSPLIB_kernelHandle blkCopy2DHandle3   = NULL;
         DSPLIB_kernelHandle firHandle1         = NULL;
         DSPLIB_kernelHandle firHandle2         = NULL;
#endif
         int32_t ref_vs_opt_comparisonDone = 0;
         int32_t ref_vs_nat_comparisonDone = 0;
         t_ssrc_cycles_opt                 = 0;
         t_ssrc_cycles_warm                = 0;
         t_ssrc_cycles_warmwrb             = 0;
         currentTestFail                   = 0;

         kerInitArgs.funcStyle        = AUDIOLIB_FUNCTION_NATC;
         kerInitArgs.inputSampleCount = currPrm.inputSampleCount;
         kerInitArgs.sampleDataType   = currPrm.sampleDataType;
         kerInitArgs.inputSampleRate  = (ssrc_sample_rate_t) currPrm.inputSampleRate;
         kerInitArgs.outputSampleRate = (ssrc_sample_rate_t) currPrm.outputSampleRate;
         kerInitArgs.numChannels      = currPrm.numChannels;
         kerInitArgs.dataFormat       = currPrm.dataFormat;
#ifdef C7X
         kerInitArgs.enableMMA = currPrm.enableMMA;
         kerInitArgs.mmaSize   = DSPLIB_MMA_SIZE_32_BIT;
         bufferFormat = AUDIOLIB_ssrc_optimalBufferFormat(kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate,
                                                          kerInitArgs.inputSampleCount, kerInitArgs.numChannels,
                                                          kerInitArgs.dataFormat, kerInitArgs.enableMMA);
#endif
#ifdef ARM_A53
         bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
#endif
         kerInitArgs.bufferFormat = bufferFormat;
#ifdef C7X
         if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) {
            kerInitArgs.enableMMA = 0;
         }
         // These fields no longer exist in the struct
         kerInitArgs.blkCopy2DHandle1 = NULL;
         kerInitArgs.blkCopy2DHandle2 = NULL;
         kerInitArgs.blkCopy2DHandle3 = NULL;
         kerInitArgs.firHandle1       = NULL;
         kerInitArgs.firHandle2       = NULL;
#endif

         // Calculate the output frame length based on the sample rate conversion ratio
         // For upsampling: output samples = input samples * ratio
         // For downsampling: output samples = input samples / ratio
         outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate,
                                                           kerInitArgs.inputSampleCount);

         /* Set data types for input and output buffers */
         bufParamsIn.data_type  = currPrm.sampleDataType;
         bufParamsOut.data_type = currPrm.sampleDataType;

         /* INTERLEAVED MODE: Circular buffer configuration
          * In interleaved mode, audio samples from different channels are stored sequentially:
          * [Ch0Sample0, Ch1Sample0, Ch2Sample0, ..., Ch0Sample1, Ch1Sample1, Ch2Sample1, ...]
          */
         if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
            if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) {
               /* Use the utility function to calculate buffer size, alignment and stride_y for circular buffer mode */
               AUDIOLIB_ssrc_bufferParams_t cbParams;
               status_init = AUDIOLIB_ssrc_getCircularBufferParams(
                   kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate, kerInitArgs.inputSampleCount,
                   kerInitArgs.numChannels, currPrm.sampleDataType, &cbParams);

               /* Store these values for later use in the code */
               inBufferCbElementsInterleaved = cbParams.alignment / AUDIOLIB_sizeof(currPrm.sampleDataType);
               inBufferCbSizeInterleaved     = cbParams.alignment;

               /* Configure buffer dimensions for interleaved format:
                * - dim_y = number of samples per channel (time dimension)
                * - dim_x = number of channels (channel dimension)
                * - stride_y = bytes to move to next sample in time (all channels)
                */
               bufParamsIn.dim_y    = currPrm.inputSampleCount;
               bufParamsIn.dim_x    = kerInitArgs.numChannels;
               bufParamsIn.stride_y = kerInitArgs.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType);

               /* Allocate memory with alignment equal to buffer size for circular addressing
                * This ensures the buffer can be accessed with hardware circular addressing
                * where buffer[index + size] automatically wraps to buffer[index]
                */
               pIn    = (void *) TI_memalign(cbParams.alignment, cbParams.bufferSize);
               pState = NULL;

               /* Configure output buffer parameters */
               bufParamsOut.dim_y    = outFrameLength;
               bufParamsOut.dim_x    = kerInitArgs.numChannels;
               bufParamsOut.stride_y = kerInitArgs.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType);
            }
            else {
               /* Linear buffer format for interleaved mode */

               /* Configure buffer dimensions for interleaved format:
                * - dim_y = number of samples per channel (time dimension)
                * - dim_x = number of channels (channel dimension)
                * - stride_y = bytes to move to next sample in time (all channels)
                */
               bufParamsIn.dim_y    = currPrm.inputSampleCount;
               bufParamsIn.dim_x    = kerInitArgs.numChannels;
               bufParamsIn.stride_y = kerInitArgs.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType);

               AUDIOLIB_ssrc_bufferParams_t lbParams;
               status_init = AUDIOLIB_ssrc_getStateBufferParams(
                   kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate, kerInitArgs.inputSampleCount,
                   kerInitArgs.numChannels, kerInitArgs.sampleDataType, kerInitArgs.dataFormat, &lbParams);

               uint32_t inputBufferSize = AUDIOLIB_ssrc_getLinearInputBufferSize(
                   kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate, kerInitArgs.inputSampleCount,
                   kerInitArgs.numChannels, kerInitArgs.sampleDataType, kerInitArgs.dataFormat);

               /* Allocate input buffer with standard alignment */
               pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inputBufferSize);
               /* Allocate state buffer with standard alignment */
               pState = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, lbParams.bufferSize);

               /* Configure output buffer parameters */
               bufParamsOut.dim_y    = outFrameLength;
               bufParamsOut.dim_x    = kerInitArgs.numChannels;
               bufParamsOut.stride_y = kerInitArgs.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType);
#ifdef C7X
               // Allocate and initialize kernel handles for the signal chain
               int32_t deinterleaveHandleSize = DSPLIB_deinterleave_getHandleSize(NULL);
               int32_t interleaveHandleSize   = DSPLIB_interleave_getHandleSize(NULL);
               deinterleaveHandle             = malloc(deinterleaveHandleSize);
               interleaveHandle               = malloc(interleaveHandleSize);

               // Allocate and initialize blkCopy2D handle
               int32_t blkCopy2DHandleSize = DSPLIB_blkCopy2D_getHandleSize(NULL);
               blkCopy2DHandle1            = malloc(blkCopy2DHandleSize);
               blkCopy2DHandle2            = malloc(blkCopy2DHandleSize);

               // Allocate and initialize fir handle
               int32_t firHandleSize = DSPLIB_fir_getHandleSize(NULL);
               firHandle1            = malloc(firHandleSize);
               firHandle2            = malloc(firHandleSize);

               kerInitArgs.deinterleaveHandle = deinterleaveHandle;
               kerInitArgs.interleaveHandle   = interleaveHandle;
               kerInitArgs.blkCopy2DHandle1   = blkCopy2DHandle1;
               kerInitArgs.blkCopy2DHandle2   = blkCopy2DHandle2;
               kerInitArgs.firHandle1         = firHandle1;
               kerInitArgs.firHandle2         = firHandle2;
#endif
            }
         }
         /* NON-INTERLEAVED MODE: Linear buffer configuration
          * In non-interleaved mode, audio samples from different channels are stored in separate blocks:
          * [Ch0Sample0, Ch0Sample1, Ch0Sample2, ..., Ch1Sample0, Ch1Sample1, Ch1Sample2, ...]
          */
         else {
            /* Linear buffer format for non-interleaved mode */

            /* Configure buffer dimensions for non-interleaved format:
             * - dim_y = number of channels (each channel gets its own row)
             * - dim_x = number of samples per channel (time dimension)
             * - stride_y = bytes to move to next sample in time (all channels)
             */
            bufParamsIn.dim_y    = currPrm.numChannels;
            bufParamsIn.dim_x    = currPrm.inputSampleCount;
            bufParamsIn.stride_y = currPrm.inputSampleCount * AUDIOLIB_sizeof(currPrm.sampleDataType);

            AUDIOLIB_ssrc_bufferParams_t lbParams;
            status_init = AUDIOLIB_ssrc_getStateBufferParams(
                kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate, kerInitArgs.inputSampleCount,
                kerInitArgs.numChannels, kerInitArgs.sampleDataType, kerInitArgs.dataFormat, &lbParams);

            uint32_t inputBufferSize = AUDIOLIB_ssrc_getLinearInputBufferSize(
                kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate, kerInitArgs.inputSampleCount,
                kerInitArgs.numChannels, currPrm.sampleDataType, kerInitArgs.dataFormat);

            /* Allocate input buffer with standard alignment */
            pIn    = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inputBufferSize);
            pState = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, lbParams.bufferSize);

            /* Configure output buffer parameters */
            bufParamsOut.dim_y    = currPrm.numChannels;
            bufParamsOut.dim_x    = outFrameLength;
            bufParamsOut.stride_y = outFrameLength * AUDIOLIB_sizeof(currPrm.sampleDataType);
#ifdef C7X
            // Allocate and initialize blkCopy2D handle
            int32_t blkCopy2DHandleSize = DSPLIB_blkCopy2D_getHandleSize(NULL);
            blkCopy2DHandle1            = malloc(blkCopy2DHandleSize);
            blkCopy2DHandle2            = malloc(blkCopy2DHandleSize);
            blkCopy2DHandle3            = malloc(blkCopy2DHandleSize);

            // Allocate and initialize fir handle
            int32_t firHandleSize = DSPLIB_fir_getHandleSize(NULL);
            firHandle1            = malloc(firHandleSize);
            firHandle2            = malloc(firHandleSize);

            kerInitArgs.blkCopy2DHandle1 = blkCopy2DHandle1;
            kerInitArgs.blkCopy2DHandle2 = blkCopy2DHandle2;
            kerInitArgs.blkCopy2DHandle3 = blkCopy2DHandle3;
            kerInitArgs.firHandle1       = firHandle1;
            kerInitArgs.firHandle2       = firHandle2;
#endif
         }

         filtCoeffSizeBytes =
             AUDIOLIB_ssrc_getFilterCoeffSize(kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate, bufferFormat,
#ifdef C7X
                                              kerInitArgs.enableMMA, kerInitArgs.mmaSize,
#endif
                                              kerInitArgs.sampleDataType);
         pFiltCoeffs = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, filtCoeffSizeBytes);

         if (status_init == AUDIOLIB_SUCCESS) {
            AUDIOLIB_ssrc_copyFilterCoeffs(kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate, bufferFormat,
#ifdef C7X
                                           kerInitArgs.enableMMA, kerInitArgs.mmaSize,
#endif
                                           kerInitArgs.sampleDataType, audiolib_ssrc_downsampleStage1,
                                           audiolib_ssrc_downsampleStage2, audiolib_ssrc_upsampleStage1,
                                           audiolib_ssrc_upsampleStage2, filtCoeffSizeBytes, pFiltCoeffs);
         }

         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, bufParamsOut.stride_y * bufParamsOut.dim_y);
            pOutCn = (void *) malloc(bufParamsOut.stride_y * bufParamsOut.dim_y);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, bufParamsOut.stride_y * bufParamsOut.dim_y);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, bufParamsOut.stride_y * bufParamsOut.dim_y);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }

#if AUDIOLIB_DEBUGPRINT
         printf(
             "AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d Input Sample Count=%d, Blocks=%d, Input Sample Rate=%u Hz, Output "
             "Sample Rate=%u Hz, Signal Frequency=%0.3f kHz, Num of Channels=%d\n",
             currPrm.inputSampleCount, (int32_t) currPrm.blockCount, (uint32_t) fsin, (uint32_t) fsout,
             (sigFreq / 1000), currPrm.numChannels);
#endif
         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn && pOut && pOutCn && pFiltCoeffs &&
             ((pState == NULL && kerInitArgs.bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) ||
              (pState != NULL && kerInitArgs.bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR))) {

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 0\n");
#endif
            if (status_init == AUDIOLIB_SUCCESS) {
               status_init = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
            }

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 1 status_init %d\n", status_init);
#endif

            // C7000 cold run
            t_ssrc_cycles_opt = 0;
            if (status_init == AUDIOLIB_SUCCESS) {
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 1");
            }

            if (status_init == AUDIOLIB_SUCCESS) {
               mode       = (uint8_t) AUDIOLIB_SSRC_MODE_RESET;
               status_opt = AUDIOLIB_ssrc_set(handle, mode, pIn, pState);
            }

            pInStartOffset = 0; // Starting position in the circular buffer
            for (block = 0; block < (int32_t) currPrm.blockCount; block++) {
               if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  pInDBuffer = (float *) pIn;
                  /* Data storage pettern is staticIn -
                     rows - channel interleaved samples (C0S0, C1S0, C2S0, C3S0, ....etc)
                     cols - different samples per channel(C0S0, C0S1, C0S2, ....etc) */

                  if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) {
                     /* Use circular buffer copy function to handle odd number of input samples
                        that might wrap around the end of the buffer */
                     pInStartOffset =
                         (block * currPrm.inputSampleCount * currPrm.numChannels) % inBufferCbElementsInterleaved;
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData =
                            ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount * currPrm.numChannels);
                        copyRoi_float_circular((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                               bufParamsIn.dim_y, bufParamsIn.stride_y, inBufferCbSizeInterleaved,
                                               pInStartOffset, AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }
                  else {
                     /* For linear buffer format, use standard copy function */
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData =
                            ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount * currPrm.numChannels);
                        copyRoi_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x, bufParamsIn.dim_y,
                                      bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }
               }
               else {
                  pInDBuffer = (float *) pIn;
                  /* Data storage pettern is staticIn -
                     rows - sample data per each channel
                     cols - store samples (sample count per row = blockCount * inputSampleCount) */

                  /* For linear buffer format, use standard copy function */
                  if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                     pInData = ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount);
                     copyRoiDualStride_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                             bufParamsIn.dim_y, bufParamsIn.stride_y,
                                             currPrm.inputSampleCount * AUDIOLIB_sizeof(currPrm.sampleDataType) *
                                                 currPrm.blockCount,
                                             AUDIOLIB_sizeof(currPrm.sampleDataType));
                  }
               }

               if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                  status_opt = AUDIOLIB_ssrc_exec_checkParams(handle, pIn, pState, pFiltCoeffs, pOut);
               }
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT);
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                  AUDIOLIB_asm(" MARK 2");
                  status_opt = AUDIOLIB_ssrc_exec(handle, pIn, pState, pFiltCoeffs, pOut);
                  AUDIOLIB_asm(" MARK 3");
               }
               TI_profile_stop();
               ssrc_cycles = TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
               t_ssrc_cycles_opt += ssrc_cycles;

#ifdef ENABLE_GENERATE_OUTPUT_FILE
               if (repCount == 0) { // Only for cold run
                  int32_t isInterleaved = (int32_t) AUDIOLIB_DATA_FORMAT_INTERLEAVED;
                  writeInOutDataToFile(pInDBuffer, currPrm.numChannels, currPrm.inputSampleCount, currPrm.testID, block,
                                       isInterleaved, 1);
                  writeInOutDataToFile((float *) pOut, currPrm.numChannels, outFrameLength, currPrm.testID, block,
                                       isInterleaved, 0);
               }
#endif
            }

            // WARM run
            for (k = 0; k < AUDIOLIB_SSRC_NUM_WARM_REPS; k++) {
               t_ssrc_cycles_warm = 0;
               if (status_init == AUDIOLIB_SUCCESS) {
                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_INIT);
                  TI_profile_start(TI_PROFILE_KERNEL_INIT);
                  AUDIOLIB_asm(" MARK 4");
                  kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
                  status_init           = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                  AUDIOLIB_asm(" MARK 5");
                  TI_profile_stop();
               }

               if (status_init == AUDIOLIB_SUCCESS) {
                  mode       = (uint8_t) AUDIOLIB_SSRC_MODE_RESET;
                  status_opt = AUDIOLIB_ssrc_set(handle, mode, pIn, pState);
               }

               pInStartOffset = 0;
               for (block = 0; block < (int32_t) currPrm.blockCount; block++) {
                  if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                     pInDBuffer = (float *) pIn;
                     if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) {
                        pInStartOffset =
                            (block * currPrm.inputSampleCount * currPrm.numChannels) % inBufferCbElementsInterleaved;
                        if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                           pInData =
                               ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount * currPrm.numChannels);
                           copyRoi_float_circular((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                                  bufParamsIn.dim_y, bufParamsIn.stride_y, inBufferCbSizeInterleaved,
                                                  pInStartOffset, AUDIOLIB_sizeof(currPrm.sampleDataType));
                        }
                     }
                     else {
                        if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                           pInData =
                               ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount * currPrm.numChannels);
                           copyRoi_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x, bufParamsIn.dim_y,
                                         bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType));
                        }
                     }
                  }
                  else {
                     pInDBuffer = (float *) pIn;
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData = ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount);
                        copyRoiDualStride_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                                bufParamsIn.dim_y, bufParamsIn.stride_y,
                                                currPrm.inputSampleCount * AUDIOLIB_sizeof(currPrm.sampleDataType) *
                                                    currPrm.blockCount,
                                                AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }

                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     status_opt = AUDIOLIB_ssrc_exec_checkParams(handle, pIn, pState, pFiltCoeffs, pOut);
                  }

                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
                  TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     // execute the optimized kernel
                     AUDIOLIB_asm(" MARK 6");
                     status_opt = AUDIOLIB_ssrc_exec(handle, pIn, pState, pFiltCoeffs, pOut);
                     AUDIOLIB_asm(" MARK 7");
                  }
                  TI_profile_stop();
                  ssrc_cycles = TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
                  t_ssrc_cycles_warm += ssrc_cycles;
               }
            }

            // initialize the kernel to use the c7000 optimized kernel
            // WARMWRB run
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 2 status_init %d\n", status_init);
#endif
            if (status_init == AUDIOLIB_SUCCESS) {
               AUDIOLIB_asm(" MARK 8");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 9");
            }

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 3 status_init %d\n", status_init);
#endif
            if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
               mode       = (uint8_t) AUDIOLIB_SSRC_MODE_RESET;
               status_opt = AUDIOLIB_ssrc_set(handle, mode, pIn, pState);
            }
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 4 status_opt %d\n", status_opt);
#endif

            // get output to L1D
            int16_t outSum   = 0;
            int8_t *pOutTemp = (int8_t *) pOut; // treat output as bytes to be data type agnostic
            for (k = 0; k < bufParamsOut.dim_x; k++) {
               outSum += *pOutTemp;
               pOutTemp++;
            }

            // dummy store of outSum to insure that the compiler does not remove it.
            volatileSum = outSum;

            totalOutputSampleCnt = 0;
            pInStartOffset       = 0;
            for (block = 0; block < (int32_t) currPrm.blockCount; block++) {
               if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  pInDBuffer = (float *) pIn;
                  if (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR) {
                     pInStartOffset =
                         (block * currPrm.inputSampleCount * currPrm.numChannels) % inBufferCbElementsInterleaved;
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData =
                            ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount * currPrm.numChannels);
                        copyRoi_float_circular((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                               bufParamsIn.dim_y, bufParamsIn.stride_y, inBufferCbSizeInterleaved,
                                               pInStartOffset, AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }
                  else {
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData =
                            ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount * currPrm.numChannels);
                        copyRoi_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x, bufParamsIn.dim_y,
                                      bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }
               }
               else {
                  pInDBuffer = (float *) pIn;
                  if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                     pInData = ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount);
                     copyRoiDualStride_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                             bufParamsIn.dim_y, bufParamsIn.stride_y,
                                             currPrm.inputSampleCount * AUDIOLIB_sizeof(currPrm.sampleDataType) *
                                                 currPrm.blockCount,
                                             AUDIOLIB_sizeof(currPrm.sampleDataType));
                  }
               }

               if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                  status_opt = AUDIOLIB_ssrc_exec_checkParams(handle, pIn, pState, pFiltCoeffs, pOut);
               }

               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARMWRB);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
               if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                  // execute the optimized kernel
                  AUDIOLIB_asm(" MARK 10");
                  status_opt = AUDIOLIB_ssrc_exec(handle, pIn, pState, pFiltCoeffs, pOut);
                  AUDIOLIB_asm(" MARK 11");
               }
               TI_profile_stop();
               ssrc_cycles = TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);
               t_ssrc_cycles_warmwrb += ssrc_cycles;
#if AUDIOLIB_DEBUGPRINT
               printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 6 status_opt %d\n", status_opt);
#endif

               if (currPrm.staticOut != NULL) {
                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                        pOutData          = (float *) currPrm.staticOut + totalOutputSampleCnt * currPrm.numChannels;
                        status_ref_vs_opt = TI_compare_mem_2D_float(
                            (void *) pOutData, pOut, 0.001, (double) powf(10, -6), currPrm.numChannels, outFrameLength,
                            currPrm.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType),
                            AUDIOLIB_sizeof(bufParamsOut.data_type));
                     }
                     else {
                        pOutData          = (float *) currPrm.staticOut + totalOutputSampleCnt;
                        status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                            (void *) pOutData, pOut, 0.001, (double) powf(10, -6), outFrameLength, bufParamsOut.dim_y,
                            outFrameLength * currPrm.blockCount * AUDIOLIB_sizeof(currPrm.sampleDataType),
                            bufParamsOut.stride_y, AUDIOLIB_sizeof(bufParamsOut.data_type));
                     }
                  }

                  if (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) {
                     currentTestFail = 1;
                  }
                  ref_vs_opt_comparisonDone = 1;
               }
               else {
                  /* Set to pass since it wasn't supposed to run. */
                  status_ref_vs_opt = TI_TEST_KERNEL_PASS;
               }

               totalOutputSampleCnt += outFrameLength;
            }
#ifdef C7X
            processorUtilization = (float) t_ssrc_cycles_warm * fsin / (currPrm.inputSampleCount * block * pow(10, 6));
#endif
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 7 status_opt %d\n", status_opt);
#endif

            /* Test _cn kernel */
            if ((kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) &&
                (bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR)) {
               if (status_init == AUDIOLIB_SUCCESS) {
                  AUDIOLIB_asm(" MARK 12");
                  kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
                  status_init           = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                  AUDIOLIB_asm(" MARK 13");
               }

#if AUDIOLIB_DEBUGPRINT
               printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 8 status_init %d\n", status_init);
#endif

               if (status_init == AUDIOLIB_SUCCESS) {
                  mode       = (uint8_t) AUDIOLIB_SSRC_MODE_RESET;
                  status_nat = AUDIOLIB_ssrc_set(handle, mode, pIn, pState);
               }
#if AUDIOLIB_DEBUGPRINT
               printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 9 status_init %d\n", status_init);
#endif

               totalOutputSampleCnt = 0;

               pInStartOffset = 0;
               for (block = 0; block < (int32_t) currPrm.blockCount; block++) {

                  pInDBuffer = (float *) pIn;
                  pInStartOffset =
                      (block * currPrm.inputSampleCount * currPrm.numChannels) % inBufferCbElementsInterleaved;
                  if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                     pInData = ((float *) prm[tpi].staticIn) + (block * currPrm.inputSampleCount * currPrm.numChannels);
                     copyRoi_float_circular((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x, bufParamsIn.dim_y,
                                            bufParamsIn.stride_y, inBufferCbSizeInterleaved, pInStartOffset,
                                            AUDIOLIB_sizeof(currPrm.sampleDataType));
                  }

                  if (status_init == AUDIOLIB_SUCCESS && status_nat == AUDIOLIB_SUCCESS) {
                     status_nat = AUDIOLIB_ssrc_exec_checkParams(handle, pIn, pState, pFiltCoeffs, pOutCn);
                  }

#if AUDIOLIB_DEBUGPRINT
                  printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 7 status_nat %d\n", status_nat);
#endif
                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_CN);
                  TI_profile_start(TI_PROFILE_KERNEL_CN);

                  if (status_init == AUDIOLIB_SUCCESS && status_nat == AUDIOLIB_SUCCESS) {
                     AUDIOLIB_asm(" MARK 14");
                     status_nat = AUDIOLIB_ssrc_exec(handle, pIn, pState, pFiltCoeffs, pOutCn);
                     AUDIOLIB_asm(" MARK 15");
                  }
                  TI_profile_stop();
#if AUDIOLIB_DEBUGPRINT
                  printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_ssrc_d CP 8 status_nat %d\n", status_nat);
#endif

                  if (currPrm.staticOut != NULL) {
                     if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                        pOutData          = (float *) currPrm.staticOut + totalOutputSampleCnt * currPrm.numChannels;
                        status_ref_vs_nat = TI_compare_mem_2D_float(
                            (void *) pOutData, pOutCn, 0.001, (double) powf(10, -6), currPrm.numChannels,
                            outFrameLength, currPrm.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType),
                            AUDIOLIB_sizeof(bufParamsOut.data_type));
                     }
                     if (status_ref_vs_nat == TI_TEST_KERNEL_FAIL) {
                        currentTestFail = 1;
                     }
                     ref_vs_nat_comparisonDone = 1;
                  }
                  else {
                     /* Set to pass since it wasn't supposed to run. */
                     status_ref_vs_nat = TI_TEST_KERNEL_PASS;
                  }

                  totalOutputSampleCnt += outFrameLength;
               }
            }
            else {
               status_ref_vs_nat         = TI_TEST_KERNEL_PASS;
               ref_vs_nat_comparisonDone = 1;
            }

            /* Set the 'fail' flag based on test vector comparison results */
            /* Only STATIC tests are supported */
            currentTestFail =
                ((status_ref_vs_nat == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (ref_vs_opt_comparisonDone == 0) ||
                 (ref_vs_nat_comparisonDone == 0) || (currentTestFail == 1) || (currPrm.testPattern != STATIC))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            pProfile[3 * tpi]     = t_ssrc_cycles_opt / currPrm.blockCount;
            pProfile[3 * tpi + 1] = t_ssrc_cycles_warm / currPrm.blockCount;
            pProfile[3 * tpi + 2] = t_ssrc_cycles_warmwrb / currPrm.blockCount;

            sprintf(desc,
                    "STATIC generated input | Input Sample Count=%d, Blocks=%d, Input Sample Rate=%u Hz, Output "
                    "Sample Rate=%u Hz, Signal Frequency=%0.3f kHz, Num of Channels=%d",
                    currPrm.inputSampleCount, block, (uint32_t) fsin, (uint32_t) fsout, (sigFreq / 1000),
                    currPrm.numChannels);

            AUDIOLIB_ssrc_perfEst(handle, &bufParamsIn, &bufParamsOut, &archCycles, &estCycles);

// write to CSV, must happen prior to write to screen because
// TI_profile_formula_add clears values in counters
#ifdef C7X
            fprintf(fpOutputCSV,
                    "SSRC, %d, %d, %d, Input Data Format = %s | Buffer Format = %s | Enable MMA = %d | "
                    " Input Sample Count=%d | Blocks=%d | Input Signal Frequency=%0.3f | "
                    "Input Sample Rate=%u | Output Sample Rate=%u | Num of Channels=%d, "
                    "%s, %s, %d, %u, %d, %0.3f, %u, %u, %d, %d, %5.2f, %lu, %lu, %.2f, %d, %d, %d, %d\n",
                    testNum, currPrm.testPattern, AUDIOLIB_sizeof(bufParamsIn.data_type) * 8,
                    (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) ? "Interleaved" : "Non-Interleaved",
                    (kerInitArgs.bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) ? "Linear" : "Circular",
                    kerInitArgs.enableMMA, kerInitArgs.inputSampleCount, block, sigFreq, (uint32_t) fsin,
                    (uint32_t) fsout, kerInitArgs.numChannels,
                    (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) ? "Interleaved" : "Non-Interleaved",
                    (kerInitArgs.bufferFormat == AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR) ? "Linear" : "Circular",
                    kerInitArgs.enableMMA, kerInitArgs.inputSampleCount, block, sigFreq, (uint32_t) fsin,
                    (uint32_t) fsout, kerInitArgs.numChannels, totalOutputSampleCnt, processorUtilization, archCycles,
                    estCycles, ((AUDIOLIB_F32) estCycles) / ((AUDIOLIB_F32) pProfile[3 * tpi + 1]), !currentTestFail,
                    pProfile[3 * tpi], pProfile[3 * tpi + 1], pProfile[3 * tpi + 2]);
#endif

            TI_profile_add_test(testNum++, currPrm.inputSampleCount * currPrm.numChannels, 0, 0, currentTestFail, desc);
         }
         else {
            sprintf(desc,
                    "Input Sample Count=%d, Blocks=%d, Input Sample Rate=%u, Output Sample Rate=%u, Num of Channels=%d",
                    currPrm.inputSampleCount, block, (uint32_t) fsin, (uint32_t) fsout, currPrm.numChannels);
            TI_profile_skip_test(desc);
            // clear the counters between runs; normally handled by
            TI_profile_clear_run_stats();
         } // end of memory allocation successful?

         /* Free buffers for each test vector */
         TI_align_free(pIn);
         /* Free state buffer if it was allocated (for linear buffer format) */
         if (pState) {
            TI_align_free(pState);
         }
         TI_align_free(pFiltCoeffs);
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            TI_align_free(pOut);
            free(pOutCn);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            TI_align_free(pOut);
#endif
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            TI_align_free(pOut);
#endif
         }
#ifdef C7X
         // Free the kernel handles
         if (deinterleaveHandle)
            free(deinterleaveHandle);
         if (interleaveHandle)
            free(interleaveHandle);
         if (blkCopy2DHandle1)
            free(blkCopy2DHandle1);
         if (blkCopy2DHandle2)
            free(blkCopy2DHandle2);
         if (blkCopy2DHandle3)
            free(blkCopy2DHandle3);
         if (firHandle1)
            free(firHandle1);
         if (firHandle2)
            free(firHandle2);
#endif
      } // end repetitions
   }    // end idat test cases

   free(handle);

/* Close results CSV */
#ifdef C7X
   fclose(fpOutputCSV);
#endif

   return fail;
}

int test_main(uint32_t *pProfile)
{
#ifdef C7X
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_ssrc");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
#endif
   {
      return AUDIOLIB_ssrc_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                testNum         = 1000;
   int32_t                currentTestFail = 0;
   AUDIOLIB_STATUS        status;
   AUDIOLIB_ssrc_InitArgs initArgs;
   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;
   int                    fail   = 0;
   AUDIOLIB_kernelHandle  handle = NULL;
   float                 *pIn = NULL, *pOut = NULL, *pState = NULL;
   float                 *pFiltCoeffs = NULL;
#ifdef C7X
   DSPLIB_kernelHandle deinterleaveHandle = NULL;
   DSPLIB_kernelHandle interleaveHandle   = NULL;
   DSPLIB_kernelHandle blkCopy2DHandle1   = NULL;
   DSPLIB_kernelHandle blkCopy2DHandle2   = NULL;
   DSPLIB_kernelHandle blkCopy2DHandle3   = NULL;
   DSPLIB_kernelHandle firHandle1         = NULL;
   DSPLIB_kernelHandle firHandle2         = NULL;
#endif
   char desc[256];
   bool handleInitialized = false;

   AUDIOLIB_DEBUG_PRINT("===== Starting SSRC Coverage Tests =====\r\n");

   // Set up valid parameters as baseline
   memset(&initArgs, 0, sizeof(AUDIOLIB_ssrc_InitArgs));
   initArgs.funcStyle        = AUDIOLIB_FUNCTION_OPTIMIZED;
   initArgs.inputSampleCount = 256;
   initArgs.sampleDataType   = AUDIOLIB_FLOAT32;
   initArgs.inputSampleRate  = SSRC_SAMPLE_RATE_48000;
   initArgs.outputSampleRate = SSRC_SAMPLE_RATE_96000;
   initArgs.numChannels      = 2;
   initArgs.dataFormat       = AUDIOLIB_DATA_FORMAT_INTERLEAVED;
   initArgs.bufferFormat     = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
#ifdef C7X
   initArgs.enableMMA = 0;
   initArgs.mmaSize   = DSPLIB_MMA_SIZE_32_BIT;
#endif

   // Initialize buffer parameters
   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = 2;   // numChannels
   bufParamsIn.dim_y     = 256; // inputSampleCount
   bufParamsIn.stride_y  = 8;   // 2 channels * 4 bytes

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 2;
   bufParamsOut.dim_y     = 512;
   bufParamsOut.stride_y  = 8;

   // Allocate memory for handle and buffers using TI memory functions
   int32_t handleSize = AUDIOLIB_ssrc_getHandleSize(&initArgs);
   handle             = malloc(handleSize);

#ifdef C7X
   // Allocate memory for required handles
   int32_t deinterleaveHandleSize = DSPLIB_deinterleave_getHandleSize(NULL);
   int32_t interleaveHandleSize   = DSPLIB_interleave_getHandleSize(NULL);
   int32_t blkCopy2DHandleSize    = DSPLIB_blkCopy2D_getHandleSize(NULL);
   int32_t firHandleSize          = DSPLIB_fir_getHandleSize(NULL);

   deinterleaveHandle = malloc(deinterleaveHandleSize);
   interleaveHandle   = malloc(interleaveHandleSize);
   blkCopy2DHandle1   = malloc(blkCopy2DHandleSize);
   blkCopy2DHandle2   = malloc(blkCopy2DHandleSize);
   blkCopy2DHandle3   = malloc(blkCopy2DHandleSize);
   firHandle1         = malloc(firHandleSize);
   firHandle2         = malloc(firHandleSize);

   // Set the handles in the initArgs structure
   initArgs.deinterleaveHandle = deinterleaveHandle;
   initArgs.interleaveHandle   = interleaveHandle;
   initArgs.blkCopy2DHandle1   = blkCopy2DHandle1;
   initArgs.blkCopy2DHandle2   = blkCopy2DHandle2;
   initArgs.blkCopy2DHandle3   = blkCopy2DHandle3;
   initArgs.firHandle1         = firHandle1;
   initArgs.firHandle2         = firHandle2;
#endif

   pIn         = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));
   pOut        = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));
   pState      = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));
   pFiltCoeffs = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));

#ifdef C7X
   if (!handle || !pIn || !pOut || !pState || !pFiltCoeffs || !deinterleaveHandle || !interleaveHandle ||
       !blkCopy2DHandle1 || !blkCopy2DHandle2 || !blkCopy2DHandle3 || !firHandle1 || !firHandle2) {
#endif
#ifdef ARM_A53
      if (!handle || !pIn || !pOut || !pState || !pFiltCoeffs) {
#endif
         AUDIOLIB_DEBUG_PRINT("Failed to allocate memory in SSRC Coverage Tests\r\n");
         fail = 1;
         goto cleanup;
      }

      while (testNum <= 1070) {
         currentTestFail = 0;

         switch (testNum) {
            // Tests for AUDIOLIB_ssrc_init_checkParams()
         case 1000:
            status          = AUDIOLIB_ssrc_init_checkParams(NULL, &bufParamsIn, &bufParamsOut, &initArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
            break;

         case 1001:
            status          = AUDIOLIB_ssrc_init_checkParams(handle, NULL, &bufParamsOut, &initArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
            break;

         case 1002:
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, NULL, &initArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
            break;

         case 1003:
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, NULL);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
            break;

         case 1004: {
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.funcStyle              = 55; // Invalid value
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1005: {
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.inputSampleCount       = 0;
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1007: {
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.sampleDataType         = 55; // Invalid type
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_TYPE);
         } break;

         case 1008: {
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.inputSampleRate        = SSRC_SAMPLE_RATE_48000;
            badArgs.outputSampleRate       = SSRC_SAMPLE_RATE_48000; // Same rate, invalid for SSRC
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1010: {
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.numChannels            = 0;
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1011: {
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.dataFormat             = 55; // Invalid format
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

#ifdef C7X
         case 1012: {
            // Test for NULL handles with interleaved format
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;
            badArgs.deinterleaveHandle     = NULL; // Set one handle to NULL
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;
#endif

         case 1013: {
            // Test for valid sample rate ratio (2:1)
            // Create a new set of buffer parameters for this test
            AUDIOLIB_bufParams2D_t validBufIn  = bufParamsIn;
            AUDIOLIB_bufParams2D_t validBufOut = bufParamsOut;

            // Create valid init args with 2:1 ratio
            AUDIOLIB_ssrc_InitArgs validArgs = initArgs;
            validArgs.inputSampleRate        = SSRC_SAMPLE_RATE_48000;
            validArgs.outputSampleRate       = SSRC_SAMPLE_RATE_96000; // 2:1 ratio (valid)

            // Adjust buffer parameters to match the new sample rates
            int outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(validArgs.inputSampleRate, validArgs.outputSampleRate,
                                                                  validArgs.inputSampleCount);
            validBufOut.dim_y  = outFrameLength;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &validBufIn, &validBufOut, &validArgs);
            currentTestFail = (status != AUDIOLIB_SUCCESS);
         } break;

         // Test for valid sample rate ratio (4:1)
         case 1014: {
            // Create a new set of buffer parameters for this test
            AUDIOLIB_bufParams2D_t validBufIn  = bufParamsIn;
            AUDIOLIB_bufParams2D_t validBufOut = bufParamsOut;

            // Create valid init args with 4:1 ratio
            AUDIOLIB_ssrc_InitArgs validArgs = initArgs;
            validArgs.inputSampleRate        = SSRC_SAMPLE_RATE_48000;
            validArgs.outputSampleRate       = SSRC_SAMPLE_RATE_192000; // 4:1 ratio (valid)

            // Adjust buffer parameters to match the new sample rates
            int outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(validArgs.inputSampleRate, validArgs.outputSampleRate,
                                                                  validArgs.inputSampleCount);
            validBufOut.dim_y  = outFrameLength;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &validBufIn, &validBufOut, &validArgs);
            currentTestFail = (status != AUDIOLIB_SUCCESS);
         } break;

         // Test for invalid sample rate ratio (3:1)
         case 1015: {
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.inputSampleRate        = SSRC_SAMPLE_RATE_48000;
            badArgs.outputSampleRate       = SSRC_SAMPLE_RATE_16000; // 3:1 ratio (invalid)
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         // Test for exceeding max channels
         case 1016: {
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.numChannels            = AUDIOLIB_SSRC_MAX_NUM_CHANNELS + 1; // Exceeds max channels
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         // Tests for AUDIOLIB_ssrc_exec_checkParams()
         case 1018:
            status          = AUDIOLIB_ssrc_exec_checkParams(handle, pIn, NULL, NULL, NULL);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
            break;
         case 1019: {
            // Test NULL handle parameter - we need to ensure we're passing valid pointers for other parameters
            status          = AUDIOLIB_ssrc_exec_checkParams(NULL, pIn, pState, pFiltCoeffs, pOut);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1020: {
            // Test NULL input parameter - we need to ensure we're passing valid pointers for other parameters
            status          = AUDIOLIB_ssrc_exec_checkParams(handle, NULL, pState, pFiltCoeffs, pOut);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1021: {
            // Test NULL filter coeffs parameter - we need to ensure we're passing valid pointers for other parameters
            status          = AUDIOLIB_ssrc_exec_checkParams(handle, pIn, pState, NULL, pOut);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;
         // Tests for AUDIOLIB_ssrc_set()
         case 1022: {
            // Test NULL handle parameter - we should only test the NULL handle parameter
            // and provide valid values for other parameters to avoid segmentation faults
            status          = AUDIOLIB_ssrc_set(NULL, AUDIOLIB_SSRC_MODE_RESET, pIn, pState);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1023: {
            // Make sure handle is initialized before testing
            if (!handleInitialized) {
               status            = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &initArgs);
               handleInitialized = (status == AUDIOLIB_SUCCESS);
            }

            // Test invalid mode parameter
            status          = AUDIOLIB_ssrc_set(handle, 55, pIn, NULL); // Invalid mode
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1024: {
            // Make sure handle is initialized before testing
            if (!handleInitialized) {
               status            = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &initArgs);
               handleInitialized = (status == AUDIOLIB_SUCCESS);
            }

            // Test NULL input parameter
            status          = AUDIOLIB_ssrc_set(handle, AUDIOLIB_SSRC_MODE_RESET, NULL, NULL); // NULL pIn
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1025: {
            // Make sure handle is initialized before testing
            if (!handleInitialized) {
               status            = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &initArgs);
               handleInitialized = (status == AUDIOLIB_SUCCESS);
            }

            // Test AUDIOLIB_ssrc_set with valid parameters
            status          = AUDIOLIB_ssrc_set(handle, AUDIOLIB_SSRC_MODE_RESET, pIn, pState);
            currentTestFail = (status != AUDIOLIB_SUCCESS);
         } break;

         case 1026: {
            // Test AUDIOLIB_ssrc_checkValidRatio with valid 1:2 ratio
            status          = AUDIOLIB_ssrc_checkValidRatio(SSRC_SAMPLE_RATE_48000, SSRC_SAMPLE_RATE_96000);
            currentTestFail = (status != AUDIOLIB_SUCCESS);
         } break;

         case 1027: {
            // Test AUDIOLIB_ssrc_checkValidRatio with valid 1:4 ratio
            status          = AUDIOLIB_ssrc_checkValidRatio(SSRC_SAMPLE_RATE_48000, SSRC_SAMPLE_RATE_192000);
            currentTestFail = (status != AUDIOLIB_SUCCESS);
         } break;

         case 1028: {
            // Test AUDIOLIB_ssrc_checkValidRatio with invalid 1:3 ratio
            status          = AUDIOLIB_ssrc_checkValidRatio(SSRC_SAMPLE_RATE_16000, SSRC_SAMPLE_RATE_48000);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1029: {
            // Test for invalid input buffer data type
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
            badBufIn.data_type              = AUDIOLIB_INT32; // Invalid type - only FLOAT32 is supported

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_TYPE);
         } break;

         case 1030: {
            // Test for invalid output buffer data type
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            badBufOut.data_type              = AUDIOLIB_INT32; // Invalid type - only FLOAT32 is supported

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_TYPE);
         } break;

         case 1031: {
            // Test for mismatched buffer data types
            AUDIOLIB_bufParams2D_t badBufIn  = bufParamsIn;
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            badBufIn.data_type               = AUDIOLIB_FLOAT32;
            badBufOut.data_type              = AUDIOLIB_INT32; // Mismatched type

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_TYPE);
         } break;

         case 1032: {
            // Test for invalid input buffer dim_y for interleaved format
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
            badBufIn.dim_y                  = initArgs.inputSampleCount + 1; // Invalid for interleaved

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1033: {
            // Test for invalid input buffer dim_x for interleaved format
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
            badBufIn.dim_x                  = initArgs.numChannels + 1; // Invalid for interleaved

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1034: {
            // Test for invalid output buffer dim_x for interleaved format
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            badBufOut.dim_x                  = initArgs.numChannels + 1; // Invalid for interleaved

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1035: {
            // Test for invalid output buffer dim_y for interleaved format
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            badBufOut.dim_y                  = 9999; // Invalid for interleaved - should match outFrameLength

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1036: {
            // Test for invalid input buffer stride_y for interleaved format
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
            badBufIn.stride_y               = 1; // Invalid stride for interleaved

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1037: {
            // Test for invalid output buffer stride_y for interleaved format
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            badBufOut.stride_y               = 1; // Invalid stride for interleaved

            // Create a copy of initArgs to ensure we're using valid parameters
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1038: {
            // Test for invalid input buffer dim_y for non-interleaved format
            // For non-interleaved, dim_y should equal numChannels
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
            badBufIn.dim_y                  = 3;                   // Invalid - should be 2 (numChannels)
            badBufIn.dim_x                  = 256;                 // Valid inputSampleCount
            badBufIn.stride_y               = 256 * sizeof(float); // Valid stride

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create valid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t validBufOut = bufParamsOut;
            int outFrameLength   = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                    testArgs.inputSampleCount);
            validBufOut.dim_y    = testArgs.numChannels;
            validBufOut.dim_x    = outFrameLength;
            validBufOut.stride_y = outFrameLength * sizeof(float);

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &validBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1039: {
            // Test for invalid input buffer dim_x for non-interleaved format
            // For non-interleaved, dim_x should equal inputSampleCount
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
            badBufIn.dim_y                  = 2;                   // Valid numChannels
            badBufIn.dim_x                  = 257;                 // Invalid - should be 256 (inputSampleCount)
            badBufIn.stride_y               = 256 * sizeof(float); // Valid stride

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create valid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t validBufOut = bufParamsOut;
            int outFrameLength   = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                    testArgs.inputSampleCount);
            validBufOut.dim_y    = testArgs.numChannels;
            validBufOut.dim_x    = outFrameLength;
            validBufOut.stride_y = outFrameLength * sizeof(float);

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &validBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1040: {
            // Test for invalid output buffer dim_x for non-interleaved format
            // For non-interleaved, dim_x should equal outFrameLength
            AUDIOLIB_bufParams2D_t validBufIn = bufParamsIn;
            validBufIn.dim_y                  = 2;                   // Valid numChannels
            validBufIn.dim_x                  = 256;                 // Valid inputSampleCount
            validBufIn.stride_y               = 256 * sizeof(float); // Valid stride

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create invalid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            int outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                  testArgs.inputSampleCount);
            badBufOut.dim_y    = testArgs.numChannels;
            badBufOut.dim_x    = outFrameLength + 100; // Invalid - should be outFrameLength
            badBufOut.stride_y = outFrameLength * sizeof(float);

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &validBufIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1041: {
            // Test for invalid output buffer dim_y for non-interleaved format
            // For non-interleaved, dim_y should equal numChannels
            AUDIOLIB_bufParams2D_t validBufIn = bufParamsIn;
            validBufIn.dim_y                  = 2;                   // Valid numChannels
            validBufIn.dim_x                  = 256;                 // Valid inputSampleCount
            validBufIn.stride_y               = 256 * sizeof(float); // Valid stride

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create invalid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            int outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                  testArgs.inputSampleCount);
            badBufOut.dim_y    = testArgs.numChannels + 1; // Invalid - should be numChannels
            badBufOut.dim_x    = outFrameLength;
            badBufOut.stride_y = outFrameLength * sizeof(float);

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &validBufIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1042: {
            // Test for valid non-interleaved format configuration
            AUDIOLIB_bufParams2D_t validBufIn = bufParamsIn;
            validBufIn.dim_y                  = 2;                   // Valid numChannels
            validBufIn.dim_x                  = 256;                 // Valid inputSampleCount
            validBufIn.stride_y               = 256 * sizeof(float); // Valid stride

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create valid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t validBufOut = bufParamsOut;
            int outFrameLength   = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                    testArgs.inputSampleCount);
            validBufOut.dim_y    = testArgs.numChannels;
            validBufOut.dim_x    = outFrameLength;
            validBufOut.stride_y = outFrameLength * sizeof(float);

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &validBufIn, &validBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_SUCCESS);
         } break;

         case 1043: {
            // Test AUDIOLIB_ssrc_getNextPowerOf2 with value 0
            uint32_t result = AUDIOLIB_ssrc_getNextPowerOf2(0);
            currentTestFail = (result != 1);
         } break;

         case 1044: {
            // Test AUDIOLIB_ssrc_getNextPowerOf2 with non-zero value
            uint32_t result = AUDIOLIB_ssrc_getNextPowerOf2(100);
            currentTestFail = (result != 128);
         } break;

         case 1045: {
            AUDIOLIB_ssrc_InitArgs newInitArgs = initArgs;
            status                             = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &newInitArgs);
            handleInitialized                  = (status == AUDIOLIB_SUCCESS);
            currentTestFail                    = (status != AUDIOLIB_SUCCESS);
         } break;

         case 1046: {
            // Test AUDIOLIB_ssrc_getSampleHistoryLength
            uint32_t historyLength =
                AUDIOLIB_ssrc_getSampleHistoryLength(SSRC_SAMPLE_RATE_48000, SSRC_SAMPLE_RATE_96000, 1);
            currentTestFail = (historyLength == 0); // Should return a non-zero value
         } break;

         case 1047: {
            // Test convertSampleRateToInt() with invalid enum value to hit default case
            int rate = AUDIOLIB_ssrc_convertSampleRateToInt((ssrc_sample_rate_t) 999);
            // Expect default value of 0 for invalid enum
            currentTestFail = (rate != 0);
         } break;

         case 1048: {
            // Test AUDIOLIB_ssrc_getOutBufferLength with valid parameters
            int outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(SSRC_SAMPLE_RATE_48000, SSRC_SAMPLE_RATE_96000, 256);
            currentTestFail    = (outFrameLength != 512); // Expect 512 for 2:1 upsampling ratio
         } break;

         case 1049: {
            // Test convertSampleRateToInt with all valid enum values
            uint32_t rate;

            rate = AUDIOLIB_ssrc_convertSampleRateToInt(SSRC_SAMPLE_RATE_8000);
            if (rate != 8000) {
               currentTestFail = 1;
               break;
            }

            rate = AUDIOLIB_ssrc_convertSampleRateToInt(SSRC_SAMPLE_RATE_11025);
            if (rate != 11025) {
               currentTestFail = 1;
               break;
            }

            rate = AUDIOLIB_ssrc_convertSampleRateToInt(SSRC_SAMPLE_RATE_192000);
            if (rate != 192000) {
               currentTestFail = 1;
               break;
            }

            currentTestFail = 0;
         } break;

         case 1050: {
            // Test convertSampleRateToInt with invalid enum value
            uint32_t rate   = AUDIOLIB_ssrc_convertSampleRateToInt((ssrc_sample_rate_t) 999); // Invalid enum value
            currentTestFail = (rate != 0); // Should return 0 for invalid values
         } break;

            // Additional test cases to improve coverage of init_checkParams

         case 1051: {
            // Test for buffer format validation with non-interleaved format
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            badArgs.bufferFormat = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR; // Invalid for non-interleaved
            status               = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail      = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

#ifdef C7X
         case 1052: {
            // Test for enableMMA validation with ping-pong circular buffer format
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.bufferFormat           = AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR;
            badArgs.enableMMA              = 1; // Invalid with ping-pong circular
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1053: {
            // Test for mmaSize validation with linear buffer format and enableMMA
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.bufferFormat           = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
            badArgs.enableMMA              = 1;
            badArgs.mmaSize                = 4; // Invalid mmaSize (should be 8)
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;
#endif

         case 1054: {
            // Test for sample rate validation (inputSampleRate < SSRC_SAMPLE_RATE_8000)
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.inputSampleRate        = (ssrc_sample_rate_t) 0; // Invalid sample rate
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1055: {
            // Test for sample rate validation (outputSampleRate > SSRC_SAMPLE_RATE_192000)
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.outputSampleRate       = (ssrc_sample_rate_t) 20; // Invalid sample rate beyond max
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;

         case 1056: {
            // Test for downsampling with inputSampleCount not a multiple of sample rate ratio
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.inputSampleRate        = SSRC_SAMPLE_RATE_96000;
            badArgs.outputSampleRate       = SSRC_SAMPLE_RATE_48000; // 2:1 downsampling
            badArgs.inputSampleCount       = 255;                    // Not divisible by 2
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

#ifdef C7X
         case 1057: {
            // Test for enableMMA validation with invalid value
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.enableMMA              = 2; // Invalid value (should be 0 or 1)
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
         } break;
#endif

         case 1058: {
            // Test for NULL output parameter in exec_checkParams
            status          = AUDIOLIB_ssrc_exec_checkParams(handle, pIn, pState, pFiltCoeffs, NULL);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1059: {
            // Test for NULL state parameter with linear buffer format in exec_checkParams
            // First initialize the handle with linear buffer format
            AUDIOLIB_ssrc_InitArgs linearFormatArgs = initArgs;
            linearFormatArgs.bufferFormat           = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
            status            = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &linearFormatArgs);
            handleInitialized = (status == AUDIOLIB_SUCCESS);
            status            = AUDIOLIB_ssrc_exec_checkParams(handle, pIn, NULL, pFiltCoeffs, pOut);
            currentTestFail   = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1060: {
            // Test for NULL state parameter with AUDIOLIB_ssrc_set for linear buffer format
            // First initialize the handle with linear buffer format
            AUDIOLIB_ssrc_InitArgs linearFormatArgs = initArgs;
            linearFormatArgs.bufferFormat           = AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR;
            status            = AUDIOLIB_ssrc_init(handle, &bufParamsIn, &bufParamsOut, &linearFormatArgs);
            handleInitialized = (status == AUDIOLIB_SUCCESS);
            status            = AUDIOLIB_ssrc_set(handle, AUDIOLIB_SSRC_MODE_RESET, pIn, NULL);
            currentTestFail   = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

#ifdef C7X
         case 1061: {
            // Test for NULL blkCopy2DHandle1 with non-interleaved format
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            badArgs.blkCopy2DHandle1       = NULL; // Set one handle to NULL
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1062: {
            // Test for NULL blkCopy2DHandle2 with non-interleaved format
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            badArgs.blkCopy2DHandle2       = NULL; // Set one handle to NULL
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1063: {
            // Test for NULL blkCopy2DHandle3 with non-interleaved format
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            badArgs.blkCopy2DHandle3       = NULL; // Set one handle to NULL
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1064: {
            // Test for NULL firHandle1 with non-interleaved format
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            badArgs.firHandle1             = NULL; // Set one handle to NULL
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1065: {
            // Test for NULL firHandle2 with non-interleaved format
            AUDIOLIB_ssrc_InitArgs badArgs = initArgs;
            badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            badArgs.firHandle2             = NULL; // Set one handle to NULL
            status          = AUDIOLIB_ssrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

#endif
         case 1066: {
            // Test for invalid stride_y in non-interleaved format for bufOut
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
            badBufIn.dim_y                  = 2;   // Valid numChannels
            badBufIn.dim_x                  = 256; // Valid inputSampleCount
            badBufIn.stride_y               = 256 * sizeof(float);

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create valid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t validBufOut = bufParamsOut;
            int outFrameLength   = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                    testArgs.inputSampleCount);
            validBufOut.dim_y    = testArgs.numChannels;
            validBufOut.dim_x    = outFrameLength;
            validBufOut.stride_y = 1;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &validBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1067: {
            // Test for invalid stride_y in non-interleaved format
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create valid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            int outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                  testArgs.inputSampleCount);
            badBufOut.dim_y    = testArgs.numChannels;
            badBufOut.dim_x    = outFrameLength;
            badBufOut.stride_y = outFrameLength * sizeof(float) * 2;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1068: {
            // Test for invalid stride_y in non-interleaved format
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create valid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
            int outFrameLength = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                  testArgs.inputSampleCount);
            badBufOut.dim_y    = testArgs.numChannels;
            badBufOut.dim_x    = outFrameLength;
            badBufOut.stride_y = outFrameLength * sizeof(float) * 2;

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &badBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         case 1069: {
            // Test for NULL pointer check in AUDIOLIB_ssrc_copyFilterCoeffs
            // This test case targets the NULL pointer check at the beginning of the function
            int32_t filtCoeffSizeBytes = 1024; // Arbitrary size for test

            // Call with pFiltCoeffs set to NULL
            status = AUDIOLIB_ssrc_copyFilterCoeffs(
                SSRC_SAMPLE_RATE_48000, SSRC_SAMPLE_RATE_96000, AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR,
#ifdef C7X
                0, 8,
#endif
                AUDIOLIB_FLOAT32, audiolib_ssrc_downsampleStage1, audiolib_ssrc_downsampleStage2,
                audiolib_ssrc_upsampleStage1, audiolib_ssrc_upsampleStage2, filtCoeffSizeBytes,
                NULL); // NULL pFiltCoeffs

            currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
         } break;

         case 1070: {
            // Test for invalid stride_y in non-interleaved format
            // For non-interleaved, stride_y should equal inputSampleCount * sizeof(sampleDataType)
            AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
            badBufIn.dim_y                  = 2;                   // Valid numChannels
            badBufIn.dim_x                  = 256;                 // Valid inputSampleCount
            badBufIn.stride_y               = 128 * sizeof(float); // Invalid stride - should be 256 * sizeof(float)

            // Create a copy of initArgs with non-interleaved format
            AUDIOLIB_ssrc_InitArgs testArgs = initArgs;
            testArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
            testArgs.numChannels            = 2;
            testArgs.inputSampleCount       = 256;

            // Create valid output buffer params for non-interleaved
            AUDIOLIB_bufParams2D_t validBufOut = bufParamsOut;
            int outFrameLength   = AUDIOLIB_ssrc_getOutBufferLength(testArgs.inputSampleRate, testArgs.outputSampleRate,
                                                                    testArgs.inputSampleCount);
            validBufOut.dim_y    = testArgs.numChannels;
            validBufOut.dim_x    = outFrameLength;
            validBufOut.stride_y = outFrameLength * sizeof(float);

            status          = AUDIOLIB_ssrc_init_checkParams(handle, &badBufIn, &validBufOut, &testArgs);
            currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
         } break;

         default:
            // Skip tests that aren't defined
            break;
         }

         // Update the fail flag using the same pattern as cascadebiquad
         fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

         // Add test to profile using TI_profile_add_test
         sprintf(desc, "%s", "COVERAGE TEST");
         TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
      }

   cleanup:
      // Clean up using TI_align_free
      if (handle)
         free(handle);
      if (pIn)
         TI_align_free(pIn);
      if (pFiltCoeffs)
         TI_align_free(pFiltCoeffs);
      if (pOut)
         TI_align_free(pOut);
      if (pState)
         TI_align_free(pState);

#ifdef C7X
      // Free the kernel handles
      if (deinterleaveHandle)
         free(deinterleaveHandle);
      if (interleaveHandle)
         free(interleaveHandle);
      if (blkCopy2DHandle1)
         free(blkCopy2DHandle1);
      if (blkCopy2DHandle2)
         free(blkCopy2DHandle2);
      if (blkCopy2DHandle3)
         free(blkCopy2DHandle3);
      if (firHandle1)
         free(firHandle1);
      if (firHandle2)
         free(firHandle2);
#endif

      return fail;
   }

/* Main call for individual test projects */
#if !defined(__ONESHOTTEST) && !defined(RTL_TEST)
   int main()
   {
#ifdef ARM_A53
      int32_t status = SystemP_SUCCESS;
      /* System initialization */
      System_init();
      /* Board initialization */
      Board_init();
      /* Drivers initialization */
      Drivers_open();
      /* Open drivers */
      status = Board_driversOpen();
      DebugP_assert(status == SystemP_SUCCESS);
#endif
      int fail = 1;

      uint32_t profile[1500 * 3];

#ifdef C7X
      AUDIOLIB_TEST_init();
#endif

      fail = test_main(&profile[0]);

#if !defined(NO_PRINTF)
      if (fail == 0)
         AUDIOLIB_DEBUG_PRINT("Test Pass!\r\n");
      else
         AUDIOLIB_DEBUG_PRINT("Test Fail!\r\n");

      int i;
      for (i = 0; i < test_cases; i++) {
         AUDIOLIB_DEBUG_PRINT("Test %4d: Cold Cycles = %8d, Warm Cycles = %8d, Warm Cycles WRB = "
                              "%8d\r\n",
                              i, profile[3 * i], profile[3 * i + 1], profile[3 * i + 2]);
      }
#endif

      fflush(stdout);

      // Run coverage tests
      fail = coverage_test_main();
      if (fail == 0) {
         AUDIOLIB_DEBUG_PRINT("Coverage Tests Pass!\n");
      }
      else {
         AUDIOLIB_DEBUG_PRINT("Coverage Tests Fail!\n");
      }
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
      return fail;
   }
#endif
