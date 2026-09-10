// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <math.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_ceil_idat.h"
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

int AUDIOLIB_ceil_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
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

   AUDIOLIB_ceil_testParams_t *prm;
   AUDIOLIB_ceil_testParams_t  currPrm;
   AUDIOLIB_ceil_getTestParams(&prm, &test_cases);

   AUDIOLIB_ceil_InitArgs kerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_ceil_d CP 0\n");

   int32_t               handleSize = AUDIOLIB_ceil_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   TI_profile_init("AUDIOLIB_ceil");

   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_ceil.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, inChannels, Arch Cycles"
                        ",Warm cycles, Pass/Fail\n");

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
         bufParamsIn.stride_y  = currPrm.strideIn;

         bufParamsOut.data_type = currPrm.dataType;
         bufParamsOut.dim_x     = currPrm.numSamples;
         bufParamsOut.dim_y     = currPrm.numChannels;
         bufParamsOut.stride_y  = currPrm.strideOut;

         // Here, stride is in bytes
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

            status_init = AUDIOLIB_ceil_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_ceil_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_ceil_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            status_opt = AUDIOLIB_ceil_exec_checkParams(handle, pIn, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_ceil_d CP 2 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_ceil_exec(handle, pIn, pOut);
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
               status_opt = AUDIOLIB_ceil_exec(handle, pIn, pOut);
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
            status_opt = AUDIOLIB_ceil_exec(handle, pIn, pOut);

            AUDIOLIB_asm(" MARK 7");
            TI_profile_stop();

#endif // #if !defined(__C7X_HOSTEM__)
            /* Test _cn kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            // initialize the kernel to use the natural C variant
            AUDIOLIB_ceil_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            status_nat = AUDIOLIB_ceil_exec(handle, pIn, pOutCn);
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_ceil_d CP 3 status_nat %d\n", status_nat);

            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {
               status_nat_vs_opt = TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(10, -2),
                                                           bufParamsOut.dim_x, bufParamsOut.dim_y,
                                                           bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.dataType));
            }
            else {
               // TBD
            }
            comparisonDone = 1;

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
                      bufParamsOut.dim_y, bufParamsOut.stride_y, bufParamsOutTemp.stride_y,
                      AUDIOLIB_sizeof(currPrm.dataType));
               }
               else {
               }

               comparisonDone = 1;
            }
            else {
               /* Set to pass since it wasn't supposed to run. */
               status_nat_vs_ref = TI_TEST_KERNEL_PASS;
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;
            }

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_nat_vs_ref == TI_TEST_KERNEL_FAIL) ||
                 (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) || (status_init != AUDIOLIB_SUCCESS) ||
                 (status_opt != AUDIOLIB_SUCCESS) || (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) ||
                 (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_ceil_perfEst(handle, &archCycles, &estCycles);

            sprintf(desc, "samples = %d channels=%d dataType = %d", currPrm.numSamples, currPrm.numChannels,
                    currPrm.dataType);

            fprintf(fpOutputCSV, "%d, %d, %d, %d, %ld, %d, %d\n", currPrm.testID, AUDIOLIB_sizeof(currPrm.dataType) * 8,
                    currPrm.numSamples, currPrm.numChannels, archCycles, pProfile[3 * tpi + 1], !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.numSamples * currPrm.numChannels), archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_add_test
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
   /* Close results CSV */
   fclose(fpOutputCSV);

   return fail;
}

int test_main(uint32_t *pProfile)
{

#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_ceil");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_ceil_d(&pProfile[0], 0);
   }
}

/* Runs both kernel variants over a 1 x len buffer of the given data type and
   returns 1 if the optimized result differs from the natural C result, or if
   any call reports a failure. Used by the adversarial value cases below, which
   all share this shape and differ only in their inputs. */
static int32_t AUDIOLIB_ceil_compareVariants(AUDIOLIB_kernelHandle handle,
                                             uint32_t              dataType,
                                             uint32_t              len,
                                             void                 *pIn,
                                             void                 *pOut,
                                             void                 *pOutCn)
{
   AUDIOLIB_ceil_InitArgs kerInitArgs;
   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;
   AUDIOLIB_STATUS        status_nat, status_opt, status_exec_nat, status_exec_opt;
   int32_t                status_nat_vs_opt;

   bufParamsIn.data_type = dataType;
   bufParamsIn.dim_x     = len;
   bufParamsIn.dim_y     = 1;
   bufParamsIn.stride_y  = len * AUDIOLIB_sizeof(dataType);
   bufParamsOut          = bufParamsIn;

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
   status_nat            = AUDIOLIB_ceil_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   status_exec_nat       = AUDIOLIB_ceil_exec(handle, pIn, pOutCn);

   kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
   status_opt            = AUDIOLIB_ceil_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
   status_exec_opt       = AUDIOLIB_ceil_exec(handle, pIn, pOut);

   status_nat_vs_opt = TI_compare_mem_2D_float(pOut, pOutCn, 0.001, (double) powf(10, -2), bufParamsOut.dim_x,
                                               bufParamsOut.dim_y, bufParamsOut.stride_y, AUDIOLIB_sizeof(dataType));

   return ((status_nat_vs_opt != TI_TEST_KERNEL_PASS) || (status_nat != AUDIOLIB_SUCCESS) ||
           (status_opt != AUDIOLIB_SUCCESS) || (status_exec_nat != AUDIOLIB_SUCCESS) ||
           (status_exec_opt != AUDIOLIB_SUCCESS))
              ? 1
              : 0;
}

