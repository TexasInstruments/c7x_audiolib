// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/* include AUDIOLIB */
#include <audiolib.h>
#include <stddef.h>

/* include test infrastructure provided by AUDIOLIB */
#include "../common/AUDIOLIB_test.h"

/* include test data for this kernel */
#include "AUDIOLIB_envelopeDetector_idat.h"
#include "AUDIOLIB_types.h"

#define AUDIOLIB_ENVELOPEDETECTOR_TEST_MAX_NUM_CHANNELS (64U)

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
#endif /* WIN32 */

int16_t volatile volatileSum = 0; /* use volatile to keep compiler from removing this operation */

int AUDIOLIB_envelopeDetector_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   uint32_t               frame;
   uint64_t               estCycles;
   uint64_t               archCycles;
   AUDIOLIB_bufParams2D_t bufParamsIn0, bufParamsOut;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_envelopeDetector_testParams_t *prm;
   AUDIOLIB_envelopeDetector_testParams_t  currPrm;
   AUDIOLIB_envelopeDetector_getTestParams(&prm, &test_cases);

   AUDIOLIB_envelopeDetector_InitArgs kerInitArgs;
   AUDIOLIB_envelopeDetector_SetArgs  kerSetArgs;

   int32_t               handleSize = AUDIOLIB_envelopeDetector_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_envelopeDetector_d CP 1\n");

   TI_profile_init("AUDIOLIB_envelopeDetector");

   /* file IO for Loki benchmarking */
   FILE *fpOutputCSV = fopen("AUDIOLIB_envelopeDetector.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, inChannels, numFrames, "
                        "EVM cycles, Est cycles, Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_PASS;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_PASS;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;
         estCycles                         = 0;
         archCycles                        = 0;
         currentTestFail                   = 0;
         comparisonDone                    = 0;

         bufParamsIn0.data_type = currPrm.dataType;
         bufParamsIn0.dim_x     = currPrm.inSamples;
         bufParamsIn0.dim_y     = currPrm.inChannels;
         bufParamsIn0.stride_y  = currPrm.strideInElements * AUDIOLIB_sizeof(currPrm.dataType);

         bufParamsOut.data_type = currPrm.dataType;
         bufParamsOut.dim_x     = currPrm.inSamples;
         bufParamsOut.dim_y     = currPrm.inChannels;
         bufParamsOut.stride_y  = currPrm.strideOutElements * AUDIOLIB_sizeof(currPrm.dataType);

         kerInitArgs.samplingRate = currPrm.samplingRate;

         kerSetArgs.attackTime  = currPrm.attackTime;
         kerSetArgs.releaseTime = currPrm.releaseTime;

         uint32_t elementSize    = AUDIOLIB_sizeof(currPrm.dataType);
         uint32_t inFrameStride  = bufParamsIn0.dim_y * bufParamsIn0.stride_y; /* bytes per padded input frame */
         uint32_t outFrameStride = bufParamsOut.dim_y * bufParamsOut.stride_y; /* bytes per padded output frame */
         uint32_t inp0SizeBytes  = inFrameStride * currPrm.numFrames;
         uint32_t outSizeBytes   = outFrameStride * currPrm.numFrames;
         uint32_t inFrameBytes   = currPrm.inChannels * currPrm.inSamples * elementSize; /* packed reference frame */

         void *pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inp0SizeBytes);

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
            uint32_t n;

            /* Fill every frame; the static reference input is stored packed, frame after frame */
            for (n = 0; n < currPrm.numFrames; n++) {
               void *pStaticIn =
                   (currPrm.staticIn0 != NULL) ? (void *) ((uint8_t *) currPrm.staticIn0 + (n * inFrameBytes)) : NULL;
               TI_fillBuffer_float(prm[tpi].testPattern, 0, (uint8_t *) pIn + n * inFrameStride, pStaticIn,
                                   bufParamsIn0.dim_x, bufParamsIn0.dim_y, bufParamsIn0.stride_y, elementSize,
                                   testPatternString);
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_envelopeDetector_d CP 0\n");
            status_init =
                AUDIOLIB_envelopeDetector_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_envelopeDetector_d CP 1 status_init %d\n",
                                  status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init = AUDIOLIB_envelopeDetector_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            if (status_init == AUDIOLIB_SUCCESS) {
               status_init = AUDIOLIB_envelopeDetector_set(handle, &kerSetArgs);
            }

            if (status_init == AUDIOLIB_SUCCESS) {
               status_opt = AUDIOLIB_envelopeDetector_exec_checkParams(handle, pIn, pOut);
            }

            if ((status_init == AUDIOLIB_SUCCESS) && (status_opt == AUDIOLIB_SUCCESS)) {
               // Execute first frame for cold cycle count
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_envelopeDetector_exec(handle, pIn, pOut);
               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();

               /* The following for loop is to call kernel repeatedly so as to
                * train the branch predictor                                   */
               // Execute frames 1 to (numFrames - 1) for warm instruction cache test
               for (frame = 1; frame < (currPrm.numFrames - 1); frame++) {
                  TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
                  TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
                  AUDIOLIB_asm(" MARK 4");
                  status_opt = AUDIOLIB_envelopeDetector_exec(handle, (uint8_t *) pIn + frame * inFrameStride,
                                                              (uint8_t *) pOut + frame * outFrameStride);
                  AUDIOLIB_asm(" MARK 5");
                  TI_profile_stop();
               }
            }

            /* run warm instruction cache test */
            // Execute final frame for WARMWRB instruction cache test
            TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
            AUDIOLIB_asm(" MARK 6");
            status_opt =
                AUDIOLIB_envelopeDetector_exec(handle, (uint8_t *) pIn + (currPrm.numFrames - 1) * inFrameStride,
                                               (uint8_t *) pOut + (currPrm.numFrames - 1) * outFrameStride);
            AUDIOLIB_asm(" MARK 7");
            TI_profile_stop();

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_envelopeDetector_d CP 4 status_opt %d\n",
                                  status_opt);

            /* natural C kernel */
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
            if ((status_init == AUDIOLIB_SUCCESS) && (status_opt == AUDIOLIB_SUCCESS)) {
               status_init = AUDIOLIB_envelopeDetector_init(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgs);
            }
            if (status_init == AUDIOLIB_SUCCESS) {
               status_init = AUDIOLIB_envelopeDetector_set(handle, &kerSetArgs);
            }

            for (frame = 0; frame < currPrm.numFrames; frame++) {
               TI_profile_start(TI_PROFILE_KERNEL_CN);
               AUDIOLIB_asm(" MARK 8");
               status_nat = AUDIOLIB_envelopeDetector_exec(handle, (uint8_t *) pIn + frame * inFrameStride,
                                                           (uint8_t *) pOutCn + frame * outFrameStride);
               AUDIOLIB_asm(" MARK 9");
               TI_profile_stop();
            }

            for (frame = 0; frame < currPrm.numFrames; frame++) {
               status_nat_vs_opt &= TI_compare_mem_2D_float(
                   (uint8_t *) pOut + frame * outFrameStride, (uint8_t *) pOutCn + frame * outFrameStride, 0.001,
                   (double) powf(2, -10), bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y, elementSize);

               if (currPrm.staticOut != NULL) {
                  /* Reference output is stored packed (no row padding), frame after frame */
                  void *pStaticOut = (void *) ((uint8_t *) currPrm.staticOut + (frame * inFrameBytes));
                  status_ref_vs_opt &= TI_compare_mem_2DDualStride_float(
                      (uint8_t *) pOut + frame * outFrameStride, pStaticOut, 0.001, (double) powf(2, -10),
                      bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y, bufParamsOut.dim_x * elementSize,
                      elementSize);
               }
            }

            AUDIOLIB_DEBUGPRINTFN(0, "frame %d status_nat_vs_opt %d status_ref_vs_opt %d\n", frame, status_nat_vs_opt,
                                  status_ref_vs_opt);

            comparisonDone = 1;

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_envelopeDetector_d status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_envelopeDetector_perfEst(handle, &archCycles, &estCycles);

            AUDIOLIB_envelopeDetector_get(handle, &kerSetArgs);

            sprintf(desc,
                    "%s generated input | inSamples = %d, inChannels = %d, numFrames = %d, attackTime = %f, "
                    "releaseTime = %f",
                    testPatternString, currPrm.inSamples, currPrm.inChannels, currPrm.numFrames, kerSetArgs.attackTime,
                    kerSetArgs.releaseTime);
            /* write to CSV, must happen prior to write to screen because
            TI_profile_formula_add clears values in counters */
            fprintf(fpOutputCSV, "%d, %d, %d, %d, %d, %d, %ld, %d\n", currPrm.testID, elementSize * 8,
                    currPrm.inSamples, currPrm.inChannels, currPrm.numFrames, pProfile[3 * tpi + 1], estCycles,
                    !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.inSamples * currPrm.inChannels), archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.inSamples, currPrm.inChannels);
            TI_profile_skip_test(desc);

            /* clear the counters between runs; normally handled by TI_profile_add_test */
            TI_profile_clear_run_stats();

         } /* end of memory allocation successful? */

         /* Free buffers for each test vector */
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            free(pOutCn);
            TI_align_free(pOut);
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            TI_align_free(pOut);
#endif
         }
         TI_align_free(pIn);

      } /* end repetitions */
   } /* end idat test cases */

   free(handle);
   /* Close results CSV */
   fclose(fpOutputCSV);

   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_envelopeDetector");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_envelopeDetector_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                            testNum         = 1000;
   int32_t                            currentTestFail = 0;
   AUDIOLIB_envelopeDetector_InitArgs kerInitArgs;
   AUDIOLIB_envelopeDetector_SetArgs  kerSetArgs;

   int32_t               handleSize = AUDIOLIB_envelopeDetector_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat;

   AUDIOLIB_bufParams2D_t bufParamsIn0;
   AUDIOLIB_bufParams2D_t bufParamsOut;

   int   fail = 0;
   void *pIn, *pOut;

   int32_t inSamples  = 16;
   int32_t inChannels = 4;

   bufParamsIn0.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn0.dim_x     = inSamples;
   bufParamsIn0.dim_y     = inChannels;
   bufParamsIn0.stride_y  = bufParamsIn0.dim_x * AUDIOLIB_sizeof(bufParamsIn0.data_type);

   bufParamsOut.data_type = bufParamsIn0.data_type;
   bufParamsOut.dim_x     = inSamples;
   bufParamsOut.dim_y     = inChannels;
   bufParamsOut.stride_y  = bufParamsIn0.dim_x * AUDIOLIB_sizeof(bufParamsIn0.data_type);

   kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgs.samplingRate = 48000;

   kerSetArgs.attackTime  = 0.010f;
   kerSetArgs.releaseTime = 0.100f;

   uint64_t In0Size = bufParamsIn0.stride_y * bufParamsIn0.dim_y;
   uint64_t OutSize = bufParamsOut.stride_y * bufParamsOut.dim_y;

   pIn  = (void *) malloc(In0Size);
   pOut = (void *) malloc(OutSize);

   while (testNum <= 1009) {
      AUDIOLIB_envelopeDetector_InitArgs kerInitArgsTemp = kerInitArgs;
      AUDIOLIB_envelopeDetector_SetArgs  kerSetArgsTemp  = kerSetArgs;

      switch (testNum) {
      case 1000:
         status_nat = AUDIOLIB_envelopeDetector_init_checkParams(NULL, &bufParamsIn0, &bufParamsOut, &kerInitArgsTemp);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) ? 1 : 0);
         break;
      case 1001:
         kerInitArgsTemp.samplingRate = 0;
         status_nat =
             AUDIOLIB_envelopeDetector_init_checkParams(handle, &bufParamsIn0, &bufParamsOut, &kerInitArgsTemp);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) ? 1 : 0);
         break;
      case 1002:
         status_nat      = AUDIOLIB_envelopeDetector_exec_checkParams(NULL, &bufParamsIn0, &bufParamsOut);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) ? 1 : 0);
         break;
      case 1003: {
         /* One more channel than the fixed-size envelope state can hold */
         AUDIOLIB_bufParams2D_t bufParamsInTemp  = bufParamsIn0;
         AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
         bufParamsInTemp.dim_y                   = AUDIOLIB_ENVELOPEDETECTOR_TEST_MAX_NUM_CHANNELS + 1U;
         bufParamsOutTemp.dim_y                  = AUDIOLIB_ENVELOPEDETECTOR_TEST_MAX_NUM_CHANNELS + 1U;
         status_nat =
             AUDIOLIB_envelopeDetector_init_checkParams(handle, &bufParamsInTemp, &bufParamsOutTemp, &kerInitArgsTemp);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) ? 1 : 0);
         break;
      }
      case 1004:
         kerSetArgsTemp.attackTime = 0.0f;
         status_nat                = AUDIOLIB_envelopeDetector_set(handle, &kerSetArgsTemp);
         currentTestFail           = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) ? 1 : 0);
         break;
      case 1005:
         kerSetArgsTemp.releaseTime = 0.0f;
         status_nat                 = AUDIOLIB_envelopeDetector_set(handle, &kerSetArgsTemp);
         currentTestFail            = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) ? 1 : 0);
         break;
      case 1006:
         status_nat      = AUDIOLIB_envelopeDetector_set(NULL, &kerSetArgsTemp);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) ? 1 : 0);
         break;
      case 1007:
         status_nat      = AUDIOLIB_envelopeDetector_set(handle, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) ? 1 : 0);
         break;
      case 1008:
         status_nat      = AUDIOLIB_envelopeDetector_get(NULL, &kerSetArgsTemp);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) ? 1 : 0);
         break;
      case 1009:
         status_nat      = AUDIOLIB_envelopeDetector_get(handle, NULL);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) ? 1 : 0);
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
   /* For every call of AUDIOLIB_envelopeDetector_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_envelopeDetector_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_envelopeDetector_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for in envelopeDetector test projects */
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
