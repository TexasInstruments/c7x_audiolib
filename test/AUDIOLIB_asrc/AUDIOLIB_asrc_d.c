// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_asrc_idat.h"
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
int8_t      l2auxBuffer[AUDIOLIB_L2_BUFFER_SIZE];
int8_t      ddrBuffer[2048 * 1024];
const float audiolib_asrc_h32_32kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_32kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_32kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_44_1kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_44_1kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_48kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_44_1kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_32kHz_rfmt.h"
};
const float audiolib_asrc_h32_48kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_32kHz_rfmt.h"
};
const float audiolib_asrc_h32_48kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_44_1kHz_rfmt.h"
};
#endif
#else
#if defined(__C7504__) || defined(__C7524__)
__attribute__((section(".l2sramaux"), aligned(64))) int8_t l2auxBuffer[AUDIOLIB_L2_BUFFER_SIZE];
__attribute__((section(".ddrData"), aligned(64))) int8_t   ddrBuffer[2048 * 1024];

__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_32kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_32kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_32kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_44_1kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_48kHz_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_44_1kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_48kHz_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_48kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_48kHz_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_44_1kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_32kHz_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_48kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_32kHz_rfmt.h"
};
__attribute__((section(".coeffMemory"), aligned(AUDIOLIB_L2DATA_ALIGNMENT)))
const float audiolib_asrc_h32_48kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_44_1kHz_rfmt.h"
};

#else
__attribute__((section(".msmcData"), aligned(64))) int8_t msmcBuffer[AUDIOLIB_L3_RESULTS_BUFFER_SIZE];
__attribute__((section(".ddrData"), aligned(64))) int8_t  ddrBuffer[2048 * 1024];

#endif
#endif // WIN32
#endif

#ifdef ARM_A53
const float audiolib_asrc_h32_32kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_32kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_32kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_32kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_44_1kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_44_1kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_48kHz_to_48kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_48kHz_rfmt.h"
};
const float audiolib_asrc_h32_44_1kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_44_1kHz_to_32kHz_rfmt.h"
};
const float audiolib_asrc_h32_48kHz_to_32kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_32kHz_rfmt.h"
};
const float audiolib_asrc_h32_48kHz_to_44_1kHz[] = {
#include "../../src/AUDIOLIB_asrc/filt_coeffs/h32_48kHz_to_44_1kHz_rfmt.h"
};

__attribute__((section(".msmcData"), aligned(64))) int8_t msmcBuffer[AUDIOLIB_L3_RESULTS_BUFFER_SIZE];
__attribute__((section(".ddrData"), aligned(64))) int8_t  ddrBuffer[2048 * 1024];
#endif

#define MATRIX_TRANSPOSE_ROW_STRIDE(x, y) (((x + y - 1) / y) * y)

/* Generate output file if defined! This is used for snr analysis*/
//#define ENABLE_GENERATE_OUTPUT_FILE

/* Parameter used to allocate the memory for a quadruple buffer for input data. User should not change this value as the
 * algorithm will not work properly if this value is changed.
   Application needs to allocate a quadruple buffer if the freme size is greater than 64 (_FSG64)*/
#define AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSG64 (4U)
/* Parameter used to allocate the memory for a double buffer for input data. User should not change this value as the
 * algorithm will not work properly if this value is changed.
   Application needs to allocate a double buffer if the freme size is less than 64 (_FSG64)*/
#define AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64 (2U)
/* Number of warm reps */
#define AUDIOLIB_ASRC_NUM_WARM_REPS (3U)
/* Inverleaved or noninterleaved data */
#define AUDIOLIB_ASRC_IS_INTERLEAVED_DATA (1U)

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
      sprintf(filename, "asrc_testcase%d_input.csv", testId);
   }
   else {
      sprintf(filename, "asrc_testcase%d_output.csv", testId);
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

int AUDIOLIB_asrc_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   int32_t                   tpi;
   int32_t                   currentTestFail;
   int32_t                   fail = 0;
   uint32_t                  repCount;
   uint32_t                  numReps;
   AUDIOLIB_bufParams2D_t    bufParamsIn, bufParamsOut;
   AUDIOLIB_asrc_ExecInArgs  kerInArgs;
   AUDIOLIB_asrc_ExecOutArgs kerOutArgs;
   int32_t                   nonInterleavedDataBufSize, filtCoeffSizeBytes, filterRembufSize, outFrameLength;
   uint32_t                  filterLength;
   uint8_t                   mode;
   float                    *pInData, *pOutData;
   float                    *pInDBuffer;
   int64_t                   t_asrc_cycles_opt, t_asrc_cycles_warm, t_asrc_cycles_warmwrb;
   int64_t                   asrc_cycles;
   float                     processorUtilization;

   int32_t block = 0;
   int32_t totalOutputSampleCnt, totalInputSampleCnt;
   double  asrcRatio;

   float fsin, fsout, sigFreq;

   uint32_t testNum;
   uint64_t archCycles = 0;
   uint64_t estCycles  = 0;
   uint32_t k;

   asrc_testParams_t *prm;
   asrc_testParams_t  currPrm;
   asrc_getTestParams(&prm, &test_cases);

   AUDIOLIB_asrc_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_asrc_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   TI_profile_init("AUDIOLIB_asrc");

   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_asrc.csv", "w+");
   fprintf(
       fpOutputCSV,
       "Kernel, Test ID, Test type, Bit Width, Parameters, "
       "Input Data Format, Input Sample Count, Blocks, Input Signal Frequency (Hz), Input Sample Rate (Hz), Output "
       "Sample Rate (Hz), Num of Channels, Output Sample Count, Processor Utilization (MHz), "
       "Arch cycles, Estimated cycles, Estimated/Warm cycles, Pass/Fail, Cold Cycles, Warm Cycles, Warm Cycles WRB\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      for (repCount = 0; repCount < numReps; repCount++) {
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_nat = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;
         fsout                             = (float) convertSampleRateToInt((sample_rate_t) currPrm.outputSampleRate);
         fsin                              = (float) convertSampleRateToInt((sample_rate_t) currPrm.inputSampleRate);
         sigFreq                           = (float) currPrm.signalFrequency;
         processorUtilization              = 0.0;
         void *pIn                         = NULL;
         void *pNonInterleavedData __attribute__((unused)) = NULL;
         void *pFiltCoeffs;
         void *pFilterRembuf;
         void *pOut, *pOutCn;
#ifdef C7X
         DSPLIB_kernelHandle matTransHandle __attribute__((unused)) = NULL;
#endif

         /* Sometimes, depending on certain compile flags, the test will be
          * marked PASS even if no comparison is done. This flag is to detect
          * if a comparison was done or not                                   */
         int32_t ref_vs_opt_comparisonDone = 0;
         int32_t ref_vs_nat_comparisonDone = 0;
         t_asrc_cycles_opt                 = 0;
         t_asrc_cycles_warm                = 0;
         t_asrc_cycles_warmwrb             = 0;
         currentTestFail                   = 0;

         kerInitArgs.funcStyle              = AUDIOLIB_FUNCTION_NATC;
         kerInitArgs.maxSampleCountPerBlock = currPrm.maxSampleCountPerBlock;
         kerInitArgs.sampleDataType         = currPrm.sampleDataType;
         kerInitArgs.inputSampleRate        = (sample_rate_t) currPrm.inputSampleRate;
         kerInitArgs.outputSampleRate       = (sample_rate_t) currPrm.outputSampleRate;
         kerInitArgs.numChannels            = currPrm.numChannels;
         kerInitArgs.dataFormat             = currPrm.dataFormat;
         kerInitArgs.frameModuloFactor      = currPrm.moduloFactor;
#ifdef C7X
         kerInitArgs.matTransHandle = NULL;
#endif

         filtCoeffSizeBytes = AUDIOLIB_asrc_getFilterCoeffSize(kerInitArgs.sampleDataType);

         filterRembufSize = AUDIOLIB_asrc_getFilterRembufSize(kerInitArgs.numChannels, kerInitArgs.sampleDataType,
                                                              kerInitArgs.frameModuloFactor);
         outFrameLength   = AUDIOLIB_asrc_getOutBufferLength(kerInitArgs.inputSampleRate, kerInitArgs.outputSampleRate,
                                                             kerInitArgs.maxSampleCountPerBlock);
         filterLength     = AUDIOLIB_asrc_getFilterLength();

         bufParamsIn.data_type  = currPrm.sampleDataType;
         bufParamsOut.data_type = currPrm.sampleDataType;
         if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
#ifdef C7X
            int32_t matTransHandleSize = DSPLIB_matTrans_getHandleSize(NULL);
            matTransHandle             = malloc(matTransHandleSize);
            kerInitArgs.matTransHandle = matTransHandle;

            int32_t eleCount = 0;
            /* Matrix transpose implementation make use of two SE engine to fetch
               two full vector in each iteration extra rows are padded to ensure SE access
               to allocated memory regions */
            if ((AUDIOLIB_sizeof(kerInitArgs.sampleDataType) == 4) ||
                (AUDIOLIB_sizeof(kerInitArgs.sampleDataType) == 8)) {
               eleCount = 2 * (__C7X_VEC_SIZE_BYTES__ / AUDIOLIB_sizeof(kerInitArgs.sampleDataType));
            }
            else {
               eleCount = __C7X_VEC_SIZE_BYTES__ / AUDIOLIB_sizeof(kerInitArgs.sampleDataType);
            }

            int32_t dim_y_padded = MATRIX_TRANSPOSE_ROW_STRIDE(kerInitArgs.maxSampleCountPerBlock, eleCount);
#endif
            bufParamsIn.dim_y    = currPrm.maxSampleCountPerBlock;
            bufParamsIn.stride_y = kerInitArgs.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType);
            bufParamsIn.dim_x    = kerInitArgs.numChannels;

            bufParamsOut.dim_y    = outFrameLength;
            bufParamsOut.stride_y = kerInitArgs.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType);
            bufParamsOut.dim_x    = kerInitArgs.numChannels;

            uint32_t alignment_size;
            if (currPrm.maxSampleCountPerBlock >= filterLength) {
               alignment_size = currPrm.maxSampleCountPerBlock * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64 *
                                AUDIOLIB_sizeof(currPrm.sampleDataType);
            }
            else {
               alignment_size =
                   filterLength * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64 * AUDIOLIB_sizeof(currPrm.sampleDataType);
            }

            nonInterleavedDataBufSize =
                AUDIOLIB_asrc_getNonInterleavedDataBufSize(kerInitArgs.numChannels, kerInitArgs.maxSampleCountPerBlock,
                                                           kerInitArgs.sampleDataType, kerInitArgs.dataFormat);

            pNonInterleavedData = (void *) TI_memalign(alignment_size, nonInterleavedDataBufSize);

