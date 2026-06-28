// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_dB20_idat.h"
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

int AUDIOLIB_dB20_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   uint64_t               estCycles;
   uint64_t               archCycles;
   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_dB20_testParams_t *prm;
   AUDIOLIB_dB20_testParams_t  currPrm;
   AUDIOLIB_dB20_getTestParams(&prm, &test_cases);

   AUDIOLIB_dB20_InitArgs kerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 0\n");

   int32_t               handleSize = AUDIOLIB_dB20_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 1\n");

   TI_profile_init("AUDIOLIB_dB20");

   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_dB20.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, inChannels, Arch Cycles"
                        ",Warm cycles, Pass/Fail\n");
   // endif //  #if !defined(EVM_TEST)

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      for (repCount = 0; repCount < numReps; repCount++) {

         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_nat_vs_ref = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;

         estCycles       = 0;
         archCycles      = 0;
         currentTestFail = 0;

         bufParamsIn.data_type = currPrm.dataType;
         bufParamsIn.dim_x     = currPrm.numSamples;
         bufParamsIn.dim_y     = currPrm.numChannels;
         bufParamsIn.stride_y  = currPrm.strideIn * sizeof(currPrm.dataType);

         bufParamsOut.data_type = currPrm.dataType;
         bufParamsOut.dim_x     = currPrm.numSamples;
         bufParamsOut.dim_y     = currPrm.numChannels;
         bufParamsOut.stride_y  = currPrm.strideOut * sizeof(currPrm.dataType);

         uint32_t inpSizeBytes = bufParamsIn.dim_y * bufParamsIn.stride_y;
         uint32_t outSizeBytes = bufParamsOut.dim_y * bufParamsOut.stride_y;

         void *pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpSizeBytes);

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
         if (pIn && pOut && pOutCn) {

            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {
               TI_fillBuffer_float(prm[tpi].testPattern, 0, pIn, prm[tpi].staticIn, bufParamsIn.dim_x,
                                   bufParamsIn.dim_y, bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.dataType),
                                   testPatternString);
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_subBlockStatistics_d CP 0\n");

            status_init = AUDIOLIB_dB20_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_dB20_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_subBlockStatistics_d CP 2 status_init %d\n",
                                  status_init);

            status_opt = AUDIOLIB_dB20_exec_checkParams(handle, pIn, pOut);
#if AUDIOLIB_DEBUGPRINT
            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 3 status_opt %d\n", status_opt);
#endif
            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_dB20_exec(handle, pIn, pOut);
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
               status_opt = AUDIOLIB_dB20_exec(handle, pIn, pOut);
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
            status_opt = AUDIOLIB_dB20_exec(handle, pIn, pOut);

            AUDIOLIB_asm(" MARK 7");
            TI_profile_stop();

