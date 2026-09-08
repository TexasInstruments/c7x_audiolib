// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_panning_d.c
*******************************************************************************
*/

// include AUDIOLIB
#include <audiolib.h>
#include <stddef.h>
// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_panning_idat.h"
#include "AUDIOLIB_types.h"

#ifdef WIN32
int8_t ddrBuffer[2048 * 1024];
#else
__attribute__((section(".ddrData"), aligned(64))) int8_t ddrBuffer[2048 * 1024];

#endif // WIN32

int16_t volatile volatileSum = 0; // use volatile to keep compiler from removing this operation

int AUDIOLIB_panning_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                tpi;
   int32_t                currentTestFail;
   int32_t                fail = 0;
   uint32_t               repCount;
   uint32_t               numReps;
   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams2D_t bufParamsOut;
   AUDIOLIB_bufParams1D_t bufParamsGain;
   uint32_t               testNum;
   uint32_t               comparisonDone = 0;

   AUDIOLIB_panning_testParams_t *prm;
   AUDIOLIB_panning_testParams_t  currPrm;
   AUDIOLIB_panning_getTestParams(&prm, &test_cases);

   AUDIOLIB_panning_InitArgs kerInitArgs;
   AUDIOLIB_panning_SetArgs  kerSetArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 0\n");

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 1\n");

   TI_profile_init("AUDIOLIB_panning");
   // #if (!defined(EVM_TEST))
   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_panning.csv", "w+");
   fprintf(fpOutputCSV,
           "Test ID, Bit Width, inSamples, inChannels, frames, isInterleave, smoothing time, panning, sampling rate,"
           "Estimated cycles, EVM cycles, EVM/Est cycles, Pass/Fail\n");
   // #endif //  #if !defined(EVM_TEST)

   for (tpi = 0; tpi < test_cases; tpi++) {
      uint64_t archCycles = 0, estCycles = 0;
      numReps = prm[tpi].numReps;
      testNum = prm[tpi].testID;
      currPrm = prm[tpi];

      kerSetArgs.sourceAzimuth = currPrm.sourceAzimuth;
      kerSetArgs.samplingRate  = currPrm.samplingRate;
      // kerSetArgs.smoothingTime = currPrm.smoothingTime;
      kerInitArgs.isInterleave = currPrm.isInterleave;
      kerSetArgs.smoothingTime = currPrm.smoothingTime;
      kerInitArgs.numChannels  = currPrm.numChannels;
      int32_t numExecReps      = currPrm.frames;
      // int32_t speakerInfoBuffSize = AUDIOLIB_panning_getSpeakerInfoSize(&kerInitArgs);
      int32_t               handleSize = AUDIOLIB_panning_getHandleSize(&kerInitArgs);
      AUDIOLIB_kernelHandle handle     = malloc(handleSize);

      for (repCount = 0; repCount < numReps; repCount++) {
         AUDIOLIB_DEBUGPRINTFN(0, "Current TestID: %d Current Repetition: %d\n", currPrm.testID, repCount + 1);

         int32_t         status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
         int32_t         status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
         AUDIOLIB_STATUS status_init       = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_opt        = AUDIOLIB_SUCCESS;
         AUDIOLIB_STATUS status_nat        = AUDIOLIB_SUCCESS;

         currentTestFail = 0;

         bufParamsIn.data_type  = currPrm.sampleDataType;
         bufParamsOut.data_type = currPrm.sampleDataType;

         if (currPrm.isInterleave == 1) {
            bufParamsIn.dim_x    = 1;
            bufParamsIn.dim_y    = currPrm.numSamples;
            bufParamsIn.stride_y = currPrm.strideIn;

            bufParamsOut.dim_x    = currPrm.numChannels;
            bufParamsOut.dim_y    = currPrm.numSamples;
            bufParamsOut.stride_y = currPrm.strideOut;
         }
         else {
            bufParamsIn.dim_x    = currPrm.numSamples;
            bufParamsIn.dim_y    = 1;
            bufParamsIn.stride_y = currPrm.strideIn;

            bufParamsOut.dim_x    = currPrm.numSamples;
            bufParamsOut.dim_y    = currPrm.numChannels;
            bufParamsOut.stride_y = currPrm.strideOut;
         }
         bufParamsGain.data_type = currPrm.sampleDataType;
         bufParamsGain.dim_x     = currPrm.numChannels;

         kerInitArgs.channelConfig = (uint32_t *) malloc(currPrm.numChannels * sizeof(uint32_t));
         memcpy((uint32_t *) kerInitArgs.channelConfig, (uint32_t *) currPrm.channelConfig,
                currPrm.numChannels * sizeof(uint32_t));

         // Here, stride is in bytes
         uint32_t inputSizeBytes;
         uint32_t outputSizeBytes;
         uint32_t gainSizeBytes;
         // Calculate the size of the input , output and gain buffers in bytes
         inputSizeBytes  = bufParamsIn.dim_y * bufParamsIn.stride_y;
         outputSizeBytes = bufParamsOut.dim_y * bufParamsOut.stride_y; // speakerInfoBuffSize
         /* The kernel consumes the gains a whole vector at a time, so the buffer is
          * rounded up to a vector multiple as required by AUDIOLIB_panning_exec() */
         gainSizeBytes = AUDIOLIB_CALC_STRIDE(bufParamsGain.dim_x * AUDIOLIB_sizeof(bufParamsGain.data_type),
                                              AUDIOLIB_ALIGN_SHIFT_64BYTES);

         void *pIn   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inputSizeBytes * numExecReps);
         void *pGain = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, gainSizeBytes);

         if (pGain) {
            memset(pGain, 0, gainSizeBytes);
         }

         void *pOut, *pOutCn;
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outputSizeBytes * numExecReps);
            pOutCn = (void *) malloc(outputSizeBytes * numExecReps);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
            pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outputSizeBytes * numExecReps);
            pOutCn = (void *) ddrBuffer;
         }
         else {
            pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outputSizeBytes * numExecReps);
            pOutCn = (void *) ddrBuffer;
         }

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOut: %p pOutCn: %p\n", pIn, pOut, pOutCn);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn && pOut && pOutCn) {

            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32) {

               // Fill input buffers frame by frame
               int32_t frame = 0;
               for (frame = 0; frame < numExecReps; frame++) {

                  TI_fillBuffer_float(prm[tpi].testPattern, 0, (void *) ((int8_t *) pIn + frame * inputSizeBytes),
                                      (void *) ((float *) prm[tpi].staticIn + (frame * currPrm.numSamples * 1)),
                                      bufParamsIn.dim_x, bufParamsIn.dim_y, bufParamsIn.stride_y,
                                      AUDIOLIB_sizeof(currPrm.sampleDataType), testPatternString);
               }
            }
            else {
               // TBD for other precisions
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 0\n");

            status_init =
                AUDIOLIB_panning_init_checkParams(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 1 status_init %d\n", status_init);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init = AUDIOLIB_panning_init(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);
               AUDIOLIB_panning_set(handle, pGain, &kerSetArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 2 status_init %d\n", status_init);

            status_opt = AUDIOLIB_panning_exec_checkParams(handle, pIn, pGain, pOut);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 3 status_opt %d\n", status_opt);

            uint32_t currFrame = 0;

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");

               if (currFrame == currPrm.changeFrame - 1) {
                  kerSetArgs.sourceAzimuth = currPrm.nextAzimuth;
                  AUDIOLIB_panning_set(handle, pGain, &kerSetArgs);
               }
               currFrame++;
               status_opt = AUDIOLIB_panning_exec(handle, pIn, pGain, pOut);
               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();
            }

            /* The following for loop is to call kernel repeatedly so as to
             * train the branch predictor                                   */
            int32_t k;
            for (k = 0; k < (numExecReps - 2); k++) {

               // run warm instruction cache test
               if (currFrame == currPrm.changeFrame - 1) {
                  kerSetArgs.sourceAzimuth = currPrm.nextAzimuth;
                  AUDIOLIB_panning_set(handle, pGain, &kerSetArgs);
               }
               currFrame += 1;

               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
               AUDIOLIB_asm(" MARK 4");
               status_opt = AUDIOLIB_panning_exec(handle, (void *) ((int8_t *) pIn + (k + 1) * inputSizeBytes), pGain,
                                                  (void *) ((int8_t *) pOut + (k + 1) * outputSizeBytes));
               AUDIOLIB_asm(" MARK 5");
               TI_profile_stop();
            }

            int16_t outSum    = 0;
            int8_t *pOutTempL = (int8_t *) pOut;
            size_t  j         = 0;
            for (j = 0; j < bufParamsIn.dim_x * currPrm.numChannels * sizeof(currPrm.sampleDataType); j++) {
               outSum += *pOutTempL;
               pOutTempL++;
            }
            volatileSum = outSum;

            if (numExecReps != 1) {
               if (currFrame == currPrm.changeFrame - 1) {
                  kerSetArgs.sourceAzimuth = currPrm.nextAzimuth;
                  AUDIOLIB_panning_set(handle, pGain, &kerSetArgs);
               }
               // run warm instruction cache test
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
               AUDIOLIB_asm(" MARK 6");
               status_opt =
                   AUDIOLIB_panning_exec(handle, (void *) ((int8_t *) pIn + (numExecReps - 1) * inputSizeBytes), pGain,
                                         (void *) ((int8_t *) pOut + (numExecReps - 1) * outputSizeBytes));
               AUDIOLIB_asm(" MARK 7");
               TI_profile_stop();
            }

            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 4 status_opt %d\n", status_opt);

            // initialize the kernel to use the natural C variant
            AUDIOLIB_panning_init(handle, &bufParamsIn, &bufParamsGain, &bufParamsOut, &kerInitArgs);
            kerSetArgs.sourceAzimuth = currPrm.sourceAzimuth;
            AUDIOLIB_panning_set(handle, pGain, &kerSetArgs);
#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 5\n");
#endif
            unsigned int currFramecn = 0;

            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            for (k = 0; k < numExecReps; k++) {
               if (currFramecn == currPrm.changeFrame - 1) {
                  kerSetArgs.sourceAzimuth = currPrm.nextAzimuth;
                  AUDIOLIB_panning_set(handle, pGain, &kerSetArgs);
               }
               currFramecn++;
               status_nat = AUDIOLIB_panning_exec(handle, (void *) ((int8_t *) pIn + k * inputSizeBytes),
                                                  (void *) pGain, (void *) ((int8_t *) pOutCn + k * outputSizeBytes));
               // status_nat = AUDIOLIB_panning_exec(handle, (void *) pIn, (void *) pGain, (void *) pOutCn);
            }
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {
               // Initialize comparison statuses as PASS; fail if any frame fails
               status_nat_vs_opt = TI_TEST_KERNEL_PASS;
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;

               int32_t frame = 0;
               for (frame = 0; frame < numExecReps; frame++) {
                  // Compare optimized vs. natural C for this frame
                  int32_t frame_status_nat_vs_opt =
                      TI_compare_mem_2D_float((void *) ((int8_t *) pOut + frame * outputSizeBytes),
                                              (void *) ((int8_t *) pOutCn + frame * outputSizeBytes), 0,
                                              (double) powf(2, -10), bufParamsOut.dim_x, bufParamsOut.dim_y,
                                              bufParamsOut.stride_y, AUDIOLIB_sizeof(currPrm.sampleDataType));

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
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 7 comparisonDone %d "
                                  "status_nat_vs_opt %d \n",
                                  comparisonDone, status_nat_vs_opt);

            if (currPrm.staticOut != NULL) {
               AUDIOLIB_bufParams2D_t bufParamsOutTemp = bufParamsOut;
               bufParamsOutTemp.stride_y = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

               if (currPrm.sampleDataType == AUDIOLIB_FLOAT32 || currPrm.sampleDataType == AUDIOLIB_FLOAT64) {

                  // Initialize reference comparison statuses
                  status_ref_vs_opt = TI_TEST_KERNEL_PASS;

                  int32_t frame = 0;
                  // Frame-by-frame comparison for reference outputs
                  for (frame = 0; frame < numExecReps; frame++) {

                     int32_t frame_status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                         (void *) ((int8_t *) pOut + frame * outputSizeBytes),
                         (void *) ((float *) currPrm.staticOut + frame * currPrm.numSamples * currPrm.numChannels), 0,
                         (double) powf(2, -10), bufParamsOut.dim_x, bufParamsOut.dim_y, bufParamsOut.stride_y,
                         bufParamsOut.dim_x * AUDIOLIB_sizeof(currPrm.sampleDataType),
                         AUDIOLIB_sizeof(currPrm.sampleDataType));

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
            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 8 status_nat_vs_opt %d status_ref_vs_opt "
                                  "%d currentTestFail "
                                  "%d\n",
                                  status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            /* Set the 'fail' flag based on test vector comparison results */
            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_panning_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            // AUDIOLIB_panning_get(handle, &kerSetArgs);
            sprintf(desc, "%s generated input | inSamples = %d, inChannels = %d, azimuth = %0.3f, fs = %d Hz",
                    testPatternString, currPrm.numSamples, currPrm.numChannels, kerSetArgs.sourceAzimuth,
                    kerSetArgs.samplingRate);

            AUDIOLIB_panning_perfEst(handle, &archCycles, &estCycles);

            // write to CSV, must happen prior to write to screen because
            // TI_profile_formula_add clears values in counters
            fprintf(fpOutputCSV, "%d, %d, %d, %d , %d, %d ,%f, %f, %d, %d, %d, %f, %d\n", currPrm.testID,
                    AUDIOLIB_sizeof(currPrm.sampleDataType) * 8, currPrm.numSamples, currPrm.numChannels,
                    currPrm.frames, currPrm.isInterleave, currPrm.smoothingTime, currPrm.sourceAzimuth,
                    currPrm.samplingRate, (int32_t) estCycles, pProfile[3 * tpi + 1],
                    ((AUDIOLIB_F32) cycles[TI_PROFILE_KERNEL_OPT_WARM]) / ((AUDIOLIB_F32) estCycles), !currentTestFail);

            TI_profile_add_test(testNum++, currPrm.numSamples * currPrm.numChannels, archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);

            // clear the counters between runs; normally handled by TI_profile_panning_test
            TI_profile_clear_run_stats();

         } // end of memory allocation successful?

         /* Free buffers for each test vector */
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            free(pOutCn);
            TI_align_free(pOut);
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
            TI_align_free(pOut);
         }
         else {
            TI_align_free(pOut);
         }
         TI_align_free(pIn);
         free(kerInitArgs.channelConfig);
         TI_align_free(pGain);
         free(handle);

      } // end repetitions
   } // end idat test cases

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
      TI_memError("AUDIOLIB_panning");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_panning_d(&pProfile[0], 0);
   }
}