int coverage_test_main()
{
   int32_t                testNum         = 1000;
   int32_t                currentTestFail = 0;
   AUDIOLIB_ceil_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_ceil_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;
   AUDIOLIB_STATUS status_exec_nat;
   AUDIOLIB_STATUS status_exec_opt;
   int32_t         status_nat_vs_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;
   AUDIOLIB_bufParams2D_t bufParamsIn0Temp, bufParamsOutTemp;

   int      fail = 0;
   uint32_t k;
   void    *pIn0, *pOut, *pOutCn;

   int32_t inSamples  = 8;
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

   pIn0   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, In0Size);
   pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, OutSize);
   pOutCn = (void *) malloc(OutSize);

   while (testNum <= 1011) {

      switch (testNum) {
      case 1000:
         /* NULL handle, and NULL buffer parameter pointers, are rejected by
            init_checkParams */
         bufParamsIn0Temp = bufParamsIn0;
         bufParamsOutTemp = bufParamsOut;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_ceil_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_ceil_init_checkParams(NULL, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_exec_nat = AUDIOLIB_ceil_init_checkParams(handle, NULL, &bufParamsOutTemp, &kerInitArgs);
         status_exec_opt = AUDIOLIB_ceil_init_checkParams(handle, &bufParamsIn0Temp, NULL, &kerInitArgs);

         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER) ||
              (status_exec_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_exec_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1001:
         /* Unsupported input data type is rejected */
         bufParamsIn0Temp           = bufParamsIn0;
         bufParamsOutTemp           = bufParamsOut;
         bufParamsIn0Temp.data_type = AUDIOLIB_INT32;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_ceil_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_ceil_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1002:
         /* Mismatched input and output data types are rejected */
         bufParamsIn0Temp           = bufParamsIn0;
         bufParamsOutTemp           = bufParamsOut;
         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT32;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_ceil_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt      = AUDIOLIB_ceil_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1003:
         /* Mismatched input and output dimensions are rejected, and NULL
            handle or buffer pointers are rejected by exec_checkParams */
         bufParamsIn0Temp       = bufParamsIn0;
         bufParamsOutTemp       = bufParamsOut;
         bufParamsOutTemp.dim_x = bufParamsOut.dim_x + 1;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_ceil_init_checkParams(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);

         status_nat      = AUDIOLIB_ceil_exec_checkParams(NULL, pIn0, pOut);
         status_exec_nat = AUDIOLIB_ceil_exec_checkParams(handle, NULL, pOut);
         status_exec_opt = AUDIOLIB_ceil_exec_checkParams(handle, pIn0, NULL);

         currentTestFail =
             ((status_opt != AUDIOLIB_ERR_INVALID_DIMENSION) || (status_nat != AUDIOLIB_ERR_NULL_POINTER) ||
              (status_exec_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_exec_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1004:
         /* Single precision values straddling the 2^23 pass-through boundary,
            exercised through the strided (2D) streaming path */
         bufParamsIn0Temp.data_type = AUDIOLIB_FLOAT32;
         bufParamsIn0Temp.dim_x     = 4;
         bufParamsIn0Temp.dim_y     = 2;
         bufParamsIn0Temp.stride_y  = 8 * AUDIOLIB_sizeof(AUDIOLIB_FLOAT32);

         bufParamsOutTemp.data_type = AUDIOLIB_FLOAT32;
         bufParamsOutTemp.dim_x     = 4;
         bufParamsOutTemp.dim_y     = 2;
         bufParamsOutTemp.stride_y  = 8 * AUDIOLIB_sizeof(AUDIOLIB_FLOAT32);

         ((float *) pIn0)[0]  = -1.25f;
         ((float *) pIn0)[1]  = 1.75f;
         ((float *) pIn0)[2]  = 8388608.0f; // exactly 2^23
         ((float *) pIn0)[3]  = -8388608.0f;
         ((float *) pIn0)[8]  = 8388607.5f; // largest non-integral float
         ((float *) pIn0)[9]  = -0.0f;
         ((float *) pIn0)[10] = 16777217.0f; // above 2^24, already integral
         ((float *) pIn0)[11] = -1000.5f;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_ceil_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         status_exec_nat       = AUDIOLIB_ceil_exec(handle, pIn0, pOutCn);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_ceil_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &kerInitArgs);
         status_exec_opt       = AUDIOLIB_ceil_exec(handle, pIn0, pOut);

         status_nat_vs_opt = TI_compare_mem_2D_float(
             (void *) pOut, (void *) pOutCn, 0.001, (double) powf(10, -2), bufParamsOutTemp.dim_x,
             bufParamsOutTemp.dim_y, bufParamsOutTemp.stride_y, AUDIOLIB_sizeof(bufParamsOutTemp.data_type));

         currentTestFail = ((status_nat_vs_opt != TI_TEST_KERNEL_PASS) || (status_nat != AUDIOLIB_SUCCESS) ||
                            (status_opt != AUDIOLIB_SUCCESS) || (status_exec_opt != AUDIOLIB_SUCCESS) ||
                            (status_exec_nat != AUDIOLIB_SUCCESS));
         break;

      case 1005: {
         /* Halfway values, where round-to-nearest ties to even and so the
            predicated correction has to fire in alternating directions, plus
            signed zeros and exact integers that must pass through untouched */
         static const float in[] = {-4.5f, -3.5f, -2.5f, -1.5f, -0.5f, 0.5f,  1.5f, 2.5f,
                                    3.5f,  4.5f,  0.0f,  -0.0f, 1.0f,  -1.0f, 2.0f, -2.0f};
         uint32_t           len  = sizeof(in) / sizeof(in[0]);
         for (k = 0; k < len; k++) {
            ((float *) pIn0)[k] = in[k];
         }
         currentTestFail = AUDIOLIB_ceil_compareVariants(handle, AUDIOLIB_FLOAT32, len, pIn0, pOut, pOutCn);
         break;
      }

      case 1006: {
         /* Same halfway and tie cases in double precision */
         static const double in[] = {-4.5, -3.5, -2.5, -1.5, -0.5, 0.5,  1.5, 2.5,
                                     3.5,  4.5,  0.0,  -0.0, 1.0,  -1.0, 2.0, -2.0};
         uint32_t            len  = sizeof(in) / sizeof(in[0]);
         for (k = 0; k < len; k++) {
            ((double *) pIn0)[k] = in[k];
         }
         currentTestFail = AUDIOLIB_ceil_compareVariants(handle, AUDIOLIB_FLOAT64, len, pIn0, pOut, pOutCn);
         break;
      }

      case 1007: {
         /* Large single precision magnitudes, up to the largest value below the
            2^31 bound of the input range precondition */
         static const float in[] = {8388605.5f,    8388606.5f,     8388607.0f,    8388607.5f,
                                    8388608.0f,    8388609.0f,     16777215.0f,   16777216.0f,
                                    -8388607.5f,   -8388608.0f,    -8388609.0f,   -16777216.0f,
                                    1073741824.0f, -1073741824.0f, 2147483520.0f, -2147483520.0f};
         uint32_t           len  = sizeof(in) / sizeof(in[0]);
         for (k = 0; k < len; k++) {
            ((float *) pIn0)[k] = in[k];
         }
         currentTestFail = AUDIOLIB_ceil_compareVariants(handle, AUDIOLIB_FLOAT32, len, pIn0, pOut, pOutCn);
         break;
      }

      case 1008: {
         /* Large double precision magnitudes, up to the largest non-integral
            value below the 2^52 bound of the input range precondition */
         static const double in[] = {4503599627370493.5,
                                     4503599627370494.0,
                                     4503599627370495.0,
                                     4503599627370495.5,
                                     -4503599627370493.5,
                                     -4503599627370494.0,
                                     -4503599627370495.0,
                                     -4503599627370495.5,
                                     2251799813685248.0,
                                     -2251799813685248.0,
                                     1.0e15,
                                     -1.0e15,
                                     1.0e12,
                                     -1.0e12,
                                     123456789.5,
                                     -123456789.5};
         uint32_t            len  = sizeof(in) / sizeof(in[0]);
         for (k = 0; k < len; k++) {
            ((double *) pIn0)[k] = in[k];
         }
         currentTestFail = AUDIOLIB_ceil_compareVariants(handle, AUDIOLIB_FLOAT64, len, pIn0, pOut, pOutCn);
         break;
      }

      case 1009: {
         /* One ulp either side of exact integers and of 2^23, and the
            smallest normal float. Denormal inputs are excluded on purpose,
            see the note on denormals in AUDIOLIB_ceil.h. */
         float   *pf  = (float *) pIn0;
         uint32_t len = 8;

         pf[0] = nextafterf(1.0f, 0.0f);        // one ulp below 1
         pf[1] = nextafterf(1.0f, 2.0f);        // one ulp above 1
         pf[2] = nextafterf(-1.0f, 0.0f);       // one ulp above -1
         pf[3] = nextafterf(-1.0f, -2.0f);      // one ulp below -1
         pf[4] = nextafterf(8388608.0f, 0.0f);  // one ulp below 2^23
         pf[5] = nextafterf(-8388608.0f, 0.0f); // one ulp above -2^23
         pf[6] = 1.17549435e-38f;               // smallest normal float
         pf[7] = -1.17549435e-38f;

         currentTestFail = AUDIOLIB_ceil_compareVariants(handle, AUDIOLIB_FLOAT32, len, pIn0, pOut, pOutCn);
         break;
      }

      case 1010: {
         /* Double precision equivalent of the one ulp cases */
         double  *pd  = (double *) pIn0;
         uint32_t len = 8;

         pd[0] = nextafter(1.0, 0.0);
         pd[1] = nextafter(1.0, 2.0);
         pd[2] = nextafter(-1.0, 0.0);
         pd[3] = nextafter(-1.0, -2.0);
         pd[4] = nextafter(4503599627370496.0, 0.0);
         pd[5] = nextafter(-4503599627370496.0, 0.0);
         pd[6] = 2.2250738585072014e-308; // smallest normal double
         pd[7] = -2.2250738585072014e-308;

         currentTestFail = AUDIOLIB_ceil_compareVariants(handle, AUDIOLIB_FLOAT64, len, pIn0, pOut, pOutCn);
         break;
      }

      case 1011: {
         /* Double precision propagates Inf and NaN, because that path is native
            floating point throughout. Single precision converts through int32
            and cannot, which the input range precondition excludes.
            TI_compare_mem_2D_float reports any NaN as a failure, so the two
            variants are compared here with isnan directly. */
         AUDIOLIB_ceil_InitArgs advInitArgs;
         uint32_t               len = 4;

         bufParamsIn0Temp.data_type = AUDIOLIB_FLOAT64;
         bufParamsIn0Temp.dim_x     = len;
         bufParamsIn0Temp.dim_y     = 1;
         bufParamsIn0Temp.stride_y  = len * AUDIOLIB_sizeof(AUDIOLIB_FLOAT64);
         bufParamsOutTemp           = bufParamsIn0Temp;

         ((double *) pIn0)[0] = (double) NAN;
         ((double *) pIn0)[1] = -(double) NAN;
         ((double *) pIn0)[2] = (double) INFINITY;
         ((double *) pIn0)[3] = -(double) INFINITY;

         advInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat            = AUDIOLIB_ceil_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &advInitArgs);
         status_exec_nat       = AUDIOLIB_ceil_exec(handle, pIn0, pOutCn);

         advInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_ceil_init(handle, &bufParamsIn0Temp, &bufParamsOutTemp, &advInitArgs);
         status_exec_opt       = AUDIOLIB_ceil_exec(handle, pIn0, pOut);

         currentTestFail = ((status_nat != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                            (status_exec_nat != AUDIOLIB_SUCCESS) || (status_exec_opt != AUDIOLIB_SUCCESS))
                               ? 1
                               : 0;

         for (k = 0; k < len; k++) {
            double valOpt = ((double *) pOut)[k];
            double valNat = ((double *) pOutCn)[k];

            if (isnan(valNat)) {
               /* NaN in, NaN out */
               if (!isnan(valOpt)) {
                  currentTestFail = 1;
               }
            }
            else if (valOpt != valNat) {
               currentTestFail = 1;
            }
            else {
               /* Values agree */
            }
         }
         break;
      }

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
   /* For every call of AUDIOLIB_ceil_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_ceil_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_ceil_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

// /* Main call for individual test projects */
#if !defined(__ONESHOTTEST) && !defined(RTL_TEST)
int main()
{
   int test_fail = 1, cov_fail = 1;

   uint32_t profile[256 * 3];

   AUDIOLIB_TEST_init();

   test_fail = test_main(&profile[0]);

#if !defined(NO_PRINTF)
   int i;
   for (i = 0; i < test_cases; i++) {
      printf("Test %4d: Cold Cycles = %8d, Warm Cycles = %8d, Warm Cycles WRB = %8d\n", i, profile[3 * i],
             profile[3 * i + 1], profile[3 * i + 2]);
   }
#endif

   cov_fail = coverage_test_main();

   if ((test_fail == 0) && (cov_fail == 0))
      printf("All Test Pass!\n");
   else
      printf("Test Fail!\n");

   return test_fail || cov_fail;
}
#endif
