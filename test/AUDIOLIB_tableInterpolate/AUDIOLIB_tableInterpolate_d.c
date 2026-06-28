// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_tableInterpolate_idat.h"
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

#define AUDIOLIB_ROW_STRIDE(x, y) (((x + y) / y) * y)

int16_t volatile volatileSum = 0; // use volatile to keep compiler from removing this operation

int AUDIOLIB_tableInterpolate_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams1D_t bufParamsIn0, bufParamsIn1, bufParamsOut;
   uint64_t               archCycles = 0;
   uint64_t               estCycles  = 0;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_tableInterpolate_testParams_t *prm;
   AUDIOLIB_tableInterpolate_testParams_t  currPrm;
   AUDIOLIB_tableInterpolate_getTestParams(&prm, &test_cases);

   AUDIOLIB_tableInterpolate_InitArgs kerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 0\n");

   TI_profile_init("AUDIOLIB_tableInterpolate");
   // file IO for EVM benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_tableInterpolate.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, tableSamples, "
                        "EVM cycles,estCycles, Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps                          = prm[tpi].numReps;
      testNum                          = prm[tpi].testID;
      currPrm                          = prm[tpi];
      kerInitArgs.minVal               = currPrm.minVal;
      kerInitArgs.maxVal               = currPrm.maxVal;
      kerInitArgs.tableInterpolateSize = currPrm.tableInterpolateSize;

      int32_t handleSize = AUDIOLIB_tableInterpolate_getHandleSize(&kerInitArgs);

      AUDIOLIB_kernelHandle handle = calloc(1, handleSize);

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;
         archCycles                        = 0;
         estCycles                         = 0;

         currentTestFail = 0;

         bufParamsIn0.data_type = currPrm.sampleDataType;
         bufParamsIn0.dim_x     = currPrm.srcSamples;

         bufParamsIn1.data_type = currPrm.sampleDataType;
         bufParamsIn1.dim_x     = currPrm.tableSamples;

         bufParamsOut.data_type = currPrm.sampleDataType;
         bufParamsOut.dim_x     = currPrm.srcSamples;

         uint64_t inp0SizeBytes, inp1SizeBytes, outSizeBytes;

         inp0SizeBytes = bufParamsIn0.dim_x * AUDIOLIB_sizeof(bufParamsIn0.data_type);
         inp1SizeBytes = (bufParamsIn1.dim_x + 1) * AUDIOLIB_sizeof(bufParamsIn1.data_type);
         outSizeBytes  = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsIn0.data_type);

         void *pIn0 = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inp0SizeBytes);
         void *pIn1 = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inp1SizeBytes);
         memset(pIn1, 0, sizeof(float) * (bufParamsIn1.dim_x + 1));

         void *pOut, *pOutCn;

         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
            pOutCn = (void *) malloc(outSizeBytes);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }

         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn0 && pIn1 && pOut && pOutCn) {

            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

               TI_fillBuffer_float(prm[tpi].testPattern, 0, pIn0, prm[tpi].staticIn0, currPrm.srcSamples, 1, 0,
                                   AUDIOLIB_sizeof(currPrm.sampleDataType), testPatternString);

               TI_fillBuffer_float(prm[tpi].testPattern, 0, pIn1, prm[tpi].staticIn1, currPrm.tableSamples, 1, 0,
                                   AUDIOLIB_sizeof(currPrm.sampleDataType), testPatternString);
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 0\n");

            status_init = AUDIOLIB_tableInterpolate_init_checkParams(handle, &bufParamsIn0, &bufParamsIn1,
                                                                     &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 1 status_init %d\n",
                                  status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init =
                   AUDIOLIB_tableInterpolate_init(handle, &bufParamsIn0, &bufParamsIn1, &bufParamsOut, &kerInitArgs);
#if defined(__C7524__)
               if (currPrm.tableInterpolateSize >= currPrm.tableSamples) {
                  if (status_init == AUDIOLIB_SUCCESS) {
                     AUDIOLIB_tableInterpolate_set_ci(handle, pIn1);
                  }
               }
#endif

               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 2 status_init %d\n",
                                  status_init);

            status_opt = AUDIOLIB_tableInterpolate_exec_checkParams(handle, pIn0, pIn1, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 3 status_opt %d\n",
                                  status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_tableInterpolate_exec(handle, pIn0, pIn1, pOut);

               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();
            }

#if !defined(__C7X_HOSTEM__)
            /* The following for loop is to call kernel repeatedly so as to
             * train the branch predictor
             */
            uint32_t k;
            for (k = 0; k < 4; k++) {
               // run warm instruction cache test
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
               AUDIOLIB_asm(" MARK 4");
               status_opt = AUDIOLIB_tableInterpolate_exec(handle, pIn0, pIn1, pOut);
               ;
               AUDIOLIB_asm(" MARK 5");
               TI_profile_stop();
            }

            // get output to L1D
            int16_t outSum   = 0;
            int8_t *pOutTemp = (int8_t *) pOut; // treat output as bytes to be data type agnostic
            for (k = 0; k < bufParamsOut.dim_x; k++) {
               outSum += *pOutTemp;
               pOutTemp++;
            }

            // dummy store of outSum to insure that the compiler does not remove it.
            volatileSum = outSum;

            // run warm instruction cache test
            TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_asm(" MARK 6");
            status_opt = AUDIOLIB_tableInterpolate_exec(handle, pIn0, pIn1, pOut);
            AUDIOLIB_asm(" MARK 7");
            TI_profile_stop();

#endif // #if !defined(__C7X_HOSTEM__)
            /* Test _cn kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 4 status_opt %d\n",
                                  status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_tableInterpolate_init(handle, &bufParamsIn0, &bufParamsIn1, &bufParamsOut, &kerInitArgs);

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            status_nat = AUDIOLIB_tableInterpolate_exec(handle, pIn0, pIn1, pOutCn);
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 6 status_nat %d\n", status_nat);
#endif

            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {
               status_nat_vs_opt =
                   TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0, (double) powf(2, -10), bufParamsOut.dim_x,
                                           1, 0, AUDIOLIB_sizeof(currPrm.sampleDataType));
            }
            else {
               // TBD
            }

            comparisonDone = 1;

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 7 comparisonDone %d status_nat_vs_opt %d\n",
                comparisonDone, status_nat_vs_opt);
            if (currPrm.staticOut != NULL) {

               if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

                  status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                      (void *) pOut, (void *) currPrm.staticOut, 0.001, (double) powf(2, -10), bufParamsOut.dim_x, 1, 0,
                      bufParamsOut.dim_x * AUDIOLIB_sizeof(currPrm.sampleDataType),
                      AUDIOLIB_sizeof(currPrm.sampleDataType));
               }
               else {
               }

               comparisonDone = 1;
            }
            else {
               /* Set to pass since it wasn't supposed to run. */
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;
            }

            AUDIOLIB_DEBUGPRINTFN(
                0,
                "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 8 status_nat_vs_opt %d status_ref_vs_opt "
                "%d currentTestFail "
                "%d\n",
                status_nat_vs_opt, status_ref_vs_opt, currentTestFail);
            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */

            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_tableInterpolate_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            sprintf(desc, "%s generated input | inSamples = %d, inChannels = %d", testPatternString, currPrm.srcSamples,
                    currPrm.tableSamples);
            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            AUDIOLIB_tableInterpolate_perfEst(handle, &archCycles, &estCycles, currPrm.sampleDataType,
                                              currPrm.tableInterpolateSize, &kerInitArgs);
            fprintf(fpOutputCSV, "%d, %d, %d,%d, %d ,%ld, %d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.srcSamples, currPrm.tableSamples,
                    pProfile[3 * tpi + 1], estCycles, !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.srcSamples), archCycles, estCycles, currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, tableSamples = %d", testPatternString,
                    currPrm.srcSamples, currPrm.tableSamples);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_tableInterpolate_test
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
         TI_align_free(pIn0);
         TI_align_free(pIn1);

      } // end repetitions
      free(handle);

   } // end idat test cases

   /* Close results CSV */
   fclose(fpOutputCSV);

   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_tableInterpolate");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_tableInterpolate_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                            testNum         = 1000;
   int32_t                            currentTestFail = 0;
   AUDIOLIB_tableInterpolate_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_tableInterpolate_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams1D_t bufParamsIn0, bufParamsIn1, bufParamsOut;

   int   fail = 0;
   void *pIn0, *pIn1, *pOut;

   int32_t srcSamples   = 16;
   int32_t tableSamples = 16;

   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = srcSamples;

   bufParamsIn1.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn1.dim_x     = tableSamples;

   bufParamsOut.data_type = bufParamsIn0.data_type;
   bufParamsOut.dim_x     = srcSamples;

   uint64_t In0Size = bufParamsIn0.dim_x * AUDIOLIB_sizeof(bufParamsIn1.data_type);
   uint64_t In1Size = bufParamsIn1.dim_x * AUDIOLIB_sizeof(bufParamsIn1.data_type);
   uint64_t OutSize = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);

   AUDIOLIB_bufParams1D_t bufParamsIn0Temp, bufParamsIn1Temp, bufParamsOutTemp;

   pIn0 = (void *) malloc(In0Size);
   pIn1 = (void *) malloc(In1Size);
   pOut = (void *) malloc(OutSize);

   while (testNum <= 1003) {

      switch (testNum) {
      case 1000:
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsIn1Temp.data_type = bufParamsIn1.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsIn1Temp.dim_x = bufParamsIn1.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_tableInterpolate_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                            &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_tableInterpolate_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                            &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1001:

         bufParamsIn0Temp.data_type = AUDIOLIB_UINT32;
         bufParamsIn1Temp.data_type = bufParamsIn1.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsIn1Temp.dim_x = bufParamsIn1.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_tableInterpolate_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                 &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_tableInterpolate_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                      &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1002:
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsIn1Temp.data_type = bufParamsIn1.data_type;
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsIn1Temp.dim_x = bufParamsIn1.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_tableInterpolate_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                 &bufParamsOutTemp, &kerInitArgs);
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsIn1Temp.data_type = AUDIOLIB_UINT32;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_tableInterpolate_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                      &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1003:
         bufParamsIn0Temp.data_type = AUDIOLIB_FLOAT32;
         bufParamsIn1Temp.data_type = AUDIOLIB_FLOAT32;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT32;

         bufParamsIn0Temp.dim_x = 64;
         bufParamsIn1Temp.dim_x = 3;
         bufParamsOutTemp.dim_x = 3;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_tableInterpolate_exec_checkParams(handle, NULL, pIn1, pOut);
         currentTestFail       = ((status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }
   free(pIn0);
   free(pIn1);

   free(pOut);

   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_tableInterpolate_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_tableInterpolate_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_tableInterpolate_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for intableInterpolateidual test projects */
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