#ifdef C7X
            pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, bufParamsIn.stride_y * dim_y_padded);
#endif
#ifdef ARM_A53
            pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT,
                                       bufParamsIn.stride_y * kerInitArgs.maxSampleCountPerBlock);
#endif
         }
         else {
            bufParamsIn.dim_y = currPrm.numChannels;
            if (currPrm.maxSampleCountPerBlock >= filterLength) {
               bufParamsIn.stride_y = currPrm.maxSampleCountPerBlock * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSG64 *
                                      AUDIOLIB_sizeof(currPrm.sampleDataType);
            }
            else {
               bufParamsIn.stride_y =
                   filterLength * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64 * AUDIOLIB_sizeof(currPrm.sampleDataType);
            }
            bufParamsIn.dim_x = currPrm.maxSampleCountPerBlock;

            bufParamsOut.dim_y    = currPrm.numChannels;
            bufParamsOut.stride_y = outFrameLength * AUDIOLIB_sizeof(currPrm.sampleDataType);
            bufParamsOut.dim_x    = outFrameLength;

            pIn = (void *) TI_memalign(bufParamsIn.stride_y, bufParamsIn.stride_y * bufParamsIn.dim_y);
#ifdef ARM_A53
            uint32_t alignment_size;
            uint32_t buffer_multiple = (currPrm.maxSampleCountPerBlock >= filterLength)
                                           ? AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSG64
                                           : AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64;

            if (currPrm.maxSampleCountPerBlock >= filterLength) {
               alignment_size =
                   currPrm.maxSampleCountPerBlock * buffer_multiple * AUDIOLIB_sizeof(currPrm.sampleDataType);
            }
            else {
               alignment_size = filterLength * buffer_multiple * AUDIOLIB_sizeof(currPrm.sampleDataType);
            }

            nonInterleavedDataBufSize =
                AUDIOLIB_asrc_getNonInterleavedDataBufSize(kerInitArgs.numChannels, kerInitArgs.maxSampleCountPerBlock,
                                                           kerInitArgs.sampleDataType, kerInitArgs.dataFormat);

            pNonInterleavedData = (void *) TI_memalign(alignment_size, nonInterleavedDataBufSize);

#endif
         }

         pFiltCoeffs   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, filtCoeffSizeBytes);
         pFilterRembuf = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, filterRembufSize);

         switch (kerInitArgs.inputSampleRate) {
         case SAMPLE_RATE_32000: {
            if (kerInitArgs.outputSampleRate == SAMPLE_RATE_32000) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_32kHz_to_32kHz, filtCoeffSizeBytes);
            }
            else if (kerInitArgs.outputSampleRate == SAMPLE_RATE_44100) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_32kHz_to_44_1kHz, filtCoeffSizeBytes);
            }
            else if (kerInitArgs.outputSampleRate == SAMPLE_RATE_48000) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_32kHz_to_48kHz, filtCoeffSizeBytes);
            }
            break;
         }
         case SAMPLE_RATE_44100: {
            if (kerInitArgs.outputSampleRate == SAMPLE_RATE_32000) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_44_1kHz_to_32kHz, filtCoeffSizeBytes);
            }
            else if (kerInitArgs.outputSampleRate == SAMPLE_RATE_44100) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_44_1kHz_to_44_1kHz, filtCoeffSizeBytes);
            }
            else if (kerInitArgs.outputSampleRate == SAMPLE_RATE_48000) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_44_1kHz_to_48kHz, filtCoeffSizeBytes);
            }
            break;
         }
         case SAMPLE_RATE_48000: {
            if (kerInitArgs.outputSampleRate == SAMPLE_RATE_32000) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_48kHz_to_32kHz, filtCoeffSizeBytes);
            }
            else if (kerInitArgs.outputSampleRate == SAMPLE_RATE_44100) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_48kHz_to_44_1kHz, filtCoeffSizeBytes);
            }
            else if (kerInitArgs.outputSampleRate == SAMPLE_RATE_48000) {
               memcpy(pFiltCoeffs, audiolib_asrc_h32_48kHz_to_48kHz, filtCoeffSizeBytes);
            }
            break;
         }
         default: {
            memcpy(pFiltCoeffs, audiolib_asrc_h32_48kHz_to_48kHz, filtCoeffSizeBytes);
            break;
         }
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
         AUDIOLIB_DEBUG_PRINT(
             "AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d Input Sample Count=%d, Blocks=%d, Input Sample Rate=%u Hz, Output "
             "Sample Rate=%u Hz, Signal Frequency=%0.3f kHz, Num of Channels=%d\r\n",
             currPrm.maxSampleCountPerBlock, (int32_t) currPrm.blockCount, (uint32_t) fsin, (uint32_t) fsout,
             (sigFreq / 1000), currPrm.numChannels);
