/*******************************************************************************
**+--------------------------------------------------------------------------+**
**|                            ****                                          |**
**|                            ****                                          |**
**|                            ******o***                                    |**
**|                      ********_///_****                                   |**
**|                      ***** /_//_/ ****                                   |**
**|                       ** ** (__/ ****                                    |**
**|                           *********                                      |**
**|                            ****                                          |**
**|                            ***                                           |**
**|                                                                          |**
**|        Copyright (c) 2025 Texas Instruments Incorporated                 |**
**|                                                                          |**
**|              All rights reserved not granted herein.                     |**
**|                         Limited License.                                 |**
**+--------------------------------------------------------------------------+**
*******************************************************************************/

// include AUDIOLIB
#include <audiolib.h>

// include test infrastructure provided by AUDIOLIB
#include "../common/AUDIOLIB_test.h"

// include test data for this kernel
#include "AUDIOLIB_types.h"
#include "AUDIOLIB_upSample_idat.h"

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


int AUDIOLIB_upSample_d(uint32_t *pProfile, uint8_t LevelOfFeedback)
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

   AUDIOLIB_upSample_testParams_t *prm;
   AUDIOLIB_upSample_testParams_t  currPrm;
   AUDIOLIB_upSample_getTestParams(&prm, &test_cases);

   AUDIOLIB_upSample_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_upSample_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   TI_profile_init("AUDIOLIB_upSample");

   // file IO for Loki benchmarking
   FILE *fpOutputCSV = fopen("AUDIOLIB_upSample.csv", "w+");
   fprintf(fpOutputCSV, "Test ID, Bit Width, inSamples, inChannels, upFactor, phase, isInterleave, Est Cycles"
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

         uint8_t  elemSize   = AUDIOLIB_sizeof(currPrm.dataType);
         uint32_t outSamples = currPrm.upFactor * currPrm.numSamples;

         // Buffer dimensions depend on the memory layout.  strideIn/strideOut
         // are BYTE strides taken straight from the idat table (tight for
         // interleaved, AUDIOLIB_CALC_STRIDE-aligned for planar).
         //   planar      : in (numChannels x numSamples)
         //                 out(numChannels x upFactor*numSamples)
         //   interleaved : in (numSamples  x numChannels)   [row = one sample's channels]
         //                 out(upFactor*numSamples x numChannels)
         bufParamsIn.data_type  = currPrm.dataType;
         bufParamsOut.data_type = currPrm.dataType;
         if (currPrm.isInterleave) {
            bufParamsIn.dim_x  = currPrm.numChannels;
            bufParamsIn.dim_y  = currPrm.numSamples;
            bufParamsOut.dim_x = currPrm.numChannels;
            bufParamsOut.dim_y = outSamples;
         }
         else {
            bufParamsIn.dim_x  = currPrm.numSamples;
            bufParamsIn.dim_y  = currPrm.numChannels;
            bufParamsOut.dim_x = outSamples;
            bufParamsOut.dim_y = currPrm.numChannels;
         }
         bufParamsIn.stride_y  = currPrm.strideIn;
         bufParamsOut.stride_y = currPrm.strideOut;

         uint32_t inpSizeBytes = bufParamsIn.dim_y * bufParamsIn.stride_y;
         uint32_t outSizeBytes =
             bufParamsOut.dim_y * bufParamsOut.stride_y ;

         void *pIn = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, inpSizeBytes);
         void *pOut, *pOutCn;
         if (currPrm.outputDataLocation == AUDIOLIB_TEST_OUTPUT_HEAP) {
            pOut   = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
            pOutCn = (void *) malloc(outSizeBytes);
         }
         else {
#if defined(__C7504__) || defined(__C7524__)
            pOut = (void *) TI_memalign(AUDIOLIB_L2DATA_ALIGNMENT, outSizeBytes);
#else
            pOut = (void *) msmcBuffer;
#endif
            pOutCn = (void *) ddrBuffer;
         }

         AUDIOLIB_DEBUGPRINTFN(0, "pIn: %p pOut: %p pOutCn: %p \n", pIn, pOut, pOutCn);

         /* Only run the test if the buffer allocations fit in the heap */
         if (pIn && pOut && pOutCn) {

            // STATIC pattern just copies staticIn -> pIn; the filler is element
            // size aware, so it serves float (4) and double (8) alike.
            TI_fillBuffer_float(prm[tpi].testPattern, 0, pIn, prm[tpi].staticIn, bufParamsIn.dim_x, bufParamsIn.dim_y,
                                bufParamsIn.stride_y, elemSize, testPatternString);

            // InitArgs must be populated BEFORE init_checkParams: it validates
            // upFactor / phase / isInterleave (range and output-dimension checks).
            kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;
            kerInitArgs.upFactor     = currPrm.upFactor;
            kerInitArgs.phase        = currPrm.phase;
            kerInitArgs.isInterleave = currPrm.isInterleave;

            status_init = AUDIOLIB_upSample_init_checkParams(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            if (status_init == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_INIT);
               status_init = AUDIOLIB_upSample_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);
               TI_profile_stop();
            }

            status_opt = AUDIOLIB_upSample_exec_checkParams(handle, pIn, pOut);

            if (status_opt == AUDIOLIB_SUCCESS) {
               TI_profile_start(TI_PROFILE_KERNEL_OPT);
               status_opt = AUDIOLIB_upSample_exec(handle, pIn, pOut);
               TI_profile_stop();
            }

