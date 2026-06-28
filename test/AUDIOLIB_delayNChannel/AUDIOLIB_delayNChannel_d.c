// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_delayNChannel_idat.h"
#include "AUDIOLIB_types.h"

#define AUDIOLIB_ROW_STRIDE(x, y) (((x + y) / y) * y)

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

int AUDIOLIB_delayNChannel_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering AUDIOLIB_delayNChannel_d function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut, bufParamsDelay;
   uint32_t               inpSizeBytes, delaySizeBytes, outSizeBytes;
   uint32_t               scratchSizeBytes;
   uint32_t               testNum;
   uint32_t               comparisonDone = 0;
   uint32_t               numExecReps    = 0, frameCnt;
   uint64_t               estCycles      = 0;
   uint64_t               archCycles     = 0;
   int32_t                eleCount       = 0;

   AUDIOLIB_delayNChannel_testParams_t *prm;
   AUDIOLIB_delayNChannel_testParams_t  currPrm;
   AUDIOLIB_delayNChannel_getTestParams(&prm, &test_cases);
   AUDIOLIB_delayNChannel_InitArgs kerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 0\n");

   int32_t               handleSize = AUDIOLIB_delayNChannel_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 1\n");

   TI_profile_init("AUDIOLIB_delayNChannel");

   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_delayNChannel.csv", "w+");
   fprintf(fpOutputCSV, "Kernel,Test ID,Test type,Bit Width,maxDelay,numSamples,numChannels,Interleave,Mode,Est "
                        "cycles,EVM cycles,EVM/Est,Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      for (repCount = 0; repCount < numReps; repCount++) {

         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t status_ref_vs_nat_out       = TI_TEST_KERNEL_FAIL;
         int32_t status_ref_vs_nat_Delay     = TI_TEST_KERNEL_FAIL;
         int32_t status_nat_vs_opt           = TI_TEST_KERNEL_FAIL;
         int32_t status_natDelay_vs_optDelay = TI_TEST_KERNEL_FAIL;
         int32_t status_ref_vs_opt           = TI_TEST_KERNEL_FAIL;
         int32_t status_refDelay_vs_optDelay = TI_TEST_KERNEL_FAIL;

         AUDIOLIB_STATUS status_init = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt  = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat  = AUDIOLIB_SUCCESS;
         currentTestFail             = 0;
         archCycles                  = 0;
         estCycles                   = 0;

         numExecReps           = currPrm.numExecReps;
         bufParamsIn.data_type = currPrm.dataType;
         bufParamsIn.stride_y  = currPrm.strideIn;

         bufParamsOut.data_type = currPrm.dataType;
         bufParamsOut.stride_y  = currPrm.strideOut;

         if (currPrm.interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
            bufParamsIn.dim_x = currPrm.numChannels;
            bufParamsIn.dim_y = currPrm.numSamples;

            bufParamsOut.dim_x = currPrm.numChannels;
            bufParamsOut.dim_y = currPrm.numSamples;
         }
         else {
            bufParamsIn.dim_x = currPrm.numSamples;
            bufParamsIn.dim_y = currPrm.numChannels;

            bufParamsOut.dim_x = currPrm.numSamples;
            bufParamsOut.dim_y = currPrm.numChannels;
         }

         uint32_t CircularBufBaseMmemSize = 512;

         if (currPrm.mode == 0) {
            bufParamsDelay.data_type = currPrm.dataType;
            bufParamsDelay.dim_x     = currPrm.maxDelay;
            bufParamsDelay.dim_y     = currPrm.numChannels;
            bufParamsDelay.stride_y  = (currPrm.maxDelay + currPrm.numSamples) * AUDIOLIB_sizeof(currPrm.dataType);
         }
         else {
            while (CircularBufBaseMmemSize < (currPrm.maxDelay + currPrm.numSamples)) {
               CircularBufBaseMmemSize *= 2;
            }

            bufParamsDelay.data_type = currPrm.dataType;
            bufParamsDelay.dim_x     = currPrm.maxDelay + currPrm.numSamples;
            bufParamsDelay.dim_y     = currPrm.numChannels;
            bufParamsDelay.stride_y  = CircularBufBaseMmemSize * AUDIOLIB_sizeof(currPrm.dataType);
         }

         kerInitArgs.mode       = currPrm.mode;
         kerInitArgs.interleave = currPrm.interleave;
         kerInitArgs.maxDelay   = currPrm.maxDelay;

         // Copy all the delay sizes for each channel
         uint32_t *pDelaySize = (uint32_t *) currPrm.staticDelaySizeCase;
         uint32_t  ch;
         for (ch = 0; ch < currPrm.numChannels; ch++) {
            kerInitArgs.delaySize[ch] = pDelaySize[ch];
         }

         eleCount             = 2 * (__C7X_VEC_SIZE_BYTES__ / AUDIOLIB_sizeof(currPrm.dataType));
         int32_t dim_y_padded = AUDIOLIB_ROW_STRIDE(bufParamsIn.dim_y, eleCount);

         // Here, stride is in bytes
         inpSizeBytes     = numExecReps * dim_y_padded * bufParamsIn.stride_y;
         outSizeBytes     = numExecReps * bufParamsOut.dim_y * bufParamsOut.stride_y;
         delaySizeBytes   = bufParamsDelay.dim_y * bufParamsDelay.stride_y;
         scratchSizeBytes = currPrm.numSamples * AUDIOLIB_ROW_STRIDE(currPrm.numChannels, eleCount) *
                            AUDIOLIB_sizeof(currPrm.dataType);

         void *pIn, *pDelay, *pOut, *pOutCn, *pDelayCn, *pScratch;

         pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpSizeBytes);

         if (currPrm.mode == 1) {
            pDelay = (void *) TI_memalign(bufParamsDelay.stride_y, delaySizeBytes);
         }
         else {
            pDelay = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, delaySizeBytes);
         }

         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOut     = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
            pScratch = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, scratchSizeBytes);
            pOutCn   = (void *) malloc(outSizeBytes);
            pDelayCn = (void *) malloc(delaySizeBytes);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            pOut     = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
            pScratch = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, scratchSizeBytes);
