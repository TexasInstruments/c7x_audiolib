// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_tableInterpolate_priv.h"
// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_unroll_init_ci(AUDIOLIB_kernelHandle                     handle,
                                                         const AUDIOLIB_bufParams1D_t             *bufParamsIn0,
                                                         const AUDIOLIB_bufParams1D_t             *bufParamsIn1,
                                                         const AUDIOLIB_bufParams1D_t             *bufParamsOut,
                                                         const AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                     status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableInterpolate_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableInterpolate_PrivArgs *) handle;
   uint8_t                            *pBlock       = pKerPrivArgs->bufPblock;

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

template AUDIOLIB_STATUS
AUDIOLIB_tableInterpolate_unroll_init_ci<float>(AUDIOLIB_kernelHandle                     handle,
                                                const AUDIOLIB_bufParams1D_t             *bufParamsIn0,
                                                const AUDIOLIB_bufParams1D_t             *bufParamsIn1,
                                                const AUDIOLIB_bufParams1D_t             *bufParamsOut,
                                                const AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS
AUDIOLIB_tableInterpolate_unroll_init_ci<double>(AUDIOLIB_kernelHandle                     handle,
                                                 const AUDIOLIB_bufParams1D_t             *bufParamsIn0,
                                                 const AUDIOLIB_bufParams1D_t             *bufParamsIn1,
                                                 const AUDIOLIB_bufParams1D_t             *bufParamsOut,
                                                 const AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_unroll_exec_ci(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn0,
                                                         void *restrict pIn1,
                                                         void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                     status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableInterpolate_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableInterpolate_PrivArgs *) handle;
   uint8_t                            *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                            nVecs        = pKerPrivArgs->nVecs;
   float                               minVal       = pKerPrivArgs->minVal;
   float                               divisor      = pKerPrivArgs->divisor;
   uint32_t                            tableSamples = pKerPrivArgs->tableSamples;

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

   // 37 + trip_cnt * 17
   for (uint32_t i = 0; i < mainVecs; i++) {

      vec srcVec = c7x::strm_eng<0, vec>::get_adv();

      srcVec = srcVec - minVec;
      srcVec = srcVec * divisorVec;

      //   check for the minimum condition,replacing with table index 0
      __vpred result = __cmp_le_pred(srcVec, vec(0));
      srcVec         = __select(result, vec(0), srcVec);

      result = __cmp_le_pred(srcVec, roundVec);
      srcVec = __select(result, srcVec, roundVec);

      c7x::int_vec index  = __float_to_int_rtz(srcVec);
      vec          fract0 = srcVec - __int_to_float(index);
      vec          fract1 = (1 - fract0);

      long val1 = __extuv_vkkkd(c7x::reinterpret<ulong4>(c7x::reinterpret<uint8>(index)), 0, 0, 32);

      long val3 = __extuv_vkkkd(c7x::reinterpret<ulong4>(c7x::reinterpret<uint8>(index)), 0, 32, 32);

      float data1     = pInLocalTable[val3];
      float nextData1 = pInLocalTable[val3 + 1];

      float result1 = data1 * fract1.s[0] + fract0.s[0] * nextData1;

      __vpred outPred = c7x::strm_agen<0, float>::get_vpred();
      float  *outPtr  = c7x::strm_agen<0, float>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, result1);

      float data2     = pInLocalTable[val1];
      float nextData2 = pInLocalTable[val1 + 1];

      result1 = data2 * fract1.s[1] + fract0.s[1] * nextData2;

      outPred = c7x::strm_agen<1, float>::get_vpred();
      outPtr  = c7x::strm_agen<1, float>::get_adv(pOutLocalOut1);
      __vstore_pred(outPred, outPtr, result1);

      val1 = __extuv_vkkkd(c7x::reinterpret<ulong4>(c7x::reinterpret<uint8>(index)), 1, 0, 32);

      val3 = __extuv_vkkkd(c7x::reinterpret<ulong4>(c7x::reinterpret<uint8>(index)), 1, 32, 32);

      data1     = pInLocalTable[val3];
      nextData1 = pInLocalTable[val3 + 1];

      result1 = data1 * fract1.s[2] + fract0.s[2] * nextData1;

      outPred = c7x::strm_agen<0, float>::get_vpred();
      outPtr  = c7x::strm_agen<0, float>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, result1);

      data2     = pInLocalTable[val1];
      nextData2 = pInLocalTable[val1 + 1];

      result1 = data2 * fract1.s[3] + fract0.s[3] * nextData2;

      outPred = c7x::strm_agen<1, float>::get_vpred();
      outPtr  = c7x::strm_agen<1, float>::get_adv(pOutLocalOut1);
      __vstore_pred(outPred, outPtr, result1);

      val1 = __extuv_vkkkd(c7x::reinterpret<ulong4>(c7x::reinterpret<uint8>(index)), 2, 0, 32);

      val3 = __extuv_vkkkd(c7x::reinterpret<ulong4>(c7x::reinterpret<uint8>(index)), 2, 32, 32);

      data1     = pInLocalTable[val3];
      nextData1 = pInLocalTable[val3 + 1];

      result1 = data1 * fract1.s[4] + fract0.s[4] * nextData1;

      outPred = c7x::strm_agen<0, float>::get_vpred();
      outPtr  = c7x::strm_agen<0, float>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, result1);

      data2     = pInLocalTable[val1];
      nextData2 = pInLocalTable[val1 + 1];

      result1 = data2 * fract1.s[5] + fract0.s[5] * nextData2;

      outPred = c7x::strm_agen<1, float>::get_vpred();
      outPtr  = c7x::strm_agen<1, float>::get_adv(pOutLocalOut1);
      __vstore_pred(outPred, outPtr, result1);

      val1 = __extuv_vkkkd(c7x::reinterpret<ulong4>(c7x::reinterpret<uint8>(index)), 3, 0, 32);

      val3 = __extuv_vkkkd(c7x::reinterpret<ulong4>(c7x::reinterpret<uint8>(index)), 3, 32, 32);

      data1     = pInLocalTable[val3];
      nextData1 = pInLocalTable[val3 + 1];

      result1 = data1 * fract1.s[6] + fract0.s[6] * nextData1;

      outPred = c7x::strm_agen<0, float>::get_vpred();
      outPtr  = c7x::strm_agen<0, float>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, result1);

      data2     = pInLocalTable[val1];
      nextData2 = pInLocalTable[val1 + 1];

      result1 = data2 * fract1.s[7] + fract0.s[7] * nextData2;

      outPred = c7x::strm_agen<1, float>::get_vpred();
      outPtr  = c7x::strm_agen<1, float>::get_adv(pOutLocalOut1);
      __vstore_pred(outPred, outPtr, result1);
   }

   // process the remaining elements that is not the multiple of elecount
   pOutLocalOut += mainVecs * eleCount;
   vec srcVec = c7x::strm_eng<0, vec>::get_adv();

   srcVec = srcVec - minVec;
   srcVec = srcVec * divisorVec;

   //   check for the minimum condition,replacing with table index 0
   __vpred result = __cmp_le_pred(srcVec, vec(0));
   srcVec         = __select(result, vec(0), srcVec);

   result = __cmp_le_pred(srcVec, roundVec);
   srcVec = __select(result, srcVec, roundVec);

   c7x::int_vec index     = __float_to_int_rtz(srcVec);
   c7x::int_vec nextIndex = index + (c7x::int_vec) 1;
   vec          fract0    = srcVec - __int_to_float(index);
   vec          fract1    = (1 - fract0);

   for (uint32_t i = 0; i < remVecs; i++) {
      float data1         = pInLocalTable[__vgetw_vrd(index, i)];
      float data2         = pInLocalTable[__vgetw_vrd(nextIndex, i)];
      float fract0_val    = __vgetw_vrd(fract0, i);
      float fract1_val    = __vgetw_vrd(fract1, i);
      *(pOutLocalOut + i) = (data1 * fract1_val) + (fract0_val * data2);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_unroll_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                         void *restrict pIn0,
                                                                         void *restrict pIn1,
                                                                         void *restrict pOut);