/* Runs init + set for one speaker layout with smoothing bypassed, so that the
 * buffer handed back holds the raw target gains. pGainOut must be at least one
 * vector (64 bytes) long, as AUDIOLIB_panning_exec requires. */
static AUDIOLIB_STATUS
AUDIOLIB_panning_covRunLayout(const int32_t *channelConfig, int32_t numChannels, float sourceAzimuth, float *pGainOut)
{
   AUDIOLIB_panning_InitArgs kerInitArgsCov;
   AUDIOLIB_panning_SetArgs  kerSetArgsLocal;
   AUDIOLIB_bufParams2D_t    bufParamsInCov, bufParamsOutCov;
   AUDIOLIB_bufParams1D_t    bufParamsGainCov;
   AUDIOLIB_STATUS           status;
   const int32_t             numSamples = 32;

   kerInitArgsCov.funcStyle     = AUDIOLIB_FUNCTION_OPTIMIZED;
   kerInitArgsCov.isInterleave  = 0;
   kerInitArgsCov.numChannels   = numChannels;
   kerInitArgsCov.channelConfig = (void *) channelConfig;

   bufParamsInCov.data_type = AUDIOLIB_FLOAT32;
   bufParamsInCov.dim_x     = numSamples;
   bufParamsInCov.dim_y     = 1;
   bufParamsInCov.stride_y  = numSamples * AUDIOLIB_sizeof(AUDIOLIB_FLOAT32);

   bufParamsOutCov.data_type = AUDIOLIB_FLOAT32;
   bufParamsOutCov.dim_x     = numSamples;
   bufParamsOutCov.dim_y     = numChannels;
   bufParamsOutCov.stride_y  = numSamples * AUDIOLIB_sizeof(AUDIOLIB_FLOAT32);

   bufParamsGainCov.data_type = AUDIOLIB_FLOAT32;
   bufParamsGainCov.dim_x     = numChannels;

   int32_t               handleSizeCov = AUDIOLIB_panning_getHandleSize(&kerInitArgsCov);
   AUDIOLIB_kernelHandle covHandle     = malloc(handleSizeCov);

   status = AUDIOLIB_panning_init_checkParams(covHandle, &bufParamsInCov, &bufParamsGainCov, &bufParamsOutCov,
                                              &kerInitArgsCov);
   if (status == AUDIOLIB_SUCCESS) {
      status = AUDIOLIB_panning_init(covHandle, &bufParamsInCov, &bufParamsGainCov, &bufParamsOutCov, &kerInitArgsCov);
   }
   if (status == AUDIOLIB_SUCCESS) {
      kerSetArgsLocal.sourceAzimuth = sourceAzimuth;
      kerSetArgsLocal.samplingRate  = 48000;
      kerSetArgsLocal.smoothingTime = 0.0f;
      status                        = AUDIOLIB_panning_set(covHandle, pGainOut, &kerSetArgsLocal);
   }

   free(covHandle);

   return status;
}

