// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>
// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_balanceFader_idat.h"
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

int AUDIOLIB_balanceFader_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   uint8_t                isInterleave;
   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams2D_t bufParamsOut;
   AUDIOLIB_bufParams1D_t bufParamsGain;
   uint32_t               testNum;
   uint32_t               comparisonDone = 0;

   AUDIOLIB_balanceFader_testParams_t *prm;
   AUDIOLIB_balanceFader_testParams_t  currPrm;
   AUDIOLIB_balanceFader_getTestParams(&prm, &test_cases);

   AUDIOLIB_balanceFader_InitArgs kerInitArgs;
   AUDIOLIB_balanceFader_SetArgs  kerSetArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 0\n");

   int32_t               handleSize = AUDIOLIB_balanceFader_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 1\n");

   TI_profile_init("AUDIOLIB_balanceFader");
   // #if (!defined(EVM_TEST))
   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_balanceFader.csv", "w+");
   fprintf(fpOutputCSV,
           "Test ID, Bit Width, inSamples, inChannels, Interleave, lfeFaderMode, lfeBalanceMode, Balance, Fade,"
           "EST cycles, EVM cycles, EVM/EST cycles, Pass/Fail\n");
   // #endif //  #if !defined(EVM_TEST)

   for (tpi = 0; tpi < test_cases; tpi++) {
      uint64_t archCycles = 0, estCycles = 0;
      numReps      = prm[tpi].numReps;
      testNum      = prm[tpi].testID;
      currPrm      = prm[tpi];
      isInterleave = currPrm.isInterleave;

      kerSetArgs.fade                 = currPrm.fade;
      kerSetArgs.balance              = currPrm.balance;
      kerSetArgs.lfeFaderMode         = currPrm.lfeFaderMode;
      kerSetArgs.lfeBalanceMode       = currPrm.lfeBalanceMode;
      kerSetArgs.sideGainFactor       = currPrm.sideGainFactor;
      kerSetArgs.lfeFaderGainFactor   = currPrm.lfeFaderGainFactor;
      kerSetArgs.lfeBalanceGainFactor = currPrm.lfeBalanceGainFactor;

      kerInitArgs.isInterleave = currPrm.isInterleave;

      kerSetArgs.channelConfig = (int32_t *) malloc(currPrm.numChannels * sizeof(int32_t));
      memcpy(kerSetArgs.channelConfig, currPrm.channelConfig, currPrm.numChannels * sizeof(int32_t));

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;

         currentTestFail = 0;

         if (isInterleave == 1) {
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

         bufParamsGain.dim_x     = currPrm.numChannels;
         bufParamsGain.data_type = currPrm.sampleDataType;

         // Here, stride is in bytes
         uint32_t inpSizeBytes;
         uint32_t outSizeBytes;
         uint32_t gainSizeBytes;
         // Calculate the size of the input and output buffers in bytes
         inpSizeBytes  = bufParamsIn.dim_y * bufParamsIn.stride_y;
         outSizeBytes  = bufParamsOut.dim_y * bufParamsOut.stride_y;
         gainSizeBytes = bufParamsGain.dim_x * sizeof(currPrm.sampleDataType);

         void *pIn   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpSizeBytes);
         void *pGain = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, gainSizeBytes);
         if (pGain)
            memset(pGain, 0, gainSizeBytes);

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
         if (pIn && pGain && pOut && pOutCn) {

            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

               TI_fillBuffer_float(prm[tpi].testPattern, 0, pIn, prm[tpi].staticIn, bufParamsIn.dim_x,
                                   bufParamsIn.dim_y, bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType),
                                   testPatternString);
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 0\n");

            status_init = AUDIOLIB_balanceFader_init_checkParams(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut,
                                                                 &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init =
                   AUDIOLIB_balanceFader_init(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_balanceFader_set(handle, pGain, &kerSetArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 2 status_init %d\n", status_init);

            status_opt = AUDIOLIB_balanceFader_exec_checkParams(handle, pIn, pGain, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 3 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_balanceFader_exec(handle, pIn, pGain, pOut);
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
               // TI_profile_clear_cycle_counts();
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
               AUDIOLIB_asm(" MARK 4");
               status_opt = AUDIOLIB_balanceFader_exec(handle, pIn, pGain, pOut);
               AUDIOLIB_asm(" MARK 5");
               TI_profile_stop();
            }

            int16_t outSum   = 0;
            int8_t *pOutTemp = (int8_t *) pOut;
            for (k = 0; k < bufParamsOut.dim_x; k++) {
               outSum += *pOutTemp;
               pOutTemp++;
            }
            volatileSum = outSum;

            // run warm instruction cache test
            TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
            AUDIOLIB_asm(" MARK 6");
            status_opt = AUDIOLIB_balanceFader_exec(handle, pIn, pGain, pOut);
            AUDIOLIB_asm(" MARK 7");
            TI_profile_stop();

#endif // #if !defined(__C7X_HOSTEM__)
            /* Test the natural C variant */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_balanceFader_init(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);
            AUDIOLIB_balanceFader_set(handle, pGain, &kerSetArgs);
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            status_nat = AUDIOLIB_balanceFader_exec(handle, pIn, pGain, pOutCn);
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {
               status_nat_vs_opt = TI_compare_mem_2D_float(
                   (void *) pOut, (void *) pOutCn, 0, (double) powf(2, -10), bufParamsOut.dim_x, bufParamsOut.dim_y,
                   bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType));
            }
            else {
               // TBD
            }

            comparisonDone = 1;

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 7 comparisonDone %d status_nat_vs_opt %d\n",
                comparisonDone, status_nat_vs_opt);

            if (currPrm.staticOut != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
               bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

               if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

                  status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                      (void *) pOut, (void *) currPrm.staticOut, 0, (double) powf(2, -10), bufParamsOut.dim_x,
                      bufParamsOut.dim_y, bufParamsOut.stride_y,
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
                "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 8 status_nat_vs_opt %d status_ref_vs_opt "
                "%d currentTestFail "
                "%d\n",
                status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_balanceFader_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_balanceFader_get(handle, &kerSetArgs);
            sprintf(desc, "%s generated input | inSamples = %d, inChannels = %d, bal = %0.3f, fade = %0.3f",
                    testPatternString, currPrm.numSamples, currPrm.numChannels, kerSetArgs.balance, kerSetArgs.fade);

            AUDIOLIB_balanceFader_perfEst(handle, &archCycles, &estCycles);

            // #if (!defined(EVM_TEST))
            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            fprintf(fpOutputCSV, "%d, %d, %d, %d, %d, %d, %d, %f, %f, %d, %d, %f, %d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.numSamples, currPrm.numChannels,
                    currPrm.isInterleave, currPrm.lfeFaderMode, currPrm.lfeBalanceMode, currPrm.balance, currPrm.fade,
                    (int32_t) estCycles, pProfile[3 * tpi + 1],
                    ((AUDIOLIB_F32) cycles[TI_PROFILE_KERNEL_OPT_WARM]) / ((AUDIOLIB_F32) estCycles), !currentTestFail);
            // #endif // #if (!defined(EVM_TEST))

            TI_profile_add_test(testNum++, currPrm.numSamples * currPrm.numChannels, archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_balanceFader_test
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
         TI_align_free(pGain);
      } // end repetitions
      free(kerSetArgs.channelConfig);
   } // end idat test cases

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
      TI_memError("AUDIOLIB_balanceFader");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_balanceFader_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                        testNum         = 1000;
   int32_t                        currentTestFail = 0;
   AUDIOLIB_balanceFader_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_balanceFader_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;
   AUDIOLIB_bufParams1D_t bufParamsIn1;

   int   fail = 0;
   void *pIn0, *pIn1, *pOut;

   int32_t inSamples  = 16;
   int32_t inChannels = 16;

   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = inChannels;
   bufParamsIn0.dim_y     = inSamples;
   bufParamsIn0.stride_y  = bufParamsIn0.dim_y * AUDIOLIB_sizeof(bufParamsIn0.data_type);

   bufParamsIn1.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn1.dim_x     = 1;

   bufParamsOut.data_type = bufParamsIn0.data_type;
   bufParamsOut.dim_x     = inChannels;
   bufParamsOut.dim_y     = inSamples;
   bufParamsOut.stride_y  = bufParamsOut.dim_y * AUDIOLIB_sizeof(bufParamsOut.data_type);

   uint64_t In0Size = bufParamsIn0.stride_y * bufParamsIn0.dim_y;
   uint64_t In1Size = bufParamsIn1.dim_x * AUDIOLIB_sizeof(bufParamsIn1.data_type);
   uint64_t OutSize = bufParamsOut.stride_y * bufParamsOut.dim_y;

   AUDIOLIB_bufParams2D_t bufParamsIn0Temp, bufParamsOutTemp;
   AUDIOLIB_bufParams1D_t bufParamsIn1Temp;

   pIn0 = (void *) malloc(In0Size);
   pIn1 = (void *) malloc(In1Size);
   pOut = (void *) malloc(OutSize);

   while (testNum <= 1005) {

      switch (testNum) {
      case 1000:
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsIn1Temp.data_type = bufParamsIn1.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsIn1Temp.dim_x = bufParamsIn1.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_balanceFader_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                        &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_balanceFader_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsIn1Temp,
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

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_balanceFader_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                        &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_balanceFader_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
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

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_balanceFader_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                        &bufParamsOutTemp, &kerInitArgs);
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsIn1Temp.data_type = AUDIOLIB_UINT32;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_balanceFader_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                        &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1003:
         /* Valid handle/buffers but NULL InitArgs -> NULL_POINTER (pKerInitArgs operand). */
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsIn1Temp.data_type = bufParamsIn1.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsIn1Temp.dim_x = bufParamsIn1.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         status_opt      = AUDIOLIB_balanceFader_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                  &bufParamsOutTemp, NULL);
         currentTestFail = (status_opt != AUDIOLIB_ERR_NULL_POINTER);
         break;
      case 1004:
         /* Gain length != channel count -> INVALID_DIMENSION. */
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsIn1Temp.data_type = bufParamsIn1.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsIn1Temp.dim_x = 8; /* != numChannels (16) */
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.isInterleave = 0; /* numChannels = dim_y */
         kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt               = AUDIOLIB_balanceFader_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsIn1Temp,
                                                                           &bufParamsOutTemp, &kerInitArgs);
         currentTestFail          = (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION);
         break;
      case 1005:
         /* exec_checkParams: NULL handle -> NULL_POINTER; all-valid -> SUCCESS. */
         status_nat      = AUDIOLIB_balanceFader_exec_checkParams(NULL, pIn0, pIn1, pOut);
         status_opt      = AUDIOLIB_balanceFader_exec_checkParams(handle, pIn0, pIn1, pOut);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_SUCCESS));
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
   /* For every call of AUDIOLIB_balanceFader_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_balanceFader_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_balanceFader_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for inbalanceFaderidual test projects */
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