#if !defined(__C7X_HOSTEM__)
            /* Call kernel repeatedly to train the branch predictor */
            uint32_t k;
            for (k = 0; k < 4; k++) {
               TI_profile_clear_cycle_count_single(TI_PROFILE_KERNEL_OPT_WARM);
               TI_profile_start(TI_PROFILE_KERNEL_OPT_WARM);
               status_opt = AUDIOLIB_upSample_exec(handle, pIn, pOut);
               TI_profile_stop();
            }

            // get output to L1D
            int16_t outSum   = 0;
            int8_t *pOutTemp = (int8_t *) pOut; // treat output as bytes to be data type agnostic
            for (k = 0; k < bufParamsOut.dim_x; k++) {
               outSum += *pOutTemp;
               pOutTemp++;
            }
            volatileSum = outSum; // dummy store so the compiler keeps the loop

            TI_profile_start(TI_PROFILE_KERNEL_OPT_WARMWRB);
            status_opt = AUDIOLIB_upSample_exec(handle, pIn, pOut);
            TI_profile_stop();
#endif // #if !defined(__C7X_HOSTEM__)

            /* Test _cn kernel */
            kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_NATC;
            kerInitArgs.upFactor     = currPrm.upFactor;
            kerInitArgs.phase        = currPrm.phase;
            kerInitArgs.isInterleave = currPrm.isInterleave;
            AUDIOLIB_upSample_init(handle, &bufParamsIn, &bufParamsOut, &kerInitArgs);

            TI_profile_start(TI_PROFILE_KERNEL_CN);
            status_nat = AUDIOLIB_upSample_exec(handle, pIn, pOutCn);
            TI_profile_stop();

            /* opt vs cn: both buffers use the (padded) output byte stride. */
            status_nat_vs_opt =
                TI_compare_mem_2D_float((void *) pOut, (void *) pOutCn, 0.001, (double) powf(4, -2), bufParamsOut.dim_x,
                                        bufParamsOut.dim_y, bufParamsOut.stride_y, elemSize);
            comparisonDone = 1;

            if (currPrm.staticOut != NULL) {
               // reference array is packed: stride = outSamples * elemSize
               uint32_t refStride = bufParamsOut.dim_x * elemSize;

               status_nat_vs_ref = TI_compare_mem_2DDualStride_float(
                   (void *) pOutCn, (void *) currPrm.staticOut, 0.001, (double) powf(4, -2), bufParamsOut.dim_x,
                   bufParamsOut.dim_y, bufParamsOut.stride_y, refStride, elemSize);

               status_ref_vs_opt = TI_compare_mem_2DDualStride_float(
                   (void *) pOut, (void *) currPrm.staticOut, 0.001, (double) powf(4, -2), bufParamsOut.dim_x,
                   bufParamsOut.dim_y, bufParamsOut.stride_y, refStride, elemSize);

               comparisonDone = 1;
            }
            else {
               status_ref_vs_opt = TI_TEST_KERNEL_PASS;
               status_nat_vs_ref = TI_TEST_KERNEL_PASS;
            }

            currentTestFail =
                ((status_nat_vs_opt == TI_TEST_KERNEL_FAIL) || (status_nat_vs_ref == TI_TEST_KERNEL_FAIL) ||
                 (status_ref_vs_opt == TI_TEST_KERNEL_FAIL) || (status_init != AUDIOLIB_SUCCESS) ||
                 (status_opt != AUDIOLIB_SUCCESS) || (status_nat != AUDIOLIB_SUCCESS) || (comparisonDone == 0))
                    ? 1
                    : 0;

            fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;

            pProfile[3 * tpi]     = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT);
            pProfile[3 * tpi + 1] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARM);
            pProfile[3 * tpi + 2] = (int32_t) TI_profile_get_cycles(TI_PROFILE_KERNEL_OPT_WARMWRB);

            AUDIOLIB_upSample_perfEst(handle, &archCycles, &estCycles);

            sprintf(desc, "samples=%d channels=%d upFactor=%d phase=%d interleave=%d dataType=%d", currPrm.numSamples,
                    currPrm.numChannels, currPrm.upFactor, currPrm.phase, currPrm.isInterleave, currPrm.dataType);

            fprintf(fpOutputCSV, "%d, %d, %d, %d, %d, %d, %d, %ld, %d, %d\n", currPrm.testID, elemSize * 8,
                    currPrm.numSamples, currPrm.numChannels, currPrm.upFactor, currPrm.phase, currPrm.isInterleave,
                    estCycles, pProfile[3 * tpi + 1], !currentTestFail);

            TI_profile_add_test(testNum++, (outSamples * currPrm.numChannels), archCycles, estCycles, currentTestFail,
                                desc);
         }
         else {
            sprintf(desc, "%s data does not fit in memory | inSamples = %d, inChannels = %d", testPatternString,
                    currPrm.numSamples, currPrm.numChannels);
            TI_profile_skip_test(desc);
            TI_profile_clear_run_stats();
         } // end of memory allocation successful?

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
      } // end repetitions
   } // end idat test cases

   free(handle);
   fclose(fpOutputCSV);

   return fail;
}

