// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_inputAggregator_priv.h"

void AUDIOLIB_inputAggregator_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_inputAggregator_PrivArgs *pKerPrivArgs = (AUDIOLIB_inputAggregator_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t *restrict pIterCountLocal              = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   uint64_t inputAggregatorStartupCycles   = 0;
   uint64_t inputAggregatorTeardownCycles  = 0;
   uint64_t inputAggregatorOperationCycles = 0;
   uint64_t inputAggregatorOverheadCycles  = 0;

   if (!pKerPrivArgs->isInputInterleave && !pKerPrivArgs->isOutputInterleave) {
      inputAggregatorStartupCycles = 13 + 1;
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         uint32_t iterationCount = pIterCountLocal[i];
         inputAggregatorOperationCycles += 29 + 1 + iterationCount * 1;
         inputAggregatorTeardownCycles += 2 + 1;
      }
      inputAggregatorTeardownCycles += 1;
   }
   else {
      inputAggregatorStartupCycles = 9 + 1;
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         uint32_t iterationCount = pIterCountLocal[i];
         inputAggregatorOperationCycles += 29 + 1 + iterationCount * 1;
         inputAggregatorTeardownCycles += 4;
      }
   }

   inputAggregatorOverheadCycles += inputAggregatorStartupCycles + inputAggregatorTeardownCycles;
   *estCycles  = inputAggregatorOperationCycles + inputAggregatorOverheadCycles;
   *archCycles = inputAggregatorOperationCycles;
}

template <typename T> constexpr __SE_TRANSPOSE SETransposeConst();

template <> constexpr __SE_TRANSPOSE SETransposeConst<float>() { return __SE_TRANSPOSE_32BIT; };

