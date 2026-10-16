// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/*******************************************************************************
 *
 * INCLUDES
 *
 ******************************************************************************/

#include "AUDIOLIB_gainOffset_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)

void AUDIOLIB_gainOffset_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   const uint64_t gainOffsetOverheadCycles = 27U; /* SE/SA open, template load, close and return (EVM measured) */
   const AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs = (const AUDIOLIB_gainOffset_PrivArgs *) handle;

   /* Loop prolog/epilog of 7 cycles, then one vector per cycle (ii = 1) */
   uint64_t gainOffsetOperationCycles = 7U + (uint64_t) pKerPrivArgs->nVecs;

   *estCycles  = gainOffsetOperationCycles + gainOffsetOverheadCycles;
   *archCycles = gainOffsetOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainOffset_init_ci(AUDIOLIB_kernelHandle               handle,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                            const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainOffset_PrivArgs                          *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   const uint32_t                                         eleCount = (uint32_t) c7x::element_count_of<vec>::value;

   /* Dimensions and strides are taken from the handle, already filled by AUDIOLIB_gainOffset_init */
   (void) bufParamsIn;
   (void) bufParamsOut;
   (void) pKerInitArgs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dimX %u dimY %u strideIn %d strideOut %d eleCount %u\n", pKerPrivArgs->dimX,
                         pKerPrivArgs->dimY, pKerPrivArgs->strideIn, pKerPrivArgs->strideOut, eleCount);

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   se0Params.ELETYPE = c7x::se_eletype<vec>::value;
   se0Params.VECLEN  = c7x::se_veclen<vec>::value;
   sa0Params.VECLEN  = c7x::sa_veclen<vec>::value;

   /* Rows are contiguous in both buffers: stream the whole buffer as one 1D block */
   if (((int32_t) pKerPrivArgs->dimX == pKerPrivArgs->strideIn) &&
       ((int32_t) pKerPrivArgs->dimX == pKerPrivArgs->strideOut)) {
      uint32_t blockSize = pKerPrivArgs->dimY * pKerPrivArgs->dimX;

      se0Params.ICNT0  = blockSize;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0  = blockSize;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;

      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(blockSize, eleCount);
   }
   /* Padded rows: walk row by row, the tail of each row is masked by the SA predicate */
   else {
      se0Params.ICNT0  = pKerPrivArgs->dimX;
      se0Params.ICNT1  = pKerPrivArgs->dimY;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = pKerPrivArgs->dimX;
      sa0Params.ICNT1  = pKerPrivArgs->dimY;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      pKerPrivArgs->nVecs = pKerPrivArgs->dimY * AUDIOLIB_ceilingDiv(pKerPrivArgs->dimX, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) (pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) (pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_gainOffset_init_ci<float>(AUDIOLIB_kernelHandle               handle,
                                                            const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                                            const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                                            const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs);
template AUDIOLIB_STATUS AUDIOLIB_gainOffset_init_ci<double>(AUDIOLIB_kernelHandle               handle,
                                                             const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                                             const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                                             const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainOffset_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainOffset_PrivArgs                          *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   const uint8_t                                         *pBlock = pKerPrivArgs->bufPblock;
   const uint32_t                                         nVecs  = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_exec_ci nVecs %u\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(const __SE_TEMPLATE_v1 *) (pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(const __SA_TEMPLATE_v1 *) (pBlock + SE_SA0_PARAM_OFFSET);

   const dataType *restrict pInLocal = (const dataType *) pIn;
   dataType *restrict pOutLocal      = (dataType *) pOut;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   const vec gain   = (vec) ((dataType) pKerPrivArgs->gain);
   const vec offset = (vec) ((dataType) pKerPrivArgs->offset);

   /* 7 + nVecs * 1 */
   for (uint32_t i = 0U; i < nVecs; i++) {
      vec inVec  = c7x::strm_eng<0, vec>::get_adv();
      vec outVec = (gain * inVec) + offset;

      __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, outVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
