// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

// include AUDIOLIB
#include <audiolib.h>
#include <stddef.h>
// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_gainNChTrimSmoothing_idat.h"
#include "AUDIOLIB_types.h"

#ifdef WIN32
#if defined(__C7504__) || defined(__C7524__)
int8_t ddrBuffer[2048 * 1024];
#else
__attribute__((section(".msmcData"), aligned(128))) int8_t msmcBuffer[AUDIOLIB_L3_RESULTS_BUFFER_SIZE];
__attribute__((section(".ddrData"), aligned(64))) int8_t   ddrBuffer[2048 * 1024];
#endif
#else
#if defined(__C7504__) || defined(__C7524__)
__attribute__((section(".ddrData"), aligned(64))) int8_t ddrBuffer[2048 * 1024];

#else
__attribute__((section(".msmcData"), aligned(128))) int8_t msmcBuffer[AUDIOLIB_L3_RESULTS_BUFFER_SIZE];
__attribute__((section(".ddrData"), aligned(64))) int8_t   ddrBuffer[2048 * 1024];

#endif
#endif // WIN32

int16_t volatile volatileSum = 0; // use volatile to keep compiler from removing this operation

int AUDIOLIB_gainNChTrimSmoothing_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams2D_t bufParamsOut;
   AUDIOLIB_bufParams1D_t bufParamsGains;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_gainNChTrimSmoothing_testParams_t *prm;
   AUDIOLIB_gainNChTrimSmoothing_testParams_t  currPrm;
   AUDIOLIB_gainNChTrimSmoothing_getTestParams(&prm, &test_cases);

   AUDIOLIB_gainNChTrimSmoothing_InitArgs kerInitArgs;
   AUDIOLIB_gainNChTrimSmoothing_SetArgs  kerSetArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 0\n");

   int32_t handleSize = AUDIOLIB_gainNChTrimSmoothing_getHandleSize(&kerInitArgs);
   /* The kernel vector-loads per-channel state from inside the handle, so it must be aligned. */
   AUDIOLIB_kernelHandle handle = TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, handleSize);
   memset(handle, 0, handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 1\n");

   TI_profile_init("AUDIOLIB_gainNChTrimSmoothing");
   // file IO for EVM benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_gainNChTrimSmoothing.csv", "w+");
   fprintf(fpOutputCSV, "Test ID,Bit Width,inSamples,inChannels,frames,isInterleaved,fadeType,fadeTime,masterGain,"
                        "sampling rate,EST cycles,EVM cycles,Pass/Fail\n");
   for (tpi = 0; tpi < test_cases; tpi++) {
      uint64_t archCycles = 0, estCycles = 0;
      numReps                   = prm[tpi].numReps;
      testNum                   = prm[tpi].testID;
      currPrm                   = prm[tpi];
      kerSetArgs.fadeTime       = currPrm.fadeTime;
      kerSetArgs.fadeType       = currPrm.fadeType;
      kerSetArgs.masterGain     = currPrm.masterGain;
      kerInitArgs.samplingRate  = (float) currPrm.samplingRate;
      kerInitArgs.isInterleaved = currPrm.isInterleaved;

      uint8_t isInterleaved = currPrm.isInterleaved;
      int32_t numExecReps   = currPrm.numExecReps;

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;

         currentTestFail = 0;
         if (isInterleaved) {
            bufParamsIn.data_type = currPrm.sampleDataType;
            bufParamsIn.dim_x     = currPrm.numChannels;
            bufParamsIn.dim_y     = currPrm.numSamples;
            bufParamsIn.stride_y  = currPrm.strideIn;

            bufParamsOut.data_type = currPrm.sampleDataType;
            bufParamsOut.dim_x     = currPrm.numChannels;
            bufParamsOut.dim_y     = currPrm.numSamples;
            bufParamsOut.stride_y  = currPrm.strideOut;
         }
         else {
            bufParamsIn.data_type = currPrm.sampleDataType;
            bufParamsIn.dim_x     = currPrm.numSamples;
            bufParamsIn.dim_y     = currPrm.numChannels;
            bufParamsIn.stride_y  = currPrm.strideIn;

            bufParamsOut.data_type = currPrm.sampleDataType;
            bufParamsOut.dim_x     = currPrm.numSamples;
            bufParamsOut.dim_y     = currPrm.numChannels;
            bufParamsOut.stride_y  = currPrm.strideOut;
         }
         bufParamsGains.data_type = AUDIOLIB_FLOAT32;
         bufParamsGains.dim_x = currPrm.numChannels;

         // Here, stride is in bytes
         uint32_t inpLeftSizeBytes;
         uint32_t outLeftSizeBytes;
         uint32_t gainSizeBytes;

         // Calculate the size of the input and output buffers in bytes

         inpLeftSizeBytes = bufParamsIn.dim_y * bufParamsIn.stride_y;
         outLeftSizeBytes = bufParamsOut.dim_y * bufParamsOut.stride_y;
         gainSizeBytes = bufParamsGains.dim_x * AUDIOLIB_sizeof(bufParamsGains.data_type);
         void *pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpLeftSizeBytes * numExecReps);
         void *initialGain = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, gainSizeBytes);
         void *trimGain = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, gainSizeBytes);
         kerSetArgs.initialGain = (float *) initialGain;
         kerSetArgs.trimGain    = (float *) trimGain;

         void *pOut, *pOutCn;
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);

            pOutCn = (void *) malloc(outLeftSizeBytes * numExecReps);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOut: %p pOutCn: %p\n", pIn, pOut, pOutCn);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn && pOut && pOutCn && initialGain && trimGain) {
            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {
               // Fill input buffers frame by frame
               int32_t frame = 0;
               for (frame = 0; frame < numExecReps; frame++) {

                  TI_fillBuffer_float(
                      prm[tpi].testPattern, 0, (void *) ((int8_t *) pIn + (frame * inpLeftSizeBytes)),
                      (void *) ((int8_t *) prm[tpi].staticIn + (frame * currPrm.numSamples * currPrm.numChannels *
                                                                AUDIOLIB_sizeof(currPrm.sampleDataType))),
                      bufParamsIn.dim_x, bufParamsIn.dim_y, bufParamsIn.stride_y,
                      AUDIOLIB_sizeof(currPrm.sampleDataType), testPatternString);
               }

               TI_fillBuffer_float(prm[tpi].testPattern, 0, initialGain, prm[tpi].currentGain, currPrm.numChannels, 1,
                                   0, sizeof(float), testPatternString);
               TI_fillBuffer_float(prm[tpi].testPattern, 0, trimGain, prm[tpi].targetGain, currPrm.numChannels, 1, 0,
                                   sizeof(float), testPatternString);
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 0\n");

            status_init =
                AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 1 status_init %d\n",
                                  status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init = AUDIOLIB_gainNChTrimSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               if (status_init == AUDIOLIB_SUCCESS) {
                  status_init = AUDIOLIB_gainNChTrimSmoothing_set(handle, &kerSetArgs);
               }
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 2 status_init %d\n",
                                  status_init);

            status_opt = AUDIOLIB_gainNChTrimSmoothing_exec_checkParams(handle, pIn, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 3 status_opt %d\n",
                                  status_opt);

            if ((status_init == AUDIOLIB_SUCCESS) && (status_opt == AUDIOLIB_SUCCESS)) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_gainNChTrimSmoothing_exec(handle, pIn, pOut);
               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();
            }

            /* The following for loop is to call kernel repeatedly so as to
             * train the branch predictor                                   */
            int32_t k;
            for (k = 0; (k < (numExecReps - 2)) && (status_opt == AUDIOLIB_SUCCESS); k++) {
               // run warm instruction cache test
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);

               status_opt =
                   AUDIOLIB_gainNChTrimSmoothing_exec(handle, (void *) ((int8_t *) pIn + (k + 1) * inpLeftSizeBytes),
                                                      (void *) ((int8_t *) pOut + (k + 1) * outLeftSizeBytes));

               TI_profile_stop();
            }

            int16_t outSum    = 0;
            int8_t *pOutTempL = (int8_t *) pOut;
            size_t  j         = 0;
            // Use the size of the output buffer for the loop condition
            for (j = 0; j < outLeftSizeBytes; j++) {
               outSum += *pOutTempL;
               pOutTempL++;
            }
            volatileSum = outSum;

            if ((numExecReps != 1) && (status_opt == AUDIOLIB_SUCCESS)) {
               // run warm instruction cache test
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);

               status_opt = AUDIOLIB_gainNChTrimSmoothing_exec(
                   handle, (void *) ((int8_t *) pIn + (numExecReps - 1) * inpLeftSizeBytes),
                   (void *) ((int8_t *) pOut + (numExecReps - 1) * outLeftSizeBytes));
               TI_profile_stop();
            }

            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 4 status_opt %d\n",
                                  status_opt);

            // initialize the kernel to use the natural C variant; set() reseeds every channel
            // from initialGain, so the natural C run starts from the same state
            status_nat = AUDIOLIB_gainNChTrimSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
            if (status_nat == AUDIOLIB_SUCCESS) {
               status_nat = AUDIOLIB_gainNChTrimSmoothing_set(handle, &kerSetArgs);
            }
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            for (k = 0; (k < numExecReps) && (status_nat == AUDIOLIB_SUCCESS); k++) {
               status_nat = AUDIOLIB_gainNChTrimSmoothing_exec(handle, (void *) ((int8_t *) pIn + k * inpLeftSizeBytes),
                                                               (void *) ((int8_t *) pOutCn + k * outLeftSizeBytes));
            }
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

               // Initialize comparison statuses as PASS; fail if any frame fails
               status_nat_vs_opt = TI_TEST_KERNEL_PASS;
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;
               int32_t frame     = 0;
               for (frame = 0; frame < numExecReps; frame++) {
                  // Compare optimized vs. natural C for this frame
                  int32_t frame_status_nat_vs_opt =
                      TI_compare_mem_2D_float((void *) ((int8_t *) pOut + frame * outLeftSizeBytes),
                                              (void *) ((int8_t *) pOutCn + frame * outLeftSizeBytes), 0.001,
                                              (double) powf(2, -10), bufParamsOut.dim_x, bufParamsOut.dim_y,
                                              bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType));

                  // Update overall status: fail if any frame fails
                  if (frame_status_nat_vs_opt == TI_TEST_KERNEL_FAIL) {
                     status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
                  }
               }
            }
            else {
               // TBD
            }

            comparisonDone = 1;

            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 7 comparisonDone %d "
                                  "status_nat_vs_opt %d\n",
                                  comparisonDone, status_nat_vs_opt);

            if (currPrm.staticOut != NULL) {
               if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {

                  // Initialize reference comparison statuses
                  status_ref_vs_opt = TI_TEST_KERNEL_PASS;
                  int32_t frame     = 0;
                  // Frame-by-frame comparison for reference outputs
                  for (frame = 0; frame < numExecReps; frame++) {
                     int32_t frame_status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                         (void *) ((int8_t *) pOut + frame * outLeftSizeBytes),
                         (void *) ((int8_t *) currPrm.staticOut + (frame * currPrm.numSamples * currPrm.numChannels *
                                                                   AUDIOLIB_sizeof(currPrm.sampleDataType))),
                         0.001, (double) powf(2, -10), bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y,
                         bufParamsOut.dim_x * AUDIOLIB_sizeof(currPrm.sampleDataType),
                         AUDIOLIB_sizeof(currPrm.sampleDataType));
                     // Update overall status: fail if any frame fails
                     if (frame_status_ref_vs_opt == TI_TEST_KERNEL_FAIL) {
                        status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
                     }
                  }
               }
               else {
                  /*No operation*/
               }

               comparisonDone = 1;
            }
            else {
               /* Set to pass since it wasn't supposed to run. */
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;
            }
            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 8 status_nat_vs_opt %d "
                                  "status_ref_vs_opt %d currentTestFail %d\n",
                                  status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 8 status_init %d "
                                  "status_opt %d status_nat %d\n",
                                  status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainNChTrimSmoothing_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_gainNChTrimSmoothing_get(handle, &kerSetArgs);
            sprintf(desc,
                    "%s generated input | inSamples = %d, inChannels = %d, frames = %d, isInterleaved = %d, "
                    "fadeType = %s, masterGain = %0.3f, fadeTime = %0.3f ms, dataType = %d",
                    testPatternString, currPrm.numSamples, currPrm.numChannels, numExecReps, isInterleaved,
                    (kerSetArgs.fadeType == AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH) ? "SMOOTH" : "SYMMETRIC",
                    kerSetArgs.masterGain, kerSetArgs.fadeTime, currPrm.sampleDataType);

            /* perfEst models the optimized kernel, so re-init it before asking. */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
            if (AUDIOLIB_gainNChTrimSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs) ==
                AUDIOLIB_SUCCESS) {
               AUDIOLIB_gainNChTrimSmoothing_set(handle, &kerSetArgs);
            }
            AUDIOLIB_gainNChTrimSmoothing_perfEst(handle, &archCycles, &estCycles);

            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            fprintf(fpOutputCSV, "%d,%d,%d,%d,%d,%u,%u,%f,%f,%d,%lld,%d,%d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.numSamples, currPrm.numChannels, numExecReps,
                    currPrm.isInterleaved, currPrm.fadeType, currPrm.fadeTime, kerSetArgs.masterGain,
                    currPrm.samplingRate, (long long) estCycles,
                    pProfile[3 * tpi + 1], // EVM cycles (warm cycles)
                    !currentTestFail);

            TI_profile_add_test(testNum++, currPrm.numSamples * currPrm.numChannels, archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs
            TI_profile_clear_run_stats();

         } // end of memory allocation successful?

         /* Free buffers for each test vector */
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            free(pOutCn);
            TI_align_free(pOut);
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
         TI_align_free(pIn);
         TI_align_free(initialGain);
         TI_align_free(trimGain);

      } // end repetitions
   } // end idat test cases

   TI_align_free(handle);
   /* Close results CSV */
   fclose(fpOutputCSV);
   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_gainNChTrimSmoothing");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_gainNChTrimSmoothing_d(&pProfile[0], 0);
   }
}

