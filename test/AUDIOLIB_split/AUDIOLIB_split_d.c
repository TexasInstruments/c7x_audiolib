// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include <audiolib.h>
#include <stddef.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "../../src/AUDIOLIB_split/AUDIOLIB_split.h"
#include "AUDIOLIB_split_idat.h"
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

int16_t volatile volatileSum = 0;

int AUDIOLIB_split_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering function");

   int32_t                 tpi;
   int32_t                 currentTestFail;
   int32_t                 fail = 0;
   uint32_t                repCount;
   uint32_t                numReps;
   AUDIOLIB_bufParams2D_t  bufParamsIn;
   AUDIOLIB_bufParams2D_t *bufParamsOut = NULL;
   uint64_t                archCycles   = 0;
   uint64_t                estCycles    = 0;

   uint32_t testNum;
   uint32_t comparisonDone = 0;

   AUDIOLIB_split_testParams_t *prm;
   AUDIOLIB_split_testParams_t  currPrm;
   AUDIOLIB_split_getTestParams(&prm, &test_cases);

   AUDIOLIB_split_InitArgs kerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 0\n");

   AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 1\n");

   TI_profile_init("AUDIOLIB_split");
   FILE *fpOutputCSV = fopen("AUDIOLIB_split.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, inOutputChannels, EVM Cycles, estCycles, Pass/Fail\n");

   for (tpi = 0; tpi < test_cases; tpi++) {
      numReps                       = prm[tpi].numReps;
      testNum                       = prm[tpi].testID;
      currPrm                       = prm[tpi];
      kerInitArgs.numOutputs        = currPrm.numOutputs;
      kerInitArgs.outChannels       = (uint32_t) (currPrm.totalInputChannels / currPrm.numOutputs);
      kerInitArgs.isInputInterleave = currPrm.isInputInterleave;

      int32_t               handleSize = AUDIOLIB_split_getHandleSize(&kerInitArgs);
      AUDIOLIB_kernelHandle handle     = malloc(handleSize);
      if (handle == NULL) {
         AUDIOLIB_DEBUGPRINTFN(0, "Memory allocation failed for handle\n");
         fail = 1;
         break;
      }
      if (currPrm.numOutputs > 0) {
         bufParamsOut = (AUDIOLIB_bufParams2D_t *) malloc(currPrm.numOutputs * sizeof(AUDIOLIB_bufParams2D_t));
         if (bufParamsOut == NULL) {
            AUDIOLIB_DEBUGPRINTFN(0, "Memory allocation failed for bufParamsOut\n");
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
         currentTestFail                   = 0;
         comparisonDone                    = 0;

         uint32_t i = 0;
         if (currPrm.isInputInterleave == 1) {
            // INTERLEAVED: dim_x = channels, dim_y = samples
            for (i = 0; i < currPrm.numOutputs; i++) {
               bufParamsOut[i].data_type = currPrm.dataType;
               bufParamsOut[i].dim_x     = (uint32_t) (currPrm.totalInputChannels / currPrm.numOutputs);
               bufParamsOut[i].dim_y     = currPrm.inSamples;
               bufParamsOut[i].stride_y  = bufParamsOut[i].dim_x * AUDIOLIB_sizeof(bufParamsOut[i].data_type);
            }

            bufParamsIn.data_type = currPrm.dataType;
            bufParamsIn.dim_x     = currPrm.totalInputChannels;
            bufParamsIn.dim_y     = currPrm.inSamples;
            bufParamsIn.stride_y  = bufParamsIn.dim_x * AUDIOLIB_sizeof(bufParamsIn.data_type);
         }
         else {
            // DEINTERLEAVED: dim_x = samples, dim_y = channels
            for (i = 0; i < currPrm.numOutputs; i++) {
               bufParamsOut[i].data_type = currPrm.dataType;
               bufParamsOut[i].dim_x     = currPrm.inSamples;
               bufParamsOut[i].dim_y     = (uint32_t) (currPrm.totalInputChannels / currPrm.numOutputs);
               bufParamsOut[i].stride_y  = bufParamsOut[i].dim_x * AUDIOLIB_sizeof(bufParamsOut[i].data_type);
            }

            bufParamsIn.data_type = currPrm.dataType;
            bufParamsIn.dim_x     = currPrm.inSamples;
            bufParamsIn.dim_y     = currPrm.totalInputChannels;
            bufParamsIn.stride_y  = bufParamsIn.dim_x * AUDIOLIB_sizeof(bufParamsIn.data_type);
         }

         void    *pIn         = NULL;
         uint32_t inSizeBytes = bufParamsIn.dim_y * bufParamsIn.stride_y;
         pIn                  = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inSizeBytes);
         if (pIn)
            memset(pIn, 0, inSizeBytes);

         void **pOutX = NULL;
         if (currPrm.numOutputs > 0) {
            pOutX = (void **) malloc(currPrm.numOutputs * sizeof(void *));
         }
         if (pOutX) {
#if !defined(__C7504__) && !defined(__C7524__)
            uint32_t msmcOffset = 0;
#endif
            for (i = 0; i < currPrm.numOutputs; i++) {
               uint32_t outSizeBytes = bufParamsOut[i].dim_y * bufParamsOut[i].stride_y;

               if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
                  pOutX[i] = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
               }
               else {
#if defined(__C7504__) || defined(__C7524__)
                  pOutX[i] = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
#else
                  pOutX[i] = (void *) ((uint8_t *) msmcBuffer + msmcOffset);
                  msmcOffset += outSizeBytes;
#endif
               }

               if (pOutX[i])
                  memset(pOutX[i], 0, outSizeBytes);
            }
         }

         void **pOutCnX = NULL;
         if (currPrm.numOutputs > 0) {
            pOutCnX = (void **) malloc(currPrm.numOutputs * sizeof(void *));
         }
         if (pOutCnX) {
            if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
               for (i = 0; i < currPrm.numOutputs; i++) {
                  uint32_t size = bufParamsOut[i].dim_y * bufParamsOut[i].stride_y;
                  pOutCnX[i]    = malloc(size);
                  if (pOutCnX[i])
                     memset(pOutCnX[i], 0, size);
               }
            }
            else {
               uint8_t *pOutCnBase = (uint8_t *) ddrBuffer;
               uint32_t offset     = 0;
               for (i = 0; i < currPrm.numOutputs; i++) {
                  uint32_t size = bufParamsOut[i].dim_y * bufParamsOut[i].stride_y;
                  pOutCnX[i]    = (void *) (pOutCnBase + offset);
                  offset += size;
               }
            }
         }

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOutX: %p pOutCnX: %p\n", pIn, pOutX, pOutCnX);

         if (pIn && pOutX && pOutCnX) {

            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {
               TI_fillBuffer_float(currPrm.testPattern, 0, pIn, currPrm.staticIn, bufParamsIn.dim_x, bufParamsIn.dim_y,
                                   bufParamsIn.stride_y, AUDIOLIB_sizeof(currPrm.dataType), testPatternString);
            }

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 0\n");

            status_init = AUDIOLIB_split_init_checkParams(handle, &bufParamsIn, bufParamsOut, &kerInitArgs);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 1 status_init %d\n", status_init);
            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               AUDIOLIB_asm(" MARK 0");
               kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
               status_init           = AUDIOLIB_split_init(handle, &bufParamsIn, bufParamsOut, &kerInitArgs);
               AUDIOLIB_asm(" MARK 1");
               TI_profile_stop();
            }
            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 2 status_init %d\n", status_init);

            status_opt = AUDIOLIB_split_exec_checkParams(handle, pIn, (const void **) pOutX);

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 3 status_opt %d\n", status_opt);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               AUDIOLIB_asm(" MARK 2");
               status_opt = AUDIOLIB_split_exec(handle, pIn, pOutX);
               AUDIOLIB_asm(" MARK 3");
               TI_profile_stop();
            }
