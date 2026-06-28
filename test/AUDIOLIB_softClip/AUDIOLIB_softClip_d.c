// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_softClip_idat.h"
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

int AUDIOLIB_softClip_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   uint64_t               estCycles;
   uint64_t               archCycles;
   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_softClip_testParams_t *prm;
   AUDIOLIB_softClip_testParams_t  currPrm;
   AUDIOLIB_softClip_getTestParams(&prm, &test_cases);

   AUDIOLIB_softClip_InitArgs kerInitArgs;
   AUDIOLIB_softClip_SetArgs  kerSetArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 0\n");

   int32_t               handleSize = AUDIOLIB_softClip_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 1\n");

   TI_profile_init("AUDIOLIB_softClip");
   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_softClip.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, inChannels,arch cycles,"
                        "EVM cycles,EVM/arch, Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps              = prm[tpi].numReps;
      testNum              = prm[tpi].testID;
      currPrm              = prm[tpi];
      kerSetArgs.threshold = currPrm.threshold;
      kerSetArgs.endKnee   = currPrm.endKnee;

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;
         estCycles                         = 0;
         archCycles                        = 0;
         currentTestFail                   = 0;

         bufParamsIn0.data_type = currPrm.dataType;
         bufParamsIn0.dim_x     = currPrm.inSamples;
         bufParamsIn0.dim_y     = currPrm.inChannels;
         bufParamsIn0.stride_y  = currPrm.strideIn;

         bufParamsOut.data_type = currPrm.dataType;
         bufParamsOut.dim_x     = currPrm.inSamples;
         bufParamsOut.dim_y     = currPrm.inChannels;
         bufParamsOut.stride_y  = currPrm.strideOut;

         // Here, stride is in bytes
         uint32_t inp0SizeBytes = bufParamsIn0.dim_y * bufParamsIn0.stride_y;
         uint32_t outSizeBytes  = bufParamsOut.dim_y * bufParamsOut.stride_y;

         void *pIn0 = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inp0SizeBytes);

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

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOut: %p pOutCn: %p\n", pIn, pOut, pOutCn);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn0 && pOut && pOutCn) {

            if (currPrm.dataType == AUDIOLIB_FLOAT32) {
               TI_fillBuffer_float(prm[tpi].testPattern, 0, pIn0, prm[tpi].staticIn0, bufParamsIn0.dim_x,
                                   bufParamsIn0.dim_y, bufParamsIn0.stride_y, AUDIOLIB_sizeof(currPrm.dataType),
                                   testPatternString);
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 0\n");

            status_init = AUDIOLIB_softClip_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_softClip_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_softClip_set(handle, &kerSetArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 2 status_init %d\n", status_init);

            status_opt = AUDIOLIB_softClip_exec_checkParams(handle, pIn0, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 3 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_softClip_exec(handle, pIn0, pOut);
               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();
            }

#if !defined(__C7X_HOSTEM__)
            /* The following for loop is to call kernel repeatedly so as to
             * train the branch predictor                                   */
            uint32_t k;
            for (k = 0; k < 4; k++) {
               // run warm instruction cache test
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
               AUDIOLIB_asm(" MARK 4");
               status_opt = AUDIOLIB_softClip_exec(handle, pIn0, pOut);
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
            status_opt = AUDIOLIB_softClip_exec(handle, pIn0, pOut);
            AUDIOLIB_asm(" MARK 7");
            TI_profile_stop();

#endif
            /* Test _cn kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_softClip_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);
            AUDIOLIB_softClip_set(handle, &kerSetArgs);
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            status_nat = AUDIOLIB_softClip_exec(handle, pIn0, pOutCn);
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType) {
               status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(2, -10),
                                                           bufParamsOut.dim_x, bufParamsOut.dim_y,
                                                           bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.dataType));
            }
            else {
               // TBD
            }

            comparisonDone = 1;

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 7 comparisonDone %d status_nat_vs_opt %d\n",
                comparisonDone, status_nat_vs_opt);

            if (currPrm.staticOut != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
               bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

               if (currPrm.dataType == AUDIOLIB_FLOAT32) {

                  status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                      (void *) pOut, (void *) currPrm.staticOut, 0.001, (double) powf(2, -10), bufParamsOut.dim_x,
                      bufParamsOut.dim_y, bufParamsOut.stride_y, bufParamsOut.dim_x * AUDIOLIB_sizeof(currPrm.dataType),
                      AUDIOLIB_sizeof(currPrm.dataType));
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
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 8 status_nat_vs_opt %d "
                                  "status_ref_vs_opt %d currentTestFail "
                                  "%d\n",
                                  status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_softClip_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_softClip_get(handle, &kerSetArgs);
            sprintf(desc, "%s generated input | inSamples = %d, inChannels = %d", testPatternString, currPrm.inSamples,
                    currPrm.inChannels);
            AUDIOLIB_softClip_perfEst(handle, &archCycles, &estCycles);

            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            fprintf(
                fpOutputCSV, "%d, %d, %d, %d,%ld,%d ,%f,%d\n", currPrm.testID, AUDIOLIB_sizeof(currPrm.dataType) * 8,
                currPrm.inSamples, currPrm.inChannels, archCycles, pProfile[3 * tpi + 1],
                ((AUDIOLIB_F32) cycles[TI_PROFILE_KERNEL_OPT_WARM]) / ((AUDIOLIB_F32) archCycles), !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.inSamples * currPrm.inChannels), archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.inSamples, currPrm.inChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_softClip_test
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

      } // end repetitions
   }    // end idat test cases

   free(handle);
#if !defined(EVM_TEST)
   /* Close results CSV */
   fclose(fpOutputCSV);
#endif

   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_softClip");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_softClip_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                    testNum         = 1000;
   int32_t                    currentTestFail = 0;
   AUDIOLIB_softClip_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_softClip_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;

   int   fail = 0;
   void *pIn0, *pOut;

   int32_t inSamples  = 16;
   int32_t inChannels = 16;

   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = inChannels;
   bufParamsIn0.dim_y     = inSamples;
   bufParamsIn0.stride_y  = bufParamsIn0.dim_y * AUDIOLIB_sizeof(bufParamsIn0.data_type);

   bufParamsOut.data_type = bufParamsIn0.data_type;
   bufParamsOut.dim_x     = inChannels;
   bufParamsOut.dim_y     = inSamples;
   bufParamsOut.stride_y  = bufParamsOut.dim_y * AUDIOLIB_sizeof(bufParamsOut.data_type);

   uint64_t In0Size = bufParamsIn0.stride_y * bufParamsIn0.dim_y;
   uint64_t OutSize = bufParamsOut.stride_y * bufParamsOut.dim_y;

   AUDIOLIB_bufParams2D_t bufParamsIn0Temp, bufParamsOutTemp;

   pIn0 = (void *) malloc(In0Size);
   pOut = (void *) malloc(OutSize);

   while (testNum <= 1002) {

      switch (testNum) {
      case 1000:
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_softClip_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_softClip_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1001:

         bufParamsIn0Temp.data_type = AUDIOLIB_UINT32;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_softClip_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_softClip_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1002:
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_softClip_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         bufParamsIn0Temp.data_type = AUDIOLIB_UINT32;
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_softClip_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
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

   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_softClip_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_softClip_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_softClip_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for insoftClipidual test projects */
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
