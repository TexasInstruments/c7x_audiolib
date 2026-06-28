// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_softClip_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_softClip_init_ci(AUDIOLIB_kernelHandle             handle,
                                          const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                          const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                          const AUDIOLIB_softClip_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_softClip_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_softClip_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_softClip_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d\n", pKerPrivArgs->dim_x,
                         pKerPrivArgs->dim_y, pKerPrivArgs->strideElements, eleCount);

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;

   // For input buffer without stride
   if (((int32_t) pKerPrivArgs->dim_x == pKerPrivArgs->inStride) &&
       ((int32_t) pKerPrivArgs->dim_x == pKerPrivArgs->outStride)) {
      uint32_t blockSize = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;

      se0Params.ICNT0  = blockSize;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0  = blockSize;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;

      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(blockSize, eleCount);
   }
   // For input buffer with stride
   else {
      se0Params.ICNT0  = pKerPrivArgs->dim_x; // number of samples
      se0Params.ICNT1  = pKerPrivArgs->dim_y; // number of channels
      se0Params.DIM1   = pKerPrivArgs->inStride;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = pKerPrivArgs->dim_x; // number of samples
      sa0Params.ICNT1  = pKerPrivArgs->dim_y; // number of channels
      sa0Params.DIM1   = pKerPrivArgs->outStride;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_softClip_init_ci<float>(AUDIOLIB_kernelHandle             handle,
                                                          const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                                          const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                                          const AUDIOLIB_softClip_InitArgs *pKerInitArgs);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_softClip_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_softClip_PrivArgs                            *pKerPrivArgs = (AUDIOLIB_softClip_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock    = pKerPrivArgs->bufPblock;
   float                                                  threshold = pKerPrivArgs->threshold;
   float                                                  endKnee   = pKerPrivArgs->endKnee;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_softClip_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);
   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   vec vecIn, vecInAbs, vecOut, vecOutNeg, vecResult;
   vec vecTemp0, vecTemp1, vecTemp2, vecTemp3;
   vec vecTwo       = (vec) 2.0f;
   vec vecZero      = (vec) 0.0f;
   vec vecThreshold = (vec) threshold;
   vec vecEndKnee   = (vec) endKnee;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   // unRolled loop by 2
   // Total cycles : 29 + trip_cnt * 11
   for (uint32_t i = 0; i < pKerPrivArgs->nVecs; i += 2) {
      vecIn    = c7x::strm_eng<0, vec>::get_adv();
      vecInAbs = __abs(vecIn);

      vecTemp0 = vecInAbs - vecThreshold;
      vecTemp1 = vecInAbs - (vecTwo * vecThreshold) + vecEndKnee;

      vecTemp2 = __recip(vecTemp1);
      vecTemp2 = vecTemp2 * (vecTwo - (vecTemp1 * vecTemp2));

      vecTemp3 = vecTemp2 * vecTemp0;

      vecOut    = (vecEndKnee - vecThreshold) * vecTemp3 + vecThreshold;
      vecOutNeg = -vecOut;

      __vpred cmpThreshold    = __cmp_lt_pred(vecInAbs, vecThreshold);
      __vpred cmpZero         = __cmp_lt_pred(vecIn, vecZero);
      __vpred cmpNotThreshold = __cmp_ge_pred(vecInAbs, vecThreshold);

      vecResult = __select(__and(cmpZero, cmpNotThreshold), vecOutNeg, vecOut);
      vecResult = __select(cmpThreshold, vecIn, vecResult);

      __vpred pred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred, addr, vecResult);

      vecIn    = c7x::strm_eng<0, vec>::get_adv();
      vecInAbs = __abs(vecIn);

      vecTemp0 = vecInAbs - vecThreshold;
      vecTemp1 = vecInAbs - (vecTwo * vecThreshold) + vecEndKnee;

      vecTemp2 = __recip(vecTemp1);
      vecTemp2 = vecTemp2 * (vecTwo - (vecTemp1 * vecTemp2));

      vecTemp3 = vecTemp2 * vecTemp0;

      vecOut    = (vecEndKnee - vecThreshold) * vecTemp3 + vecThreshold;
      vecOutNeg = -vecOut;

      cmpThreshold    = __cmp_lt_pred(vecInAbs, vecThreshold);
      cmpZero         = __cmp_lt_pred(vecIn, vecZero);
      cmpNotThreshold = __cmp_ge_pred(vecInAbs, vecThreshold);

      vecResult = __select(__and(cmpZero, cmpNotThreshold), vecOutNeg, vecOut);
      vecResult = __select(cmpThreshold, vecIn, vecResult);

      pred = c7x::strm_agen<0, vec>::get_vpred();
      addr = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred, addr, vecResult);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_softClip_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

void AUDIOLIB_softClip_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   uint64_t                    softClipStartupCycles   = 11;
   uint64_t                    softClipTeardownCycles  = 3;
   uint64_t                    softClipOperationCycles = 0;
   uint64_t                    softClipOverheadCycles  = 0;
   uint32_t                    nVecs;
   AUDIOLIB_softClip_PrivArgs *pKerPrivArgs = (AUDIOLIB_softClip_PrivArgs *) handle;

   nVecs                   = pKerPrivArgs->nVecs;
   softClipOperationCycles = (29 + (nVecs / 2) * 11);
   softClipOverheadCycles  = softClipStartupCycles + softClipTeardownCycles;
   *estCycles              = softClipOperationCycles + softClipOverheadCycles;
   *archCycles             = softClipOperationCycles;
}
