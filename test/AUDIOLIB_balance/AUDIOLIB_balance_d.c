// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>
// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_balance_idat.h"
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

int AUDIOLIB_balance_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams2D_t bufParamsInL;
   AUDIOLIB_bufParams2D_t bufParamsOutL;
   uint32_t               testNum;
   uint32_t               comparisonDone = 0;

   AUDIOLIB_balance_testParams_t *prm;
   AUDIOLIB_balance_testParams_t  currPrm;
   AUDIOLIB_balance_getTestParams(&prm, &test_cases);

   AUDIOLIB_balance_InitArgs kerInitArgs;
   AUDIOLIB_balance_SetArgs  kerSetArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 0\n");

   int32_t               handleSize = AUDIOLIB_balance_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 1\n");

   TI_profile_init("AUDIOLIB_balance");
   // #if (!defined(EVM_TEST))
   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_balance.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, inChannels, frames, smoothing time, balance, sampling rate,"
                        "Arch cycles, Loki cycles, Loki/Arch cycles, Pass/Fail\n");
   // #endif //  #if !defined(EVM_TEST)

   for (tpi = 0; tpi < test_cases; tpi++) {
      uint64_t archCycles = 0, estCycles = 0;
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      kerSetArgs.balance       = currPrm.balance;
      kerSetArgs.samplingRate  = currPrm.samplingRate;
      kerSetArgs.smoothingTime = currPrm.smoothingTime;
      int32_t numExecReps      = currPrm.frames;

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t         status_nat_vs_opt_left  = TI_TEST_KERNEL_FAIL;
         int32_t         status_nat_vs_opt_right = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt_left  = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt_right = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init             = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt              = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat              = AUDIOLIB_SUCCESS;

         currentTestFail = 0;

         bufParamsInL.data_type = currPrm.sampleDataType;
         bufParamsInL.dim_x     = currPrm.numSamples;
         bufParamsInL.dim_y     = currPrm.numChannels;
         bufParamsInL.stride_y  = currPrm.strideIn;

         bufParamsOutL.data_type = currPrm.sampleDataType;
         bufParamsOutL.dim_x     = currPrm.numSamples;
         bufParamsOutL.dim_y     = currPrm.numChannels;
         bufParamsOutL.stride_y  = currPrm.strideOut;

         // Here, stride is in bytes
         uint32_t inpLeftSizeBytes;
         uint32_t outLeftSizeBytes;

         // Calculate the size of the input and output buffers in bytes
         inpLeftSizeBytes = bufParamsInL.dim_y * bufParamsInL.stride_y;
         outLeftSizeBytes = bufParamsOutL.dim_y * bufParamsOutL.stride_y;

         void *pInL = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpLeftSizeBytes * numExecReps);
         void *pInR = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpLeftSizeBytes * numExecReps);

         void *pOutL, *pOutR, *pOutCnL, *pOutCnR;
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOutL   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
            pOutR   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
            pOutCnL = (void *) malloc(outLeftSizeBytes * numExecReps);
            pOutCnR = (void *) malloc(outLeftSizeBytes * numExecReps);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            pOutL = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
            pOutR = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
#else
            pOutL = (void *) msmcBuffer;
            pOutR = (void *) &msmcBuffer[outLeftSizeBytes * numExecReps];
#endif
            pOutCnL = (void *) ddrBuffer;
            pOutCnR = (void *) &ddrBuffer[outLeftSizeBytes * numExecReps];
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            pOutL = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
            pOutR = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outLeftSizeBytes * numExecReps);
#else
            pOutL = (void *) msmcBuffer;
            pOutR = (void *) &msmcBuffer[outLeftSizeBytes * numExecReps];