int coverage_test_main()
{
   int32_t                   testNum         = 1000;
   int32_t                   currentTestFail = 0;
   AUDIOLIB_panning_InitArgs kerInitArgs;

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;
   AUDIOLIB_STATUS status_init;

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;
   AUDIOLIB_bufParams1D_t bufParamsGain;
   int                    fail = 0;
   void                  *pIn0, *pOut, *pGain;

   int32_t channelConfigData[4] = {0, 1, 2, 3};
   kerInitArgs.isInterleave     = 0U;
   kerInitArgs.numChannels      = 2;
   kerInitArgs.channelConfig    = channelConfigData;

   /* Size the handle only after kerInitArgs is fully populated */
   int32_t               handleSize = AUDIOLIB_panning_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = 32;
   bufParamsIn.dim_y     = 8;
   bufParamsIn.stride_y  = bufParamsIn.dim_x * AUDIOLIB_sizeof(bufParamsIn.data_type);

   bufParamsOut.data_type = bufParamsIn.data_type;
   bufParamsOut.dim_x     = 32;
   bufParamsOut.dim_y     = 8;
   bufParamsOut.stride_y  = bufParamsOut.dim_x * AUDIOLIB_sizeof(bufParamsOut.data_type);

   bufParamsGain.data_type = AUDIOLIB_FLOAT32;
   bufParamsGain.dim_x     = 8;

   uint64_t In0Size = bufParamsIn.stride_y * bufParamsIn.dim_y;
   uint64_t OutSize = bufParamsOut.stride_y * bufParamsOut.dim_y;
   /* Rounded up to a vector multiple as required by AUDIOLIB_panning_exec() */
   uint64_t gainSize = AUDIOLIB_CALC_STRIDE(bufParamsGain.dim_x * AUDIOLIB_sizeof(bufParamsGain.data_type),
                                            AUDIOLIB_ALIGN_SHIFT_64BYTES);

   AUDIOLIB_bufParams2D_t   bufParamsInTemp, bufParamsOutTemp;
   AUDIOLIB_bufParams1D_t   bufParamsGainTemp;
   AUDIOLIB_panning_SetArgs kerSetArgsCov;
   pIn0  = (void *) malloc(In0Size);
   pOut  = (void *) malloc(OutSize);
   pGain = (void *) malloc(gainSize);

   while (testNum <= 1012) {
      status_nat  = AUDIOLIB_SUCCESS;
      status_opt  = AUDIOLIB_SUCCESS;
      status_init = AUDIOLIB_SUCCESS;
      /* Reset structs to baseline valid configuration before each test case.
       * The dimensions must be populated too, since init_checkParams validates
       * numChannels against the output dimensions and the gain buffer length. */
      bufParamsInTemp.data_type = AUDIOLIB_FLOAT32;
      bufParamsInTemp.dim_x     = 32;
      bufParamsInTemp.dim_y     = 1;
      bufParamsInTemp.stride_y  = bufParamsInTemp.dim_x * AUDIOLIB_sizeof(bufParamsInTemp.data_type);

      bufParamsOutTemp.data_type = AUDIOLIB_FLOAT32;
      bufParamsOutTemp.dim_x     = 32;
      bufParamsOutTemp.dim_y     = 2;
      bufParamsOutTemp.stride_y  = bufParamsOutTemp.dim_x * AUDIOLIB_sizeof(bufParamsOutTemp.data_type);

      bufParamsGainTemp.data_type = AUDIOLIB_FLOAT32;
      bufParamsGainTemp.dim_x     = 2;

      kerInitArgs.isInterleave  = 0U;
      kerInitArgs.numChannels   = 2;
      kerInitArgs.channelConfig = channelConfigData;

      switch (testNum) {
      /* Trigger (handle == NULL) */
      case 1000:
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_panning_init_checkParams(NULL, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt =
             AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp, NULL);
         status_init     = AUDIOLIB_panning_exec_checkParams(handle, NULL, pOut, pGain);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER) ||
                            (status_init != AUDIOLIB_ERR_NULL_POINTER))
                               ? 1
                               : 0;
         break;

      /* Trigger (bufParamsIn->data_type != AUDIOLIB_FLOAT32) */
      case 1001:
         bufParamsInTemp.data_type = AUDIOLIB_UINT32;
         kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_NATC;

         status_init = AUDIOLIB_panning_exec_checkParams(handle, pIn0, NULL, pGain);
         status_nat = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE) ||
                            (status_init != AUDIOLIB_ERR_NULL_POINTER))
                               ? 1
                               : 0;
         break;

      /*  Trigger (bufParamsIn->data_type != bufParamsOut->data_type) */
      case 1002:
         bufParamsOutTemp.data_type = AUDIOLIB_UINT32;
         kerInitArgs.funcStyle      = AUDIOLIB_FUNCTION_NATC;

         status_init = AUDIOLIB_panning_exec_checkParams(handle, pIn0, pOut, NULL);
         status_nat = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE) ||
                            (status_init != AUDIOLIB_ERR_NULL_POINTER))
                               ? 1
                               : 0;
         break;

         /* Trigger (pKerInitArgs->isInterleave > 1) */
         /* Trigger (pKerInitArgs->numChannels < 0) */

      case 1003:
         kerInitArgs.isInterleave = 2U;
         status_init              = AUDIOLIB_panning_exec_checkParams(NULL, pIn0, pOut, pGain);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         kerInitArgs.numChannels = -1;
         kerInitArgs.funcStyle   = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_VALUE) || (status_opt != AUDIOLIB_ERR_INVALID_VALUE) ||
                            (status_init != AUDIOLIB_ERR_NULL_POINTER))
                               ? 1
                               : 0;
         break;

         /* Trigger (pKerInitArgs->channelConfig == NULL) */

      case 1004:
         kerInitArgs.channelConfig = NULL;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER)) ? 1 : 0;
         break;

      /* Trigger (bufParams pointer == NULL) */
      case 1005:
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_panning_init_checkParams(handle, NULL, &bufParamsGainTemp, &bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;

         status_opt =
             AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, NULL, &kerInitArgs);
         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER)) ? 1 : 0;
         break;

      /* Trigger (bufParamsGain == NULL) */
      case 1006:
         kerInitArgs.numChannels = 0;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat =
             AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, NULL, &bufParamsOutTemp, &kerInitArgs);
         currentTestFail = (status_nat != AUDIOLIB_ERR_NULL_POINTER) ? 1 : 0;
         break;

      /* Trigger (bufParamsGain->data_type mismatch) */
      case 1007:
         bufParamsGainTemp.data_type = AUDIOLIB_UINT32;
         status_nat = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         currentTestFail = (status_nat != AUDIOLIB_ERR_INVALID_TYPE) ? 1 : 0;
         break;

      /* AUDIOLIB_panning_get() null and success paths */
      case 1008:
         status_nat = AUDIOLIB_panning_get(NULL, &kerSetArgsCov);
         status_opt = AUDIOLIB_panning_get(handle, NULL);

         currentTestFail =
             ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER)) ? 1 : 0;

         if (AUDIOLIB_panning_get(handle, &kerSetArgsCov) != AUDIOLIB_SUCCESS) {
            currentTestFail = 1;
         }
         break;

         /* Trigger (numChannels disagrees with the output dimensions) */
         /* Trigger (gain buffer shorter than numChannels) */

      case 1009:

         kerInitArgs.funcStyle   = AUDIOLIB_FUNCTION_NATC;
         kerInitArgs.numChannels = 4;
         bufParamsGainTemp.dim_x = 4;
         status_nat = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);

         kerInitArgs.funcStyle   = AUDIOLIB_FUNCTION_OPTIMIZED;
         bufParamsGainTemp.dim_x = 1;
         status_opt = AUDIOLIB_panning_init_checkParams(handle, &bufParamsInTemp, &bufParamsGainTemp, &bufParamsOutTemp,
                                                        &kerInitArgs);
         currentTestFail =
             (status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) || (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION) ? 1 : 0;
         break;

      /* co-located channels share one node and both receive its gain */
      case 1010: {
         const int32_t layoutCoLocated[3] = {AUDIOLIB_CHANNEL_FRONT_LEFT, AUDIOLIB_CHANNEL_FRONT_LEFT_WOOFER,
                                             AUDIOLIB_CHANNEL_FRONT_RIGHT};
         float         covGain[16];

         memset(covGain, 0, sizeof(covGain));
         status_nat = AUDIOLIB_panning_covRunLayout(layoutCoLocated, 3, 0.0f, covGain);

         currentTestFail = (status_nat != AUDIOLIB_SUCCESS) ? 1 : 0;
         if ((covGain[0] <= 0.0f) || (fabsf(covGain[0] - covGain[1]) > 1e-5f) ||
             (fabsf(covGain[0] - 0.707107f) > 1e-4f) || (fabsf(covGain[2] - 0.707107f) > 1e-4f)) {
            currentTestFail = 1;
         }
         break;
      }

      /*every channel co-located leaves one direction, which cannot pan */
      case 1011: {
         const int32_t layoutSingleNode[2] = {AUDIOLIB_CHANNEL_FRONT_LEFT, AUDIOLIB_CHANNEL_FRONT_LEFT_WOOFER};
         float         covGain[16];

         memset(covGain, 0, sizeof(covGain));
         status_nat      = AUDIOLIB_panning_covRunLayout(layoutSingleNode, 2, 0.0f, covGain);
         currentTestFail = (status_nat != AUDIOLIB_ERR_INVALID_DIMENSION) ? 1 : 0;
         break;
      }

      /* antipodal pair is degenerate; must fall back, never emit NaN */
      case 1012: {
         const int32_t layoutAntipodal[2] = {AUDIOLIB_CHANNEL_SIDE_LEFT, AUDIOLIB_CHANNEL_SIDE_RIGHT};
         float         covGain[16];

         memset(covGain, 0, sizeof(covGain));
         status_nat = AUDIOLIB_panning_covRunLayout(layoutAntipodal, 2, 0.0f, covGain);

         currentTestFail = (status_nat != AUDIOLIB_SUCCESS) ? 1 : 0;
         /* self-comparison detects NaN without depending on isnan() */
         if ((covGain[0] != covGain[0]) || (covGain[1] != covGain[1])) {
            currentTestFail = 1;
         }
         if ((fabsf(covGain[0] - 1.0f) > 1e-5f) || (fabsf(covGain[1]) > 1e-5f)) {
            currentTestFail = 1;
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
   free(pIn0);
   free(pOut);
   free(pGain);
   free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   /* For every call of AUDIOLIB_panning_getHandleSize() the execution history is pushed
      as this function is the anchor point for LDRA in .cpp kernel files.
      Therefore calling AUDIOLIB_panning_getHandleSize() to push the execution history
      at the end of coverage test cases. */
   int32_t handleSize_LDRA = AUDIOLIB_panning_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

/* Main call for inpanningidual test projects */
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

/* ======================================================================== */
/*  End of file:  AUDIOLIB_panning_d.c                          */
/* ======================================================================== */
