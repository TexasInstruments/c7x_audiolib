// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_inputAggregator_idat.h"
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

int AUDIOLIB_inputAggregator_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                 tpi;
   int32_t                 currentTestFail;
   int32_t                 fail = 0;
   uint32_t                repCount;
   uint32_t                numReps;
   uint8_t                 isInputInterleave, isOutputInterleave;
   AUDIOLIB_bufParams2D_t *bufParamsInX = NULL; // Array for input buffer parameters
   AUDIOLIB_bufParams2D_t  bufParamsOut;
   uint64_t                archCycles = 0;
   uint64_t                estCycles  = 0;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_inputAggregator_testParams_t *prm;
   AUDIOLIB_inputAggregator_testParams_t  currPrm;
   AUDIOLIB_inputAggregator_getTestParams(&prm, &test_cases);

   AUDIOLIB_inputAggregator_InitArgs kerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 0\n");

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 1\n");

   TI_profile_init("AUDIOLIB_inputAggregator");
   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_inputAggregator.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, isInputInterLeave, isOutputInterleave, inSamples, inOutputChannels, "
                        "EVM cycles,estCycles, Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps                        = prm[tpi].numReps;
      testNum                        = prm[tpi].testID;
      currPrm                        = prm[tpi];
      isInputInterleave              = prm[tpi].isInputInterleave;
      isOutputInterleave             = prm[tpi].isOutputInterleave;
      kerInitArgs.numInputs          = currPrm.numInputs;
      kerInitArgs.inChannels         = currPrm.inChannels;
      kerInitArgs.totalInChannels    = currPrm.totalInChannels;
      kerInitArgs.isInputInterleave  = currPrm.isInputInterleave;
      kerInitArgs.isOutputInterleave = currPrm.isOutputInterleave;

      int32_t               handleSize = AUDIOLIB_inputAggregator_getHandleSize(&kerInitArgs);
      AUDIOLIB_kernelHandle handle     = malloc(handleSize);
      // Allocate array of bufParams for each input
      if (currPrm.numInputs > 0) {
         bufParamsInX = (AUDIOLIB_bufParams2D_t *) malloc(currPrm.numInputs * sizeof(AUDIOLIB_bufParams2D_t));
         if (bufParamsInX == NULL) {
            AUDIOLIB_DEBUGPRINTFN(0, "Memory allocation failed for bufParamsInX\n");
            fail = 1;
            free(handle);
            break;
         }
      }

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;
         archCycles                        = 0;
         estCycles                         = 0;
         int32_t eleCount                  = 0;

         currentTestFail = 0;
         if ((AUDIOLIB_sizeof(currPrm.dataType) == 4) || (AUDIOLIB_sizeof(currPrm.dataType) == 8))
            eleCount = (__C7X_VEC_SIZE_BYTES__ / AUDIOLIB_sizeof(currPrm.dataType));

         uint32_t i = 0;
         for (i = 0; i < currPrm.numInputs; i++) {
            bufParamsInX[i].data_type = currPrm.dataType;
            if (isInputInterleave == 1) {
               bufParamsInX[i].dim_x    = currPrm.inChannels[i]; // Channels
               bufParamsInX[i].dim_y    = currPrm.inSamples;     // Samples
               bufParamsInX[i].stride_y = bufParamsInX[i].dim_x * AUDIOLIB_sizeof(bufParamsInX[i].data_type);
            }
            else {
               bufParamsInX[i].dim_x    = currPrm.inSamples;     // Samples
               bufParamsInX[i].dim_y    = currPrm.inChannels[i]; // Channels
               bufParamsInX[i].stride_y = bufParamsInX[i].dim_x * AUDIOLIB_sizeof(bufParamsInX[i].data_type);
            }
         }

         if (isOutputInterleave == 1) {
            bufParamsOut.data_type = currPrm.dataType;
            bufParamsOut.dim_x     = currPrm.totalInChannels;
            bufParamsOut.dim_y     = currPrm.inSamples;
            bufParamsOut.stride_y  = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);
         }

         else {

            bufParamsOut.data_type = currPrm.dataType;
            bufParamsOut.dim_x     = currPrm.inSamples;
            bufParamsOut.dim_y     = currPrm.totalInChannels;
            bufParamsOut.stride_y  = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);
         }

         // Allocate array of input buffers
         void **pInX = NULL;
         if (currPrm.numInputs > 0) {
            pInX = (void **) malloc(currPrm.numInputs * sizeof(void *));
         }
         for (i = 0; i < currPrm.numInputs; i++) {
            uint32_t input_dim_y_padded = AUDIOLIB_ROW_STRIDE(bufParamsInX[i].dim_y, eleCount);
            uint32_t inp0SizeBytes      = input_dim_y_padded * bufParamsInX[i].stride_y;
            pInX[i]                     = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inp0SizeBytes);
            memset(pInX[i], 0, inp0SizeBytes);
         }
         int32_t  dim_y_padded = AUDIOLIB_ROW_STRIDE(bufParamsOut.dim_y, eleCount);
         uint32_t outSizeBytes = dim_y_padded * bufParamsOut.stride_y;

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
         memset(pOut, 0, outSizeBytes);
         memset(pOutCn, 0, outSizeBytes);

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOut: %p pOutCn: %p\n", pIn, pOut, pOutCn);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pInX && pOut && pOutCn) {

            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {

               for (i = 0; i < currPrm.numInputs; i++) {
                  TI_fillBuffer_float(currPrm.testPattern, 0, pInX[i], currPrm.staticIn[i], bufParamsInX[i].dim_x,
                                      bufParamsInX[i].dim_y, bufParamsInX[i].stride_y,
                                      AUDIOLIB_sizeof(currPrm.dataType), testPatternString);
               }
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 0\n");

            status_init = AUDIOLIB_inputAggregator_init_checkParams(handle, bufParamsInX, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 1 status_init %d\n",
                                  status_init);
            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_inputAggregator_init(handle, bufParamsInX, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }
            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 2 status_init %d\n",
                                  status_init);

            status_opt = AUDIOLIB_inputAggregator_exec_checkParams(handle, (const void **) pInX, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 3 status_opt %d\n",
                                  status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_inputAggregator_exec(handle, pInX, pOut);
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
               status_opt = AUDIOLIB_inputAggregator_exec(handle, pInX, pOut);
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
            status_opt = AUDIOLIB_inputAggregator_exec(handle, pInX, pOut);
            AUDIOLIB_asm(" MARK 7");
            TI_profile_stop();

#endif // #if !defined(__C7X_HOSTEM__)
            /* Test _cn kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 4 status_opt %d\n",
                                  status_opt);
            // initialize the kernel to use the natural C variant
            AUDIOLIB_inputAggregator_init(handle, bufParamsInX, &bufParamsOut, &kerInitArgs);

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            status_nat = AUDIOLIB_inputAggregator_exec(handle, pInX, pOutCn);
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {
               status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0, (double) powf(2, -10),
                                                           bufParamsOut.dim_x, bufParamsOut.dim_y,
                                                           bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.dataType));
            }
            else {
               // TBD
            }

            comparisonDone = 1;
            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 7 comparisonDone %d status_nat_vs_opt %d\n",
                comparisonDone, status_nat_vs_opt);

            if (currPrm.staticOut != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
               bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

               if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {

                  status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                      (void *) pOut, (void *) currPrm.staticOut, 0, (double) powf(2, -10), bufParamsOut.dim_x,
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
            AUDIOLIB_DEBUGPRINTFN(
                0,
                "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 8 status_nat_vs_opt %d status_ref_vs_opt "
                "%d currentTestFail "
                "%d\n",
                status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;
            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_inputAggregator_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            sprintf(desc, "%s generated input | inSamples = %d, inOutputChannels = %d", testPatternString,
                    currPrm.inSamples, currPrm.totalInChannels);
            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            AUDIOLIB_inputAggregator_perfEst(handle, &archCycles, &estCycles);
            fprintf(fpOutputCSV, "%d, %d, %d,%d, %d, %d, %d ,%ld, %d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.dataType) * 8, isInputInterleave, isOutputInterleave, currPrm.inSamples,
                    currPrm.totalInChannels, pProfile[3 * tpi + 1], estCycles, !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.inSamples * currPrm.totalInChannels), archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inOutputChannels = %d", testPatternString,
                    currPrm.inSamples, currPrm.totalInChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_inputAggregator_test
            TI_profile_clear_run_stats();
         }
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
         if (pInX) {
            for (i = 0; i < currPrm.numInputs; i++) {
               if (pInX[i])
                  TI_align_free(pInX[i]);
            }
            free(pInX);
         }

      } // end repetitions
      if (bufParamsInX)
         free(bufParamsInX);
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
      TI_memError("AUDIOLIB_inputAggregator");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_inputAggregator_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                           testNum         = 1000;
   int32_t                           currentTestFail = 0;
   AUDIOLIB_inputAggregator_InitArgs kerInitArgs;

   memset(&kerInitArgs, 0, sizeof(AUDIOLIB_inputAggregator_InitArgs));
   kerInitArgs.numInputs = 2; // We are testing with 2 inputs

   int32_t               handleSize = AUDIOLIB_inputAggregator_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn[2];
   AUDIOLIB_bufParams2D_t bufParamsOut;

   int fail = 0;

   // Setup default valid parameters
   int32_t inSamples  = 16;
   int32_t inChannels = 16;

   // Input 0
   bufParamsIn[0].data_type = AUDIOLIB_FLOAT32;
   bufParamsIn[0].dim_x     = inChannels;
   bufParamsIn[0].dim_y     = inSamples;
   bufParamsIn[0].stride_y  = bufParamsIn[0].dim_y * AUDIOLIB_sizeof(bufParamsIn[0].data_type);

   // Input 1
   bufParamsIn[1].data_type = AUDIOLIB_FLOAT32;
   bufParamsIn[1].dim_x     = 1;
   bufParamsIn[1].dim_y     = inSamples;
   bufParamsIn[1].stride_y  = bufParamsIn[1].dim_y * AUDIOLIB_sizeof(bufParamsIn[1].data_type);

   // Output
   bufParamsOut.data_type = bufParamsIn[0].data_type;
   bufParamsOut.dim_x     = inChannels;
   bufParamsOut.dim_y     = inSamples;
   bufParamsOut.stride_y  = bufParamsOut.dim_y * AUDIOLIB_sizeof(bufParamsOut.data_type);

   // Temps for modification inside loop
   AUDIOLIB_bufParams2D_t bufParamsInTemp[2];
   AUDIOLIB_bufParams2D_t bufParamsOutTemp;

   while (testNum <= 1002) {

      // Reset temps to valid state
      bufParamsInTemp[0] = bufParamsIn[0];
      bufParamsInTemp[1] = bufParamsIn[1];
      bufParamsOutTemp   = bufParamsOut;

      switch (testNum) {
      case 1000:
         // Test Case: Null Pointer Check
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         // Pass NULL as bufParamsIn to trigger error, or NULL handle
         status_nat = AUDIOLIB_inputAggregator_init_checkParams(NULL, bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_inputAggregator_init_checkParams(NULL, bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);

         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1001:
         // Test Case: Invalid Data Type (Input)
         bufParamsInTemp[0].data_type = AUDIOLIB_UINT32; // Invalid type

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_inputAggregator_init_checkParams(handle, bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt =
             AUDIOLIB_inputAggregator_init_checkParams(handle, bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);

         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1002:
         // Test Case: Invalid Data Type (Output Mismatch)
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32; // Mismatch with float inputs

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_inputAggregator_init_checkParams(handle, bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);

         // Reset output for next sub-check
         bufParamsOutTemp.data_type = bufParamsIn[0].data_type;

         // Create mismatch in second input
         bufParamsInTemp[1].data_type = AUDIOLIB_UINT32;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt =
             AUDIOLIB_inputAggregator_init_checkParams(handle, bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);

         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }

   // Free handle
   if (handle)
      free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   int32_t handleSize_LDRA = AUDIOLIB_inputAggregator_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for inputAggregator test projects */
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