#endif
            pOutCnL = (void *) ddrBuffer;
            pOutCnR = (void *) &ddrBuffer[outLeftSizeBytes * numExecReps];
         }

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOut: %p pOutCn: %p\n", pIn, pOut, pOutCn);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pInL && pInR && pOutL && pOutCnL && pOutR && pOutCnR) {

            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

               // Fill input buffers frame by frame
               int32_t frame = 0;
               for (frame = 0; frame < numExecReps; frame++) {

                  TI_fillBuffer_float(
                      prm[tpi].testPattern, 0, (void *) ((int8_t *) pInL + frame * inpLeftSizeBytes),
                      (void *) ((float *) prm[tpi].staticInLeft + (frame * currPrm.numSamples * currPrm.numChannels)),
                      bufParamsInL.dim_x, bufParamsInL.dim_y, bufParamsInL.stride_y,
                      AUDIOLIB_sizeof(currPrm.sampleDataType), testPatternString);

                  TI_fillBuffer_float(
                      prm[tpi].testPattern, 0, (void *) ((int8_t *) pInR + frame * inpLeftSizeBytes),
                      (void *) ((float *) prm[tpi].staticInRight + (frame * currPrm.numSamples * currPrm.numChannels)),
                      bufParamsInL.dim_x, bufParamsInL.dim_y, bufParamsInL.stride_y,
                      AUDIOLIB_sizeof(currPrm.sampleDataType), testPatternString);
               }
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 0\n");

            status_init = AUDIOLIB_balance_init_checkParams(handle, &bufParamsInL, &bufParamsOutL, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_balance_init(handle, &bufParamsInL, &bufParamsOutL, &kerInitArgs);
               AUDIOLIB_balance_set(handle, &kerSetArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 2 status_init %d\n", status_init);

            status_opt = AUDIOLIB_balance_exec_checkParams(handle, pInL, pInR, pOutL, pOutR);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 3 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_balance_exec(handle, pInL, pInR, pOutL, pOutR);
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
               status_opt = AUDIOLIB_balance_exec(handle, (void *) ((int8_t *) pInL + (k + 1) * inpLeftSizeBytes),
                                                  (void *) ((int8_t *) pInR + (k + 1) * inpLeftSizeBytes),
                                                  (void *) ((int8_t *) pOutL + (k + 1) * outLeftSizeBytes),
                                                  (void *) ((int8_t *) pOutR + (k + 1) * outLeftSizeBytes));
               ;
               AUDIOLIB_asm(" MARK 5");
               TI_profile_stop();
            }

            int16_t outSum    = 0;
            int8_t *pOutTempL = (int8_t *) pOutL;
            int8_t *pOutTempR = (int8_t *) pOutR;
            size_t  j         = 0;
            for (j = 0; j < bufParamsInL.dim_x * currPrm.numChannels * sizeof(currPrm.sampleDataType); j++) {
               outSum += *pOutTempL;
               pOutTempL++;
               outSum += *pOutTempR;
               pOutTempR++;
            }
            volatileSum = outSum;

            if (numExecReps != 1) {
               // run warm instruction cache test
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
               AUDIOLIB_asm(" MARK 6");
               status_opt =
                   AUDIOLIB_balance_exec(handle, (void *) ((int8_t *) pInL + (numExecReps - 1) * inpLeftSizeBytes),
                                         (void *) ((int8_t *) pInR + (numExecReps - 1) * inpLeftSizeBytes),
                                         (void *) ((int8_t *) pOutL + (numExecReps - 1) * outLeftSizeBytes),
                                         (void *) ((int8_t *) pOutR + (numExecReps - 1) * outLeftSizeBytes));
               AUDIOLIB_asm(" MARK 7");
               TI_profile_stop();
            }

            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_balance_init(handle, &bufParamsInL, &bufParamsOutL, &kerInitArgs);
            AUDIOLIB_balance_set(handle, &kerSetArgs);
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            for (k = 0; k < numExecReps; k++) {
               status_nat = AUDIOLIB_balance_exec(handle, (void *) ((int8_t *) pInL + k * inpLeftSizeBytes),
                                                  (void *) ((int8_t *) pInR + k * inpLeftSizeBytes),
                                                  (void *) ((int8_t *) pOutCnL + k * inpLeftSizeBytes),
                                                  (void *) ((int8_t *) pOutCnR + k * inpLeftSizeBytes));
            }
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

               // Initialize comparison statuses as PASS; fail if any frame fails
               status_nat_vs_opt_left  = TI_TEST_KERNEL_PASS;
               status_nat_vs_opt_right = TI_TEST_KERNEL_PASS;
               status_ref_vs_opt_left  = TI_TEST_KERNEL_PASS;
               status_ref_vs_opt_right = TI_TEST_KERNEL_PASS;

               int32_t frame = 0;
               for (frame = 0; frame < numExecReps; frame++) {
                  // Compare optimized vs. natural C for this frame
                  int32_t frame_status_nat_vs_opt_left =
                      TI_compare_mem_2D_float((void *) ((int8_t *) pOutL + frame * outLeftSizeBytes),
                                              (void *) ((int8_t *) pOutCnL + frame * outLeftSizeBytes), 0,
                                              (double) powf(2, -10), bufParamsOutL.dim_x, bufParamsOutL.dim_y,
                                              bufParamsOutL.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType));

                  int32_t frame_status_nat_vs_opt_right =
                      TI_compare_mem_2D_float((void *) ((int8_t *) pOutR + frame * outLeftSizeBytes),
                                              (void *) ((int8_t *) pOutCnR + frame * outLeftSizeBytes), 0,
                                              (double) powf(2, -10), bufParamsOutL.dim_x, bufParamsOutL.dim_y,
                                              bufParamsOutL.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType));

                  // Update overall status: fail if any frame fails
                  if (frame_status_nat_vs_opt_left == TI_TEST_KERNEL_FAIL) {
                     status_nat_vs_opt_left = TI_TEST_KERNEL_FAIL;
                  }
                  if (frame_status_nat_vs_opt_right == TI_TEST_KERNEL_FAIL) {
                     status_nat_vs_opt_right = TI_TEST_KERNEL_FAIL;
                  }
               }
            }
            else {
               // TBD
            }

            comparisonDone = 1;

            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 7 comparisonDone %d "
                                  "status_nat_vs_opt_left %d status_nat_vs_opt_right %d\n",
                                  comparisonDone, status_nat_vs_opt_left, status_nat_vs_opt_right);

            if (currPrm.staticOutLeft != NULL && currPrm.staticOutRight != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutLTemp = bufParamsOutL;
               bufParamsOutLTemp.stride_y = bufParamsOutLTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutLTemp.data_type);

               if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

                  // Initialize reference comparison statuses
                  status_ref_vs_opt_left  = TI_TEST_KERNEL_PASS;
                  status_ref_vs_opt_right = TI_TEST_KERNEL_PASS;

                  int32_t frame = 0;
                  // Frame-by-frame comparison for reference outputs
                  for (frame = 0; frame < numExecReps; frame++) {
                     int32_t frame_status_ref_vs_opt_left = TI_compare_mem_2DDualStride_float(
                         (void *) ((int8_t *) pOutL + frame * outLeftSizeBytes),
                         (void *) ((float *) currPrm.staticOutLeft + frame * currPrm.numSamples * currPrm.numChannels),
                         0, (double) powf(2, -10), bufParamsOutL.dim_x, bufParamsOutL.dim_y, bufParamsOutL.stride_y,
                         bufParamsOutL.dim_x * AUDIOLIB_sizeof(currPrm.sampleDataType),
                         AUDIOLIB_sizeof(currPrm.sampleDataType));

                     int32_t frame_status_ref_vs_opt_right = TI_compare_mem_2DDualStride_float(
                         (void *) ((int8_t *) pOutR + frame * outLeftSizeBytes),
                         (void *) ((float *) currPrm.staticOutRight + frame * currPrm.numSamples * currPrm.numChannels),
                         0, (double) powf(2, -10), bufParamsOutL.dim_x, bufParamsOutL.dim_y, bufParamsOutL.stride_y,
                         bufParamsOutL.dim_x * AUDIOLIB_sizeof(currPrm.sampleDataType),
                         AUDIOLIB_sizeof(currPrm.sampleDataType));

                     // Update overall status: fail if any frame fails
                     if (frame_status_ref_vs_opt_left == TI_TEST_KERNEL_FAIL) {
                        status_ref_vs_opt_left = TI_TEST_KERNEL_FAIL;
                     }
                     if (frame_status_ref_vs_opt_right == TI_TEST_KERNEL_FAIL) {
                        status_ref_vs_opt_right = TI_TEST_KERNEL_FAIL;
                     }
                  }
               }
               else {
               }

               comparisonDone = 1;
            }
            else {
               /* Set to pass since it wasn't supposed to run. */
               status_ref_vs_opt_left  = TI_TEST_KERNEL_PASS;
               status_ref_vs_opt_right = TI_TEST_KERNEL_PASS;
            }
            AUDIOLIB_DEBUGPRINTFN(
                0,
                "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 8 status_nat_vs_opt_left %d status_ref_vs_opt "
                "%d  status_nat_vs_opt_right %d status_ref_vs_opt_right %d currentTestFail "
                "%d\n",
                status_nat_vs_opt_left, status_ref_vs_opt_left, status_nat_vs_opt_right, status_ref_vs_opt_right,
                currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt_left == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt_left == TI_TEST_KERNEL_FAIL) ||
                 (status_nat_vs_opt_right == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt_right == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balance_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_balance_get(handle, &kerSetArgs);
            sprintf(
                desc,
                "%s generated input | inSamples = %d, inChannels = %d, bal = %0.3f, smoothTime = %0.3f ms, fs = %d kHz",
                testPatternString, currPrm.numSamples, currPrm.numChannels, kerSetArgs.balance,
                kerSetArgs.smoothingTime, kerSetArgs.samplingRate);

            AUDIOLIB_balance_perfEst(handle, &archCycles, &estCycles);

            // #if (!defined(EVM_TEST))
            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            fprintf(fpOutputCSV, "%d, %d, %d, %d, %d , %f, %f, %d, %d, %d, %f, %d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.numSamples, currPrm.numChannels,
                    currPrm.frames, currPrm.smoothingTime, currPrm.balance, currPrm.samplingRate, (int32_t) archCycles,
                    pProfile[3 * tpi + 1],
                    ((AUDIOLIB_F32) cycles[TI_PROFILE_KERNEL_OPT_WARM]) / ((AUDIOLIB_F32) archCycles),
                    !currentTestFail);
            // #endif // #if (!defined(EVM_TEST))

            TI_profile_add_test(testNum++, currPrm.numSamples * currPrm.numChannels, archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_balance_test
            TI_profile_clear_run_stats();

         } // end of memory allocation successful?

         /* Free buffers for each test vector */
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            free(pOutCnL);
            free(pOutCnR);
            TI_align_free(pOutL);
            TI_align_free(pOutR);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            TI_align_free(pOutL);
            TI_align_free(pOutR);
#endif
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            TI_align_free(pOutL);
            TI_align_free(pOutR);
#endif
         }
         TI_align_free(pInL);
         TI_align_free(pInR);

      } // end repetitions
   }    // end idat test cases

   free(handle);
   // #if !defined(EVM_TEST)
   /* Close results CSV */
   fclose(fpOutputCSV);
   // #endif // #if !defined(EVM_TEST)

   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_balance");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_balance_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                   testNum         = 1000;
   int32_t                   currentTestFail = 0;
   AUDIOLIB_balance_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_balance_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsInL, bufParamsOutL;

   int   fail = 0;
   void *pIn0, *pOut;

   int32_t inSamples  = 16;
   int32_t inChannels = 16;

   bufParamsInL.data_type = AUDIOLIB_FLOAT32;
   bufParamsInL.dim_x     = inSamples;
   bufParamsInL.dim_y     = inChannels;
   bufParamsInL.stride_y  = bufParamsInL.dim_x * AUDIOLIB_sizeof(bufParamsInL.data_type);

   bufParamsOutL.data_type = bufParamsInL.data_type;
   bufParamsOutL.dim_x     = inSamples;
   bufParamsOutL.dim_y     = inChannels;
   bufParamsOutL.stride_y  = bufParamsOutL.dim_x * AUDIOLIB_sizeof(bufParamsOutL.data_type);

   uint64_t In0Size = bufParamsInL.stride_y * bufParamsInL.dim_y;
   uint64_t OutSize = bufParamsOutL.stride_y * bufParamsOutL.dim_y;

   AUDIOLIB_bufParams2D_t bufParamsInLTemp, bufParamsOutLTemp;

   pIn0 = (void *) malloc(In0Size);
   pOut = (void *) malloc(OutSize);

   while (testNum <= 1002) {

      switch (testNum) {
      case 1000:
         bufParamsInLTemp.data_type  = bufParamsInL.data_type;
         bufParamsOutLTemp.data_type = bufParamsOutL.data_type;

         bufParamsInLTemp.dim_x  = bufParamsInL.dim_x;
         bufParamsOutLTemp.dim_x = bufParamsOutL.dim_x;

         bufParamsInLTemp.dim_y  = bufParamsInL.dim_y;
         bufParamsOutLTemp.dim_y = bufParamsOutL.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_balance_init_checkParams(NULL, &bufParamsInLTemp, &bufParamsOutLTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_balance_init_checkParams(NULL, &bufParamsInLTemp, &bufParamsOutLTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1001:

         bufParamsInLTemp.data_type  = AUDIOLIB_UINT32;
         bufParamsOutLTemp.data_type = bufParamsOutL.data_type;

         bufParamsInLTemp.dim_x  = bufParamsInL.dim_x;
         bufParamsOutLTemp.dim_x = bufParamsOutL.dim_x;

         bufParamsInLTemp.dim_y  = bufParamsInL.dim_y;
         bufParamsOutLTemp.dim_y = bufParamsOutL.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_balance_init_checkParams(handle, &bufParamsInLTemp, &bufParamsOutLTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_balance_init_checkParams(handle, &bufParamsInLTemp, &bufParamsOutLTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1002:
         bufParamsInLTemp.data_type  = bufParamsInL.data_type;
         bufParamsOutLTemp.data_type = AUDIOLIB_UINT32;

         bufParamsInLTemp.dim_x  = bufParamsInL.dim_x;
         bufParamsOutLTemp.dim_x = bufParamsOutL.dim_x;

         bufParamsInLTemp.dim_y  = bufParamsInL.dim_y;
         bufParamsOutLTemp.dim_y = bufParamsOutL.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_balance_init_checkParams(handle, &bufParamsInLTemp, &bufParamsOutLTemp, &kerInitArgs);
         bufParamsInLTemp.data_type  = AUDIOLIB_UINT32;
         bufParamsOutLTemp.data_type = AUDIOLIB_UINT32;
         kerInitArgs.funcStyle       = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_balance_init_checkParams(handle, &bufParamsInLTemp, &bufParamsOutLTemp, &kerInitArgs);
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
   /* For every call of AUDIOLIB_balance_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_balance_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_balance_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for inbalanceidual test projects */
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