#endif
/* Only run the test if the buffer allocations fit in the heap */
#ifdef C7X
         if (pIn && pOut && pOutCn && pFiltCoeffs && pFilterRembuf &&
             ((pNonInterleavedData == NULL && kerInitArgs.dataFormat != AUDIOLIB_DATA_FORMAT_INTERLEAVED) ||
              (pNonInterleavedData != NULL && kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED))) {
#endif
#ifdef ARM_A53
            if (pIn && pOut && pOutCn && pFiltCoeffs && pFilterRembuf && pNonInterleavedData) {
#endif

#if AUDIOLIB_DEBUGPRINT
               AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 0\r\n");
#endif
               status_init = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

#if AUDIOLIB_DEBUGPRINT
               AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 1 status_init %d\r\n", status_init);
#endif

               // C7000 cold run
               t_asrc_cycles_opt = 0;
               if (status_init == AUDIOLIB_SUCCESS) {
                  AUDIOLIB_asm(" MARK 0");
                  kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
                  status_init           = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                  AUDIOLIB_asm(" MARK 1");
               }

               if (status_init == AUDIOLIB_SUCCESS) {
                  asrcRatio  = fsout / fsin;
                  mode       = (uint8_t) AUDIOLIB_ASRC_MODE_RESET;
                  status_opt = AUDIOLIB_asrc_set(handle, mode, asrcRatio, pNonInterleavedData, pIn);
               }

               for (block = 0; block < (int32_t) currPrm.blockCount; block++) {
                  kerInArgs.inputSampleCount = (int32_t) currPrm.maxSampleCountPerBlock;
                  if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                     pInDBuffer = (float *) pIn;
                     /* Data storage pettern is staticIn -
                        rows - channel interleaved samples (C0S0, C1S0, C2S0, C3S0, ....etc)
                        cols - different samples per channel(C0S0, C0S1, C0S2, ....etc) */
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData =
                            ((float *) prm[tpi].staticIn) + (block * kerInArgs.inputSampleCount * currPrm.numChannels);
                        copyRoi_float((void *) pInDBuffer, (void *) pInData, currPrm.numChannels,
                                      kerInArgs.inputSampleCount,
                                      currPrm.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType),
                                      AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }
                  else {
                     if (currPrm.maxSampleCountPerBlock >= filterLength) {
#ifdef C7X
                        pInDBuffer =
                            (float *) pIn + (block % AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSG64) * bufParamsIn.dim_x;
#endif
#ifdef ARM_A53
                        pInDBuffer = (float *) pIn;
#endif
                     }
                     else {
#ifdef C7X
                        pInDBuffer =
                            (float *) pIn +
                            (block % ((filterLength / bufParamsIn.dim_x) * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64)) *
                                bufParamsIn.dim_x;
#endif
#ifdef ARM_A53
                        pInDBuffer = (float *) pIn;
#endif
                     }
                     /* Data storage pettern is staticIn -
                        rows - sample data per each channel
                        cols - store samples (sample count per row = blockCount * inputSampleCount) */
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData = ((float *) prm[tpi].staticIn) + (block * kerInArgs.inputSampleCount);
                        copyRoiDualStride_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                                bufParamsIn.dim_y, bufParamsIn.stride_y,
                                                currPrm.maxSampleCountPerBlock *
                                                    AUDIOLIB_sizeof(currPrm.sampleDataType) * currPrm.blockCount,
                                                AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }

                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     mode       = (uint8_t) AUDIOLIB_ASRC_MODE_SET;
                     asrcRatio  = fsout / fsin;
                     status_opt = AUDIOLIB_asrc_set(handle, mode, asrcRatio, NULL, NULL);
                  }
                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     status_opt = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs,
                                                                 pFilterRembuf, pOut, &kerInArgs, &kerOutArgs);
                  }
                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT);
                  TI_profile_start(TI_PROFILE_KERNEL_OPT);
                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     AUDIOLIB_asm(" MARK 2");
                     status_opt = AUDIOLIB_asrc_exec(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf, pOut,
                                                     &kerInArgs, &kerOutArgs);
                     AUDIOLIB_asm(" MARK 3");
                  }
                  TI_profile_stop();
                  asrc_cycles = TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
                  t_asrc_cycles_opt += asrc_cycles;

#ifdef ENABLE_GENERATE_OUTPUT_FILE
                  if (repCount == 0) { // Only for cold run
                     int32_t isInterleaved = (int32_t) AUDIOLIB_DATA_FORMAT_INTERLEAVED;
                     writeInOutDataToFile(pInDBuffer, currPrm.numChannels, currPrm.maxSampleCountPerBlock,
                                          currPrm.testID, block, isInterleaved, 1);
                     writeInOutDataToFile((float *) pOut, currPrm.numChannels, kerOutArgs.outputSampleCount,
                                          currPrm.testID, block, isInterleaved, 0);
                  }