#if !defined(__C7X_HOSTEM__)
            uint32_t k;
            for (k = 0; k < 4; k++) {
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
               AUDIOLIB_asm(" MARK 4");
               status_opt = AUDIOLIB_split_exec(handle, pIn, pOutX);
               AUDIOLIB_asm(" MARK 5");
               TI_profile_stop();
            }
            int16_t outSum   = 0;
            int8_t *pOutTemp = (int8_t *) pOutX[0];
            for (k = 0; k < bufParamsOut[0].dim_x; k++) {
               outSum += *pOutTemp;
               pOutTemp++;
            }

            volatileSum = outSum;

            TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_asm(" MARK 6");
            status_opt = AUDIOLIB_split_exec(handle, pIn, pOutX);
            AUDIOLIB_asm(" MARK 7");
            TI_profile_stop();

#endif
            kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 4 status_opt %d\n", status_opt);
            AUDIOLIB_split_init(handle, &bufParamsIn, bufParamsOut, &kerInitArgs);

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 5\n");
#endif
            TI_profile_start(TI_PROFILE_KERNEL_CN);
            AUDIOLIB_asm(" MARK 8");
            status_nat = AUDIOLIB_split_exec(handle, pIn, pOutCnX);
            AUDIOLIB_asm(" MARK 9");
            TI_profile_stop();

