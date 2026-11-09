// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>
// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_muteNCh_idat.h"
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

int AUDIOLIB_muteNCh_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
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

   AUDIOLIB_muteNCh_testParams_t *prm;
   AUDIOLIB_muteNCh_testParams_t  currPrm;
   AUDIOLIB_muteNCh_getTestParams(&prm, &test_cases);

   AUDIOLIB_muteNCh_InitArgs kerInitArgs;
   AUDIOLIB_muteNCh_SetArgs  kerSetArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 0\n");

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 1\n");

   TI_profile_init("AUDIOLIB_muteNCh");
   // file IO for EVM benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_muteNCh.csv", "w+");
   fprintf(fpOutputCSV, "Test ID,Bit Width,inSamples,inChannels,frames,isInterleaved,fadeTime,fadeType,sampling "
                        "rate,EST cycles,EVM cycles,Pass/Fail\n");
   for (tpi = 0; tpi < test_cases; tpi++) {
      uint64_t archCycles = 0, estCycles = 0;
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      kerSetArgs.isMute = (uint32_t *) malloc(sizeof(uint32_t) * currPrm.numChannels);
      if (currPrm.isMute != NULL) {
         memcpy(kerSetArgs.isMute, currPrm.isMute, sizeof(uint32_t) * currPrm.numChannels);
      }
      else {
         // Handle null: zero out or error
         memset(kerSetArgs.isMute, 0, sizeof(uint32_t) * currPrm.numChannels);
      }
      kerSetArgs.fadeTime       = currPrm.fadeTime;
      kerSetArgs.fadeType       = currPrm.fadeType;
      kerInitArgs.samplingRate  = currPrm.samplingRate;
      kerInitArgs.isInterleaved = currPrm.isInterleaved;
      kerInitArgs.numChannels   = currPrm.numChannels;

      int32_t               handleSize = AUDIOLIB_muteNCh_getHandleSize(&kerInitArgs);
      AUDIOLIB_kernelHandle handle     = malloc(handleSize);

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
            bufParamsIn.stride_y  = bufParamsIn.dim_x * AUDIOLIB_sizeof(bufParamsIn.data_type);

            bufParamsOut.data_type = currPrm.sampleDataType;
            bufParamsOut.dim_x     = currPrm.numChannels;
            bufParamsOut.dim_y     = currPrm.numSamples;
            bufParamsOut.stride_y  = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);
         }
         else {
            bufParamsIn.data_type = currPrm.sampleDataType;
            bufParamsIn.dim_x     = currPrm.numSamples;
            bufParamsIn.dim_y     = currPrm.numChannels;
            bufParamsIn.stride_y  = bufParamsIn.dim_x * AUDIOLIB_sizeof(bufParamsIn.data_type);

            bufParamsOut.data_type = currPrm.sampleDataType;
            bufParamsOut.dim_x     = currPrm.numSamples;
            bufParamsOut.dim_y     = currPrm.numChannels;
            bufParamsOut.stride_y  = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);
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
                      prm[tpi].testPattern, 0, (void *) ((int8_t *) pIn + frame * inpLeftSizeBytes),
                      (void *) ((float *) prm[tpi].staticIn + (frame * currPrm.numSamples * currPrm.numChannels)),
                      bufParamsIn.dim_x, bufParamsIn.dim_y, bufParamsIn.stride_y,
                      AUDIOLIB_sizeof(currPrm.sampleDataType), testPatternString);
               }
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 0\n");

            status_init = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_muteNCh_set(handle, &kerSetArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 2 status_init %d\n", status_init);

            status_opt = AUDIOLIB_muteNCh_exec_checkParams(handle, pIn, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 3 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_muteNCh_exec(handle, pIn, pOut);
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
               AUDIOLIB_asm(" MARK 4");
               status_opt = AUDIOLIB_muteNCh_exec(handle, (void *) ((int8_t *) pIn + (k + 1) * inpLeftSizeBytes),
                                                  (void *) ((int8_t *) pOut + (k + 1) * outLeftSizeBytes));
               ;
               AUDIOLIB_asm(" MARK 5");
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
               AUDIOLIB_asm(" MARK 6");
               status_opt =
                   AUDIOLIB_muteNCh_exec(handle, (void *) ((int8_t *) pIn + (numExecReps - 1) * inpLeftSizeBytes),
                                         (void *) ((int8_t *) pOut + (numExecReps - 1) * outLeftSizeBytes));
               AUDIOLIB_asm(" MARK 7");
               TI_profile_stop();
            }

            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
            AUDIOLIB_muteNCh_set(handle, &kerSetArgs);
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            for (k = 0; k < numExecReps; k++) {
               status_nat = AUDIOLIB_muteNCh_exec(handle, (void *) ((int8_t *) pIn + k * inpLeftSizeBytes),
                                                  (void *) ((int8_t *) pOutCn + k * outLeftSizeBytes));
            }
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 6 status_nat %d\n", status_nat);
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
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 7 comparisonDone %d status_nat_vs_opt %d "
                                  "status_nat_vs_opt_right %d\n",
                                  comparisonDone, status_nat_vs_opt, status_nat_vs_opt_right);

            if (currPrm.staticOut != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
               bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

               if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

                  // Initialize reference comparison statuses
                  status_ref_vs_opt = TI_TEST_KERNEL_PASS;
                  int32_t frame     = 0;
                  // Frame-by-frame comparison for reference outputs
                  for (frame = 0; frame < numExecReps; frame++) {
                     int32_t frame_status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                         (void *) ((int8_t *) pOut + frame * outLeftSizeBytes),
                         (void *) ((float *) currPrm.staticOut + frame * currPrm.numSamples * currPrm.numChannels),
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
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 8 status_nat_vs_opt %d status_ref_vs_opt "
                                  "%d  status_nat_vs_opt_right %d status_ref_vs_opt_right %d currentTestFail "
                                  "%d\n",
                                  status_nat_vs_opt, status_ref_vs_opt, status_nat_vs_opt_right,
                                  status_ref_vs_opt_right, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_muteNCh_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_muteNCh_get(handle, &kerSetArgs);
            sprintf(desc, "%s generated input | inSamples = %d, inChannels = %d, fadeTime = %0.3f ms",
                    testPatternString, currPrm.numSamples, currPrm.numChannels, kerSetArgs.fadeTime);

            AUDIOLIB_muteNCh_perfEst(handle, &archCycles, &estCycles);

            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            fprintf(fpOutputCSV, "%d,%d,%d,%d,%d,%u,%f,%u,%d,%lld,%d,%d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.numSamples, currPrm.numChannels,
                    currPrm.frames, currPrm.isInterleaved, currPrm.fadeTime, currPrm.fadeType, currPrm.samplingRate,
                    (long long) estCycles,
                    pProfile[3 * tpi + 1], // EVM cycles (warm cycles)
                    !currentTestFail);

            TI_profile_add_test(testNum++, currPrm.numSamples * currPrm.numChannels, archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_muteNCh_test
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
      free(handle);
      free(kerSetArgs.isMute);
   } // end idat test cases
   /* Close results CSV */
   fclose(fpOutputCSV);
   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_muteNCh");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_muteNCh_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                   testNum         = 1000;
   int32_t                   currentTestFail = 0;
   AUDIOLIB_muteNCh_InitArgs kerInitArgs;
   int32_t                   inSamples  = 16;
   int32_t                   inChannels = 16;

   memset(&kerInitArgs, 0, sizeof(kerInitArgs));
   kerInitArgs.numChannels = inChannels;

   int32_t               handleSize = AUDIOLIB_muteNCh_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   int   fail = 0;
   void *pIn0, *pOut;

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

   /* Valid per-channel mute-flag array (all unmuted) reused by the set() coverage cases. */
   uint32_t *covMuteFlags = (uint32_t *) malloc(inChannels * sizeof(uint32_t));
   int32_t   c;
   for (c = 0; c < inChannels; c++) {
      covMuteFlags[c] = 0;
   }

   while (testNum <= 1017) {

      switch (testNum) {
      case 1000:
         bufParamsInTemp.data_type  = bufParamsIn.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsInTemp.dim_x  = bufParamsIn.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y  = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_muteNCh_init_checkParams(NULL, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_muteNCh_init_checkParams(NULL, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1001:

         bufParamsInTemp.data_type  = AUDIOLIB_UINT32;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsInTemp.dim_x  = bufParamsIn.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y  = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1002:
         bufParamsInTemp.data_type  = bufParamsIn.data_type;
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;

         bufParamsInTemp.dim_x  = bufParamsIn.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y  = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         bufParamsInTemp.data_type  = AUDIOLIB_UINT32;
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;
         kerInitArgs.funcStyle      = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1003: {
         /* AUDIOLIB_muteNCh_set: NULL handle / NULL pKerSetArgs -> AUDIOLIB_ERR_NULL_POINTER */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         covSetArgs.isMute   = covMuteFlags;
         covSetArgs.fadeTime = 5.0f;
         covSetArgs.fadeType = 0; /* LINEAR */
         status_nat          = AUDIOLIB_muteNCh_set(NULL, &covSetArgs);
         status_opt          = AUDIOLIB_muteNCh_set(handle, NULL);
         currentTestFail     = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      }
      case 1004: {
         /* AUDIOLIB_muteNCh_set: NULL isMute array -> AUDIOLIB_ERR_NULL_POINTER */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         covSetArgs.isMute   = NULL;
         covSetArgs.fadeTime = 5.0f;
         covSetArgs.fadeType = 0; /* LINEAR */
         status_opt          = AUDIOLIB_muteNCh_set(handle, &covSetArgs);
         currentTestFail     = (status_opt != AUDIOLIB_ERR_NULL_POINTER);
         break;
      }
      case 1005: {
         /* AUDIOLIB_muteNCh_set: fadeType out of range (valid 0..3) -> AUDIOLIB_ERR_INVALID_VALUE */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         covSetArgs.isMute   = covMuteFlags;
         covSetArgs.fadeTime = 5.0f;
         covSetArgs.fadeType = 4; /* invalid */
         status_opt          = AUDIOLIB_muteNCh_set(handle, &covSetArgs);
         currentTestFail     = (status_opt != AUDIOLIB_ERR_INVALID_VALUE);
         break;
      }
      case 1006: {
         /* AUDIOLIB_muteNCh_set: negative fadeTime -> AUDIOLIB_ERR_INVALID_VALUE */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         covSetArgs.isMute   = covMuteFlags;
         covSetArgs.fadeTime = -1.0f;
         covSetArgs.fadeType = 1; /* SMOOTH */
         status_opt          = AUDIOLIB_muteNCh_set(handle, &covSetArgs);
         currentTestFail     = (status_opt != AUDIOLIB_ERR_INVALID_VALUE);
         break;
      }
      case 1007: {
         /* AUDIOLIB_muteNCh_set: per-channel mute flag not 0/1 -> AUDIOLIB_ERR_INVALID_VALUE.
          * init() first so pKerPrivArgs->channels is populated before the per-channel check. */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         uint32_t                 covBadFlags[16];
         int32_t                  c;
         for (c = 0; c < inChannels; c++) {
            covBadFlags[c] = 0;
         }
         covBadFlags[5]        = 2; /* out-of-range flag */
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
         covSetArgs.isMute   = covBadFlags;
         covSetArgs.fadeTime = 5.0f;
         covSetArgs.fadeType = 0; /* LINEAR */
         status_opt          = AUDIOLIB_muteNCh_set(handle, &covSetArgs);
         currentTestFail     = (status_opt != AUDIOLIB_ERR_INVALID_VALUE);
         break;
      }
      case 1008:
         /* init_checkParams: numChannels != input channel axis -> AUDIOLIB_ERR_INVALID_DIMENSION
          * (kerInitArgs.numChannels = 16, non-interleaved channel axis = dim_y = 8). */
         bufParamsInTemp.data_type  = bufParamsIn.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;
         bufParamsInTemp.dim_x      = bufParamsIn.dim_x;
         bufParamsInTemp.dim_y      = 8;
         bufParamsOutTemp.dim_x     = bufParamsOut.dim_x;
         bufParamsOutTemp.dim_y     = 8;
         kerInitArgs.funcStyle      = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION);
         break;
      case 1009:
         /* init_checkParams: input/output dims differ -> AUDIOLIB_ERR_INVALID_DIMENSION
          * (numChannels matches input channel axis, but out dim_y differs). */
         bufParamsInTemp.data_type  = bufParamsIn.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;
         bufParamsInTemp.dim_x      = bufParamsIn.dim_x;
         bufParamsInTemp.dim_y      = bufParamsIn.dim_y; /* = numChannels (16) */
         bufParamsOutTemp.dim_x     = bufParamsOut.dim_x;
         bufParamsOutTemp.dim_y     = 8; /* != input dim_y */
         kerInitArgs.funcStyle      = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION);
         break;
      case 1010:
         /* init_checkParams: NULL pKerInitArgs -> AUDIOLIB_ERR_NULL_POINTER (new operand). */
         bufParamsInTemp.data_type  = bufParamsIn.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;
         bufParamsInTemp.dim_x      = bufParamsIn.dim_x;
         bufParamsInTemp.dim_y      = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_x     = bufParamsOut.dim_x;
         bufParamsOutTemp.dim_y     = bufParamsOut.dim_y;
         status_opt      = AUDIOLIB_muteNCh_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, NULL);
         currentTestFail = (status_opt != AUDIOLIB_ERR_NULL_POINTER);
         break;
      case 1011:
         /* init(): NULL handle -> AUDIOLIB_ERR_NULL_POINTER. */
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_muteNCh_init(NULL, &bufParamsIn, &bufParamsOut, &kerInitArgs);
         currentTestFail       = (status_opt != AUDIOLIB_ERR_NULL_POINTER);
         break;
      case 1012:
         /* exec_checkParams: NULL handle -> NULL_POINTER; all-valid -> SUCCESS. */
         status_nat      = AUDIOLIB_muteNCh_exec_checkParams(NULL, pIn0, pOut);
         status_opt      = AUDIOLIB_muteNCh_exec_checkParams(handle, pIn0, pOut);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_SUCCESS));
         break;
      case 1013: {
         /* A smooth unmute settles on exactly unity gain: after 30 time constants every output sample equals its input
          * sample, bit for bit, in both layouts and both implementations (1 ms at 48 kHz is 48 samples, 90 frames of
          * 16). */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         int32_t                  layout, style, frame, i;
         for (i = 0; i < inSamples * inChannels; i++) {
            ((float *) pIn0)[i] = (float) (i + 1) / 7.0f;
         }
         covSetArgs.isMute   = covMuteFlags;
         covSetArgs.fadeTime = 1.0f;
         covSetArgs.fadeType = 1; /* SMOOTH */
         currentTestFail     = 0;
         for (layout = 0; layout < 2; layout++) {
            for (style = 0; style < 2; style++) {
               kerInitArgs.samplingRate  = 48000;
               kerInitArgs.isInterleaved = (uint8_t) layout;
               kerInitArgs.funcStyle     = (style == 0) ? AUDIOLIB_FUNCTION_NATC : AUDIOLIB_FUNCTION_OPTIMIZED;
               AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_muteNCh_set(handle, &covSetArgs);
               for (frame = 0; frame < 90; frame++) {
                  AUDIOLIB_muteNCh_exec(handle, pIn0, pOut);
               }
               if (memcmp(pIn0, pOut, OutSize) != 0) {
                  currentTestFail = 1;
               }
            }
         }
         kerInitArgs.isInterleaved = 0;
         break;
      }
      case 1014: {
         /* A later set() changes only the channels whose flag changed. Channel 0 is muted and every other channel
          * unmuted; once all are at rest, a second set() also mutes channel 1. Channel 0 must stay at exactly 0 and
          * channels 2..15 at exactly 1, while channel 1 starts its fade from 1. Linear, smooth and symmetric fades, 1
          * ms at 48 kHz; 60 frames of 16 samples bring every channel to rest. */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         const uint32_t           fadeTypes[3] = {0, 1, 3}; /* LINEAR, SMOOTH, SYMMETRIC */
         int32_t                  layout, style, fade, frame, ch, s, idx;
         for (s = 0; s < inSamples * inChannels; s++) {
            ((float *) pIn0)[s] = 1.0f;
         }
         covSetArgs.isMute   = covMuteFlags;
         covSetArgs.fadeTime = 1.0f;
         currentTestFail     = 0;
         for (fade = 0; fade < 3; fade++) {
            for (layout = 0; layout < 2; layout++) {
               for (style = 0; style < 2; style++) {
                  covSetArgs.fadeType       = fadeTypes[fade];
                  kerInitArgs.samplingRate  = 48000;
                  kerInitArgs.isInterleaved = (uint8_t) layout;
                  kerInitArgs.funcStyle     = (style == 0) ? AUDIOLIB_FUNCTION_NATC : AUDIOLIB_FUNCTION_OPTIMIZED;
                  AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                  covMuteFlags[0] = 1;
                  covMuteFlags[1] = 0;
                  AUDIOLIB_muteNCh_set(handle, &covSetArgs);
                  for (frame = 0; frame < 60; frame++) {
                     AUDIOLIB_muteNCh_exec(handle, pIn0, pOut);
                  }
                  covMuteFlags[1] = 1;
                  AUDIOLIB_muteNCh_set(handle, &covSetArgs);
                  AUDIOLIB_muteNCh_exec(handle, pIn0, pOut);
                  for (ch = 0; ch < inChannels; ch++) {
                     for (s = 0; s < inSamples; s++) {
                        idx = (layout == 1) ? (s * inChannels + ch) : (ch * inSamples + s);
                        if (ch == 1) {
                           int32_t prev = (layout == 1) ? ((s - 1) * inChannels + ch) : (ch * inSamples + s - 1);
                           if (!(((float *) pOut)[idx] < 1.0f) || ((s == 0) && !(((float *) pOut)[idx] > 0.5f)) ||
                               ((s > 0) && (((float *) pOut)[idx] > ((float *) pOut)[prev]))) {
                              currentTestFail = 1;
                           }
                        }
                        else if (((float *) pOut)[idx] != ((ch == 0) ? 0.0f : 1.0f)) {
                           currentTestFail = 1;
                        }
                     }
                  }
               }
            }
         }
         covMuteFlags[0]           = 0;
         covMuteFlags[1]           = 0;
         kerInitArgs.isInterleaved = 0;
         break;
      }
      case 1015: {
         /* A set() during a fade turns the fade round from the running gain, without a jump: every channel fades in
          * for one frame, then a second set() mutes them all. The first sample after it must be no higher than the
          * last sample before it, and no lower than half of it. Linear, smooth and symmetric fades, 1 ms at 48 kHz. */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         const uint32_t           fadeTypes[3] = {0, 1, 3}; /* LINEAR, SMOOTH, SYMMETRIC */
         int32_t                  layout, style, fade, c2, s;
         for (s = 0; s < inSamples * inChannels; s++) {
            ((float *) pIn0)[s] = 1.0f;
         }
         covSetArgs.isMute   = covMuteFlags;
         covSetArgs.fadeTime = 1.0f;
         currentTestFail     = 0;
         for (fade = 0; fade < 3; fade++) {
            for (layout = 0; layout < 2; layout++) {
               for (style = 0; style < 2; style++) {
                  /* Channel 0 and the last channel: the first and the last lane */
                  const int32_t checkCh[2] = {0, inChannels - 1};
                  float         lastGain[2];
                  covSetArgs.fadeType       = fadeTypes[fade];
                  kerInitArgs.samplingRate  = 48000;
                  kerInitArgs.isInterleaved = (uint8_t) layout;
                  kerInitArgs.funcStyle     = (style == 0) ? AUDIOLIB_FUNCTION_NATC : AUDIOLIB_FUNCTION_OPTIMIZED;
                  AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                  for (c = 0; c < inChannels; c++) {
                     covMuteFlags[c] = 0;
                  }
                  AUDIOLIB_muteNCh_set(handle, &covSetArgs);
                  AUDIOLIB_muteNCh_exec(handle, pIn0, pOut);
                  for (c2 = 0; c2 < 2; c2++) {
                     lastGain[c2] = ((float *) pOut)[(layout == 1) ? ((inSamples - 1) * inChannels + checkCh[c2])
                                                                   : (checkCh[c2] * inSamples + inSamples - 1)];
                  }
                  for (c = 0; c < inChannels; c++) {
                     covMuteFlags[c] = 1;
                  }
                  AUDIOLIB_muteNCh_set(handle, &covSetArgs);
                  AUDIOLIB_muteNCh_exec(handle, pIn0, pOut);
                  for (c2 = 0; c2 < 2; c2++) {
                     float firstGain = ((float *) pOut)[(layout == 1) ? checkCh[c2] : (checkCh[c2] * inSamples)];
                     if (!(lastGain[c2] > 0.0f) || (firstGain > lastGain[c2]) || (firstGain < 0.5f * lastGain[c2])) {
                        currentTestFail = 1;
                     }
                  }
               }
            }
         }
         for (c = 0; c < inChannels; c++) {
            covMuteFlags[c] = 0;
         }
         kerInitArgs.isInterleaved = 0;
         break;
      }
      case 1016: {
         /* A hard change leaves each channel at rest on its target, and the next fade starts from there: even channels
          * muted and odd channels unmuted with a hard set(), then a fade that unmutes channel 0 and mutes channel 1.
          * The other channels must stay at exactly 0 (even) and 1 (odd); channel 0 rises from 0 and channel 1 falls
          * from 1. Linear, smooth and symmetric fades, 1 ms at 48 kHz. */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         const uint32_t           fadeTypes[3] = {0, 1, 3}; /* LINEAR, SMOOTH, SYMMETRIC */
         int32_t                  layout, style, fade, ch, s, idx;
         for (s = 0; s < inSamples * inChannels; s++) {
            ((float *) pIn0)[s] = 1.0f;
         }
         covSetArgs.isMute   = covMuteFlags;
         covSetArgs.fadeTime = 1.0f;
         currentTestFail     = 0;
         for (fade = 0; fade < 3; fade++) {
            for (layout = 0; layout < 2; layout++) {
               for (style = 0; style < 2; style++) {
                  kerInitArgs.samplingRate  = 48000;
                  kerInitArgs.isInterleaved = (uint8_t) layout;
                  kerInitArgs.funcStyle     = (style == 0) ? AUDIOLIB_FUNCTION_NATC : AUDIOLIB_FUNCTION_OPTIMIZED;
                  AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
                  for (c = 0; c < inChannels; c++) {
                     covMuteFlags[c] = ((c % 2) == 0) ? 1 : 0;
                  }
                  covSetArgs.fadeType = 2; /* HARD */
                  AUDIOLIB_muteNCh_set(handle, &covSetArgs);
                  AUDIOLIB_muteNCh_exec(handle, pIn0, pOut);
                  covMuteFlags[0]     = 0;
                  covMuteFlags[1]     = 1;
                  covSetArgs.fadeType = fadeTypes[fade];
                  AUDIOLIB_muteNCh_set(handle, &covSetArgs);
                  AUDIOLIB_muteNCh_exec(handle, pIn0, pOut);
                  for (ch = 0; ch < inChannels; ch++) {
                     for (s = 0; s < inSamples; s++) {
                        idx = (layout == 1) ? (s * inChannels + ch) : (ch * inSamples + s);
                        if (ch == 0) {
                           /* A symmetric fade starts on the -200 dB floor, which is output as 0 */
                           if (((s == 0) && !((((float *) pOut)[idx] >= 0.0f) && (((float *) pOut)[idx] < 0.1f))) ||
                               ((s == inSamples - 1) && !(((float *) pOut)[idx] > 0.0f))) {
                              currentTestFail = 1;
                           }
                        }
                        else if (ch == 1) {
                           if ((s == 0) && !((((float *) pOut)[idx] > 0.5f) && (((float *) pOut)[idx] < 1.0f))) {
                              currentTestFail = 1;
                           }
                        }
                        else if (((float *) pOut)[idx] != (((ch % 2) == 0) ? 0.0f : 1.0f)) {
                           currentTestFail = 1;
                        }
                     }
                  }
               }
            }
         }
         for (c = 0; c < inChannels; c++) {
            covMuteFlags[c] = 0;
         }
         kerInitArgs.isInterleaved = 0;
         break;
      }
      case 1017: {
         /* perfEst follows the exec dispatch: while a fade runs it estimates the fading variant of the layout, and once
          * a linear or symmetric fade has converged it estimates the hard path. Every channel unmutes from the muted
          * initial state; 1 ms at 48 kHz is 48 samples, so 10 frames of 16 bring a linear or symmetric fade to rest
          * (a smooth fade never converges). Linear, smooth and symmetric fades, both layouts. */
         AUDIOLIB_muteNCh_SetArgs covSetArgs;
         const uint32_t           fadeTypes[3] = {0, 1, 3}; /* LINEAR, SMOOTH, SYMMETRIC */
         int32_t                  layout, fade, frame;
         uint64_t                 archFade, estFade, archDone, estDone, archHard, estHard;
         covSetArgs.isMute   = covMuteFlags;
         covSetArgs.fadeTime = 1.0f;
         currentTestFail     = 0;
         for (fade = 0; fade < 3; fade++) {
            for (layout = 0; layout < 2; layout++) {
               covSetArgs.fadeType       = fadeTypes[fade];
               kerInitArgs.samplingRate  = 48000;
               kerInitArgs.isInterleaved = (uint8_t) layout;
               kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_OPTIMIZED;
               AUDIOLIB_muteNCh_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_muteNCh_set(handle, &covSetArgs);
               AUDIOLIB_muteNCh_perfEst(handle, &archFade, &estFade);
               for (frame = 0; frame < 10; frame++) {
                  AUDIOLIB_muteNCh_exec(handle, pIn0, pOut);
               }
               AUDIOLIB_muteNCh_perfEst(handle, &archDone, &estDone);
               covSetArgs.fadeType = 2; /* HARD */
               AUDIOLIB_muteNCh_set(handle, &covSetArgs);
               AUDIOLIB_muteNCh_perfEst(handle, &archHard, &estHard);
               if ((archFade == 0U) || (estFade <= archFade) || (archFade == archHard) ||
                   ((fadeTypes[fade] != 1U) && ((archDone != archHard) || (estDone != estHard)))) {
                  currentTestFail = 1;
               }
            }
         }
         kerInitArgs.isInterleaved = 0;
         break;
      }

      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }
   free(pIn0);
   free(pOut);
   free(covMuteFlags);
   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_muteNCh_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_muteNCh_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_muteNCh_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for inmuteNChidual test projects */
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
