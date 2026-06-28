// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>
// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_types.h"
#include "AUDIOLIB_whiteNoiseGenerator_idat.h"

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

int AUDIOLIB_whiteNoiseGenerator_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams1D_t bufParamsOut;
   uint32_t               testNum;
   uint32_t               comparisonDone = 0;

   AUDIOLIB_whiteNoiseGenerator_testParams_t *prm;
   AUDIOLIB_whiteNoiseGenerator_testParams_t  currPrm;
   AUDIOLIB_whiteNoiseGenerator_getTestParams(&prm, &test_cases);

   AUDIOLIB_whiteNoiseGenerator_InitArgs kerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 0\n");

   int32_t               handleSize = AUDIOLIB_whiteNoiseGenerator_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 1\n");

   TI_profile_init("AUDIOLIB_whiteNoiseGenerator");

   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_whiteNoiseGenerator.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, frames,"
                        "Arch cycles, EVM cycles, EVM/EST cycles, Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      uint64_t archCycles = 0, estCycles = 0;
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      kerInitArgs.seed    = currPrm.seed;
      kerInitArgs.range   = currPrm.range;
      int32_t numExecReps = currPrm.numExecReps;

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t status_ref_vs_opt = TI_TEST_KERNEL_FAIL;

         AUDIOLIB_STATUS status_init = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt  = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat  = AUDIOLIB_SUCCESS;

         currentTestFail = 0;

         bufParamsOut.data_type = currPrm.sampleDataType;
         bufParamsOut.dim_x     = currPrm.numSamples;

         uint32_t outSizeBytes = bufParamsOut.dim_x * bufParamsOut.data_type;

         void *pOut, *pOutCn;
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes * numExecReps);
            pOutCn = (void *) malloc(outSizeBytes * numExecReps);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes * numExecReps);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes * numExecReps);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }
         memset(pOut, 0, outSizeBytes * numExecReps);
         AUDIOLIB_DEBUGPRINTFN(0, "pOut: %p pOutCn: %p\n", pOut, pOutCn);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pOut && pOutCn) {
            if (prm[tpi].testPattern == STATIC) {
               sprintf(testPatternString, "STATIC");
            }
            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 0\n");

            status_init = AUDIOLIB_whiteNoiseGenerator_init_checkParams(handle, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 1 status_init %d\n",
                                  status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_whiteNoiseGenerator_init(handle, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 2 status_init %d\n",
                                  status_init);

            status_opt = AUDIOLIB_whiteNoiseGenerator_exec_checkParams(handle, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 3 status_opt %d\n",
                                  status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_whiteNoiseGenerator_exec(handle, pOut);
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
               status_opt =
                   AUDIOLIB_whiteNoiseGenerator_exec(handle, (void *) ((int8_t *) pOut + (k + 1) * outSizeBytes));
               AUDIOLIB_asm(" MARK 5");
               TI_profile_stop();
            }

            // get output to L1D
            int16_t  outSum   = 0;
            int8_t  *pOutTemp = (int8_t *) pOut; // treat output as bytes to be data type agnostic
            uint32_t j        = 0;
            for (j = 0; j < bufParamsOut.dim_x; j++) {
               outSum += *pOutTemp;
               pOutTemp++;
            }

            // dummy store of outSum to insure that the compiler does not remove it.
            volatileSum = outSum;

            if (numExecReps != 1) {
               // run warm instruction cache test
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
               AUDIOLIB_asm(" MARK 6");
               status_opt = AUDIOLIB_whiteNoiseGenerator_exec(
                   handle, (void *) ((int8_t *) pOut + (numExecReps - 1) * outSizeBytes));
               AUDIOLIB_asm(" MARK 7");
               TI_profile_stop();
            }

            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 4 status_opt %d\n",
                                  status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_whiteNoiseGenerator_init(handle, &bufParamsOut, &kerInitArgs);
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 5\n");
#endif
            for (k = 0; k < numExecReps; k++) {
               TI_profile_start(TI_PROFILE_KERNEL_CN);
               AUDIOLIB_asm(" MARK 8");
               status_nat = AUDIOLIB_whiteNoiseGenerator_exec(handle, (void *) ((int8_t *) pOutCn + k * outSizeBytes));
               AUDIOLIB_asm(" MARK 9");
               TI_profile_stop();
            }
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

               // Initialize comparison statuses as PASS; fail if any frame fails
               status_nat_vs_opt = TI_TEST_KERNEL_PASS;
               int32_t frame     = 0;
               for (frame = 0; frame < numExecReps; frame++) {
                  int32_t frame_status_nat_vs_opt = TI_compare_mem_2D_float(
                      (void *) ((int8_t *) pOut + frame * outSizeBytes),
                      (void *) ((int8_t *) pOutCn + frame * outSizeBytes), 0.01, (double) powf(2, -10),
                      bufParamsOut.dim_x, 1, 0, AUDIOLIB_sizeof(currPrm.sampleDataType));

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

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 7 comparisonDone %d status_nat_vs_opt %d\n",
                comparisonDone, status_nat_vs_opt);

            if (currPrm.staticOut != NULL) {

               if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

                  // Initialize reference comparison statuses
                  status_ref_vs_opt = TI_TEST_KERNEL_PASS;

                  int32_t frame = 0;
                  // Frame-by-frame comparison for reference outputs
                  for (frame = 0; frame < numExecReps; frame++) {

                     int32_t frame_status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                         (void *) ((int8_t *) pOut + frame * outSizeBytes),
                         (void *) ((float *) currPrm.staticOut + frame * currPrm.numSamples), 0.01,
                         (double) powf(2, -10), bufParamsOut.dim_x, 1, 0, 0, AUDIOLIB_sizeof(currPrm.sampleDataType));

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
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 8 status_nat_vs_opt %d "
                                  "status_ref_vs_opt %d currentTestFail "
                                  "%d\n",
                                  status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0,
                "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_whiteNoiseGenerator_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            sprintf(desc, "%s generated input | numSamples = %d", testPatternString, currPrm.numSamples);

            AUDIOLIB_whiteNoiseGenerator_perfEst(handle, &archCycles, &estCycles);

            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            fprintf(fpOutputCSV, "%d, %d, %d, %d, %ld, %d, %f, %d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.numSamples, currPrm.numExecReps, estCycles,
                    pProfile[3 * tpi + 1],
                    ((AUDIOLIB_F32) cycles[TI_PROFILE_KERNEL_OPT_WARM]) / ((AUDIOLIB_F32) estCycles), !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.numSamples * 1), archCycles, estCycles, currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d", testPatternString, currPrm.numSamples);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_whiteNoiseGenerator_test
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
      } // end repetitions
   }    // end idat test cases

   free(handle);
   /* Close results CSV */
   fclose(fpOutputCSV);

   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_whiteNoiseGenerator");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_whiteNoiseGenerator_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                               testNum         = 1000;
   int32_t                               currentTestFail = 0;
   AUDIOLIB_whiteNoiseGenerator_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_whiteNoiseGenerator_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams1D_t bufParamsOut;

   int   fail = 0;
   void *pOut;

   int32_t numSamples = 16;

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = numSamples;

   uint64_t OutSize = bufParamsOut.dim_x * bufParamsOut.data_type;

   AUDIOLIB_bufParams1D_t bufParamsOutTemp;

   pOut = (void *) malloc(OutSize);

   while (testNum <= 1001) {

      switch (testNum) {
      case 1000:
         bufParamsOutTemp.data_type = bufParamsOut.data_type;
         bufParamsOutTemp.dim_x     = bufParamsOut.dim_x;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_whiteNoiseGenerator_init_checkParams(NULL, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_whiteNoiseGenerator_init_checkParams(NULL, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1001:
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;
         bufParamsOutTemp.dim_x     = bufParamsOut.dim_x;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_whiteNoiseGenerator_init_checkParams(handle, &bufParamsOutTemp, &kerInitArgs);
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_whiteNoiseGenerator_init_checkParams(handle, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }
   free(pOut);

   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_whiteNoiseGenerator_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_whiteNoiseGenerator_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_whiteNoiseGenerator_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for inwhiteNoiseGeneratoridual test projects */
#if !defined(__ONESHOTTEST)
int main()
{
   int test_fail = 1, cov_fail = 1;

   uint32_t profile[1024 * 3];

   AUDIOLIB_TEST_init();

   test_fail = test_main(&profile[0]);
   cov_fail  = coverage_test_main();
   // cov_fail = 0; // coverage test is not run yet
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