#else
            pOut     = (void *) msmcBuffer;
            pScratch = (void *) msmcBuffer + outSizeBytes;
#endif
            pOutCn   = (void *) ddrBuffer;
            pDelayCn = (void *) (ddrBuffer + outSizeBytes);
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            pOut     = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
            pScratch = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, scratchSizeBytes);
#else
            pOut     = (void *) msmcBuffer;
            pScratch = (void *) msmcBuffer + outSizeBytes;
#endif
            pOutCn   = (void *) ddrBuffer;
            pDelayCn = (void *) (ddrBuffer + outSizeBytes);
         }

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOut: %p pOutCn: %p pScratch: %p\n", pIn, pOut, pOutCn, pScratch);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn && pDelay && pOut && pOutCn && pScratch && pDelayCn) {
            // Copy data to input buffer for all repetitions (frames)
            for (frameCnt = 0; frameCnt < numExecReps; frameCnt++) {
               if (currPrm.interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
                  TI_fillBuffer_float(
                      prm[tpi].testPattern, 0, (uint8_t *) pIn + frameCnt * bufParamsIn.dim_y * bufParamsIn.stride_y,
                      (uint8_t *) prm[tpi].staticRefInCase +
                          frameCnt * bufParamsIn.dim_x * bufParamsIn.dim_y * AUDIOLIB_sizeof(currPrm.dataType),
                      bufParamsIn.dim_x, bufParamsIn.dim_y, bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.dataType),
                      testPatternString);
               }
               else {
                  TI_fillBuffer_float(
                      prm[tpi].testPattern, 0, (uint8_t *) pIn + frameCnt * bufParamsIn.dim_y * bufParamsIn.stride_y,
                      (uint8_t *) prm[tpi].staticRefInCase + frameCnt * bufParamsIn.dim_y * bufParamsIn.stride_y,
                      bufParamsIn.dim_x, bufParamsIn.dim_y, bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.dataType),
                      testPatternString);
               }
            }

            // Delay buffer is set to zero
            memset(pDelay, 0, delaySizeBytes);
            memset(pDelayCn, 0, delaySizeBytes);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 0\n");

            status_init = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsIn, &bufParamsDelay, &bufParamsOut,
                                                                  &kerInitArgs);
            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 1 status_init %d\n",
                                  status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

               status_init =
                   AUDIOLIB_delayNChannel_init(handle, &bufParamsIn, &bufParamsDelay, &bufParamsOut, &kerInitArgs);

               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 2 status_init %d\n",
                                  status_init);

            status_opt = AUDIOLIB_delayNChannel_exec_checkParams(handle, pIn, pDelay, pOut, pScratch);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 3 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               // Execute first frame for cold cycle count
               status_opt = AUDIOLIB_delayNChannel_exec(handle, pIn, pDelay, pOut, pScratch);
               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();

               AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 4 status_opt %d\n",
                                     status_opt);

               /* The following for loop is to call kernel repeatedly so as to
                * train the branch predictor                                     */
               // Execute frames 1 to (numExecReps - 1) for warm instruction cache test
               for (frameCnt = 1; frameCnt < (numExecReps - 1); frameCnt++) {
                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
                  TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
                  AUDIOLIB_asm(" MARK 4");
                  status_opt = AUDIOLIB_delayNChannel_exec(
                      handle, (uint8_t *) pIn + frameCnt * bufParamsIn.dim_y * bufParamsIn.stride_y, pDelay,
                      (uint8_t *) pOut + (frameCnt * bufParamsOut.dim_y * bufParamsOut.stride_y), pScratch);
                  AUDIOLIB_asm(" MARK 5");
                  TI_profile_stop();
               }

               AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 5 status_opt %d\n",
                                     status_opt);

               // run warm instruction cache test
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
               AUDIOLIB_asm(" MARK 6");
               // Execute final frame for WARMWRB instruction cache test
               status_opt = AUDIOLIB_delayNChannel_exec(
                   handle, (uint8_t *) pIn + (numExecReps - 1) * bufParamsIn.dim_y * bufParamsIn.stride_y, pDelay,
                   (uint8_t *) pOut + (numExecReps - 1) * bufParamsOut.dim_y * bufParamsOut.stride_y, pScratch);
               AUDIOLIB_asm(" MARK 7");
               TI_profile_stop();

               AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 6 status_opt %d\n",
                                     status_opt);
            }

            /* Test _cn kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            // initialize the kernel to use the natural C variant
            status_init =
                AUDIOLIB_delayNChannel_init(handle, &bufParamsIn, &bufParamsDelay, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 7 status_init %d\n",
                                  status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               // Execute all frames for _cn kernel
               for (frameCnt = 0; frameCnt < numExecReps; frameCnt++) {
                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_CN);
                  TI_profile_start(TI_PROFILE_KERNEL_CN);
                  AUDIOLIB_asm(" MARK 8");
                  status_nat = AUDIOLIB_delayNChannel_exec(
                      handle, (uint8_t *) pIn + frameCnt * bufParamsIn.dim_y * bufParamsIn.stride_y, pDelayCn,
                      (uint8_t *) pOutCn + (frameCnt * bufParamsOut.dim_y * bufParamsOut.stride_y), pScratch);
                  AUDIOLIB_asm(" MARK 9");
                  TI_profile_stop();
               }
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 8 status_nat %d\n", status_nat);

            if (status_nat == AUDIOLIB_SUCCESS) {
               status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(2, -10),
                                                           bufParamsOut.dim_x, numExecReps * bufParamsOut.dim_y,
                                                           bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.dataType));

               status_natDelay_vs_optDelay = TI_compare_mem_2D_float(
                   (void *) pDelay, (void *) pDelayCn, 0.001, (double) powf(2, -10), bufParamsDelay.dim_x,
                   bufParamsDelay.dim_y, bufParamsDelay.stride_y, AUDIOLIB_sizeof(currPrm.dataType));

               if (currPrm.staticRefOutCase != NULL) {
                  AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
                  bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

                  status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                      (void *) pOut, (void *) currPrm.staticRefOutCase, 0.001, (double) powf(2, -10),
                      bufParamsOut.dim_x, numExecReps * bufParamsOut.dim_y, bufParamsOut.stride_y,
                      bufParamsOut.dim_x * AUDIOLIB_sizeof(currPrm.dataType), AUDIOLIB_sizeof(currPrm.dataType));

                  status_refDelay_vs_optDelay =
                      TI_compare_mem_2D_float((void *) pDelay, currPrm.staticUpdatedDelayCase, 0.001,
                                              (double) powf(2, -10), bufParamsDelay.dim_x, bufParamsDelay.dim_y,
                                              bufParamsDelay.stride_y, AUDIOLIB_sizeof(currPrm.dataType));

                  // For internal testing only
                  status_ref_vs_nat_out = TI_compare_mem_2DDualStride_float(
                      (void *) pOutCn, currPrm.staticRefOutCase, 0.001, (double) powf(2, -10), bufParamsOut.dim_x,
                      numExecReps * bufParamsOut.dim_y, bufParamsOut.stride_y,
                      bufParamsOut.dim_x * AUDIOLIB_sizeof(currPrm.dataType), AUDIOLIB_sizeof(currPrm.dataType));

                  status_ref_vs_nat_Delay =
                      TI_compare_mem_2D_float((void *) pDelayCn, currPrm.staticUpdatedDelayCase, 0.001,
                                              (double) powf(2, -10), bufParamsDelay.dim_x, bufParamsDelay.dim_y,
                                              bufParamsDelay.stride_y, AUDIOLIB_sizeof(currPrm.dataType));

                  AUDIOLIB_DEBUGPRINTFN(0,
                                        "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 9 status_ref_vs_nat_out: %d "
                                        "status_ref_vs_nat_Delay: %d\n",
                                        status_ref_vs_nat_out, status_ref_vs_nat_Delay);

                  comparisonDone = 1;
               }
               else {
                  /* Set to pass since it wasn't supposed to run. */
                  status_ref_vs_opt           = TI_TEST_KERNEL_PASS;
                  status_refDelay_vs_optDelay = TI_TEST_KERNEL_PASS;
               }
            }

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_ref_vs_nat_out == TI_TEST_KERNEL_FAIL) || (status_ref_vs_nat_Delay == TI_TEST_KERNEL_FAIL) ||
                 (status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_natDelay_vs_optDelay == TI_TEST_KERNEL_FAIL) ||
                 (status_refDelay_vs_optDelay == TI_TEST_KERNEL_FAIL) || (status_init != AUDIOLIB_SUCCESS) ||
                 (status_opt != AUDIOLIB_SUCCESS) || (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) ||
                 (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 10 status_nat_vs_opt %d "
                                  "status_natDelay_vs_optDelay: %d; status_ref_vs_opt: %d;"
                                  " status_refDelay_vs_optDelay: %d currentTestFail: %d \n",
                                  status_nat_vs_opt, status_natDelay_vs_optDelay, status_ref_vs_opt,
                                  status_refDelay_vs_optDelay, currentTestFail);
            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 11 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_delayNChannel_d CP 12 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            sprintf(desc, "%s generated input | numSamples = %d, numChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);

            AUDIOLIB_delayNChannel_perfEst(handle, &archCycles, &estCycles, currPrm.dataType);

            //  write to CSV, must happen prior to write to screen because
            //  TI_profile_formula_add clears values in counters
            fprintf(fpOutputCSV, "AUDIOLIB_delayNChannel, %d, %d, %d, %d, %d, %d, %d, %d, %ld, %ld, %f, %u\n", testNum,
                    currPrm.testPattern, AUDIOLIB_sizeof(currPrm.dataType) * 8, currPrm.maxDelay, currPrm.numSamples,
                    currPrm.numChannels, currPrm.interleave, currPrm.mode, estCycles,
                    cycles[TI_PROFILE_KERNEL_OPT_WARM],
                    ((AUDIOLIB_F32) cycles[TI_PROFILE_KERNEL_OPT_WARM]) / ((AUDIOLIB_F32) estCycles), !currentTestFail);

            TI_profile_add_test(testNum++, currPrm.numSamples, archCycles, estCycles, currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | numSamples = %d, numChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_delayNChannel_test
            TI_profile_clear_run_stats();
         } // end of memory allocation successful?

         /* Free buffers for each test vector */
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            TI_align_free(pOut);
            TI_align_free(pScratch);
            free(pOutCn);
            free(pDelayCn);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
#if defined(__C7504__) || defined(__C7524__)
            TI_align_free(pOut);
            TI_align_free(pScratch);
#else
            free(pOut);
            free(pScratch);
#endif
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            TI_align_free(pOut);
            TI_align_free(pScratch);
#else
            free(pOut);
            free(pScratch);

#endif
         }

         TI_align_free(pIn);
         TI_align_free(pDelay);

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

      TI_memError("AUDIOLIB_delayNChannel");

      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif

   {
      return AUDIOLIB_delayNChannel_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                         testNum         = 1000;
   int32_t                         currentTestFail = 0;
   AUDIOLIB_delayNChannel_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_delayNChannel_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut, bufParamsDelay;

   int   fail = 0;
   void *pIn, *pDelay, *pOut;

   int32_t inSamples  = 16;
   int32_t inChannels = 16;

   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = inChannels;
   bufParamsIn.dim_y     = inSamples;
   bufParamsIn.stride_y  = bufParamsIn.dim_y * AUDIOLIB_sizeof(bufParamsIn.data_type);

   bufParamsDelay.data_type = AUDIOLIB_FLOAT32;
   bufParamsDelay.dim_x     = 1;
   bufParamsDelay.dim_y     = inChannels;

   bufParamsOut.data_type = bufParamsIn.data_type;
   bufParamsOut.dim_x     = inChannels;
   bufParamsOut.dim_y     = inSamples;
   bufParamsOut.stride_y  = bufParamsOut.dim_y * AUDIOLIB_sizeof(bufParamsOut.data_type);

   uint64_t In0Size = bufParamsIn.stride_y * bufParamsIn.dim_y;
   uint64_t In1Size = bufParamsDelay.dim_x * AUDIOLIB_sizeof(bufParamsDelay.data_type);
   uint64_t OutSize = bufParamsOut.stride_y * bufParamsOut.dim_y;

   AUDIOLIB_bufParams2D_t bufParamsInTemp, bufParamsOutTemp;
   AUDIOLIB_bufParams2D_t bufParamsDelayTemp;

   pIn    = (void *) malloc(In0Size);
   pDelay = (void *) malloc(In1Size);
   pOut   = (void *) malloc(OutSize);

   /* Seed init args read by the init() copy loop in the cases below. */
   kerInitArgs.maxDelay = 0;
   int32_t ch;
   for (ch = 0; ch < MAX_NUM_CHANNELS; ch++) {
      kerInitArgs.delaySize[ch] = 0;
   }

   while (testNum <= 1010) {

      switch (testNum) {
      case 1000:
         bufParamsInTemp.data_type    = bufParamsIn.data_type;
         bufParamsDelayTemp.data_type = bufParamsDelay.data_type;
         bufParamsOutTemp.data_type   = bufParamsOut.data_type;

         bufParamsInTemp.dim_x    = bufParamsIn.dim_x;
         bufParamsDelayTemp.dim_x = bufParamsDelay.dim_x;
         bufParamsOutTemp.dim_x   = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y  = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_delayNChannel_init_checkParams(NULL, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_delayNChannel_init_checkParams(NULL, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1001:
         bufParamsInTemp.data_type    = AUDIOLIB_UINT128;
         bufParamsDelayTemp.data_type = AUDIOLIB_UINT128;
         bufParamsOutTemp.data_type   = AUDIOLIB_UINT128;

         bufParamsInTemp.dim_x    = bufParamsIn.dim_x;
         bufParamsDelayTemp.dim_x = bufParamsDelay.dim_x;
         bufParamsOutTemp.dim_x   = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y  = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;
         kerInitArgs.mode       = 0;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1002:
         bufParamsInTemp.data_type    = AUDIOLIB_UINT16;
         bufParamsDelayTemp.data_type = bufParamsDelay.data_type;
         bufParamsOutTemp.data_type   = AUDIOLIB_UINT32;
         kerInitArgs.mode             = 0;

         bufParamsInTemp.dim_x    = bufParamsIn.dim_x;
         bufParamsDelayTemp.dim_x = bufParamsDelay.dim_x;
         bufParamsOutTemp.dim_x   = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y  = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         bufParamsInTemp.data_type    = bufParamsIn.data_type;
         bufParamsDelayTemp.data_type = AUDIOLIB_UINT32;
         bufParamsOutTemp.data_type   = bufParamsOut.data_type;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1003:
         bufParamsInTemp.data_type    = bufParamsIn.data_type;
         bufParamsDelayTemp.data_type = bufParamsDelay.data_type;
         bufParamsOutTemp.data_type   = bufParamsOut.data_type;

         bufParamsInTemp.dim_x    = bufParamsIn.dim_x;
         bufParamsDelayTemp.dim_x = bufParamsDelay.dim_x;
         bufParamsOutTemp.dim_x   = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y  = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;
         kerInitArgs.mode       = 5;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_NOT_IMPLEMENTED) || (status_opt != AUDIOLIB_ERR_NOT_IMPLEMENTED));
         break;
      case 1004:
         bufParamsInTemp.data_type    = bufParamsIn.data_type;
         bufParamsDelayTemp.data_type = bufParamsDelay.data_type;
         bufParamsOutTemp.data_type   = bufParamsOut.data_type;

         bufParamsInTemp.dim_x    = bufParamsIn.dim_x;
         bufParamsDelayTemp.dim_x = bufParamsDelay.dim_x;
         bufParamsOutTemp.dim_x   = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y    = 1000; /* Extremely large number of channels > MAX_NUM_CHANNELS */
         bufParamsDelayTemp.dim_y = bufParamsDelay.dim_y;
         bufParamsOutTemp.dim_y   = bufParamsOut.dim_y;
         kerInitArgs.mode         = 0;
         kerInitArgs.interleave   = 0; /* non-interleaved, so dim_y is numChannels */

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) || (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION));
         break;
      case 1005:
         /* checkParams: input/output dimension mismatch -> INVALID_DIMENSION */
         bufParamsInTemp.data_type    = bufParamsIn.data_type;
         bufParamsDelayTemp.data_type = bufParamsDelay.data_type;
         bufParamsOutTemp.data_type   = bufParamsOut.data_type;

         bufParamsInTemp.dim_x    = bufParamsIn.dim_x;
         bufParamsDelayTemp.dim_x = bufParamsDelay.dim_x;
         bufParamsOutTemp.dim_x   = bufParamsOut.dim_x;

         bufParamsInTemp.dim_y  = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y / 2; /* != input dim_y */
         kerInitArgs.mode       = 0;
         kerInitArgs.interleave = 0;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_delayNChannel_init_checkParams(handle, &bufParamsInTemp, &bufParamsDelayTemp,
                                                                         &bufParamsOutTemp, &kerInitArgs);
         currentTestFail       = (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION);
         break;
      case 1006:
         /* init() with NULL handle -> NULL_POINTER */
         bufParamsInTemp.data_type    = bufParamsIn.data_type;
         bufParamsDelayTemp.data_type = bufParamsDelay.data_type;
         bufParamsOutTemp.data_type   = bufParamsOut.data_type;
         bufParamsInTemp.dim_x        = bufParamsIn.dim_x;
         bufParamsInTemp.dim_y        = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_x       = bufParamsOut.dim_x;
         bufParamsOutTemp.dim_y       = bufParamsOut.dim_y;
         bufParamsDelayTemp.dim_x     = bufParamsDelay.dim_x;
         kerInitArgs.mode             = 0;
         kerInitArgs.interleave       = 0;
         kerInitArgs.funcStyle        = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt =
             AUDIOLIB_delayNChannel_init(NULL, &bufParamsInTemp, &bufParamsDelayTemp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = (status_opt != AUDIOLIB_ERR_NULL_POINTER);
         break;
      case 1007:
         /* init() with numChannels > MAX_NUM_CHANNELS -> INVALID_DIMENSION (bound re-assert) */
         bufParamsInTemp.data_type    = bufParamsIn.data_type;
         bufParamsDelayTemp.data_type = bufParamsDelay.data_type;
         bufParamsOutTemp.data_type   = bufParamsOut.data_type;
         bufParamsInTemp.dim_x        = bufParamsIn.dim_x;
         bufParamsInTemp.dim_y        = 1000; /* > MAX_NUM_CHANNELS (non-interleave channel axis) */
         bufParamsInTemp.stride_y     = bufParamsIn.stride_y;
         bufParamsOutTemp.dim_x       = bufParamsOut.dim_x;
         bufParamsOutTemp.dim_y       = 1000;
         bufParamsOutTemp.stride_y    = bufParamsOut.stride_y;
         bufParamsDelayTemp.dim_x     = bufParamsDelay.dim_x;
         bufParamsDelayTemp.stride_y  = bufParamsIn.stride_y;
         kerInitArgs.mode             = 0;
         kerInitArgs.interleave       = 0;
         kerInitArgs.funcStyle        = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_delayNChannel_init(handle, &bufParamsInTemp, &bufParamsDelayTemp, &bufParamsOutTemp,
                                                       &kerInitArgs);
         currentTestFail = (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION);
         break;
      case 1008:
         /* exec_checkParams: NULL handle -> NULL_POINTER; all-valid -> SUCCESS */
         status_nat      = AUDIOLIB_delayNChannel_exec_checkParams(NULL, pIn, pDelay, pOut, pOut);
         status_opt      = AUDIOLIB_delayNChannel_exec_checkParams(handle, pIn, pDelay, pOut, pOut);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_SUCCESS));
         break;
      case 1009:
         /* init() dispatch with unsupported type, non-interleaved -> INVALID_TYPE (NATC + OPTIMIZED) */
         bufParamsInTemp.data_type    = AUDIOLIB_UINT16;
         bufParamsDelayTemp.data_type = AUDIOLIB_UINT16;
         bufParamsOutTemp.data_type   = AUDIOLIB_UINT16;
         bufParamsInTemp.dim_x        = bufParamsIn.dim_x;
         bufParamsInTemp.dim_y        = bufParamsIn.dim_y;
         bufParamsInTemp.stride_y     = bufParamsIn.stride_y;
         bufParamsOutTemp.dim_x       = bufParamsOut.dim_x;
         bufParamsOutTemp.dim_y       = bufParamsOut.dim_y;
         bufParamsOutTemp.stride_y    = bufParamsOut.stride_y;
         bufParamsDelayTemp.dim_x     = bufParamsDelay.dim_x;
         bufParamsDelayTemp.stride_y  = bufParamsIn.stride_y;
         kerInitArgs.mode             = 0;
         kerInitArgs.interleave       = 0;
         kerInitArgs.funcStyle        = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_delayNChannel_init(handle, &bufParamsInTemp, &bufParamsDelayTemp, &bufParamsOutTemp,
                                                  &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_delayNChannel_init(handle, &bufParamsInTemp, &bufParamsDelayTemp, &bufParamsOutTemp,
                                                       &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1010:
         /* init() dispatch with unsupported type, interleaved -> INVALID_TYPE (NATC + OPTIMIZED) */
         bufParamsInTemp.data_type    = AUDIOLIB_UINT16;
         bufParamsDelayTemp.data_type = AUDIOLIB_UINT16;
         bufParamsOutTemp.data_type   = AUDIOLIB_UINT16;
         bufParamsInTemp.dim_x        = bufParamsIn.dim_x;
         bufParamsInTemp.dim_y        = bufParamsIn.dim_y;
         bufParamsInTemp.stride_y     = bufParamsIn.stride_y;
         bufParamsOutTemp.dim_x       = bufParamsOut.dim_x;
         bufParamsOutTemp.dim_y       = bufParamsOut.dim_y;
         bufParamsOutTemp.stride_y    = bufParamsOut.stride_y;
         bufParamsDelayTemp.dim_x     = bufParamsDelay.dim_x;
         bufParamsDelayTemp.stride_y  = bufParamsIn.stride_y;
         kerInitArgs.mode             = 0;
         kerInitArgs.interleave       = 1;
         kerInitArgs.funcStyle        = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_delayNChannel_init(handle, &bufParamsInTemp, &bufParamsDelayTemp, &bufParamsOutTemp,
                                                  &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_delayNChannel_init(handle, &bufParamsInTemp, &bufParamsDelayTemp, &bufParamsOutTemp,
                                                       &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }
   free(pIn);
   free(pDelay);

   free(pOut);

   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_delayNChannel_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_delayNChannel_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_delayNChannel_getHandleSize(&kerInitArgs);
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
