// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

// /*******************************************************************************
//  *
//  * INCLUDES
//  *
//  ******************************************************************************/

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_gainSmoothing_priv.h"

void AUDIOLIB_gainSmoothing_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs                 = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint32_t                         wBlocks                      = pKerPrivArgs->wBlocks;
   uint8_t                          fadeType                     = pKerPrivArgs->setArgs.fadeType;
   uint32_t                         numBlocks                    = pKerPrivArgs->numBlocks;
   int32_t                          bypassSmoothing              = pKerPrivArgs->bypassSmoothing;
   uint32_t                         gainSmoothingStartupCycles   = 0;
   uint32_t                         gainSmoothingOperationCycles = 0;
   uint32_t                         gainSmoothingTearDownCycles  = 0;
   uint32_t                         gainSmoothingOverheadCycles  = 0;

   switch (fadeType) {
   case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC:
      if (!bypassSmoothing) {
         gainSmoothingStartupCycles   = 20;
         gainSmoothingOperationCycles = (2 + (4 + wBlocks * 1) + 1) * numBlocks;
         gainSmoothingTearDownCycles  = 3 + 7;
      }
      else {
         gainSmoothingStartupCycles   = 22;
         gainSmoothingOperationCycles = 4 + (wBlocks * numBlocks) * 1;
         gainSmoothingTearDownCycles  = 2 + 9;
      }
      break;

   case AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH:
      if (!bypassSmoothing) {
         gainSmoothingStartupCycles   = 20 + 7;
         gainSmoothingOperationCycles = (4 + (4 + wBlocks * 1) + 4) * numBlocks;
         gainSmoothingTearDownCycles  = 3 + 2 + 7;
      }
      else {
         gainSmoothingStartupCycles   = 22;
         gainSmoothingOperationCycles = 4 + (wBlocks * numBlocks) * 1;
         gainSmoothingTearDownCycles  = 2 + 9;
      }
      break;

   default:
      return;
   }

   *archCycles += gainSmoothingStartupCycles + gainSmoothingOperationCycles + gainSmoothingTearDownCycles;
   *estCycles += gainSmoothingStartupCycles + gainSmoothingOperationCycles + gainSmoothingOverheadCycles +
                 gainSmoothingTearDownCycles;
}

// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_init_ci(AUDIOLIB_kernelHandle                  handle,
                                               const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothing_init_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                  status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs      = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock            = pKerPrivArgs->bufPblock;
   uint32_t                         numCols           = pKerPrivArgs->numCols;
   uint32_t                         eleCount          = c7x::element_count_of<vec>::value;
   uint32_t                         numRows           = pKerPrivArgs->numRows;
   uint8_t                          isInterleaved     = pKerInitArgs->isInterleaved;
   uint32_t                         strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t                         strideOutElements = pKerPrivArgs->strideOutElements;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   if (isInterleaved == 0) {
      if (numRows == 1) {
         // **********************************************************************/
         // /* Prepare streaming engine 0 to fetch input samples                  */
         // /**********************************************************************/
         se0Params.ICNT0   = numCols;
         se0Params.ELETYPE = SE_ELETYPE;
         se0Params.VECLEN  = SE_VECLEN;
         se0Params.DIMFMT  = __SE_DIMFMT_1D;

         /********************************************************************* */
         /* Prepare SA0 template to store output                                */
         /********************************************************************* */
         sa0Params.ICNT0  = numCols;
         sa0Params.VECLEN = SA_VECLEN;
         sa0Params.DIMFMT = __SA_DIMFMT_1D;
      }
      else {
         // /*********************************************************************************/
         // /* Prepare streaming engine 0 to fetch input samples from each channels */
         // /*********************************************************************************/
         se0Params.ICNT0         = eleCount;
         se0Params.ICNT1         = numRows;
         se0Params.DIM1          = strideInElements;
         se0Params.DIM2          = eleCount;
         se0Params.ICNT2         = AUDIOLIB_ceilingDiv(numCols, eleCount);
         se0Params.ELETYPE       = SE_ELETYPE;
         se0Params.VECLEN        = SE_VECLEN;
         se0Params.DIMFMT        = __SE_DIMFMT_3D;
         se0Params.DECDIM1       = __SE_DECDIM_DIM2;
         se0Params.DECDIM1_WIDTH = numCols;

         // /*********************************************************************************/
         // /* Prepare Streaming address generator 0 to store output from Streaming Engine 0 */
         // /*********************************************************************************/
         sa0Params.ICNT0         = eleCount;
         sa0Params.ICNT1         = numRows;
         sa0Params.DIM1          = strideOutElements;
         sa0Params.DIM2          = eleCount;
         sa0Params.ICNT2         = AUDIOLIB_ceilingDiv(numCols, eleCount);
         sa0Params.VECLEN        = SA_VECLEN;
         sa0Params.DIMFMT        = __SA_DIMFMT_3D;
         sa0Params.DECDIM1       = __SA_DECDIM_DIM2;
         sa0Params.DECDIM1_WIDTH = numCols;
      }
   }
   else // isInterleaved == 1
   {
      // /*********************************************************************************/
      // /* Prepare streaming engine 0 to fetch input samples from each samples  */
      // /*********************************************************************************/
      se0Params.ICNT0         = numCols < eleCount ? numCols : eleCount;
      se0Params.ICNT1         = AUDIOLIB_ceilingDiv(numCols, eleCount);
      se0Params.DIM1          = eleCount;
      se0Params.DIM2          = strideInElements;
      se0Params.ICNT2         = numRows;
      se0Params.VECLEN        = SE_VECLEN;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.ELETYPE       = SE_ELETYPE;
      se0Params.DECDIM1_WIDTH = numCols;

      // /*********************************************************************************/
      // /* Prepare Streaming address generator 0 to store output from Streaming Engine 0 */
      // /*********************************************************************************/
      sa0Params.ICNT0         = numCols < eleCount ? numCols : eleCount;
      sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(numCols, eleCount);
      sa0Params.DIM1          = eleCount;
      sa0Params.DIM2          = strideOutElements;
      sa0Params.ICNT2         = numRows;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.VECLEN        = SA_VECLEN;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = numCols;
   }
   // Store the prepared SE and SA templates in the pblock
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   // Pre-calculate the execution loop bounds. numBlocks is the number of gain updates
   // (one per sample); wBlocks is the number of vectors covered by each of them.
   if (isInterleaved) {
      pKerPrivArgs->numBlocks = numRows;
      pKerPrivArgs->wBlocks   = AUDIOLIB_ceilingDiv(numCols, eleCount);
   }
   else {
      pKerPrivArgs->numBlocks = AUDIOLIB_ceilingDiv(numCols, eleCount);
      pKerPrivArgs->wBlocks   = numRows;
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_init_ci<float>(AUDIOLIB_kernelHandle                  handle,
                                                               const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingBypass_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothingBypass_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                   = (dataType *) pIn;
   dataType *restrict pOutLocal                  = (dataType *) pOut;
   uint32_t numBlocks                            = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                              = pKerPrivArgs->wBlocks;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   const vec targetGainVec = (vec) (pKerPrivArgs->targetGain);

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   // 4 + trip_cnt * 1.
   for (size_t j = 0; j < numBlocks * wBlocks; j++) {
      vec     inVec     = c7x::strm_eng<0, vec>::get_adv();
      __vpred storePred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *pOutVec   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(storePred, pOutVec, inVec * targetGainVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   // Keep the running gain in step with the natural C variant, which also pins it here.
   pKerPrivArgs->currentGain = pKerPrivArgs->targetGain;

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingBypass_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType, bool fadeUp>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSymmetricFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothingSymmetricFade_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                   = (dataType *) pIn;
   dataType *restrict pOutLocal                  = (dataType *) pOut;
   uint32_t numCols                              = pKerPrivArgs->numCols;
   uint32_t numBlocks                            = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                              = pKerPrivArgs->wBlocks;
   uint32_t eleCount                             = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   const vec targetGainVec  = (vec) (pKerPrivArgs->targetGain);
   const vec initialGainVec = (vec) (pKerPrivArgs->currentGain);

   // Per-lane gain ratios for the first block, and the factor that advances them by
   // one full vector of samples per block.
   vec gainStepVec           = *stov_ptr(vec, &pKerPrivArgs->alphaCoeff[0]);
   vec gainStepMultiplierVec = vec(pKerPrivArgs->gainStepMultiplier);

   vec currentGainVec;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   // 4 + trip_cnt * 1
   for (uint32_t i = 0; i < numBlocks; i++) {
      currentGainVec = initialGainVec * gainStepVec;
      currentGainVec = AUDIOLIB_gainSmoothing_clamp<fadeUp>::apply(currentGainVec, targetGainVec);
      gainStepVec    = gainStepVec * gainStepMultiplierVec;

#pragma MUST_ITERATE(1, 4294967295, 1)
      for (size_t j = 0; j < wBlocks; j++) {
         vec     inVec     = c7x::strm_eng<0, vec>::get_adv();
         vec     outVec    = inVec * currentGainVec;
         __vpred storePred = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pOutVec   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(storePred, pOutVec, outVec);
      }
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   // Save the gain of the last sample so the next frame resumes the ramp from here.
   pKerPrivArgs->currentGain = __get_vector_element(currentGainVec, (numCols - 1) % eleCount);

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSymmetricFade_exec_ci<float, true>(AUDIOLIB_kernelHandle handle,
                                                                                  void *restrict pIn,
                                                                                  void *restrict pOut);
template AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSymmetricFade_exec_ci<float, false>(AUDIOLIB_kernelHandle handle,
                                                                                   void *restrict pIn,
                                                                                   void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_gainSmoothingSmoothFade_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_gainSmoothing_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainSmoothing_PrivArgs *) handle;
   uint8_t                         *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                   = (dataType *) pIn;
   dataType *restrict pOutLocal                  = (dataType *) pOut;
   uint32_t numCols                              = pKerPrivArgs->numCols;
   uint32_t numBlocks                            = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                              = pKerPrivArgs->wBlocks;
   uint32_t eleCount                             = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   const vec targetGainVec  = (vec) (pKerPrivArgs->targetGain);
   const vec initialGainVec = (vec) (pKerPrivArgs->currentGain);

   // Per-lane one-pole coefficients for the first block, and the scalar that advances
   // them by one full vector of samples per block.
   vec alphaVec      = *stov_ptr(vec, &pKerPrivArgs->alphaCoeff[0]);
   vec alphaPerBlock = vec(pKerPrivArgs->alphaMultiplier);
   vec ones          = vec(1);
   vec currentGainVec;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < numBlocks; i++) {
      currentGainVec = alphaVec * initialGainVec + targetGainVec * (ones - alphaVec);
      alphaVec       = alphaVec * alphaPerBlock;

#pragma MUST_ITERATE(1, 4294967295, 1)
      // 4 + trip_cnt * 1
      for (size_t j = 0; j < wBlocks; j++) {
         vec     inVec     = c7x::strm_eng<0, vec>::get_adv();
         vec     outVec    = inVec * currentGainVec;
         __vpred storePred = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pOutVec   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(storePred, pOutVec, outVec);
      }
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   pKerPrivArgs->currentGain = __get_vector_element(currentGainVec, (numCols - 1) % eleCount);

   return AUDIOLIB_SUCCESS;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