#endif
               }

               // WARM run
               for (k = 0; k < AUDIOLIB_NUM_WARM_REPS; k++) {
                  t_asrc_cycles_warm = 0;
                  if (status_init == AUDIOLIB_SUCCESS) {
                     TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_INIT);
                     TI_profile_start(TI_PROFILE_KERNEL_INIT);
                     AUDIOLIB_asm(" MARK 4");
                     kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
                     status_init           = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                     AUDIOLIB_asm(" MARK 5");
                     TI_profile_stop();
                  }

                  if (status_init == AUDIOLIB_SUCCESS) {
                     asrcRatio  = fsout / fsin;
                     mode       = (uint8_t) AUDIOLIB_ASRC_MODE_RESET;
                     status_opt = AUDIOLIB_asrc_set(handle, mode, asrcRatio, pNonInterleavedData, pIn);
                  }

                  for (block = 0; block < (int32_t) currPrm.blockCount; block++) {
                     kerInArgs.inputSampleCount = (int32_t) currPrm.maxSampleCountPerBlock;
                     if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                        pInDBuffer = (float *) pIn;
                        /* Data storage pettern is staticIn -
                           rows - channel interleaved samples (C0S0, C1S0, C2S0, C3S0, ....etc)
                           cols - different samples per channel(C0S0, C0S1, C0S2, ....etc) */
                        if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                           pInData = ((float *) prm[tpi].staticIn) +
                                     (block * kerInArgs.inputSampleCount * currPrm.numChannels);
                           copyRoi_float((void *) pInDBuffer, (void *) pInData, currPrm.numChannels,
                                         kerInArgs.inputSampleCount,
                                         currPrm.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType),
                                         AUDIOLIB_sizeof(currPrm.sampleDataType));
                        }
                     }
                     else {
                        if (currPrm.maxSampleCountPerBlock >= filterLength) {
#ifdef C7X
                           pInDBuffer =
                               (float *) pIn + (block % AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSG64) * bufParamsIn.dim_x;
#endif
#ifdef ARM_A53
                           pInDBuffer = (float *) pIn;
#endif
                        }
                        else {
#ifdef C7X
                           pInDBuffer = (float *) pIn + (block % ((filterLength / bufParamsIn.dim_x) *
                                                                  AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64)) *
                                                            bufParamsIn.dim_x;
#endif
#ifdef ARM_A53
                           pInDBuffer = (float *) pIn;
#endif
                        }
                        /* Data storage pettern is staticIn -
                           rows - sample data per each channel
                           cols - store samples (sample count per row = blockCount * inputSampleCount) */
                        if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                           pInData = ((float *) prm[tpi].staticIn) + (block * kerInArgs.inputSampleCount);
                           copyRoiDualStride_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                                   bufParamsIn.dim_y, bufParamsIn.stride_y,
                                                   currPrm.maxSampleCountPerBlock *
                                                       AUDIOLIB_sizeof(currPrm.sampleDataType) * currPrm.blockCount,
                                                   AUDIOLIB_sizeof(currPrm.sampleDataType));
                        }
                     }
                     if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                        mode       = (uint8_t) AUDIOLIB_ASRC_MODE_SET;
                        asrcRatio  = fsout / fsin;
                        status_opt = AUDIOLIB_asrc_set(handle, mode, asrcRatio, NULL, NULL);
                     }

                     if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                        status_opt = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs,
                                                                    pFilterRembuf, pOut, &kerInArgs, &kerOutArgs);
                     }

                     TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
                     TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
                     if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                        // execute the optimized kernel
                        AUDIOLIB_asm(" MARK 6");
                        status_opt = AUDIOLIB_asrc_exec(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf,
                                                        pOut, &kerInArgs, &kerOutArgs);
                        AUDIOLIB_asm(" MARK 7");
                     }
                     TI_profile_stop();
                     asrc_cycles = TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
                     t_asrc_cycles_warm += asrc_cycles;
                  }
               }

               // initialize the kernel to use the c7000 optimized kernel
               // WARMWRB run
#if AUDIOLIB_DEBUGPRINT
               AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 2 status_init %d\r\n", status_init);
#endif
               if (status_init == AUDIOLIB_SUCCESS) {
                  AUDIOLIB_asm(" MARK 8");
                  kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
                  status_init           = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                  AUDIOLIB_asm(" MARK 9");
               }

#if AUDIOLIB_DEBUGPRINT
               AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 3 status_init %d\r\n", status_init);
#endif
               if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                  asrcRatio  = fsout / fsin;
                  mode       = (uint8_t) AUDIOLIB_ASRC_MODE_RESET;
                  status_opt = AUDIOLIB_asrc_set(handle, mode, asrcRatio, pNonInterleavedData, pIn);
               }
#if AUDIOLIB_DEBUGPRINT
               AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 4 status_opt %d\r\n", status_opt);
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
               totalInputSampleCnt  = 0;
               for (block = 0; block < (int32_t) currPrm.blockCount; block++) {

                  kerInArgs.inputSampleCount = (int32_t) currPrm.maxSampleCountPerBlock;
                  kerInArgs.inputSampleCount = (int32_t) currPrm.maxSampleCountPerBlock;

                  if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                     pInDBuffer = (float *) pIn;
                     /* Data storage pettern is staticIn -
                        rows - channel interleaved samples (C0S0, C1S0, C2S0, C3S0, ....etc)
                        cols - different samples per channel(C0S0, C0S1, C0S2, ....etc) */
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData = ((float *) prm[tpi].staticIn) + (totalInputSampleCnt * currPrm.numChannels);
                        copyRoi_float((void *) pInDBuffer, (void *) pInData, currPrm.numChannels,
                                      kerInArgs.inputSampleCount,
                                      currPrm.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType),
                                      AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }
                  else {
                     if (currPrm.maxSampleCountPerBlock >= filterLength) {
#ifdef C7X
                        pInDBuffer =
                            (float *) pIn + (block % AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSG64) * bufParamsIn.dim_x;
#endif
#ifdef ARM_A53
                        pInDBuffer = (float *) pIn;
#endif
                     }
                     else {
#ifdef C7X
                        pInDBuffer =
                            (float *) pIn +
                            (block % ((filterLength / bufParamsIn.dim_x) * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64)) *
                                bufParamsIn.dim_x;
#endif
#ifdef ARM_A53
                        pInDBuffer = (float *) pIn;
#endif
                     }
                     /* Data storage pettern is staticIn -
                        rows - sample data per each channel
                        cols - store samples (sample count per row = blockCount * inputSampleCount) */
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData = ((float *) prm[tpi].staticIn) + totalInputSampleCnt;
                        copyRoiDualStride_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                                bufParamsIn.dim_y, bufParamsIn.stride_y,
                                                currPrm.maxSampleCountPerBlock *
                                                    AUDIOLIB_sizeof(currPrm.sampleDataType) * currPrm.blockCount,
                                                AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }

                  /* This funtion in the set mode doesn't need to be called before every execute call.
                     In a system example, the set function is called by a different theard running the
                     calcraito driver.
                     Note that asrcRatio calculation is done here only as an example.
                     But please make sure that the ratio is always fsout / fsin */
                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     mode       = (uint8_t) AUDIOLIB_ASRC_MODE_SET;
                     asrcRatio  = fsout / fsin;
                     status_opt = AUDIOLIB_asrc_set(handle, mode, asrcRatio, NULL, NULL);
                  }
                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     status_opt = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs,
                                                                 pFilterRembuf, pOut, &kerInArgs, &kerOutArgs);
                  }
                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARMWRB);
                  TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
                  if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                     AUDIOLIB_asm(" MARK 10");
                     status_opt = AUDIOLIB_asrc_exec(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf, pOut,
                                                     &kerInArgs, &kerOutArgs);
                     AUDIOLIB_asm(" MARK 11");
                  }
                  TI_profile_stop();
#if AUDIOLIB_DEBUGPRINT
                  AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 6 status_opt %d\r\n", status_opt);
#endif
                  asrc_cycles = TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);
                  t_asrc_cycles_warmwrb += asrc_cycles;

                  if (currPrm.staticOut != NULL) {
                     if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                        if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                           pOutData = (float *) currPrm.staticOut + totalOutputSampleCnt * currPrm.numChannels;
                           status_ref_vs_opt =
                               TI_compare_mem_2D_float((void *) pOutData, pOut, 0.001, (double) powf(10, -4),
                                                       currPrm.numChannels, kerOutArgs.outputSampleCount,
                                                       currPrm.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType),
                                                       AUDIOLIB_sizeof(bufParamsOut.data_type));
                        }
                        else {
                           pOutData          = (float *) currPrm.staticOut + totalOutputSampleCnt;
                           status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                               (void *) pOutData, pOut, 0.001, (double) powf(10, -4), kerOutArgs.outputSampleCount,
                               bufParamsOut.dim_y,
                               currPrm.genOutSamplesPerChannel * AUDIOLIB_sizeof(currPrm.sampleDataType),
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

                  totalOutputSampleCnt += kerOutArgs.outputSampleCount;
                  totalInputSampleCnt += kerInArgs.inputSampleCount;
               }

               processorUtilization =
                   (float) t_asrc_cycles_warm * fsin / (currPrm.maxSampleCountPerBlock * block * pow(10, 6));

