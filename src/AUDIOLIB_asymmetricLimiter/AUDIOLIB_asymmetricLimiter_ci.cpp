// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

// /*******************************************************************************
//  *
//  * INCLUDES
//  *
//  ******************************************************************************/

#include "AUDIOLIB_asymmetricLimiter_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)

void AUDIOLIB_asymmetricLimiter_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   uint64_t                asymmetricLimiterStartupCycles   = 30;
   uint64_t                asymmetricLimiterTeardownCycles  = 3;
   uint64_t                asymmetricLimiterOperationCycles = 0;
   uint64_t                asymmetricLimiterOverheadCycles  = 0;
   uint32_t                nVecs;
   AUDIOLIB_asymmetricLimiter_PrivArgs *pKerPrivArgs = (AUDIOLIB_asymmetricLimiter_PrivArgs *) handle;

   nVecs               = pKerPrivArgs->nVecs;
   asymmetricLimiterOperationCycles = (4 + nVecs * 1);
   asymmetricLimiterOverheadCycles  = asymmetricLimiterStartupCycles + asymmetricLimiterTeardownCycles;
   *estCycles          = asymmetricLimiterOperationCycles + asymmetricLimiterOverheadCycles;
   *archCycles         = asymmetricLimiterOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_init_ci(AUDIOLIB_kernelHandle         handle,
                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      const AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_asymmetricLimiter_PrivArgs                                *pKerPrivArgs = (AUDIOLIB_asymmetricLimiter_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_asymmetricLimiter_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideIn %d strideOut %d eleCount %d\n", pKerPrivArgs->dim_x,
                         pKerPrivArgs->dim_y, pKerPrivArgs->strideIn, pKerPrivArgs->strideOut, eleCount);

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   // /**********************************************************************/
   // /* Prepare SE/SA template                                             */
   // /**********************************************************************/
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;

   /* For input buffer without stride */
   if ((int32_t) pKerPrivArgs->dim_x == pKerPrivArgs->strideIn &&
       (int32_t) pKerPrivArgs->dim_x == pKerPrivArgs->strideOut) {
      uint32_t blockSize = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;

      se0Params.ICNT0  = blockSize;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0  = blockSize;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;

      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(blockSize, eleCount);
   }
   /* For input buffer with stride */
   else {
      se0Params.ICNT0  = pKerPrivArgs->dim_x;
      se0Params.ICNT1  = pKerPrivArgs->dim_y;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = pKerPrivArgs->dim_x;
      sa0Params.ICNT1  = pKerPrivArgs->dim_y;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_init_ci<float>(AUDIOLIB_kernelHandle         handle,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                      const AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs);
template AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_init_ci<double>(AUDIOLIB_kernelHandle         handle,
                                                       const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                       const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                       const AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_asymmetricLimiter_PrivArgs                                *pKerPrivArgs = (AUDIOLIB_asymmetricLimiter_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_asymmetricLimiter_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   vec asymmetricLimiterLower = (vec) (pKerPrivArgs->setArgs.asymmetricLimiterLower);
   vec asymmetricLimiterUpper = (vec) (pKerPrivArgs->setArgs.asymmetricLimiterUpper);

   for (size_t i = 0; i < pKerPrivArgs->nVecs; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      inVec = __max(inVec, asymmetricLimiterLower);

      vec outVec = __min(inVec, asymmetricLimiterUpper);

      __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, outVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
