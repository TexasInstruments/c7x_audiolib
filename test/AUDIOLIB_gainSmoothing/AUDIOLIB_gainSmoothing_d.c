// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

// include AUDIOLIB
#include <audiolib.h>
#include <stddef.h>
// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_gainSmoothing_idat.h"
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

int AUDIOLIB_gainSmoothing_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams2D_t bufParamsOut;
   uint32_t               testNum;
   uint32_t               comparisonDone = 0;

   AUDIOLIB_gainSmoothing_testParams_t *prm;
   AUDIOLIB_gainSmoothing_testParams_t  currPrm;
   AUDIOLIB_gainSmoothing_getTestParams(&prm, &test_cases);

   AUDIOLIB_gainSmoothing_InitArgs kerInitArgs;
   AUDIOLIB_gainSmoothing_SetArgs  kerSetArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 0\n");

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
   int32_t handleSize    = AUDIOLIB_gainSmoothing_getHandleSize(&kerInitArgs);
   /* The kernel vector-loads coefficients from inside the handle, so it must be aligned. */
   AUDIOLIB_kernelHandle handle = TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, handleSize);
   memset(handle , 0, handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 1\n");

   TI_profile_init("AUDIOLIB_gainSmoothing");
   // file IO for EVM benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_gainSmoothing.csv", "w+");
   fprintf(fpOutputCSV,
           "Test ID,Bit Width,inSamples,inChannels,frames,isInterleaved,fadeTime,fadeType, TargetGain,sampling "
           "rate,EST cycles,EVM cycles,Pass/Fail\n");
   for (tpi = 0; tpi < test_cases; tpi++) {
      uint64_t archCycles = 0, estCycles = 0;
      numReps                   = prm[tpi].numReps;
      testNum                   = prm[tpi].testID;
      currPrm                   = prm[tpi];
      kerSetArgs.targetGain     = (float) currPrm.targetGain;
      kerSetArgs.currentGain    = (float) currPrm.currentGain;
      kerSetArgs.fadeTime       = (float) currPrm.fadeTime;
      kerSetArgs.fadeType       = currPrm.fadeType;
      kerInitArgs.samplingRate  = (float) currPrm.samplingRate;
      kerInitArgs.isInterleaved = currPrm.isInterleaved;

      uint8_t isInterleaved = currPrm.isInterleaved;
      int32_t numExecReps   = currPrm.frames;

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;
         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;

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

         // Here, stride is in bytes
         uint32_t inpLeftSizeBytes;
         uint32_t outLeftSizeBytes;

         // Calculate the size of the input and output buffers in bytes

         inpLeftSizeBytes = bufParamsIn.dim_y * bufParamsIn.stride_y;
         outLeftSizeBytes = bufParamsOut.dim_y * bufParamsOut.stride_y;

         void *pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpLeftSizeBytes * numExecReps);

         void *pOut, *pOutCn;
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
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
         if (pIn && pOut && pOutCn) {

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
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 0\n");

            status_init = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 1 status_init %d\n",
                                  status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_gainSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_gainSmoothing_set(handle, &kerSetArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 2 status_init %d\n",
                                  status_init);

            status_opt = AUDIOLIB_gainSmoothing_exec_checkParams(handle, pIn, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 3 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_gainSmoothing_exec(handle, pIn, pOut);
               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();
            }

            /* The following for loop is to call kernel repeatedly so as to
             * train the branch predictor                                   */
            int32_t k;
            for (k = 0; k < (numExecReps - 2); k++) {
               // run warm instruction cache test
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);

               status_opt = AUDIOLIB_gainSmoothing_exec(handle, (void *) ((int8_t *) pIn + (k + 1) * inpLeftSizeBytes),
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

            if (numExecReps != 1) {
               // run warm instruction cache test
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);

               status_opt =
                   AUDIOLIB_gainSmoothing_exec(handle, (void *) ((int8_t *) pIn + (numExecReps - 1) * inpLeftSizeBytes),
                                               (void *) ((int8_t *) pOut + (numExecReps - 1) * outLeftSizeBytes));
               TI_profile_stop();
            }

            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_gainSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
            AUDIOLIB_gainSmoothing_set(handle, &kerSetArgs);
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            for (k = 0; k < numExecReps; k++) {
               status_nat = AUDIOLIB_gainSmoothing_exec(handle, (void *) ((int8_t *) pIn + k * inpLeftSizeBytes),
                                                        (void *) ((int8_t *) pOutCn + k * outLeftSizeBytes));
            }
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 6 status_nat %d\n", status_nat);
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
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 7 comparisonDone %d "
                                  "status_nat_vs_opt %d\n",
                                  comparisonDone, status_nat_vs_opt);

            if (currPrm.staticOut != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
               bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);
               // print_matrix("staticOut", (void *) currPrm.staticOut, &bufParamsOutTemp);

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
               }

               comparisonDone = 1;
            }
            else {
               /* Set to pass since it wasn't supposed to run. */
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;
            }
            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 8 status_nat_vs_opt %d "
                                  "status_ref_vs_opt %d currentTestFail %d\n",
                                  status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_gainSmoothing_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_gainSmoothing_get(handle, &kerSetArgs);
            sprintf(desc,
                    "%s generated input | inSamples = %d, inChannels = %d, currentGain = %0.3f, TargetGain = %0.3f, "
                    "fadeTime = %0.3f ms, dataType = %d",
                    testPatternString, currPrm.numSamples, currPrm.numChannels, kerSetArgs.currentGain,
                    kerSetArgs.targetGain, kerSetArgs.fadeTime, currPrm.sampleDataType);

            AUDIOLIB_gainSmoothing_perfEst(handle, &archCycles, &estCycles);

            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            fprintf(fpOutputCSV, "%d,%d,%d,%d,%d,%u,%f,%u,%f,%d,%lld,%d,%d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.numSamples, currPrm.numChannels,
                    currPrm.frames, currPrm.isInterleaved, currPrm.fadeTime, currPrm.fadeType,
                    (float) currPrm.targetGain, currPrm.samplingRate, (long long) estCycles,
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
      TI_memError("AUDIOLIB_gainSmoothing");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_gainSmoothing_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                         testNum         = 1000;
   int32_t                         currentTestFail = 0;
   AUDIOLIB_gainSmoothing_InitArgs kerInitArgs;

   /* Init args are read by init_checkParams, so give them valid values rather than
      letting the tests depend on whatever is on the stack. */
   kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.samplingRate  = 48000.0f;
   kerInitArgs.isInterleaved = 0;

   int32_t               handleSize = AUDIOLIB_gainSmoothing_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;
   AUDIOLIB_STATUS status_init;

   AUDIOLIB_gainSmoothing_SetArgs setArgs;

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   int   fail = 0;
   void *pIn0, *pOut;

   int32_t inSamples  = 16;
   int32_t inChannels = 16;

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

   pIn0 = (void *) malloc(In0Size);
   pOut = (void *) malloc(OutSize);
   AUDIOLIB_gainSmoothing_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

   while (testNum <= 1010) {

      switch (testNum) {
      case 1000:
         /* init_checkParams(): NULL handle (nat), NULL input buffer descriptor (opt). */
         status_nat      = AUDIOLIB_gainSmoothing_init_checkParams(NULL, &bufParamsIn, &bufParamsOut, &kerInitArgs);
         status_opt      = AUDIOLIB_gainSmoothing_init_checkParams(handle, NULL, &bufParamsOut, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1001:
         /* init_checkParams(): NULL output buffer descriptor (nat), NULL init args (opt). */
         status_nat      = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, NULL, &kerInitArgs);
         status_opt      = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOut, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1002:
         /* init_checkParams(): FLOAT64 input (nat), output type differs from input (opt). */
         bufParamsInTemp            = bufParamsIn;
         bufParamsOutTemp           = bufParamsOut;
         bufParamsInTemp.data_type  = AUDIOLIB_FLOAT64;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT64;
         status_nat =
             AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         status_opt = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1003:
         /* init_checkParams(): zero sampling rate (nat), out-of-range isInterleaved (opt). */
         kerInitArgs.samplingRate = 0.0f;
         status_nat = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
         kerInitArgs.samplingRate  = 48000.0f;
         kerInitArgs.isInterleaved = 2;
         status_opt = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
         kerInitArgs.isInterleaved = 0;
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) || (status_opt != AUDIOLIB_ERR_INVALID_VALUE));
         break;

      case 1004:
         /* init_checkParams(): zero width (nat), zero height (opt). */
         bufParamsInTemp        = bufParamsIn;
         bufParamsOutTemp       = bufParamsOut;
         bufParamsInTemp.dim_x  = 0;
         bufParamsOutTemp.dim_x = 0;
         status_nat =
             AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         bufParamsInTemp        = bufParamsIn;
         bufParamsOutTemp       = bufParamsOut;
         bufParamsInTemp.dim_y  = 0;
         bufParamsOutTemp.dim_y = 0;
         status_opt =
             AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) || (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION));
         break;

      case 1005:
         /* init_checkParams(): output width (nat) and output height (opt) differ from the input. */
         bufParamsOutTemp       = bufParamsOut;
         bufParamsOutTemp.dim_x = bufParamsIn.dim_x - 1;
         status_nat = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOutTemp, &kerInitArgs);
         bufParamsOutTemp       = bufParamsOut;
         bufParamsOutTemp.dim_y = bufParamsIn.dim_y + 1;
         status_opt = AUDIOLIB_gainSmoothing_init_checkParams(handle, &bufParamsIn, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) || (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION));
         break;

      case 1006:
         /* set(): NULL handle (nat), NULL set args (opt). */
         setArgs.currentGain = 1.0f;
         setArgs.targetGain  = 2.0f;
         setArgs.fadeTime    = 10.0f;
         setArgs.fadeType    = AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC;
         status_nat          = AUDIOLIB_gainSmoothing_set(NULL, &setArgs);
         status_opt          = AUDIOLIB_gainSmoothing_set(handle, NULL);
         currentTestFail     = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1007:
         /* set(): fadeType out of range (nat), negative fadeTime (opt). */
         setArgs.currentGain = 1.0f;
         setArgs.targetGain  = 2.0f;
         setArgs.fadeTime    = 10.0f;
         setArgs.fadeType    = AUDIOLIB_GAINSMOOTHING_FADE_TYPE_MAX + 1;
         status_nat          = AUDIOLIB_gainSmoothing_set(handle, &setArgs);
         setArgs.fadeType    = AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC;
         setArgs.fadeTime    = -1.0f;
         status_opt          = AUDIOLIB_gainSmoothing_set(handle, &setArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) || (status_opt != AUDIOLIB_ERR_INVALID_VALUE));
         break;

      case 1008:
         /* set(): a zero gain is rejected by SYMMETRIC (nat, pow() would give inf/NaN) but
            accepted by SMOOTH (opt). */
         setArgs.currentGain = 0.0f;
         setArgs.targetGain  = 2.0f;
         setArgs.fadeTime    = 10.0f;
         setArgs.fadeType    = AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC;
         status_nat          = AUDIOLIB_gainSmoothing_set(handle, &setArgs);
         setArgs.fadeType    = AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH;
         status_opt          = AUDIOLIB_gainSmoothing_set(handle, &setArgs);
         currentTestFail     = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) || (status_opt != AUDIOLIB_SUCCESS));
         break;

      case 1009:
         /* get(): NULL handle (nat), NULL set args (opt). */
         status_nat      = AUDIOLIB_gainSmoothing_get(NULL, &setArgs);
         status_opt      = AUDIOLIB_gainSmoothing_get(handle, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1010:
         /* exec_checkParams(): NULL handle (nat), NULL input (opt), NULL output (init). */
         status_nat      = AUDIOLIB_gainSmoothing_exec_checkParams(NULL, pIn0, pOut);
         status_opt      = AUDIOLIB_gainSmoothing_exec_checkParams(handle, NULL, pOut);
         status_init     = AUDIOLIB_gainSmoothing_exec_checkParams(handle, pIn0, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER) ||
                            (status_init != AUDIOLIB_ERR_NULL_POINTER));
         break;

      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }
   free(pIn0);
   free(pOut);
   TI_align_free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_gainSmoothing_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_gainSmoothing_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_gainSmoothing_getHandleSize(&kerInitArgs);
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

   return test_fail && cov_fail;
}
#endif

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainSmoothing_d.c                          */
/* ======================================================================== */