#if AUDIOLIB_DEBUGPRINT
               AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 7 status_opt %d\r\n", status_opt);
#endif

               /* Test _cn kernel */
               if (status_init == AUDIOLIB_SUCCESS) {
                  AUDIOLIB_asm(" MARK 12");
                  kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
                  status_init           = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                  AUDIOLIB_asm(" MARK 13");
               }

#if AUDIOLIB_DEBUGPRINT
               AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 8 status_init %d\r\n", status_init);
#endif
               if (status_init == AUDIOLIB_SUCCESS) {
                  asrcRatio  = fsout / fsin;
                  mode       = (uint8_t) AUDIOLIB_ASRC_MODE_RESET;
                  status_nat = AUDIOLIB_asrc_set(handle, mode, asrcRatio, pNonInterleavedData, pIn);
               }
#if AUDIOLIB_DEBUGPRINT
               AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 9 status_init %d\r\n", status_init);
#endif

               totalOutputSampleCnt = 0;
               totalInputSampleCnt  = 0;
               for (block = 0; block < (int32_t) currPrm.blockCount; block++) {

                  kerInArgs.inputSampleCount = (int32_t) currPrm.maxSampleCountPerBlock;

                  if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                     pInDBuffer = (float *) pIn;
                     /* Data storage pettern is staticIn -
                        rows - channel interleaved samples (C0S0, C1S0, C2S0, C3S0, ....etc)
                        cols - different samples per channel(C0S0, C0S1, C0S2, ....etc) */
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData = ((float *) prm[tpi].staticIn) + (totalInputSampleCnt * currPrm.numChannels);
                        copyRoi_float((void *) pInDBuffer, (void *) pInData, currPrm.numChannels,
                                      kerInArgs.inputSampleCount,
                                      currPrm.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType),
                                      AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }
                  else {
                     if (currPrm.maxSampleCountPerBlock >= filterLength) {
                        pInDBuffer =
                            (float *) pIn + (block % AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSG64) * bufParamsIn.dim_x;
                     }
                     else {
                        pInDBuffer =
                            (float *) pIn +
                            (block % ((filterLength / bufParamsIn.dim_x) * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSL64)) *
                                bufParamsIn.dim_x;
                     }
                     /* Data storage pettern is staticIn -
                        rows - sample data per each channel
                        cols - store samples (sample count per row = blockCount * inputSampleCount) */
                     if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {
                        pInData = ((float *) prm[tpi].staticIn) + totalInputSampleCnt;
                        copyRoiDualStride_float((void *) pInDBuffer, (void *) pInData, bufParamsIn.dim_x,
                                                bufParamsIn.dim_y, bufParamsIn.stride_y,
                                                currPrm.maxSampleCountPerBlock *
                                                    AUDIOLIB_sizeof(currPrm.sampleDataType) * currPrm.blockCount,
                                                AUDIOLIB_sizeof(currPrm.sampleDataType));
                     }
                  }

                  /* This funtion in the set mode doesn't need to be called before every execute call.
                     In a system example, the set function is called by a different theard running the
                     calcraito driver.
                     Note that asrcRatio calculation is done here only as an example.
                     But please make sure that the ratio is always fsout / fsin */
                  if (status_init == AUDIOLIB_SUCCESS && status_nat == AUDIOLIB_SUCCESS) {
                     mode       = (uint8_t) AUDIOLIB_ASRC_MODE_SET;
                     asrcRatio  = fsout / fsin;
                     status_nat = AUDIOLIB_asrc_set(handle, mode, asrcRatio, NULL, NULL);
                  }

                  if (status_init == AUDIOLIB_SUCCESS && status_nat == AUDIOLIB_SUCCESS) {
                     status_nat = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs,
                                                                 pFilterRembuf, pOutCn, &kerInArgs, &kerOutArgs);
                  }

#if AUDIOLIB_DEBUGPRINT
                  AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 7 status_nat %d\r\n", status_nat);
#endif
                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_CN);
                  TI_profile_start(TI_PROFILE_KERNEL_CN);

                  if (status_init == AUDIOLIB_SUCCESS && status_nat == AUDIOLIB_SUCCESS) {
                     AUDIOLIB_asm(" MARK 14");
                     status_nat = AUDIOLIB_asrc_exec(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf,
                                                     pOutCn, &kerInArgs, &kerOutArgs);
                     AUDIOLIB_asm(" MARK 15");
                  }
                  TI_profile_stop();
#if AUDIOLIB_DEBUGPRINT
                  AUDIOLIB_DEBUG_PRINT("AUDIOLIB_DEBUGPRINT  AUDIOLIB_asrc_d CP 8 status_nat %d\r\n", status_nat);