template <> constexpr __SE_TRANSPOSE SETransposeConst<double>() { return __SE_TRANSPOSE_64BIT; };

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_inputAggregator_init_ci(AUDIOLIB_kernelHandle                    handle,
                                                 const AUDIOLIB_bufParams2D_t            *bufParamsIn,
                                                 const AUDIOLIB_bufParams2D_t            *bufParamsOut,
                                                 const AUDIOLIB_inputAggregator_InitArgs *pKerInitArgs)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_inputAggregator_PrivArgs                     *pKerPrivArgs = (AUDIOLIB_inputAggregator_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount     = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_inputAggregator_init_ci \n");

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se0Params[pKerPrivArgs->numInputs];
   __SA_TEMPLATE_v1 sa1Params[pKerPrivArgs->numInputs];

   __SE_TEMPLATE_v1 *restrict pSe0Params = se0Params;
   __SA_TEMPLATE_v1 *restrict pSa1Params = sa1Params;

   uint32_t *restrict pOutOffsetLocal = (uint32_t *) ((uint8_t *) pBlock + SE_OUTOFFSET_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   if (!pKerInitArgs->isInputInterleave && !pKerInitArgs->isOutputInterleave) {
      // /**********************************************************************/
      // /* Prepare streaming engine 0 to fetch input samples                  */
      // /**********************************************************************/
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         pSe0Params[i]         = __gen_SE_TEMPLATE_v1();
         pSe0Params[i].ICNT0   = pKerPrivArgs->numInputSamples[i];
         pSe0Params[i].ICNT1   = pKerPrivArgs->numInputChannels[i];
         pSe0Params[i].DIM1    = pKerPrivArgs->strideIn[i];
         pSe0Params[i].DIMFMT  = __SE_DIMFMT_2D;
         pSe0Params[i].ELETYPE = SE_ELETYPE;
         pSe0Params[i].VECLEN  = SE_VECLEN;

         pIterCountLocal[i] =
             (AUDIOLIB_ceilingDiv(pKerPrivArgs->numInputSamples[i], eleCount)) * pKerPrivArgs->numInputChannels[i];
      }

      // /********************************************************************* */
      // /* Prepare SA0 template to store output                                */
      // /********************************************************************* */
      sa0Params.ICNT0  = pKerPrivArgs->numOutputSamples;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->numOutputChannels;
      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;
   }

   if (pKerInitArgs->isInputInterleave && !pKerInitArgs->isOutputInterleave) {
      // Initialize a variable to track the cumulative offset before the loop
      uint32_t cumulativeOffset = 0;

      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         // /***********************************************************************/
         // /* Prepare streaming engine 0 to fetch input samples                   */
         // /***********************************************************************/
         pSe0Params[i] = __gen_SE_TEMPLATE_v1();
         pSe0Params[i].ICNT1 =
             pKerPrivArgs->numInputSamples[i] > eleCount ? eleCount : pKerPrivArgs->numInputSamples[i];
         pSe0Params[i].ICNT0     = pKerPrivArgs->numInputChannels[i];
         pSe0Params[i].DIM1      = pKerPrivArgs->strideIn[i];
         pSe0Params[i].DIM2      = eleCount * pKerPrivArgs->strideIn[i];
         pSe0Params[i].ICNT2     = AUDIOLIB_ceilingDiv(pKerPrivArgs->numInputSamples[i], eleCount);
         pSe0Params[i].DIMFMT    = __SE_DIMFMT_3D;
         pSe0Params[i].ELETYPE   = SE_ELETYPE;
         pSe0Params[i].VECLEN    = SE_VECLEN;
         pSe0Params[i].TRANSPOSE = SETransposeConst<dataType>();

         // /********************************************************************* */
         // /* Prepare SA1 template to store output                                */
         // /********************************************************************* */
         pSa1Params[i]               = __gen_SA_TEMPLATE_v1();
         pSa1Params[i].ICNT0         = eleCount;
         pSa1Params[i].DIM1          = pKerPrivArgs->strideOut;
         pSa1Params[i].ICNT1         = pKerPrivArgs->numInputChannels[i];
         pSa1Params[i].DIM2          = eleCount;
         pSa1Params[i].ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->numOutputSamples, eleCount);
         pSa1Params[i].VECLEN        = SA_VECLEN;
         pSa1Params[i].DECDIM1       = __SA_DECDIM_DIM2;
         pSa1Params[i].DECDIM1_WIDTH = pKerPrivArgs->numOutputSamples;
         pSa1Params[i].DIMFMT        = __SA_DIMFMT_3D;

         pOutOffsetLocal[i] = cumulativeOffset;
         cumulativeOffset += pKerPrivArgs->numInputChannels[i] * pKerPrivArgs->strideOut;
         pIterCountLocal[i] =
             (AUDIOLIB_ceilingDiv(pKerPrivArgs->numInputSamples[i], eleCount)) * pKerPrivArgs->numInputChannels[i];
      }
   }
   if (pKerInitArgs->isInputInterleave && pKerInitArgs->isOutputInterleave) {
      // Initialize a variable to track the cumulative offset before the loop
      uint32_t cumulativeOffset = 0;
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         // /**********************************************************************/
         // /* Prepare streaming engine 0 to fetch input samples                  */
         // /**********************************************************************/
         pSe0Params[i]         = __gen_SE_TEMPLATE_v1();
         pSe0Params[i].ICNT0   = pKerPrivArgs->numInputChannels[i];
         pSe0Params[i].ICNT1   = pKerPrivArgs->numInputSamples[i];
         pSe0Params[i].DIM1    = pKerPrivArgs->strideIn[i];
         pSe0Params[i].ELETYPE = SE_ELETYPE;
         pSe0Params[i].VECLEN  = SE_VECLEN;
         pSe0Params[i].DIMFMT  = __SE_DIMFMT_2D;

         // /********************************************************************* */
         // /* Prepare SA1 template to store output                                */
         // /********************************************************************* */
         pSa1Params[i]        = __gen_SA_TEMPLATE_v1();
         pSa1Params[i].ICNT0  = pKerPrivArgs->numInputChannels[i];
         pSa1Params[i].DIM1   = pKerPrivArgs->strideOut;
         pSa1Params[i].ICNT1  = pKerPrivArgs->numOutputSamples;
         pSa1Params[i].DIM2   = eleCount;
         pSa1Params[i].ICNT2  = AUDIOLIB_ceilingDiv(pKerPrivArgs->numOutputChannels, eleCount);
         pSa1Params[i].VECLEN = SA_VECLEN;
         pSa1Params[i].DIMFMT = __SA_DIMFMT_3D;

         pOutOffsetLocal[i] = cumulativeOffset;
         cumulativeOffset += pKerPrivArgs->numInputChannels[i];
         pIterCountLocal[i] =
             (AUDIOLIB_ceilingDiv(pKerPrivArgs->numInputChannels[i], eleCount)) * pKerPrivArgs->numInputSamples[i];
      }
   }
   if (!pKerInitArgs->isInputInterleave && pKerInitArgs->isOutputInterleave) {
      // Initialize a variable to track the cumulative offset before the loop
      uint32_t cumulativeOffset = 0;
      for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
         // /**********************************************************************/
         // /* Prepare streaming engine 0 to fetch input samples                  */
         // /**********************************************************************/
         pSe0Params[i] = __gen_SE_TEMPLATE_v1();
         pSe0Params[i].ICNT1 =
             pKerPrivArgs->numInputChannels[i] > eleCount ? eleCount : pKerPrivArgs->numInputChannels[i];
         pSe0Params[i].DIM1      = pKerPrivArgs->strideIn[i];
         pSe0Params[i].ICNT0     = pKerPrivArgs->numInputSamples[i];
         pSe0Params[i].DIM2      = pKerPrivArgs->numInputChannels[i] > eleCount
                                       ? eleCount * pKerPrivArgs->strideIn[i]
                                       : pKerPrivArgs->numInputChannels[i] * pKerPrivArgs->strideIn[i];
         pSe0Params[i].ICNT2     = AUDIOLIB_ceilingDiv(pKerPrivArgs->numInputChannels[i], eleCount);
         pSe0Params[i].DIMFMT    = __SE_DIMFMT_3D;
         pSe0Params[i].VECLEN    = SE_VECLEN;
         pSe0Params[i].ELETYPE   = SE_ELETYPE;
         pSe0Params[i].TRANSPOSE = SETransposeConst<dataType>();

         pIterCountLocal[i] =
             (AUDIOLIB_ceilingDiv(pKerPrivArgs->numInputChannels[i], eleCount)) * pKerPrivArgs->numInputSamples[i];
         pOutOffsetLocal[i] = cumulativeOffset;
         cumulativeOffset += pKerPrivArgs->numInputChannels[i];

         // /********************************************************************* */
         // /* Prepare SA0 template to store output                                */
         // /********************************************************************* */
         pSa1Params[i] = __gen_SA_TEMPLATE_v1();
         pSa1Params[i].ICNT0 =
             pKerPrivArgs->numInputChannels[i] < eleCount ? pKerPrivArgs->numInputChannels[i] : eleCount;
         pSa1Params[i].DIM1  = pKerPrivArgs->strideOut;
         pSa1Params[i].ICNT1 = pKerPrivArgs->numOutputSamples;
         pSa1Params[i].DIM2 =
             pKerPrivArgs->numInputChannels[i] > eleCount ? eleCount : pKerPrivArgs->numInputChannels[i];
         pSa1Params[i].ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->numOutputChannels, eleCount);
         pSa1Params[i].VECLEN        = SA_VECLEN;
         pSa1Params[i].DECDIM1       = __SA_DECDIM_DIM2;
         pSa1Params[i].DECDIM1_WIDTH = pKerPrivArgs->numInputChannels[i];
         pSa1Params[i].DIMFMT        = __SA_DIMFMT_3D;
      }
   }
   memcpy((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET, pSe0Params, sizeof(se0Params));
   memcpy((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET, pSa1Params, sizeof(sa1Params));

   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_inputAggregator_init_ci<float>(AUDIOLIB_kernelHandle                    handle,
                                                                 const AUDIOLIB_bufParams2D_t            *bufParamsIn,
                                                                 const AUDIOLIB_bufParams2D_t            *bufParamsOut,
                                                                 const AUDIOLIB_inputAggregator_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS
AUDIOLIB_inputAggregator_init_ci<double>(AUDIOLIB_kernelHandle                    handle,
                                         const AUDIOLIB_bufParams2D_t            *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t            *bufParamsOut,
                                         const AUDIOLIB_inputAggregator_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_inputAggregatorDeinterleaveToDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                           void **restrict pIn,
                                                                           void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_inputAggregator_PrivArgs                     *pKerPrivArgs = (AUDIOLIB_inputAggregator_PrivArgs *) handle;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;

   dataType *restrict pOutLocal = (dataType *) pOut;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_inputAggregatorDeinterleaveToDeinterleave_exec_ci");

   __SE_TEMPLATE_v1 *restrict se0Params = (__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params           = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal   = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   __SA0_OPEN(sa0Params);
   for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {

      dataType *restrict pInLocal = (dataType *) pIn[i];
      uint32_t iterationCount     = pIterCountLocal[i];

      __SE0_OPEN(pInLocal, se0Params[i]);
      for (uint32_t j = 0; j < iterationCount; j++) {
         vec inputSe0 = c7x::strm_eng<0, vec>::get_adv();

         // store outputSa0 to output buffer pOutLocal
         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, inputSe0);
      }
      __SE0_CLOSE();
   }
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_inputAggregatorDeinterleaveToDeinterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                           void **restrict pIn,
                                                                                           void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_inputAggregatorDeinterleaveToDeinterleave_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                   void **restrict pIn,
                                                                   void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_inputAggregatorGeneric_exec_ci(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_inputAggregator_PrivArgs                     *pKerPrivArgs = (AUDIOLIB_inputAggregator_PrivArgs *) handle;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;

   dataType *restrict pOutLocal  = (dataType *) pOut;
   dataType *restrict pOutLocal1 = (dataType *) pOut;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_inputAggregatorGeneric_exec_ci");

   __SE_TEMPLATE_v1 *restrict se0Params = (__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 *restrict sa1Params = (__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   uint32_t *restrict pOutOffsetLocal   = (uint32_t *) ((uint8_t *) pBlock + SE_OUTOFFSET_PARAM_OFFSET);
   uint32_t *restrict pIterCountLocal   = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
      dataType *restrict pInLocal = (dataType *) pIn[i];
      uint32_t offset             = pOutOffsetLocal[i];
      pOutLocal1                  = pOutLocal + offset;
      uint32_t iterationCount     = pIterCountLocal[i];

      __SA0_OPEN(sa1Params[i]);
      __SE0_OPEN(pInLocal, se0Params[i]);
      for (uint32_t j = 0; j < iterationCount; j++) {
         vec inputSe0 = c7x::strm_eng<0, vec>::get_adv();

         // store outputSa0 to output buffer pOutLocal
         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal1);
         __vstore_pred(tmpSa0, outPtrSa0, inputSe0);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_inputAggregatorGeneric_exec_ci<float>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_inputAggregatorGeneric_exec_ci<double>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);
