// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_tableLookup_priv.h"
// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableLookup_unroll_init_ci(AUDIOLIB_kernelHandle                handle,
                                                    const AUDIOLIB_bufParams1D_t        *bufParamsIn0,
                                                    const AUDIOLIB_bufParams1D_t        *bufParamsIn1,
                                                    const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                                    const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;
   uint8_t                       *pBlock       = pKerPrivArgs->bufPblock;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

   se0Params.ICNT0   = pKerPrivArgs->srcSamples;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_1D;

   sa0Params.ICNT0  = 1;
   sa0Params.DIM1   = 2;
   sa0Params.ICNT1  = AUDIOLIB_ceilingDiv(pKerPrivArgs->srcSamples, 2);
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->nVecs = pKerPrivArgs->srcSamples;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_tableLookup_unroll_init_ci<float>(AUDIOLIB_kernelHandle                handle,
                                                                    const AUDIOLIB_bufParams1D_t        *bufParamsIn0,
                                                                    const AUDIOLIB_bufParams1D_t        *bufParamsIn1,
                                                                    const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                                                    const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_tableLookup_unroll_init_ci<double>(AUDIOLIB_kernelHandle                handle,
                                                                     const AUDIOLIB_bufParams1D_t        *bufParamsIn0,
                                                                     const AUDIOLIB_bufParams1D_t        *bufParamsIn1,
                                                                     const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                                                     const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableLookup_unroll_exec_ci(AUDIOLIB_kernelHandle handle,
                                                    void *restrict pIn0,
                                                    void *restrict pIn1,
                                                    void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;
   uint8_t                       *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                       nVecs        = pKerPrivArgs->nVecs;
   float                          minVal       = pKerPrivArgs->minVal;
   float                          divisor      = pKerPrivArgs->divisor;
   uint32_t                       tableSamples = pKerPrivArgs->tableSamples;

   uint32_t eleCount = c7x::element_count_of<vec>::value;

   dataType *restrict pInLocalSrc   = (dataType *) pIn0;
   dataType *restrict pInLocalTable = (dataType *) pIn1;
   dataType *restrict pOutLocalOut  = (dataType *) pOut;
   dataType *restrict pOutLocalOut1 = pOutLocalOut + 1;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   __SE0_OPEN(pInLocalSrc, se0Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa0Params);

   vec      minVec     = (vec) minVal;
   vec      divisorVec = (vec) divisor;
   vec      roundVec   = (vec) (tableSamples - 1);
   uint32_t mainVecs   = nVecs / eleCount;
   uint32_t remVecs    = nVecs % eleCount;

   // 39 + mainVecs * 8
   for (uint32_t i = 0; i < mainVecs; i++) {

      vec srcVec = c7x::strm_eng<0, vec>::get_adv();

      srcVec = srcVec - minVec;
      srcVec = srcVec * divisorVec;
      srcVec = srcVec + (vec) 0.5;

      //   check for the minimum condition,replacing with table index 0
      __vpred result = __cmp_le_pred(srcVec, vec(0));
      srcVec         = __select(result, vec(0), srcVec);

      result = __cmp_le_pred(srcVec, roundVec);
      srcVec = __select(result, srcVec, roundVec);

      c7x::int_vec index = __float_to_int_rtz(srcVec);

      float data1 = pInLocalTable[__vgetw_vrd(index, 0)];
      float data2 = pInLocalTable[__vgetw_vrd(index, 1)];
      float data3 = pInLocalTable[__vgetw_vrd(index, 2)];
      float data4 = pInLocalTable[__vgetw_vrd(index, 3)];
      float data5 = pInLocalTable[__vgetw_vrd(index, 4)];
      float data6 = pInLocalTable[__vgetw_vrd(index, 5)];
      float data7 = pInLocalTable[__vgetw_vrd(index, 6)];
      float data8 = pInLocalTable[__vgetw_vrd(index, 7)];

      __vpred outPred = c7x::strm_agen<0, float>::get_vpred();
      float  *outPtr  = c7x::strm_agen<0, float>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, data1);

      __vpred outPred1 = c7x::strm_agen<1, float>::get_vpred();
      float  *outPtr1  = c7x::strm_agen<1, float>::get_adv(pOutLocalOut1);
      __vstore_pred(outPred1, outPtr1, data2);

      outPred = c7x::strm_agen<0, float>::get_vpred();
      outPtr  = c7x::strm_agen<0, float>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, data3);

      outPred1 = c7x::strm_agen<1, float>::get_vpred();
      outPtr1  = c7x::strm_agen<1, float>::get_adv(pOutLocalOut1);
      __vstore_pred(outPred1, outPtr1, data4);

      outPred = c7x::strm_agen<0, float>::get_vpred();
      outPtr  = c7x::strm_agen<0, float>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, data5);

      outPred1 = c7x::strm_agen<1, float>::get_vpred();
      outPtr1  = c7x::strm_agen<1, float>::get_adv(pOutLocalOut1);
      __vstore_pred(outPred1, outPtr1, data6);

      outPred = c7x::strm_agen<0, float>::get_vpred();
      outPtr  = c7x::strm_agen<0, float>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, data7);

      outPred1 = c7x::strm_agen<1, float>::get_vpred();
      outPtr1  = c7x::strm_agen<1, float>::get_adv(pOutLocalOut1);
      __vstore_pred(outPred1, outPtr1, data8);
   }

   // process the remaining elements that is not the multiple of elecount
   pOutLocalOut += mainVecs * eleCount;
   vec srcVec = c7x::strm_eng<0, vec>::get_adv();

   srcVec = srcVec - minVec;
   srcVec = srcVec * divisorVec;
   srcVec = srcVec + (vec) 0.5;

   //   check for the minimum condition,replacing with table index 0
   __vpred result = __cmp_le_pred(srcVec, vec(0));
   srcVec         = __select(result, vec(0), srcVec);

   result = __cmp_le_pred(srcVec, roundVec);
   srcVec = __select(result, srcVec, roundVec);

   c7x::int_vec index = __float_to_int_rtz(srcVec);

   for (uint32_t i = 0; i < remVecs; i++) {
      float data          = pInLocalTable[__vgetw_vrd(index, i)];
      *(pOutLocalOut + i) = data;
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_tableLookup_unroll_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                    void *restrict pIn0,
                                                                    void *restrict pIn1,
                                                                    void *restrict pOut);