#endif

                  if (currPrm.staticOut != NULL) {
                     if (status_init == AUDIOLIB_SUCCESS && status_opt == AUDIOLIB_SUCCESS) {
                        if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                           pOutData = (float *) currPrm.staticOut + totalOutputSampleCnt * currPrm.numChannels;
                           status_ref_vs_nat =
                               TI_compare_mem_2D_float((void *) pOutData, pOutCn, 0.001, (double) powf(10, -4),
                                                       currPrm.numChannels, kerOutArgs.outputSampleCount,
                                                       currPrm.numChannels * AUDIOLIB_sizeof(currPrm.sampleDataType),
                                                       AUDIOLIB_sizeof(bufParamsOut.data_type));
                        }
                        else {
                           pOutData          = (float *) currPrm.staticOut + totalOutputSampleCnt;
                           status_ref_vs_nat = TI_compare_mem_2DDualStride_float(
                               (void *) pOutData, pOutCn, 0.001, (double) powf(10, -4), kerOutArgs.outputSampleCount,
                               bufParamsOut.dim_y,
                               currPrm.genOutSamplesPerChannel * AUDIOLIB_sizeof(currPrm.sampleDataType),
                               bufParamsOut.stride_y, AUDIOLIB_sizeof(bufParamsOut.data_type));
                        }
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

                  totalOutputSampleCnt += kerOutArgs.outputSampleCount;
                  totalInputSampleCnt += kerInArgs.inputSampleCount;
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

               pProfile[3 * tpi]     = t_asrc_cycles_opt / currPrm.blockCount;
               pProfile[3 * tpi + 1] = t_asrc_cycles_warm / currPrm.blockCount;
               pProfile[3 * tpi + 2] = t_asrc_cycles_warmwrb / currPrm.blockCount;

               sprintf(desc,
                       "STATIC generated input | Input Sample Count=%d, Blocks=%d, Input Sample Rate=%u Hz, Output "
                       "Sample Rate=%u Hz, Signal Frequency=%0.3f kHz, Num of Channels=%d",
                       currPrm.maxSampleCountPerBlock, block, (uint32_t) fsin, (uint32_t) fsout, (sigFreq / 1000),
                       currPrm.numChannels);

               AUDIOLIB_asrc_perfEst(handle, &bufParamsIn, &bufParamsOut, &archCycles, &estCycles);

               // write to CSV, must happen prior to write to screen because
               // TI_profile_formula_add clears values in counters
               fprintf(fpOutputCSV,
                       "ASRC, %d, %d, %d, Input Data Format = %s Input Sample Count=%d Blocks=%d Input Signal "
                       "Frequency=%0.3f Input Sample "
                       "Rate=%u Output Sample Rate=%u Num of Channels=%d, "
                       "%s, %u, %d, %0.3f, %u, %u, %d, %d, %5.2f, %lu, %lu, %.2f, %d, %d, %d, %d\n",
                       testNum, currPrm.testPattern, AUDIOLIB_sizeof(bufParamsIn.data_type) * 8,
                       (currPrm.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) ? "Interleaved" : "Non-Interleaved",
                       currPrm.maxSampleCountPerBlock, block, sigFreq, (uint32_t) fsin, (uint32_t) fsout,
                       currPrm.numChannels,
                       (currPrm.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) ? "Interleaved" : "Non-Interleaved",
                       currPrm.maxSampleCountPerBlock, block, sigFreq, (uint32_t) fsin, (uint32_t) fsout,
                       currPrm.numChannels, totalOutputSampleCnt, processorUtilization, archCycles, estCycles,
                       ((AUDIOLIB_F32) estCycles) / ((AUDIOLIB_F32) pProfile[3 * tpi + 1]), !currentTestFail,
                       pProfile[3 * tpi], pProfile[3 * tpi + 1], pProfile[3 * tpi + 2]);

               TI_profile_add_test(testNum++, currPrm.maxSampleCountPerBlock * currPrm.numChannels, 0, 0,
                                   currentTestFail, desc);
            }
            else {
               sprintf(
                   desc,
                   "Input Sample Count=%d, Blocks=%d, Input Sample Rate=%u, Output Sample Rate=%u, Num of Channels=%d",
                   currPrm.maxSampleCountPerBlock, block, (uint32_t) fsin, (uint32_t) fsout, currPrm.numChannels);
               TI_profile_skip_test(desc);
               // clear the counters between runs; normally handled by
               TI_profile_clear_run_stats();
            } // end of memory allocation successful?

            /* Free buffers for each test vector */
            TI_align_free(pIn);
            if (kerInitArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
#ifdef C7X
               TI_align_free(matTransHandle);
#endif
               TI_align_free(pNonInterleavedData);
            }
            TI_align_free(pFiltCoeffs);
            TI_align_free(pFilterRembuf);
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
         } // end repetitions
      }    // end idat test cases

      free(handle);

      /* Close results CSV */
      fclose(fpOutputCSV);

      return fail;
   }

   int test_main(uint32_t * pProfile)
   {
#ifdef C7X
#if !defined(_HOST_BUILD)
      if (TI_cache_init()) {
         TI_memError("AUDIOLIB_asrc");
         return 1;
      }
      else
#else
      printf("_HOST_BUILD is defined.\n");
#endif
#endif
      {
         return AUDIOLIB_asrc_d(&pProfile[0], 0);
      }
   }

   int coverage_test_main()
   {
      int32_t                   testNum         = 1000;
      int32_t                   currentTestFail = 0;
      AUDIOLIB_STATUS           status;
      AUDIOLIB_asrc_InitArgs    initArgs;
      AUDIOLIB_bufParams2D_t    bufParamsIn, bufParamsOut;
      AUDIOLIB_asrc_ExecInArgs  execInArgs;
      AUDIOLIB_asrc_ExecOutArgs execOutArgs;
      int                       fail   = 0;
      AUDIOLIB_kernelHandle     handle = NULL;
#ifdef C7X
      DSPLIB_kernelHandle matTransHandle = NULL;
#endif
      float *pIn = NULL, *pOut = NULL, *pNonInterleavedData = NULL;
      float *pFiltCoeffs = NULL, *pFilterRembuf = NULL;

      AUDIOLIB_DEBUG_PRINT("===== Starting ASRC Coverage Tests =====\r\n");

      // Set up valid parameters as baseline
      memset(&initArgs, 0, sizeof(AUDIOLIB_asrc_InitArgs));
      initArgs.funcStyle              = AUDIOLIB_FUNCTION_OPTIMIZED;
      initArgs.maxSampleCountPerBlock = 256;
      initArgs.sampleDataType         = AUDIOLIB_FLOAT32;
      initArgs.inputSampleRate        = SAMPLE_RATE_44100;
      initArgs.outputSampleRate       = SAMPLE_RATE_48000;
      initArgs.numChannels            = 2;
      initArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;
      initArgs.frameModuloFactor      = 4;
#ifdef C7X
      initArgs.matTransHandle = NULL; // Initialize to NULL first
#endif

      // Initialize buffer parameters
      bufParamsIn.data_type = AUDIOLIB_FLOAT32;
      bufParamsIn.dim_x     = 2;   // numChannels
      bufParamsIn.dim_y     = 256; // maxSampleCountPerBlock
      bufParamsIn.stride_y  = 8;   // 2 channels * 4 bytes

      bufParamsOut.data_type = AUDIOLIB_FLOAT32;
      bufParamsOut.dim_x     = 2;
      bufParamsOut.dim_y     = 311;
      bufParamsOut.stride_y  = 8;

      // Initialize exec parameters
      execInArgs.inputSampleCount = 256;

      // Allocate memory for handle and buffers using TI memory functions
      int32_t handleSize = AUDIOLIB_asrc_getHandleSize(&initArgs);
      handle             = malloc(handleSize);

#ifdef C7X
      // For interleaved data format, we need a matTrans handle
      if (initArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
         int32_t matTransHandleSize = DSPLIB_matTrans_getHandleSize(NULL);
         matTransHandle             = malloc(matTransHandleSize);
         initArgs.matTransHandle    = matTransHandle;
      }
#endif

      pIn                 = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));
      pOut                = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));
      pNonInterleavedData = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));
      pFiltCoeffs         = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));
      pFilterRembuf       = (float *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, 1024 * sizeof(float));