int test_main(uint32_t *pProfile)
{
#if !defined(_HOST_BUILD)
   if (TI_cache_init()) {
      TI_memError("AUDIOLIB_upSample");
      return 1;
   }
   else
#else
   printf("_HOST_BUILD is defined.\n");
#endif
   {
      return AUDIOLIB_upSample_d(&pProfile[0], 0);
   }
}

int coverage_test_main()
{
   int32_t                    testNum         = 1000;
   int32_t                    currentTestFail = 0;
   int32_t                    fail            = 0;
   AUDIOLIB_upSample_InitArgs kerInitArgs;

   int32_t               handleSize = AUDIOLIB_upSample_getHandleSize(&kerInitArgs);
   AUDIOLIB_kernelHandle handle     = malloc(handleSize);

   AUDIOLIB_STATUS status_nat, status_opt;

   AUDIOLIB_bufParams2D_t bufParamsIn, bufParamsOut;

   const int32_t inSamples  = 4;
   const int32_t inChannels = 2;
   const int32_t upFactor   = 3;

   bufParamsIn.data_type  = AUDIOLIB_FLOAT32;
   bufParamsIn.dim_x      = inSamples;
   bufParamsIn.dim_y      = inChannels;
   bufParamsIn.stride_y   = inSamples * AUDIOLIB_sizeof(AUDIOLIB_FLOAT32);
   bufParamsOut.data_type = AUDIOLIB_FLOAT32;
   bufParamsOut.dim_x     = upFactor * inSamples;
   bufParamsOut.dim_y     = inChannels;
   bufParamsOut.stride_y  = upFactor * inSamples * AUDIOLIB_sizeof(AUDIOLIB_FLOAT32);

   while (testNum <= 1007) {
      AUDIOLIB_bufParams2D_t in  = bufParamsIn;
      AUDIOLIB_bufParams2D_t out = bufParamsOut;
      kerInitArgs.funcStyle      = AUDIOLIB_FUNCTION_NATC;
      kerInitArgs.upFactor       = upFactor;
      kerInitArgs.phase          = 0;
      kerInitArgs.isInterleave   = 0;

      switch (testNum) {
      case 1000: // NULL handle -> NULL_POINTER
         status_nat = AUDIOLIB_upSample_init_checkParams(NULL, &in, &out, &kerInitArgs);
         currentTestFail = (status_nat != AUDIOLIB_ERR_NULL_POINTER);
         break;
      case 1001:
         status_nat      = AUDIOLIB_upSample_exec_checkParams(NULL, &in, &out);
         currentTestFail = (status_nat != AUDIOLIB_ERR_NULL_POINTER);
         break;
      case 1002: // unsupported data type -> INVALID_TYPE
         in.data_type    = AUDIOLIB_INT32;
         out.data_type   = AUDIOLIB_INT32;
         status_nat      = AUDIOLIB_upSample_init_checkParams(handle, &in, &out, &kerInitArgs);
         currentTestFail = (status_nat != AUDIOLIB_ERR_INVALID_TYPE);
         break;
      case 1003: // phase >= upFactor -> INVALID_VALUE
         kerInitArgs.phase = upFactor;
         status_nat        = AUDIOLIB_upSample_init_checkParams(handle, &in, &out, &kerInitArgs);
         currentTestFail   = (status_nat != AUDIOLIB_ERR_INVALID_VALUE);
         break;
      case 1004:                      // wrong output dim_x (!= upFactor*inSamples) -> INVALID_DIMENSION
         out.dim_x       = inSamples; // too small
         status_nat      = AUDIOLIB_upSample_init_checkParams(handle, &in, &out, &kerInitArgs);
         currentTestFail = (status_nat != AUDIOLIB_ERR_INVALID_DIMENSION);
         break;
      case 1005: // valid planar params (NATC and OPTIMIZED) -> SUCCESS
         status_nat            = AUDIOLIB_upSample_init_checkParams(handle, &in, &out, &kerInitArgs);
         kerInitArgs.funcStyle = AUDIOLIB_FUNCTION_OPTIMIZED;
         status_opt            = AUDIOLIB_upSample_init_checkParams(handle, &in, &out, &kerInitArgs);
         currentTestFail       = ((status_nat != AUDIOLIB_SUCCESS) || (status_opt != AUDIOLIB_SUCCESS));
         break;
      case 1007: // valid interleaved params -> SUCCESS
         // interleaved: in (inSamples x inChannels), out (upFactor*inSamples x inChannels)
         in.dim_x                 = inChannels;
         in.dim_y                 = inSamples;
         out.dim_x                = 1;
         out.dim_y                = 1;
         kerInitArgs.isInterleave = 1;
         kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;

         status_nat      = AUDIOLIB_upSample_init_checkParams(handle, &in, &out, &kerInitArgs);
         currentTestFail = (status_nat != AUDIOLIB_ERR_INVALID_DIMENSION);
         case 1006: // valid interleaved params -> SUCCESS
         // interleaved: in (inSamples x inChannels), out (upFactor*inSamples x inChannels)
         in.dim_x                 = inChannels;
         in.dim_y                 = inSamples;
         out.dim_x                = inChannels;
         out.dim_y                = 1;
         kerInitArgs.isInterleave = 1;
         kerInitArgs.funcStyle    = AUDIOLIB_FUNCTION_OPTIMIZED;

         status_nat      = AUDIOLIB_upSample_init_checkParams(handle, &in, &out, &kerInitArgs);
         currentTestFail = (status_nat != AUDIOLIB_ERR_INVALID_DIMENSION);

         break;
      default:
         break;
      }

      fail = ((fail == 1) || (currentTestFail == 1)) ? 1 : 0;
      sprintf(desc, "%s", "COVERAGE TEST");
      TI_profile_add_test(testNum++, 0, 0, 0, currentTestFail, desc);
   }

   free(handle);
   return fail;
}

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

   fail |= coverage_test_main();
   if (fail == 0)
      printf("All Test Pass!\n");
   else
      printf("Test Fail!\n");
   return fail;
}
#endif

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample_d.c                                      */
/* ======================================================================== */