/* Size of the buffers used by the coverage tests: 16 channels of 16 samples. */
#define COVERAGE_NUM_SAMPLES (16)
#define COVERAGE_NUM_CHANNELS (16)

/* Index of sample s of channel ch in a packed coverage buffer of the given layout. */
static int32_t coverageIndex(uint8_t isInterleaved, int32_t ch, int32_t s)
{
   return isInterleaved ? (s * COVERAGE_NUM_CHANNELS + ch) : (ch * COVERAGE_NUM_SAMPLES + s);
}

/* Initialises a packed COVERAGE_NUM_CHANNELS x COVERAGE_NUM_SAMPLES kernel instance. */
static AUDIOLIB_STATUS
coverageInit(AUDIOLIB_kernelHandle handle, int8_t funcStyle, uint8_t isInterleaved, float samplingRate)
{
   AUDIOLIB_bufParams2D_t                 bufParams;
   AUDIOLIB_gainNChTrimSmoothing_InitArgs initArgs;

   bufParams.data_type = AUDIOLIB_FLOAT32;
   bufParams.dim_x     = isInterleaved ? COVERAGE_NUM_CHANNELS : COVERAGE_NUM_SAMPLES;
   bufParams.dim_y     = isInterleaved ? COVERAGE_NUM_SAMPLES : COVERAGE_NUM_CHANNELS;
   bufParams.stride_y  = bufParams.dim_x * AUDIOLIB_sizeof(bufParams.data_type);

   initArgs.funcStyle     = funcStyle;
   initArgs.samplingRate  = samplingRate;
   initArgs.isInterleaved = isInterleaved;

   return AUDIOLIB_gainNChTrimSmoothing_init(handle, &bufParams, &bufParams, &initArgs);
}

