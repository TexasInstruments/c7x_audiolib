// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_rmsSmoothed_idat.h"
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

int AUDIOLIB_rmsSmoothed_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams1D_t bufParamsOut;
   uint64_t               archCycles = 0;
   uint64_t               estCycles  = 0;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_rmsSmoothed_testParams_t *prm;
   AUDIOLIB_rmsSmoothed_testParams_t  currPrm;
   AUDIOLIB_rmsSmoothed_getTestParams(&prm, &test_cases);

   AUDIOLIB_rmsSmoothed_InitArgs kerInitArgs;
   AUDIOLIB_rmsSmoothed_SetArgs  kerSetArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 0\n");

   TI_profile_init("AUDIOLIB_rmsSmoothed");
   // file IO for EVM benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_rmsSmoothed.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, blockSize, numChannels, frames, "
                        "EVM cycles,estCycles, Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps                  = prm[tpi].numReps;
      testNum                  = prm[tpi].testID;
      currPrm                  = prm[tpi];
      kerInitArgs.samplingRate = currPrm.samplingRate;
      kerSetArgs.smoothingTime = currPrm.smoothingTime;

      int32_t handleSize = AUDIOLIB_rmsSmoothed_getHandleSize(&kerInitArgs);

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

         int32_t numExecReps = (int32_t) currPrm.frames;

         bufParamsIn.data_type = currPrm.sampleDataType;
         bufParamsIn.dim_x     = currPrm.blockSize;
         bufParamsIn.dim_y     = currPrm.numChannels;
         bufParamsIn.stride_y  = currPrm.strideIn * AUDIOLIB_sizeof(currPrm.sampleDataType);

         bufParamsOut.data_type = currPrm.sampleDataType;
         bufParamsOut.dim_x     = 1;

         uint64_t inpSizeBytes, outSizeBytes;

         inpSizeBytes = bufParamsIn.stride_y * bufParamsIn.dim_y;
         outSizeBytes = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);

         void *pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpSizeBytes * numExecReps);

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

         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn && pOut && pOutCn) {

            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {
               // Fill input buffers frame by frame
               int32_t frame = 0;
               for (frame = 0; frame < numExecReps; frame++) {
                  TI_fillBuffer_float(
                      prm[tpi].testPattern, 0, (void *) ((int8_t *) pIn + frame * inpSizeBytes),
                      (void *) ((int8_t *) prm[tpi].staticIn0 + (frame * currPrm.blockSize * currPrm.numChannels *
                                                                 AUDIOLIB_sizeof(currPrm.sampleDataType))),
                      bufParamsIn.dim_x, bufParamsIn.dim_y, bufParamsIn.stride_y,
                      AUDIOLIB_sizeof(currPrm.sampleDataType), testPatternString);
               }
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 0\n");

            status_init = AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_rmsSmoothed_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               if (status_init == AUDIOLIB_SUCCESS) {
                  status_init = AUDIOLIB_rmsSmoothed_set(handle, &kerSetArgs);
               }
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 2 status_init %d\n", status_init);

            status_opt = AUDIOLIB_rmsSmoothed_exec_checkParams(handle, pIn, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 3 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_rmsSmoothed_exec(handle, pIn, pOut);
               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();
            }

            /* The following for loop is to call kernel repeatedly so as to
             * train the branch predictor. Each call processes the next frame,
             * so the filter state carried in the handle is exercised. */
            int32_t k;
            for (k = 0; k < (numExecReps - 2); k++) {
               // run warm instruction cache test
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
               AUDIOLIB_asm(" MARK 4");
               status_opt = AUDIOLIB_rmsSmoothed_exec(handle, (void *) ((int8_t *) pIn + (k + 1) * inpSizeBytes),
                                                      (void *) ((int8_t *) pOut + (k + 1) * outSizeBytes));
               AUDIOLIB_asm(" MARK 5");
               TI_profile_stop();
            }

            // get output to L1D
            int16_t outSum   = 0;
            int8_t *pOutTemp = (int8_t *) pOut; // treat output as bytes
            for (k = 0; k < (int32_t) (outSizeBytes * numExecReps); k++) {
               outSum += *pOutTemp;
               pOutTemp++;
            }

            // dummy store of outSum to insure that the compiler does not remove it.
            volatileSum = outSum;

            if (numExecReps != 1) {
               // run warm instruction cache test
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
               AUDIOLIB_asm(" MARK 6");
               status_opt =
                   AUDIOLIB_rmsSmoothed_exec(handle, (void *) ((int8_t *) pIn + (numExecReps - 1) * inpSizeBytes),
                                             (void *) ((int8_t *) pOut + (numExecReps - 1) * outSizeBytes));
               AUDIOLIB_asm(" MARK 7");
               TI_profile_stop();
            }

            /* Test _cn kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant; this also resets the filter state
            AUDIOLIB_rmsSmoothed_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
            AUDIOLIB_rmsSmoothed_set(handle, &kerSetArgs);

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            for (k = 0; k < numExecReps; k++) {
               status_nat = AUDIOLIB_rmsSmoothed_exec(handle, (void *) ((int8_t *) pIn + k * inpSizeBytes),
                                                      (void *) ((int8_t *) pOutCn + k * outSizeBytes));
            }
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 6 status_nat %d\n", status_nat);
#endif

            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {
               // Compare optimized vs. natural C over all frames
               status_nat_vs_opt =
                   TI_compare_mem_2D_float(pOut, pOutCn, 0.001, (double) powf(2, -10), bufParamsOut.dim_x * numExecReps,
                                           1, 0, AUDIOLIB_sizeof(currPrm.sampleDataType));
            }
            else {
               // TBD
            }

            comparisonDone = 1;

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 7 comparisonDone %d status_nat_vs_opt %d\n",
                comparisonDone, status_nat_vs_opt);
            if (currPrm.staticOut != NULL) {

               if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

                  // Compare against the reference over all frames
                  status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                      pOut, currPrm.staticOut, 0.001, (double) powf(2, -10), bufParamsOut.dim_x * numExecReps, 1, 0,
                      outSizeBytes, AUDIOLIB_sizeof(currPrm.sampleDataType));
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
                "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 8 status_nat_vs_opt %d status_ref_vs_opt "
                "%d currentTestFail "
                "%d\n",
                status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */

            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_rmsSmoothed_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            sprintf(desc, "%s generated input | blockSize = %d, numChannels = %d, frames = %d", testPatternString,
                    currPrm.blockSize, currPrm.numChannels, numExecReps);
            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            AUDIOLIB_rmsSmoothed_perfEst(handle, &archCycles, &estCycles, currPrm.sampleDataType, &kerInitArgs);
            fprintf(fpOutputCSV, "%d, %d, %d, %d, %d, %d ,%ld, %d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.blockSize, currPrm.numChannels, numExecReps,
                    pProfile[3 * tpi + 1], estCycles, !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.blockSize * currPrm.numChannels), archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | blockSize = %d, numChannels = %d, frames = %d",
                    testPatternString, currPrm.blockSize, currPrm.numChannels, numExecReps);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_rms_test
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

   } // end idat test cases

   /* Close results CSV */
   fclose(fpOutputCSV);

   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_rmsSmoothed");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_rmsSmoothed_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                       testNum         = 1000;
   int32_t                       currentTestFail = 0;
   AUDIOLIB_rmsSmoothed_InitArgs kerInitArgs;

   AUDIOLIB_rmsSmoothed_SetArgs kerSetArgs;
   AUDIOLIB_rmsSmoothed_SetArgs kerSetArgsTemp;

   kerInitArgs.samplingRate = 48000;
   kerSetArgs.smoothingTime = 1000.0f;

   int32_t               handleSize = AUDIOLIB_rmsSmoothed_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams1D_t bufParamsOut;

   int   fail = 0;
   void *pIn, *pOut;

   int32_t blockSize   = 16;
   int32_t numChannels = 2;

   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = blockSize;
   bufParamsIn.dim_y     = numChannels;
   bufParamsIn.stride_y  = blockSize * AUDIOLIB_sizeof(bufParamsIn.data_type);

   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = 1;

   uint64_t InSize  = bufParamsIn.stride_y * bufParamsIn.dim_y;
   uint64_t OutSize = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);

   AUDIOLIB_bufParams2D_t        bufParamsInTemp;
   AUDIOLIB_bufParams1D_t        bufParamsOutTemp;
   AUDIOLIB_rmsSmoothed_InitArgs kerInitArgsTemp;

   pIn  = (void *) malloc(InSize);
   pOut = (void *) malloc(OutSize);

   while (testNum <= 1008) {

      bufParamsInTemp  = bufParamsIn;
      bufParamsOutTemp = bufParamsOut;
      kerInitArgsTemp  = kerInitArgs;

      switch (testNum) {
      case 1000:
         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_rmsSmoothed_init_checkParams(NULL, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);
         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1001:
         bufParamsInTemp.data_type = AUDIOLIB_INT32;

         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);

         bufParamsInTemp.data_type  = bufParamsIn.data_type;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT64;

         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt =
             AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1002:
         bufParamsOutTemp.dim_x = 2;

         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);

         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;
         bufParamsInTemp.dim_y  = 0;

         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt =
             AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) || (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION));
         break;
      case 1003:
         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt                = AUDIOLIB_rmsSmoothed_exec_checkParams(handle, NULL, pOut);
         status_nat                = AUDIOLIB_rmsSmoothed_exec_checkParams(handle, pIn, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1004:
         bufParamsInTemp.stride_y = (blockSize - 1) * AUDIOLIB_sizeof(bufParamsIn.data_type);

         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);

         bufParamsInTemp.stride_y = bufParamsIn.stride_y;
         bufParamsInTemp.dim_x    = 0;

         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt =
             AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) || (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION));
         break;
      case 1005:
         kerInitArgsTemp.samplingRate = 0;

         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_rmsSmoothed_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);

         kerSetArgsTemp.smoothingTime = -1.0f;
         status_opt                   = AUDIOLIB_rmsSmoothed_set(handle, &kerSetArgsTemp);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) || (status_opt != AUDIOLIB_ERR_INVALID_VALUE));
         break;
      case 1006:
         status_nat      = AUDIOLIB_rmsSmoothed_set(NULL, &kerSetArgs);
         status_opt      = AUDIOLIB_rmsSmoothed_set(handle, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1007:
         status_nat      = AUDIOLIB_rmsSmoothed_get(NULL, &kerSetArgsTemp);
         status_opt      = AUDIOLIB_rmsSmoothed_get(handle, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1008:
         kerInitArgsTemp.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_rmsSmoothed_init(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);
         if (status_nat == AUDIOLIB_SUCCESS) {
            status_nat = AUDIOLIB_rmsSmoothed_set(handle, &kerSetArgs);
         }
         kerSetArgsTemp.smoothingTime = 0.0f;
         status_opt                   = AUDIOLIB_rmsSmoothed_get(handle, &kerSetArgsTemp);
         currentTestFail              = ((status_nat != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                            (kerSetArgsTemp.smoothingTime != kerSetArgs.smoothingTime));
         break;

      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }
   free(pIn);
   free(pOut);

   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_rmsSmoothed_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_rmsSmoothed_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_rmsSmoothed_getHandleSize(&kerInitArgs);
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