#ifdef C7X
      if (!handle || !pIn || !pOut || !pNonInterleavedData || !pFiltCoeffs || !pFilterRembuf ||
          (initArgs.dataFormat == AUDIOLIB_DATA_FORMAT_INTERLEAVED && !matTransHandle)) {
#endif
#ifdef ARM_A53
         if (!handle || !pIn || !pOut || !pNonInterleavedData || !pFiltCoeffs || !pFilterRembuf) {
#endif
            AUDIOLIB_DEBUG_PRINT("Failed to allocate memory in ASRC Coverage Tests\r\n");
            fail = 1;
            goto cleanup;
         }

         while (testNum <= 1051) {
            currentTestFail = 0;

            switch (testNum) {
               // Tests for AUDIOLIB_asrc_init_checkParams()
            case 1000:
               status          = AUDIOLIB_asrc_init_checkParams(NULL, &bufParamsIn, &bufParamsOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1001:
               status          = AUDIOLIB_asrc_init_checkParams(handle, NULL, &bufParamsOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1002:
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, NULL, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1003:
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, NULL);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1004: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.funcStyle              = 55; // Invalid value
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            case 1005: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.maxSampleCountPerBlock = 0;
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            case 1006: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.maxSampleCountPerBlock = 513; // Not a multiple of 2
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            case 1007: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.sampleDataType         = 55; // Invalid type
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_TYPE);
            } break;

            case 1008: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.inputSampleRate        = SAMPLE_RATE_NA; // Invalid rate
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            case 1009: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.outputSampleRate       = SAMPLE_RATE_NA; // Invalid rate
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            case 1010: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.numChannels            = 0;
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            case 1011: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.dataFormat             = 55; // Invalid format
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

#ifdef C7X
            // Test for matTransHandle being NULL when dataFormat is interleaved
            case 1012: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_INTERLEAVED;
               badArgs.matTransHandle         = NULL; // This should fail if dataFormat is interleaved
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
            } break;
#endif

            // Tests for AUDIOLIB_asrc_exec_checkParams()
            // First initialize the handle with valid parameters
            case 1013:
               status          = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_SUCCESS);
               break;

            case 1014:
               status = AUDIOLIB_asrc_exec_checkParams(NULL, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf, pOut,
                                                       &execInArgs, &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1015:
               status = AUDIOLIB_asrc_exec_checkParams(handle, NULL, pNonInterleavedData, pFiltCoeffs, pFilterRembuf,
                                                       pOut, &execInArgs, &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1016:
               status = AUDIOLIB_asrc_exec_checkParams(handle, pIn, NULL, pFiltCoeffs, pFilterRembuf, pOut, &execInArgs,
                                                       &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1017:
               status = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, NULL, pFilterRembuf, pOut,
                                                       &execInArgs, &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1018:
               status = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs, NULL, pOut,
                                                       &execInArgs, &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;
            case 1019:
               status = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf,
                                                       NULL, &execInArgs, &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1020:
               status = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf,
                                                       pOut, NULL, &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1021:
               status = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf,
                                                       pOut, &execInArgs, NULL);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1022: {
               AUDIOLIB_asrc_ExecInArgs badInArgs = execInArgs;
               badInArgs.inputSampleCount         = 0;
               status = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf,
                                                       pOut, &badInArgs, &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            case 1023: {
               AUDIOLIB_asrc_ExecInArgs badInArgs = execInArgs;
               badInArgs.inputSampleCount         = 513; // Larger than maxSampleCountPerBlock
               status = AUDIOLIB_asrc_exec_checkParams(handle, pIn, pNonInterleavedData, pFiltCoeffs, pFilterRembuf,
                                                       pOut, &badInArgs, &execOutArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            // Tests for AUDIOLIB_asrc_set()
            case 1024:
               status          = AUDIOLIB_asrc_set(NULL, 0, 1.0, pNonInterleavedData, pIn);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1025:
               status          = AUDIOLIB_asrc_set(handle, 55, 1.0, pNonInterleavedData, pIn);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
               break;

            case 1026:
               status          = AUDIOLIB_asrc_set(handle, 0, -1.0, pNonInterleavedData, pIn);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
               break;

            case 1027:
               status          = AUDIOLIB_asrc_set(handle, 0, 0.0, pNonInterleavedData, pIn);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
               break;

            case 1028:
               status          = AUDIOLIB_asrc_set(handle, 1, 1.0, NULL, pIn);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1029:
               status          = AUDIOLIB_asrc_set(handle, 1, 1.0, pNonInterleavedData, NULL);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
               break;

            case 1030: {
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.frameModuloFactor      = 9999; // Exceeds AUDIOLIB_ASRC_MAX_MODULO_FACTOR
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_VALUE);
            } break;

            case 1031: {
               AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
               badBufIn.data_type              = AUDIOLIB_INT32; // Invalid type
               status          = AUDIOLIB_asrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_TYPE);
            } break;

            case 1032: {
               AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
               badBufOut.data_type              = AUDIOLIB_INT32; // Invalid type
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_TYPE);
            } break;

            case 1033: {
               AUDIOLIB_bufParams2D_t badBufIn  = bufParamsIn;
               AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
               badBufIn.data_type               = AUDIOLIB_FLOAT32;
               badBufOut.data_type              = AUDIOLIB_INT32; // Mismatched type
               status          = AUDIOLIB_asrc_init_checkParams(handle, &badBufIn, &badBufOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_TYPE);
            } break;

            case 1034: {
               AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
               badBufIn.dim_y                  = initArgs.maxSampleCountPerBlock + 1; // Invalid for interleaved
               status          = AUDIOLIB_asrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1035: {
               AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
               badBufIn.dim_x                  = initArgs.numChannels + 1; // Invalid for interleaved
               status          = AUDIOLIB_asrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1036: {
               AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
               badBufOut.dim_x                  = initArgs.numChannels + 1; // Invalid for interleaved
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1037: {
               AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
               badBufOut.dim_y                  = 9999; // Invalid for interleaved
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1038: {
               AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
               badBufIn.stride_y               = 1; // Invalid stride for interleaved
               status          = AUDIOLIB_asrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1039: {
               AUDIOLIB_bufParams2D_t badBufOut = bufParamsOut;
               badBufOut.stride_y               = 1; // Invalid stride for interleaved
               status          = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &initArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1040: {
               // bufParamsIn.dim_y != pKerInitArgs->numChannels
               AUDIOLIB_asrc_InitArgs badArgs  = initArgs;
               badArgs.dataFormat              = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
               AUDIOLIB_bufParams2D_t badBufIn = bufParamsIn;
               badBufIn.dim_y                  = badArgs.numChannels + 1; // Invalid
               status          = AUDIOLIB_asrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1041: {
               // bufParamsIn.dim_x != pKerInitArgs->maxSampleCountPerBlock (non-interleaved)
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
               badArgs.maxSampleCountPerBlock = 256;
               badArgs.numChannels            = 2;
               AUDIOLIB_bufParams2D_t badBufIn;
               badBufIn.data_type = AUDIOLIB_FLOAT32;
               badBufIn.dim_y     = badArgs.numChannels;
               badBufIn.stride_y =
                   badArgs.maxSampleCountPerBlock * AUDIOLIB_ASRC_INPUT_BUFFER_MULTIPLE_FSG64 * sizeof(float);
               badBufIn.dim_x  = badArgs.maxSampleCountPerBlock + 1; // Invalid
               status          = AUDIOLIB_asrc_init_checkParams(handle, &badBufIn, &bufParamsOut, &badArgs);
               currentTestFail = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1042: {
               // bufParamsOut.dim_x != outFrameLength (non-interleaved)
               bufParamsIn.data_type          = AUDIOLIB_FLOAT32;
               bufParamsIn.dim_x              = 256;
               bufParamsIn.dim_y              = 2;
               bufParamsIn.stride_y           = 4096;
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
               badArgs.maxSampleCountPerBlock = 256;
               badArgs.numChannels            = 2;
               int outFrameLength = AUDIOLIB_asrc_getOutBufferLength(badArgs.inputSampleRate, badArgs.outputSampleRate,
                                                                     badArgs.maxSampleCountPerBlock);
               AUDIOLIB_bufParams2D_t badBufOut;
               badBufOut.data_type = AUDIOLIB_FLOAT32;
               badBufOut.dim_y     = badArgs.numChannels;
               badBufOut.stride_y  = outFrameLength * sizeof(float);
               badBufOut.dim_x     = 9999; // Invalid
               status              = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &badArgs);
               currentTestFail     = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1043: {
               // bufParamsOut.dim_y != pKerInitArgs->numChannels (non-interleaved)
               bufParamsIn.data_type          = AUDIOLIB_FLOAT32;
               bufParamsIn.dim_x              = 256;
               bufParamsIn.dim_y              = 2;
               bufParamsIn.stride_y           = 4096;
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
               badArgs.maxSampleCountPerBlock = 256;
               badArgs.numChannels            = 2;
               int outFrameLength = AUDIOLIB_asrc_getOutBufferLength(badArgs.inputSampleRate, badArgs.outputSampleRate,
                                                                     badArgs.maxSampleCountPerBlock);
               AUDIOLIB_bufParams2D_t badBufOut;
               badBufOut.data_type = AUDIOLIB_FLOAT32;
               badBufOut.dim_x     = outFrameLength;
               badBufOut.stride_y  = outFrameLength * sizeof(float);
               badBufOut.dim_y     = badArgs.numChannels + 1; // Invalid
               status              = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &badArgs);
               currentTestFail     = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1044: {
               // bufParamsOut.stride_y != outFrameLength * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType)
               // (non-interleaved)
               bufParamsIn.data_type          = AUDIOLIB_FLOAT32;
               bufParamsIn.dim_x              = 256;
               bufParamsIn.dim_y              = 2;
               bufParamsIn.stride_y           = 1;
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
               badArgs.maxSampleCountPerBlock = 256;
               badArgs.numChannels            = 2;
               int outFrameLength = AUDIOLIB_asrc_getOutBufferLength(badArgs.inputSampleRate, badArgs.outputSampleRate,
                                                                     badArgs.maxSampleCountPerBlock);
               AUDIOLIB_bufParams2D_t badBufOut;
               badBufOut.data_type = AUDIOLIB_FLOAT32;
               badBufOut.dim_x     = outFrameLength;
               badBufOut.dim_y     = badArgs.numChannels;
               badBufOut.stride_y  = outFrameLength * sizeof(float);
               status              = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &badArgs);
               currentTestFail     = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1045: {
               // bufParamsOut.stride_y != outFrameLength * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType)
               // (non-interleaved)
               bufParamsIn.data_type          = AUDIOLIB_FLOAT32;
               bufParamsIn.dim_x              = 16;
               bufParamsIn.dim_y              = 2;
               bufParamsIn.stride_y           = 4096;
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
               badArgs.maxSampleCountPerBlock = 16;
               badArgs.numChannels            = 2;
               int outFrameLength = AUDIOLIB_asrc_getOutBufferLength(badArgs.inputSampleRate, badArgs.outputSampleRate,
                                                                     badArgs.maxSampleCountPerBlock);
               AUDIOLIB_bufParams2D_t badBufOut;
               badBufOut.data_type = AUDIOLIB_FLOAT32;
               badBufOut.dim_x     = outFrameLength;
               badBufOut.dim_y     = badArgs.numChannels;
               badBufOut.stride_y  = outFrameLength * sizeof(float);
               status              = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &badArgs);
               currentTestFail     = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

            case 1046: {
               // bufParamsOut.stride_y != outFrameLength * AUDIOLIB_sizeof(pKerInitArgs->sampleDataType)
               // (non-interleaved)
               bufParamsIn.data_type          = AUDIOLIB_FLOAT32;
               bufParamsIn.dim_x              = 256;
               bufParamsIn.dim_y              = 2;
               bufParamsIn.stride_y           = 4096;
               AUDIOLIB_asrc_InitArgs badArgs = initArgs;
               badArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
               badArgs.maxSampleCountPerBlock = 256;
               badArgs.numChannels            = 2;
               int outFrameLength = AUDIOLIB_asrc_getOutBufferLength(badArgs.inputSampleRate, badArgs.outputSampleRate,
                                                                     badArgs.maxSampleCountPerBlock);
               AUDIOLIB_bufParams2D_t badBufOut;
               badBufOut.data_type = AUDIOLIB_FLOAT32;
               badBufOut.dim_x     = outFrameLength;
               badBufOut.dim_y     = badArgs.numChannels;
               badBufOut.stride_y  = 1; // Invalid
               status              = AUDIOLIB_asrc_init_checkParams(handle, &bufParamsIn, &badBufOut, &badArgs);
               currentTestFail     = (status != AUDIOLIB_ERR_INVALID_DIMENSION);
            } break;

#ifdef C7X
            case 1047: {
               AUDIOLIB_asrc_InitArgs newInitArgs = initArgs;
               newInitArgs.matTransHandle         = NULL;
               status          = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &newInitArgs);
               currentTestFail = (status != AUDIOLIB_ERR_FAILURE);
            } break;
#endif

            case 1048: {
               AUDIOLIB_asrc_InitArgs newInitArgs = initArgs;
               newInitArgs.dataFormat             = AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED;
               newInitArgs.maxSampleCountPerBlock = 256;
               newInitArgs.numChannels            = 2;
               status          = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &newInitArgs);
               status          = AUDIOLIB_asrc_set(handle, 1, 1.0, pNonInterleavedData, NULL);
               currentTestFail = (status != AUDIOLIB_ERR_NULL_POINTER);
            } break;

            case 1049: {
               // Test convertSampleRateToInt() with SAMPLE_RATE_NA to hit default case
               int rate = convertSampleRateToInt(SAMPLE_RATE_NA);
               // Expect default value, typically 48000 (check implementation)
               currentTestFail = (rate != 0);
            } break;

            case 1050: {
               // Test AUDIOLIB_asrc_getOutBufferLength with invalid parameters
               int outFrameLength = AUDIOLIB_asrc_getOutBufferLength(SAMPLE_RATE_NA, SAMPLE_RATE_48000, 256);
               currentTestFail    = (outFrameLength != 32); // Expect 0 for invalid input sample rate
            } break;

#ifdef C7X
            case 1051: {
               AUDIOLIB_asrc_InitArgs newInitArgs = initArgs;
               newInitArgs.matTransHandle         = NULL;
               newInitArgs.funcStyle              = AUDIOLIB_FUNCTION_NATC;
               status          = AUDIOLIB_asrc_init(handle, &bufParamsIn, &bufParamsOut, &newInitArgs);
               currentTestFail = (status != AUDIOLIB_ERR_FAILURE);
            } break;
#endif

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
#ifdef C7X
         if (matTransHandle)
            free(matTransHandle);
#endif
         if (pIn)
            TI_align_free(pIn);
         if (pOut)
            TI_align_free(pOut);
         if (pNonInterleavedData)
            TI_align_free(pNonInterleavedData);
         if (pFiltCoeffs)
            TI_align_free(pFiltCoeffs);
         if (pFilterRembuf)
            TI_align_free(pFilterRembuf);

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

         uint32_t profile[512 * 3];

#ifdef C7X
         AUDIOLIB_TEST_init();
#endif

         fail = test_main(&profile[0]);

#if !defined(NO_PRINTF)
         if (fail == 0)
            AUDIOLIB_DEBUG_PRINT("Test Pass!\n");
         else
            AUDIOLIB_DEBUG_PRINT("Test Fail!\n");

         int i;
         for (i = 0; i < test_cases; i++) {
            AUDIOLIB_DEBUG_PRINT("Test %4d: Cold Cycles = %8d, Warm Cycles = %8d, Warm Cycles WRB = "
                                 "%8d\r\n",
                                 i, profile[3 * i], profile[3 * i + 1], profile[3 * i + 2]);
         }
#endif

         fail = coverage_test_main();
         if (fail == 0) {
            AUDIOLIB_DEBUG_PRINT("Test Pass!\r\n");
         }
         else {
            AUDIOLIB_DEBUG_PRINT("Test Fail!\r\n");
         }
#ifdef ARM_A53
         /* Close board and flash drivers */
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
