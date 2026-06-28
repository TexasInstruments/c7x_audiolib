// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_nlms_idat.h"
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

int AUDIOLIB_nlms_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;
   uint64_t               archCycles = 0;
   uint64_t               estCycles  = 0;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_nlms_testParams_t *prm;
   AUDIOLIB_nlms_testParams_t  currPrm;
   AUDIOLIB_nlms_getTestParams(&prm, &test_cases);

   AUDIOLIB_nlms_InitArgs kerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 0\n");

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 1\n");

   TI_profile_init("AUDIOLIB_nlms");
   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_nlms.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width,inSamples, inChannels, Filter Length, "
                        "EVM cycles,estCycles, Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      kerInitArgs.filterLength          = currPrm.filterLength;
      kerInitArgs.stepSize              = currPrm.stepSize;
      kerInitArgs.numChannels           = currPrm.inChannels;
      uint32_t              numExecReps = currPrm.numExecReps;
      int32_t               handleSize  = AUDIOLIB_nlms_getHandleSize(&kerInitArgs);
      AUDIOLIB_kernelHandle handle      = malloc(handleSize);

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t status_ref_vs_nat = TI_TEST_KERNEL_FAIL;
         int32_t status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t status_nat_vs_opt = TI_TEST_KERNEL_FAIL;

         AUDIOLIB_STATUS status_init = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt  = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat  = AUDIOLIB_SUCCESS;
         archCycles                  = 0;
         estCycles                   = 0;

         currentTestFail = 0;

         bufParamsIn.data_type = currPrm.dataType;
         bufParamsIn.dim_x     = currPrm.inSamples;
         bufParamsIn.dim_y     = currPrm.inChannels;
         bufParamsIn.stride_y  = currPrm.strideIn;

         bufParamsOut.data_type = currPrm.dataType;
         bufParamsOut.dim_x     = currPrm.inSamples;
         bufParamsOut.dim_y     = currPrm.inChannels;
         bufParamsOut.stride_y  = currPrm.strideOut;

         uint32_t inpSizeBytes = bufParamsIn.dim_y * bufParamsIn.stride_y;
         uint32_t outSizeBytes = bufParamsOut.dim_y * bufParamsOut.stride_y;

         // --- State Buffer Allocation (Circular Buffer) ---
         // Calculate Power-of-2 Stride for Circular Addressing
         uint32_t CircularBufBaseMmemSize = 512;
         while (CircularBufBaseMmemSize < (currPrm.filterLength + currPrm.inSamples)) {
            CircularBufBaseMmemSize *= 2;
         }

         // This stride is used for alignment and memory allocation
         uint32_t stateStrideBytes = CircularBufBaseMmemSize * AUDIOLIB_sizeof(currPrm.dataType);
         uint32_t stateBufferSize  = currPrm.inChannels * stateStrideBytes;

         // --- Scratch Buffer Allocation (Accumulators) ---
         uint32_t scratchBuffSizeBytes = 2 * inpSizeBytes * AUDIOLIB_sizeof(currPrm.dataType);

         void *pIn          = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpSizeBytes * numExecReps);
         void *pInDesired   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes * numExecReps);
         void *pStateBuffer = (void *) TI_memalign(stateStrideBytes, stateBufferSize);

         void *pScratchBuffer = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, scratchBuffSizeBytes);
         void *pCoefficients  = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, scratchBuffSizeBytes);

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

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOut: %p pOutCn: %p\n", pIn, pOut, pOutCn);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn && pInDesired && pOut && pOutCn && pStateBuffer && pScratchBuffer && pCoefficients) {

            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {

               uint32_t frame = 0;
               for (frame = 0; frame < numExecReps; frame++) {
                  TI_fillBuffer_float(
                      prm[tpi].testPattern, 0, (void *) ((int8_t *) pIn + frame * inpSizeBytes),
                      (void *) ((float *) prm[tpi].staticIn0 + (frame * currPrm.inSamples * currPrm.inChannels)),
                      bufParamsIn.dim_x, bufParamsIn.dim_y, bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.dataType),
                      testPatternString);
                  TI_fillBuffer_float(
                      prm[tpi].testPattern, 0, (void *) ((int8_t *) pInDesired + frame * inpSizeBytes),
                      (void *) ((float *) prm[tpi].staticDesired + (frame * currPrm.inSamples * currPrm.inChannels)),
                      bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.dataType),
                      testPatternString);
               }
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 0\n");

            // Note: Update checkParams to accept new buffers if needed, or keep standard bufParams
            status_init = AUDIOLIB_nlms_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_nlms_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 2 status_init %d\n", status_init);

            status_opt = AUDIOLIB_nlms_exec_checkParams(handle, pIn, pInDesired, pStateBuffer, pScratchBuffer,
                                                        pCoefficients, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 3 status_opt %d\n", status_opt);

            memset(pCoefficients, 0, scratchBuffSizeBytes);
            memset(pStateBuffer, 0, stateBufferSize);
            memset(pScratchBuffer, 0, scratchBuffSizeBytes); // Clear accumulators

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               // Updated exec call
               status_opt =
                   AUDIOLIB_nlms_exec(handle, pIn, pInDesired, pStateBuffer, pScratchBuffer, pCoefficients, pOut);

               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();
            }

            uint32_t k;
            for (k = 0; k < (numExecReps - 2); k++) {
               // run warm instruction cache test
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
               AUDIOLIB_asm(" MARK 4");

               status_opt = AUDIOLIB_nlms_exec(handle, (void *) ((int8_t *) pIn + (k + 1) * inpSizeBytes),
                                               (void *) ((int8_t *) pInDesired + (k + 1) * outSizeBytes), pStateBuffer,
                                               pScratchBuffer, pCoefficients,
                                               (void *) ((int8_t *) pOut + (k + 1) * outSizeBytes));
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
            if (numExecReps != 1) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
               AUDIOLIB_asm(" MARK 6");
               status_opt = AUDIOLIB_nlms_exec(handle, (void *) ((int8_t *) pIn + (numExecReps - 1) * inpSizeBytes),
                                               (void *) ((int8_t *) pInDesired + (numExecReps - 1) * outSizeBytes),
                                               pStateBuffer, pScratchBuffer, pCoefficients,
                                               (void *) ((int8_t *) pOut + (numExecReps - 1) * outSizeBytes));
               AUDIOLIB_asm(" MARK 7");
               TI_profile_stop();
            }

            /* Test _cn kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_nlms_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 5\n");
#endif
            memset(pCoefficients, 0, scratchBuffSizeBytes);
            memset(pStateBuffer, 0, stateBufferSize);
            memset(pScratchBuffer, 0, scratchBuffSizeBytes);

            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            for (k = 0; k < numExecReps; k++) {
               // Updated exec call for CN
               status_nat =
                   AUDIOLIB_nlms_exec(handle, (void *) ((int8_t *) pIn + k * inpSizeBytes),
                                      (void *) ((int8_t *) pInDesired + k * outSizeBytes), pStateBuffer, pScratchBuffer,
                                      pCoefficients, (void *) ((int8_t *) pOutCn + k * outSizeBytes));
            }
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 6 status_nat %d\n", status_nat);
#endif

            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {

               // Initialize comparison statuses as PASS; fail if any frame fails
               status_nat_vs_opt = TI_TEST_KERNEL_PASS;
               uint32_t frame    = 0;
               for (frame = 0; frame < numExecReps; frame++) {
                  // Compare optimized vs. natural C for this frame
                  int32_t frame_status_nat_vs_opt = TI_compare_mem_2D_float(
                      (void *) ((int8_t *) pOut + frame * outSizeBytes),
                      (void *) ((int8_t *) pOutCn + frame * outSizeBytes), 0.001, (double) powf(2, -10),
                      bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.dataType));

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
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 7 comparisonDone %d status_nat_vs_opt %d\n",
                                  comparisonDone, 0);

            if (currPrm.staticOut != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
               bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

               if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {
                  // Frame-by-frame comparison for reference outputs
                  status_ref_vs_nat = TI_TEST_KERNEL_PASS;
                  status_ref_vs_opt = TI_TEST_KERNEL_PASS;
                  uint32_t frame    = 0;
                  for (frame = 0; frame < numExecReps; frame++) {
                     int32_t frame_status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                         (void *) ((int8_t *) pOut + frame * outSizeBytes),
                         (void *) ((float *) currPrm.staticOut + frame * currPrm.inSamples * currPrm.inChannels), 0.001,
                         (double) powf(2, -10), bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y,
                         bufParamsOut.dim_x * AUDIOLIB_sizeof(currPrm.dataType), AUDIOLIB_sizeof(currPrm.dataType));

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
            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail = ((status_ref_vs_nat == TI_TEST_KERNEL_FAIL) || (status_init != AUDIOLIB_SUCCESS) ||
                               (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                               (status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_opt != AUDIOLIB_SUCCESS) ||
                               (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                                  ? 1
                                  : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;
            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_nlms_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            sprintf(desc, "%s generated input | inSamples = %d, inChannels = %d", testPatternString, currPrm.inSamples,
                    currPrm.inChannels);
            AUDIOLIB_nlms_perfEst(handle, &archCycles, &estCycles);
            fprintf(fpOutputCSV, "%d,  %d,%d, %d, %d, %d ,%ld, %d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.dataType) * 8, currPrm.inSamples, currPrm.inChannels, currPrm.filterLength,
                    pProfile[3 * tpi + 1], estCycles, !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.inSamples * currPrm.inChannels), archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.inSamples, currPrm.inChannels);
            TI_profile_skip_test(desc);
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
         TI_align_free(pIn);
         TI_align_free(pInDesired);
         TI_align_free(pStateBuffer);   // New free
         TI_align_free(pScratchBuffer); // Updated free
         TI_align_free(pCoefficients);

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
      TI_memError("AUDIOLIB_nlms");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_nlms_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                testNum         = 1000;
   int32_t                currentTestFail = 0;
   AUDIOLIB_nlms_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_nlms_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   int   fail = 0;
   void *pIn, *pOut;

   int32_t inSamples  = 16;
   int32_t inChannels = 16;

   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = inChannels;
   bufParamsIn.dim_y     = inSamples;
   bufParamsIn.stride_y  = bufParamsIn.dim_y * AUDIOLIB_sizeof(bufParamsIn.data_type);

   bufParamsOut.data_type = bufParamsIn.data_type;
   bufParamsOut.dim_x     = inChannels;
   bufParamsOut.dim_y     = inSamples;
   bufParamsOut.stride_y  = bufParamsOut.dim_y * AUDIOLIB_sizeof(bufParamsOut.data_type);

   uint64_t InSize = bufParamsIn.stride_y * bufParamsIn.dim_y;

   uint64_t OutSize = bufParamsOut.stride_y * bufParamsOut.dim_y;

   AUDIOLIB_bufParams2D_t bufParamsInTemp, bufParamsOutTemp;

   pIn  = (void *) malloc(InSize);
   pOut = (void *) malloc(OutSize);

   while (testNum <= 1002) {

      switch (testNum) {
      case 1000:

         bufParamsInTemp.data_type  = bufParamsIn.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;
         bufParamsInTemp.dim_x      = bufParamsIn.dim_x;
         bufParamsOutTemp.dim_x     = bufParamsOut.dim_x;
         bufParamsInTemp.dim_y      = bufParamsIn.dim_y;
         bufParamsOutTemp.dim_y     = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_nlms_init_checkParams(NULL, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgs);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_nlms_init_checkParams(NULL, &bufParamsIn, &bufParamsOut, &kerInitArgs);

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
         status_nat            = AUDIOLIB_nlms_init_checkParams(handle, &bufParamsInTemp, &bufParamsOut, &kerInitArgs);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_nlms_init_checkParams(handle, &bufParamsInTemp, &bufParamsOut, &kerInitArgs);

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
         status_nat            = AUDIOLIB_nlms_init_checkParams(handle, &bufParamsIn, &bufParamsOutTemp, &kerInitArgs);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_nlms_init_checkParams(handle, &bufParamsIn, &bufParamsOutTemp, &kerInitArgs);

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

   free(pOut);

   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_nlms_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_nlms_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_nlms_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for nlms test projects */
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