int coverage_test_main()
{
   int32_t testNum = 1000;
   /* Loop counters, declared up front: the target compiler builds this file as C89. */
   int32_t                                ch, i, st;
   uint8_t                                il;
   int32_t                                currentTestFail = 0;
   AUDIOLIB_gainNChTrimSmoothing_InitArgs kerInitArgs;

   /* Init args are read by init_checkParams, so give them valid values rather than
      letting the tests depend on whatever is on the stack. */
   kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.samplingRate  = 48000.0f;
   kerInitArgs.isInterleaved = 0;

   int32_t               handleSize = AUDIOLIB_gainNChTrimSmoothing_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   int   fail = 0;
   void *pIn0, *pOut;

   int32_t inSamples  = COVERAGE_NUM_SAMPLES;
   int32_t inChannels = COVERAGE_NUM_CHANNELS;

   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = inSamples;
   bufParamsIn.dim_y     = inChannels;
   bufParamsIn.stride_y  = bufParamsIn.dim_x * AUDIOLIB_sizeof(bufParamsIn.data_type);

   bufParamsOut.data_type = bufParamsIn.data_type;
   bufParamsOut.dim_x     = inSamples;
   bufParamsOut.dim_y     = inChannels;
   bufParamsOut.stride_y  = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);

   uint64_t In0Size = bufParamsIn.stride_y * bufParamsIn.dim_y;
   uint64_t OutSize = bufParamsOut.stride_y * bufParamsOut.dim_y;

   AUDIOLIB_bufParams2D_t bufParamsInTemp, bufParamsOutTemp;

   float initialGain[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS];
   float trimGain[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS];
   for (ch = 0; ch < AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS; ch++) {
      initialGain[ch] = 1.0f;
      trimGain[ch]    = 0.5f;
   }

   AUDIOLIB_gainNChTrimSmoothing_SetArgs setArgs;
   setArgs.initialGain = initialGain;
   setArgs.trimGain    = trimGain;
   setArgs.masterGain  = 1.0f;
   setArgs.fadeTime    = 10.0f;
   setArgs.fadeType    = AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SYMMETRIC;

   /* The optimized kernels stream these buffers, so they must be vector-aligned. */
   pIn0 = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, In0Size);
   pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, OutSize);
   AUDIOLIB_gainNChTrimSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   float *pInF  = (float *) pIn0;
   float *pOutF = (float *) pOut;

   while (testNum <= 1011) {
      bufParamsInTemp  = bufParamsIn;
      bufParamsOutTemp = bufParamsOut;
      currentTestFail  = 0;

      switch (testNum) {
      case 1000:
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_gainNChTrimSmoothing_init_checkParams(NULL, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1001:
         /* Unsupported input type, then input/output type mismatch. */
         bufParamsInTemp.data_type = AUDIOLIB_UINT32;
         status_nat =
             AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         bufParamsInTemp.data_type  = AUDIOLIB_FLOAT32;
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;
         status_opt =
             AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1002:
         /* FLOAT64 is not implemented and must be rejected. */
         bufParamsInTemp.data_type  = AUDIOLIB_FLOAT64;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT64;
         status_opt =
             AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = (status_opt != AUDIOLIB_ERR_INVALID_TYPE);
         break;

      case 1003:
         /* Non-positive sampling rate, out-of-range isInterleaved. */
         kerInitArgs.samplingRate = 0.0f;
         status_nat =
             AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.samplingRate  = 48000.0f;
         kerInitArgs.isInterleaved = 2;
         status_opt =
             AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.isInterleaved = 0;
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) || (status_opt != AUDIOLIB_ERR_INVALID_VALUE));
         break;

      case 1004:
         /* More channels than the per-channel state holds, and in/out shape mismatch. */
         bufParamsInTemp.dim_y  = AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS + 1;
         bufParamsOutTemp.dim_y = AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS + 1;
         status_nat =
             AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         bufParamsInTemp.dim_y = bufParamsIn.dim_y;
         status_opt =
             AUDIOLIB_gainNChTrimSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) || (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION));
         break;

      case 1005: {
         /* set(): NULL handle, NULL args, NULL trimGain. A NULL initialGain is valid. */
         AUDIOLIB_gainNChTrimSmoothing_SetArgs badArgs  = setArgs;
         badArgs.trimGain                               = NULL;
         AUDIOLIB_gainNChTrimSmoothing_SetArgs keepArgs = setArgs;
         keepArgs.initialGain                           = NULL;
         currentTestFail = ((AUDIOLIB_gainNChTrimSmoothing_set(NULL, &setArgs) != AUDIOLIB_ERR_NULL_POINTER) ||
                            (AUDIOLIB_gainNChTrimSmoothing_set(handle, NULL) != AUDIOLIB_ERR_NULL_POINTER) ||
                            (AUDIOLIB_gainNChTrimSmoothing_set(handle, &badArgs) != AUDIOLIB_ERR_NULL_POINTER) ||
                            (AUDIOLIB_gainNChTrimSmoothing_set(handle, &keepArgs) != AUDIOLIB_SUCCESS));
         break;
      }

      case 1006: {
         /* set(): fadeType out of range, negative fadeTime, and negative gains, which only
            the SYMMETRIC (geometric) fade rejects. */
         AUDIOLIB_gainNChTrimSmoothing_SetArgs badArgs = setArgs;
         badArgs.fadeType                              = AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_MAX + 1;
         currentTestFail = (AUDIOLIB_gainNChTrimSmoothing_set(handle, &badArgs) != AUDIOLIB_ERR_INVALID_VALUE);

         badArgs          = setArgs;
         badArgs.fadeTime = -1.0f;
         currentTestFail =
             currentTestFail || (AUDIOLIB_gainNChTrimSmoothing_set(handle, &badArgs) != AUDIOLIB_ERR_INVALID_VALUE);

         badArgs        = setArgs;
         initialGain[3] = -1.0f;
         currentTestFail =
             currentTestFail || (AUDIOLIB_gainNChTrimSmoothing_set(handle, &badArgs) != AUDIOLIB_ERR_INVALID_VALUE);
         initialGain[3] = 1.0f;

         badArgs.masterGain = -1.0f;
         currentTestFail =
             currentTestFail || (AUDIOLIB_gainNChTrimSmoothing_set(handle, &badArgs) != AUDIOLIB_ERR_INVALID_VALUE);

         badArgs.fadeType = AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH;
         currentTestFail = currentTestFail || (AUDIOLIB_gainNChTrimSmoothing_set(handle, &badArgs) != AUDIOLIB_SUCCESS);
         break;
      }

      case 1007: {
         /* get(): NULL handle and NULL args, then a round trip of the last set(). */
         AUDIOLIB_gainNChTrimSmoothing_SetArgs getArgs;
         currentTestFail = ((AUDIOLIB_gainNChTrimSmoothing_get(NULL, &getArgs) != AUDIOLIB_ERR_NULL_POINTER) ||
                            (AUDIOLIB_gainNChTrimSmoothing_get(handle, NULL) != AUDIOLIB_ERR_NULL_POINTER));
         currentTestFail = currentTestFail ||
                           (AUDIOLIB_gainNChTrimSmoothing_set(handle, &setArgs) != AUDIOLIB_SUCCESS) ||
                           (AUDIOLIB_gainNChTrimSmoothing_get(handle, &getArgs) != AUDIOLIB_SUCCESS) ||
                           (getArgs.fadeTime != setArgs.fadeTime) || (getArgs.trimGain != setArgs.trimGain);
         break;
      }

      case 1008:
         /* exec_checkParams(): NULL input and output buffers. */
         currentTestFail =
             ((AUDIOLIB_gainNChTrimSmoothing_exec_checkParams(handle, NULL, pOut) != AUDIOLIB_ERR_NULL_POINTER) ||
              (AUDIOLIB_gainNChTrimSmoothing_exec_checkParams(handle, pIn0, NULL) != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1009: {
         /* SYMMETRIC fade-out to 0 spanning two frames, for both styles and layouts. With
            fs = 1000 Hz and fadeTime = 20 ms, N = 20; the 0 target is floored to 1e-10, so
            the ratio is 1e-10^(1/20). Sample n gets ratio^n for n < 20 (sample 0 is the
            start gain, 1), and exactly 0 from sample 20 on, which is frame 1, sample 4. */
         AUDIOLIB_gainNChTrimSmoothing_SetArgs fadeArgs = setArgs;
         for (ch = 0; ch < inChannels; ch++) {
            trimGain[ch] = 0.0f;
         }
         fadeArgs.fadeTime      = 20.0f;
         const float  ratio     = powf(1e-10f, 1.0f / 20.0f);
         const int8_t styles[2] = {AUDIOLIB_FUNCTION_NATC, AUDIOLIB_FUNCTION_OPTIMIZED};

         for (i = 0; i < inSamples * inChannels; i++) {
            pInF[i] = 1.0f;
         }
         for (st = 0; st < 2; st++) {
            for (il = 0; il < 2; il++) {
               int32_t last = inChannels - 1;
               status_nat   = coverageInit(handle, styles[st], il, 1000.0f);
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_set(handle, &fadeArgs);
               }
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_exec(handle, pIn0, pOut);
               }
               /* Frame 0: samples 0..15 of the fade. */
               currentTestFail = currentTestFail || (status_nat != AUDIOLIB_SUCCESS) ||
                                 (pOutF[coverageIndex(il, last, 0)] != 1.0f) ||
                                 (fabsf(pOutF[coverageIndex(il, last, 1)] - ratio) > 1e-4f * ratio);

               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_exec(handle, pIn0, pOut);
               }
               /* Frame 1: samples 16..31. Sample 19 is still fading, sample 20 lands on 0. */
               currentTestFail = currentTestFail || (status_nat != AUDIOLIB_SUCCESS) ||
                                 !(pOutF[coverageIndex(il, last, 3)] > 0.0f) ||
                                 (pOutF[coverageIndex(il, last, 4)] != 0.0f) ||
                                 (pOutF[coverageIndex(il, last, inSamples - 1)] != 0.0f);
            }
         }

         for (ch = 0; ch < AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS; ch++) {
            trimGain[ch] = 0.5f;
         }
         break;
      }

      case 1010: {
         /* A later set() with initialGain = NULL, as from a user interface: channel 0 gets a
            new target and restarts its fade from its running gain, while the other channels
            keep the fade they are in and land on 0.5 at sample 20 (frame 1, sample 4). */
         AUDIOLIB_gainNChTrimSmoothing_SetArgs fadeArgs = setArgs;
         fadeArgs.fadeTime                              = 20.0f;
         const int8_t styles[2]                         = {AUDIOLIB_FUNCTION_NATC, AUDIOLIB_FUNCTION_OPTIMIZED};

         for (i = 0; i < inSamples * inChannels; i++) {
            pInF[i] = 1.0f;
         }
         for (st = 0; st < 2; st++) {
            for (il = 0; il < 2; il++) {
               trimGain[0]          = 0.5f;
               fadeArgs.initialGain = initialGain;
               status_nat           = coverageInit(handle, styles[st], il, 1000.0f);
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_set(handle, &fadeArgs);
               }
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_exec(handle, pIn0, pOut);
               }
               float runningGain = pOutF[coverageIndex(il, 1, inSamples - 1)] * powf(0.5f, 1.0f / 20.0f);

               trimGain[0]          = 2.0f;
               fadeArgs.initialGain = NULL;
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_set(handle, &fadeArgs);
               }
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_exec(handle, pIn0, pOut);
               }
               currentTestFail = currentTestFail || (status_nat != AUDIOLIB_SUCCESS) ||
                                 (fabsf(pOutF[coverageIndex(il, 1, 0)] - runningGain) > 1e-4f) ||
                                 (pOutF[coverageIndex(il, 1, 4)] != 0.5f) ||
                                 (fabsf(pOutF[coverageIndex(il, 0, 0)] - runningGain) > 1e-4f) ||
                                 !(pOutF[coverageIndex(il, 0, 4)] > pOutF[coverageIndex(il, 0, 0)]) ||
                                 !(pOutF[coverageIndex(il, 0, inSamples - 1)] < 2.0f);
            }
         }
         trimGain[0] = 0.5f;
         break;
      }

      case 1011: {
         /* SMOOTH: fadeTime = 0 applies the target from the first sample, and otherwise the
            first sample is already one one-pole step from the start gain:
            k = 1 - exp(-1 / (fs * tau)) = 1 - exp(-1) with fs = 1000 Hz and tau = 1 ms. */
         AUDIOLIB_gainNChTrimSmoothing_SetArgs smoothArgs = setArgs;
         smoothArgs.fadeType                              = AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH;
         const int8_t styles[2]                           = {AUDIOLIB_FUNCTION_NATC, AUDIOLIB_FUNCTION_OPTIMIZED};
         const float  k                                   = 1.0f - expf(-1.0f);

         for (i = 0; i < inSamples * inChannels; i++) {
            pInF[i] = 2.0f;
         }
         for (ch = 0; ch < inChannels; ch++) {
            initialGain[ch] = 0.0f;
            trimGain[ch]    = 1.0f;
         }
         for (st = 0; st < 2; st++) {
            for (il = 0; il < 2; il++) {
               smoothArgs.fadeTime = 0.0f;
               status_nat          = coverageInit(handle, styles[st], il, 1000.0f);
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_set(handle, &smoothArgs);
               }
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_exec(handle, pIn0, pOut);
               }
               currentTestFail = currentTestFail || (status_nat != AUDIOLIB_SUCCESS);
               for (i = 0; i < inSamples * inChannels; i++) {
                  currentTestFail = currentTestFail || (pOutF[i] != 2.0f);
               }

               smoothArgs.fadeTime = 1.0f;
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_set(handle, &smoothArgs);
               }
               if (status_nat == AUDIOLIB_SUCCESS) {
                  status_nat = AUDIOLIB_gainNChTrimSmoothing_exec(handle, pIn0, pOut);
               }
               currentTestFail = currentTestFail || (status_nat != AUDIOLIB_SUCCESS) ||
                                 (fabsf(pOutF[coverageIndex(il, 5, 0)] - 2.0f * k) > 1e-5f) ||
                                 !(pOutF[coverageIndex(il, 5, inSamples - 1)] > pOutF[coverageIndex(il, 5, 0)]) ||
                                 !(pOutF[coverageIndex(il, 5, inSamples - 1)] < 2.0f);
            }
         }
         for (ch = 0; ch < AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS; ch++) {
            initialGain[ch] = 1.0f;
            trimGain[ch]    = 0.5f;
         }
         break;
      }

      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }
   TI_align_free(pIn0);
   TI_align_free(pOut);
   TI_align_free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_gainNChTrimSmoothing_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_gainNChTrimSmoothing_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_gainNChTrimSmoothing_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for individual test projects */
#if !defined(__ONESHOTTEST)
int main()
{
   int test_fail = 1, cov_fail = 1;

   uint32_t profile[1024 * 3];

   AUDIOLIB_TEST_init();

   test_fail = test_main(&profile[0]);
   cov_fail  = coverage_test_main();

#if !defined(NO_PRINTF)
   if ((test_fail == 0) && (cov_fail == 0))
      printf("All Test Pass!\n");
   else
      printf("Test Fail!\n");

   int i;
   for (i = 0; i < test_cases; i++) {
      printf("Test %4d: Cold Cycles = %8d, Warm Cycles = %8d, Warm Cycles WRB = %8d\n", i, profile[3 * i],
             profile[3 * i + 1], profile[3 * i + 2]);
   }
#endif

   return test_fail || cov_fail;
}
#endif

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainNChTrimSmoothing_d.c                          */
/* ======================================================================== */