#if AUDIOLIB_DEBUGPRINT
            printf("AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 6 status_nat %d\n", status_nat);
#endif
            if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {
               status_nat_vs_opt = TI_TEST_KERNEL_PASS;
               for (i = 0; i < currPrm.numOutputs; i++) {
                  int32_t temp_status = TI_compare_mem_2D_float(
                      (void *) pOutX[i], (void *) pOutCnX[i], 0, (double) powf(2, -10), bufParamsOut[i].dim_x,
                      bufParamsOut[i].dim_y, bufParamsOut[i].stride_y, AUDIOLIB_sizeof(currPrm.dataType));
                  if (temp_status == TI_TEST_KERNEL_FAIL) {
                     status_nat_vs_opt = TI_TEST_KERNEL_FAIL;
                     break;
                  }
               }
            }
            else {
               status_nat_vs_opt = TI_TEST_KERNEL_PASS;
            }

            comparisonDone = 1;
            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 7 comparisonDone %d status_nat_vs_opt %d\n",
                                  comparisonDone, status_nat_vs_opt);

            if (currPrm.staticOut != NULL) {
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;
               for (i = 0; i < currPrm.numOutputs; i++) {
                  if (currPrm.dataType == AUDIOLIB_FLOAT32 || currPrm.dataType == AUDIOLIB_FLOAT64) {
                     int32_t temp_status = TI_compare_mem_2DDualStride_float(
                         (void *) pOutX[i], (void *) currPrm.staticOut[i], 0, (double) powf(2, -10),
                         bufParamsOut[i].dim_x, bufParamsOut[i].dim_y, bufParamsOut[i].stride_y,
                         bufParamsOut[i].dim_x * AUDIOLIB_sizeof(currPrm.dataType), AUDIOLIB_sizeof(currPrm.dataType));
                     if (temp_status == TI_TEST_KERNEL_FAIL) {
                        status_ref_vs_opt = TI_TEST_KERNEL_FAIL;
                        break;
                     }
                  }
               }
               comparisonDone = 1;
            }
            else {
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;
            }
            AUDIOLIB_DEBUGPRINTFN(0,
                                  "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 8 status_nat_vs_opt %d status_ref_vs_opt "
                                  "%d currentTestFail "
                                  "%d\n",
                                  status_nat_vs_opt, status_ref_vs_opt, currentTestFail);

            AUDIOLIB_DEBUGPRINTFN(
                0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 8 status_init %d status_opt %d status_nat %d\n",
                status_init, status_opt, status_nat);

            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) ||
                 (status_init != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS) ||
                 (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0) || (currentTestFail == 1))
                    ? 1
                    : 0;
            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            AUDIOLIB_DEBUGPRINTFN(0, "AUDIOLIB_DEBUGPRINT  AUDIOLIB_split_d CP 8 fail %d\n", fail);

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            sprintf(desc, "%s generated input | inSamples = %d, outChannels = %d", testPatternString, currPrm.inSamples,
                    currPrm.totalInputChannels);
            AUDIOLIB_split_perfEst(handle, &archCycles, &estCycles);
            fprintf(fpOutputCSV, "%d, %d, %d, %d, %d, %lu, %d\n", currPrm.testID, AUDIOLIB_sizeof(currPrm.dataType) * 8,
                    currPrm.inSamples, currPrm.totalInputChannels, pProfile[3 * tpi + 1], estCycles, !currentTestFail);

            TI_profile_add_test(testNum++, (currPrm.inSamples * currPrm.totalInputChannels), archCycles, estCycles,
                                currentTestFail, desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, outChannels = %d", testPatternString,
                    currPrm.inSamples, currPrm.totalInputChannels);
            TI_profile_skip_test(desc);

            TI_profile_clear_run_stats();
         }
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            if (pOutCnX) {
               for (i = 0; i < currPrm.numOutputs; i++) {
                  if (pOutCnX[i])
                     free(pOutCnX[i]);
               }
               free(pOutCnX);
            }
            if (pOutX) {
               for (i = 0; i < currPrm.numOutputs; i++) {
                  if (pOutX[i])
                     TI_align_free(pOutX[i]);
               }
               free(pOutX);
            }
         }
         else if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_MSMC) {
            if (pOutCnX)
               free(pOutCnX);
#if defined(__C7504__) || defined(__C7524__)
            if (pOutX) {
               for (i = 0; i < currPrm.numOutputs; i++) {
                  if (pOutX[i])
                     TI_align_free(pOutX[i]);
               }
               free(pOutX);
            }
#else
            if (pOutX)
               free(pOutX);
#endif
         }
         else {
            if (pOutCnX)
               free(pOutCnX);
#if defined(__C7504__) || defined(__C7524__)
            if (pOutX) {
               for (i = 0; i < currPrm.numOutputs; i++) {
                  if (pOutX[i])
                     TI_align_free(pOutX[i]);
               }
               free(pOutX);
            }
#else
            if (pOutX)
               free(pOutX);
#endif
         }
         TI_align_free(pIn);
      }
      if (bufParamsOut)
         free(bufParamsOut);
      free(handle);
   }

   fclose(fpOutputCSV);

   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_split");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_split_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                 testNum         = 1000;
   int32_t                 currentTestFail = 0;
   AUDIOLIB_split_InitArgs kerInitArgs;

   memset(&kerInitArgs, 0, sizeof(AUDIOLIB_split_InitArgs));
   kerInitArgs.numOutputs = 2;

   int32_t               handleSize = AUDIOLIB_split_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);
   if (handle == NULL) {
      return 1;
   }

   AUDIOLIB_STATUS status_nat;
   AUDIOLIB_STATUS status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn;
   AUDIOLIB_bufParams2D_t bufParamsOut[2];

   int fail = 0;

   int32_t inSamples   = 16;
   int32_t outChannels = 16;

   /* dim_x carries channels in the buffers below, so mark the input as interleaved and set the
    * per-output channel count consistently (total 16 / numOutputs 2 = 8) so the dimension checks
    * in init_checkParams pass for the type-validation cases. */
   kerInitArgs.isInputInterleave = 1;
   kerInitArgs.outChannels       = (uint32_t) (outChannels / 2);

   bufParamsIn.data_type = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x     = outChannels;
   bufParamsIn.dim_y     = inSamples;
   bufParamsIn.stride_y  = bufParamsIn.dim_x * AUDIOLIB_sizeof(bufParamsIn.data_type);

   bufParamsOut[0].data_type = AUDIOLIB_FLOAT32;
   bufParamsOut[0].dim_x     = outChannels / 2;
   bufParamsOut[0].dim_y     = inSamples;
   bufParamsOut[0].stride_y  = bufParamsOut[0].dim_x * AUDIOLIB_sizeof(bufParamsOut[0].data_type);

   bufParamsOut[1].data_type = AUDIOLIB_FLOAT32;
   bufParamsOut[1].dim_x     = outChannels / 2;
   bufParamsOut[1].dim_y     = inSamples;
   bufParamsOut[1].stride_y  = bufParamsOut[1].dim_x * AUDIOLIB_sizeof(bufParamsOut[1].data_type);

   AUDIOLIB_bufParams2D_t bufParamsInTemp;
   AUDIOLIB_bufParams2D_t bufParamsOutTemp[2];

   while (testNum <= 1010) {

      bufParamsInTemp     = bufParamsIn;
      bufParamsOutTemp[0] = bufParamsOut[0];
      bufParamsOutTemp[1] = bufParamsOut[1];

      switch (testNum) {
      case 1000:
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_split_init_checkParams(NULL, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_split_init_checkParams(NULL, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);

         currentTestFail = ((status_nat != AUDIOLIB_ERR_NULL_POINTER) || (status_opt != AUDIOLIB_ERR_NULL_POINTER));
         break;

      case 1001:
         bufParamsInTemp.data_type = AUDIOLIB_UINT32;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_split_init_checkParams(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_split_init_checkParams(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);

         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1002:
         bufParamsOutTemp[0].data_type = AUDIOLIB_UINT32;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_NATC;
         status_nat = AUDIOLIB_split_init_checkParams(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);

         bufParamsOutTemp[0].data_type = bufParamsIn.data_type;

         bufParamsOutTemp[1].data_type = AUDIOLIB_UINT32;

         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt = AUDIOLIB_split_init_checkParams(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);

         currentTestFail = ((status_nat != AUDIOLIB_ERR_INVALID_TYPE) || (status_opt != AUDIOLIB_ERR_INVALID_TYPE));
         break;

      case 1003:
         /* numOutputs == 0 -> AUDIOLIB_ERR_INVALID_VALUE */
         kerInitArgs.numOutputs = 0;
         status_opt = AUDIOLIB_split_init_checkParams(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);
         kerInitArgs.numOutputs = 2;
         currentTestFail        = (status_opt != AUDIOLIB_ERR_INVALID_VALUE);
         break;

      case 1004:
         /* input channels != numOutputs * outChannels -> AUDIOLIB_ERR_INVALID_DIMENSION */
         bufParamsInTemp.dim_x = 20; /* 20 != numOutputs(2) * outChannels(8) = 16 */
         status_opt      = AUDIOLIB_split_init_checkParams(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);
         currentTestFail = (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION);
         break;

      case 1005:
         /* one output's channel count != outChannels -> AUDIOLIB_ERR_INVALID_DIMENSION */
         bufParamsOutTemp[1].dim_x = 4; /* 4 != outChannels(8); input total still 16 = 2*8 */
         status_opt      = AUDIOLIB_split_init_checkParams(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);
         currentTestFail = (status_opt != AUDIOLIB_ERR_INVALID_DIMENSION);
         break;

      case 1006:
         /* AUDIOLIB_split_init with NULL handle -> AUDIOLIB_ERR_NULL_POINTER */
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_split_init(NULL, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);
         currentTestFail       = (status_opt != AUDIOLIB_ERR_NULL_POINTER);
         break;

      case 1007:
         /* AUDIOLIB_split_init NATC with unsupported type -> AUDIOLIB_ERR_INVALID_TYPE */
         bufParamsInTemp.data_type = AUDIOLIB_UINT32;
         kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_NATC;
         status_nat                = AUDIOLIB_split_init(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);
         currentTestFail           = (status_nat != AUDIOLIB_ERR_INVALID_TYPE);
         break;

      case 1008:
         /* AUDIOLIB_split_init OPTIMIZED with unsupported type -> AUDIOLIB_ERR_INVALID_TYPE */
         bufParamsInTemp.data_type = AUDIOLIB_UINT32;
         kerInitArgs.funcStyle     = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt                = AUDIOLIB_split_init(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs);
         currentTestFail           = (status_opt != AUDIOLIB_ERR_INVALID_TYPE);
         break;

      case 1009:
         /* AUDIOLIB_split_exec_checkParams with NULL input -> AUDIOLIB_ERR_NULL_POINTER */
         status_opt      = AUDIOLIB_split_exec_checkParams(handle, NULL, NULL);
         currentTestFail = (status_opt != AUDIOLIB_ERR_NULL_POINTER);
         break;

      case 1010: {
         /* AUDIOLIB_split_exec_checkParams with a NULL output element -> AUDIOLIB_ERR_NULL_POINTER */
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         (void) AUDIOLIB_split_init(handle, &bufParamsInTemp, bufParamsOutTemp, &kerInitArgs); /* sets numOutputs */
         const void *pOutArr[2] = {(const void *) &bufParamsIn, NULL};
         status_opt             = AUDIOLIB_split_exec_checkParams(handle, (const void *) &bufParamsIn, pOutArr);
         currentTestFail        = (status_opt != AUDIOLIB_ERR_NULL_POINTER);
         break;
      }

      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }

   if (handle)
      free(handle);

#if defined(ENABLE_LDRA_COVERAGE)
   int32_t handleSize_LDRA = AUDIOLIB_split_getHandleSize(&kerInitArgs);
   printf("!!! Pushing final execution history. handleSize_LDRA: %d\n", handleSize_LDRA);
#endif

   return fail;
}

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
