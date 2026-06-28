// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_concat_priv.h"

void AUDIOLIB_concat_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_concat_PrivArgs *pKerPrivArgs   = (AUDIOLIB_concat_PrivArgs *) handle;
   uint8_t                  *pBlock         = pKerPrivArgs->bufPblock;
   uint32_t                  iterationCount = *(uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   uint64_t concatStartupCycles   = 13;
   uint64_t concatTeardownCycles  = 0;
   uint64_t concatOperationCycles = 0;
   uint64_t concatOverheadCycles  = 0;

   for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {
      concatOperationCycles += 26 + 2 + 1 + iterationCount * 1;
      concatTeardownCycles += 2 + 1;
   }
   concatTeardownCycles += 1;

   concatOverheadCycles += concatStartupCycles + concatTeardownCycles;
   *estCycles  = concatOperationCycles + concatOverheadCycles;
   *archCycles = concatOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_concat_init_ci(AUDIOLIB_kernelHandle           handle,
                                        const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                        const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                        const AUDIOLIB_concat_InitArgs *pKerInitArgs)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_concat_PrivArgs                              *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount     = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concat_init_ci \n");

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();

   uint32_t *restrict pIterCountLocal = (uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   if (!pKerInitArgs->isInterleave) {
      /**********************************************************************/
      /* Prepare streaming engine 0 to fetch input samples                  */
      /**********************************************************************/
      se0Params.ICNT0   = pKerPrivArgs->inSamples;
      se0Params.ICNT1   = pKerPrivArgs->inChannels;
      se0Params.DIM1    = pKerPrivArgs->strideIn;
      se0Params.DIMFMT  = __SE_DIMFMT_2D;
      se0Params.ELETYPE = SE_ELETYPE;
      se0Params.VECLEN  = SE_VECLEN;

      *pIterCountLocal = (AUDIOLIB_ceilingDiv(pKerPrivArgs->inSamples, eleCount)) * pKerPrivArgs->inChannels;

      /********************************************************************* */
      /* Prepare SA0 template to store output                                */
      /********************************************************************* */
      uint32_t totalOutChannels = pKerPrivArgs->inChannels * pKerPrivArgs->numInputs;
      sa0Params.ICNT0           = pKerPrivArgs->inSamples;
      sa0Params.DIM1            = pKerPrivArgs->strideOut;
      sa0Params.ICNT1           = totalOutChannels;
      sa0Params.VECLEN          = SA_VECLEN;
      sa0Params.DIMFMT          = __SA_DIMFMT_2D;
   }

   else {
      // Interleave: samples in dim_y, channels in dim_x
      /**********************************************************************/
      /* Prepare streaming engine 0 to fetch input samples                  */
      /**********************************************************************/
      se0Params.ICNT0   = pKerPrivArgs->inChannels;
      se0Params.ICNT1   = pKerPrivArgs->inSamples;
      se0Params.DIM1    = pKerPrivArgs->strideIn;
      se0Params.ELETYPE = SE_ELETYPE;
      se0Params.VECLEN  = SE_VECLEN;
      se0Params.DIMFMT  = __SE_DIMFMT_2D;

      /********************************************************************* */
      /* Prepare SA0 template to store output (3D for multiple inputs)       */
      /********************************************************************* */
      sa0Params.ICNT0  = pKerPrivArgs->inChannels;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->inSamples;
      sa0Params.DIM2   = pKerPrivArgs->inChannels;
      sa0Params.ICNT2  = pKerPrivArgs->numInputs;
      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.DIMFMT = __SA_DIMFMT_3D;

      // Store iteration count
      *pIterCountLocal = (AUDIOLIB_ceilingDiv(pKerPrivArgs->inChannels, eleCount)) * pKerPrivArgs->inSamples;
   }
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_concat_init_ci<float>(AUDIOLIB_kernelHandle           handle,
                                                        const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                                        const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                                        const AUDIOLIB_concat_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_concat_init_ci<double>(AUDIOLIB_kernelHandle           handle,
                                                         const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                                         const AUDIOLIB_concat_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_concat_exec_ci(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_concat_PrivArgs                              *pKerPrivArgs = (AUDIOLIB_concat_PrivArgs *) handle;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;

   dataType *restrict pOutLocal = (dataType *) pOut;
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_concat_exec_ci");

   __SA_TEMPLATE_v1 sa0Params      = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   uint32_t         iterationCount = *(uint32_t *) ((uint8_t *) pBlock + SE_ITERCOUNT_PARAM_OFFSET);

   __SE_TEMPLATE_v1 se0ParamsLocal = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);

   __SA0_OPEN(sa0Params);
   for (uint32_t i = 0; i < pKerPrivArgs->numInputs; i++) {

      dataType *restrict pInLocal = (dataType *) pIn[i];

      __SE0_OPEN(pInLocal, se0ParamsLocal);
      // 1 + iterationCount * 1
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

template AUDIOLIB_STATUS
AUDIOLIB_concat_exec_ci<float>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_concat_exec_ci<double>(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);