#endif // #if !defined(__C7X_HOSTEM__)
            /* Test _cn kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_dB20_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            status_nat = AUDIOLIB_dB20_exec(handle, pIn, pOutCn);
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {
               status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(10, -2),
                                                           bufParamsOut.dim_x, bufParamsOut.dim_y,
                                                           bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.dataType));
            }
            else {
               // TBD
            }
            comparisonDone = 1;
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 7 comparisonDone %d status_nat_vs_opt %d\n", comparisonDone,
                   status_nat_vs_opt);
#endif

            if (currPrm.staticOut != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
               bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

               if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {

                  status_nat_vs_ref = TI_compare_mem_2DDualStride_float(
                      (void *) pOutCn, (void *) currPrm.staticOut, 0.001, (double) powf(10, -2), bufParamsOut.dim_x,
                      bufParamsOut.dim_y, bufParamsOut.stride_y, bufParamsOutTemp.stride_y,
                      AUDIOLIB_sizeof(currPrm.dataType));

                  status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                      (void *) pOut, (void *) currPrm.staticOut, 0.001, (double) powf(10, -2), bufParamsOut.dim_x,
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

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 8 status_nat_vs_opt %d status_ref_vs_opt %d "
                   "currentTestFail %d\n",
                   status_nat_vs_opt, status_ref_vs_opt, currentTestFail);
#endif

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_nat_vs_ref == TI_TEST_KERNEL_FAIL) ||
                 (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) || (status_init != AUDIOLIB_SUCCESS) ||
                 (status_opt != AUDIOLIB_SUCCESS) || (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) ||
                 (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_dB20_d CP 8 fail %d\n", fail);
#endif
            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_dB20_perfEst(handle, &archCycles, &estCycles);

            sprintf(desc, "samples = %d channels=%d dataType = %d", currPrm.numSamples, currPrm.numChannels,
                    currPrm.dataType);

            // #if (!defined(EVM_TEST))

            fprintf(fpOutputCSV, "%d, %d, %d, %d, %ld, %d, %d\n", currPrm.testID, AUDIOLIB_sizeof(currPrm.dataType) * 8,
                    currPrm.numSamples, currPrm.numChannels, archCycles, pProfile[3 * tpi + 1], !currentTestFail);
            // #endif // #if (!defined(EVM_TEST))

            TI_profile_add_test(testNum++, (currPrm.numSamples * currPrm.numChannels), archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_subBlockStatistics_test
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
      TI_memError("AUDIOLIB_dB20");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_dB20_d(&pProfile[0], 0);
   }
}
int coverage_test_main()
{
   int32_t                testNum         = 1000;
   int32_t                currentTestFail = 0;
   AUDIOLIB_dB20_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_dB20_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;
   AUDIOLIB_STATUS status_exec_nat;
   AUDIOLIB_STATUS status_exec_opt;
   int32_t         status_nat_vs_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;

   int   fail = 0;
   void *pIn0, *pOut, *pOutCn;

   int32_t inSamples  = 1;
   int32_t inChannels = 4;

   bufParamsIn0.data_type = AUDIOLIB_FLOAT64;
   bufParamsIn0.dim_x     = inSamples;
   bufParamsIn0.dim_y     = inChannels;
   bufParamsIn0.stride_y  = bufParamsIn0.dim_x * AUDIOLIB_sizeof(bufParamsIn0.data_type);

   bufParamsOut.data_type = bufParamsIn0.data_type;
   bufParamsOut.dim_x     = inSamples;
   bufParamsOut.dim_y     = inChannels;
   bufParamsOut.stride_y  = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);

   uint64_t In0Size = bufParamsIn0.stride_y * bufParamsIn0.dim_y;
   uint64_t OutSize = bufParamsOut.stride_y * bufParamsOut.dim_y;

   AUDIOLIB_bufParams2D_t bufParamsIn0Temp, bufParamsOutTemp;

   pIn0   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, In0Size);
   pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, OutSize);
   pOutCn = (void *) malloc(OutSize);

   while (testNum <= 1007) {

      switch (testNum) {
      case 1000:
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_dB20_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_dB20_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;
      case 1001:

         bufParamsIn0Temp.data_type = AUDIOLIB_FLOAT32;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_dB20_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_dB20_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;
      case 1002:
         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT32;

         bufParamsIn0Temp.dim_x = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_dB20_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_dB20_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));

         break;
      case 1003:

         bufParamsIn0Temp.data_type = bufParamsIn0.data_type;
         bufParamsOutTemp.data_type = bufParamsOut.data_type;

         bufParamsIn0Temp.dim_x    = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x    = bufParamsOut.dim_x;
         bufParamsOutTemp.stride_y = bufParamsOut.stride_y;
         bufParamsIn0Temp.stride_y = bufParamsIn0.stride_y;

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         /* Sample Values */
         ((double *) pIn0)[0] = 3.402823466E+38 * 1.1; // Overflow value 3.402823466E+38f
         ((double *) pIn0)[1] = 4057.471923828125;
         ((double *) pIn0)[2] = 4112.8203125;
         ((double *) pIn0)[3] = 0.0; // underflow value

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_nat = AUDIOLIB_dB20_exec(handle, pIn0, pOutCn);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_opt = AUDIOLIB_dB20_exec(handle, pIn0, pOut);

         status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(10, -2),
                                                     bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y,
                                                     AUDIOLIB_sizeof(bufParamsOut.data_type));

         currentTestFail = ((status_nat_vs_opt == AUDIOLIB_SUCCESS) || (status_exec_opt != AUDIOLIB_SUCCESS) ||
                            (status_exec_nat != AUDIOLIB_SUCCESS));

         break;
      case 1004:

         bufParamsIn0Temp.data_type = AUDIOLIB_FLOAT32;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT32;

         bufParamsIn0Temp.dim_x    = bufParamsIn0.dim_x;
         bufParamsOutTemp.dim_x    = bufParamsOut.dim_x;
         bufParamsOutTemp.stride_y = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);
         bufParamsIn0Temp.stride_y = bufParamsIn0.dim_x * AUDIOLIB_sizeof(bufParamsIn0Temp.data_type);

         bufParamsIn0Temp.dim_y = bufParamsIn0.dim_y;
         bufParamsOutTemp.dim_y = bufParamsOut.dim_y;

         /* Sample Values */
         ((float *) pIn0)[0] = 1.7976931348623157e+308 * 1.1; // Overflow value
         ((float *) pIn0)[1] = 5962.165189341692;
         ((float *) pIn0)[2] = 9447.06615167546;
         ((float *) pIn0)[3] = 0.0f; // underflow value

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_nat = AUDIOLIB_dB20_exec(handle, pIn0, pOutCn);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_opt = AUDIOLIB_dB20_exec(handle, pIn0, pOut);

         status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(10, -2),
                                                     bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y,
                                                     AUDIOLIB_sizeof(bufParamsOut.data_type));

         currentTestFail = ((status_nat_vs_opt != AUDIOLIB_ERR_FAILURE) || (status_exec_opt != AUDIOLIB_SUCCESS) ||
                            (status_exec_nat != AUDIOLIB_SUCCESS));

         break;
      case 1005:

         bufParamsIn0Temp.data_type = AUDIOLIB_FLOAT64;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT64;

         bufParamsIn0Temp.dim_x    = 1;
         bufParamsOutTemp.dim_x    = 1;
         bufParamsOutTemp.stride_y = 1;
         bufParamsIn0Temp.stride_y = 1;

         bufParamsIn0Temp.dim_y = 1;
         bufParamsOutTemp.dim_y = 1;

         ((double *) pIn0)[0] = 0.0f; // underflow value

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_nat = AUDIOLIB_dB20_exec(handle, pIn0, pOutCn);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_opt = AUDIOLIB_dB20_exec(handle, pIn0, pOut);

         status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(10, -2),
                                                     bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y,
                                                     AUDIOLIB_sizeof(bufParamsOut.data_type));

         currentTestFail = ((status_nat_vs_opt != AUDIOLIB_ERR_FAILURE) || (status_exec_opt != AUDIOLIB_SUCCESS) ||
                            (status_exec_nat != AUDIOLIB_SUCCESS));

         break;
      case 1006:

         bufParamsIn0Temp.data_type = AUDIOLIB_FLOAT64;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT64;

         bufParamsIn0Temp.dim_x    = 1;
         bufParamsOutTemp.dim_x    = 1;
         bufParamsOutTemp.stride_y = 1;
         bufParamsIn0Temp.stride_y = 1;

         bufParamsIn0Temp.dim_y = 1;
         bufParamsOutTemp.dim_y = 1;

         ((double *) pIn0)[0] = 3.402823466E+38f * 1.1f; // Overflow value 3.402823466E+38f

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_nat = AUDIOLIB_dB20_exec(handle, pIn0, pOutCn);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_opt = AUDIOLIB_dB20_exec(handle, pIn0, pOut);

         status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.01, (double) powf(10, -2),
                                                     bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y,
                                                     AUDIOLIB_sizeof(bufParamsOut.data_type));

         currentTestFail = ((status_nat_vs_opt != AUDIOLIB_ERR_FAILURE) || (status_exec_opt != AUDIOLIB_SUCCESS) ||
                            (status_exec_nat != AUDIOLIB_SUCCESS));

         break;
      case 1007:

         bufParamsIn0Temp.data_type = AUDIOLIB_FLOAT64;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT64;

         bufParamsIn0Temp.dim_x    = 1;
         bufParamsOutTemp.dim_x    = 1;
         bufParamsOutTemp.stride_y = 1;
         bufParamsIn0Temp.stride_y = 1;

         bufParamsIn0Temp.dim_y = 1;
         bufParamsOutTemp.dim_y = 1;

         ((double *) pIn0)[0] = 0.8f;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_nat = AUDIOLIB_dB20_exec(handle, pIn0, pOutCn);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_dB20_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_opt = AUDIOLIB_dB20_exec(handle, pIn0, pOut);

         status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(10, -2),
                                                     bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y,
                                                     AUDIOLIB_sizeof(bufParamsOut.data_type));

         currentTestFail = ((status_nat_vs_opt != AUDIOLIB_ERR_FAILURE) || (status_exec_opt != AUDIOLIB_SUCCESS) ||
                            (status_exec_nat != AUDIOLIB_SUCCESS));

         break;
      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }

   free(pOutCn);
   TI_align_free(pOut);
   TI_align_free(pIn0);
   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_dB20_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_dB20_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_dB20_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

// /* Main call for individual test projects */
#if !defined(__ONESHOTTEST) && !defined(RTL_TEST)
int main()
{
   int fail = 1;

   uint32_t profile[256 * 3];

   AUDIOLIB_TEST_init();

   fail = test_main(&profile[0]);

#if !defined(NO_PRINTF)
   if (fail == 0)
      printf("Test Pass!\n");
   else
      printf("Test Fail!\n");

   int i;
   for (i = 0; i < test_cases; i++) {
      printf("Test %4d: Cold Cycles = %8d, Warm Cycles = %8d, Warm Cycles WRB = %8d\n", i, profile[3 * i],
             profile[3 * i + 1], profile[3 * i + 2]);
   }
#endif

   fail = coverage_test_main();
   if (fail == 0)
      printf("Test Pass!\n");
   else
      printf("Test Fail!\n");
   return fail;
}
#endif
