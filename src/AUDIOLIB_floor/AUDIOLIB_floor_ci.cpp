// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_floor_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floor_init_ci(AUDIOLIB_kernelHandle          handle,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                       const AUDIOLIB_floor_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_floor_PrivArgs                               *pKerPrivArgs = (AUDIOLIB_floor_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floor_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d inStride %d outStride %d eleCount %d\n", pKerPrivArgs->dim_x,
                         pKerPrivArgs->dim_y, pKerPrivArgs->inStride, pKerPrivArgs->outStride, eleCount);

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

   // For input and output buffers without stride, the whole 2D block is one
   // contiguous 1D stream, which avoids a partial vector on every row
   if (((int32_t) pKerPrivArgs->dim_x == pKerPrivArgs->inStride) &&
       ((int32_t) pKerPrivArgs->dim_x == pKerPrivArgs->outStride)) {
      uint32_t blockSize = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;

      se0Params.ICNT0  = blockSize;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0  = blockSize;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;

      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(blockSize, eleCount);
   }
   // For buffers with stride
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

template AUDIOLIB_STATUS AUDIOLIB_floor_init_ci<float>(AUDIOLIB_kernelHandle          handle,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                       const AUDIOLIB_floor_InitArgs *pKerInitArgs);
template AUDIOLIB_STATUS AUDIOLIB_floor_init_ci<double>(AUDIOLIB_kernelHandle          handle,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                        const AUDIOLIB_floor_InitArgs *pKerInitArgs);

/*!
 * @brief Floor of each lane, float: truncate @p vInForTrunc to int32 and back, then
 *        subtract one where @p vInForCompare, the same input from SE1, is below the
 *        truncation.
 */
static inline c7x::float_vec AUDIOLIB_floor_computeFloor(c7x::float_vec vInForTrunc, c7x::float_vec vInForCompare)
{
   c7x::float_vec vTruncated = __int_to_float(__float_to_int_rtz(vInForTrunc));

   __vpred cmpTruncAboveInput = __cmp_lt_pred(vInForCompare, vTruncated);

   return __select(cmpTruncAboveInput, vTruncated - (c7x::float_vec) 1.0f, vTruncated);
}

/*!
 * @brief Floor of each lane, double, by the same method. Each int32 result sits
 *        in the low word of its 64-bit lane, which the conversion back reads.
 */
static inline c7x::double_vec AUDIOLIB_floor_computeFloor(c7x::double_vec vInForTrunc, c7x::double_vec vInForCompare)
{
   c7x::double_vec vTruncated = __low_int_to_double(__double_to_int_rtz(vInForTrunc));

   __vpred cmpTruncAboveInput = __cmp_lt_pred(vInForCompare, vTruncated);

   return __select(cmpTruncAboveInput, vTruncated - (c7x::double_vec) 1.0, vTruncated);
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floor_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_floor_PrivArgs                               *pKerPrivArgs = (AUDIOLIB_floor_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floor_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   int32_t nVecs  = (int32_t) pKerPrivArgs->nVecs;
   int32_t nPairs = (nVecs + 1) >> 1;

   // SE1 streams the same input again for the late compare, so no register
   // holds it across the conversion chain.
   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   // ii = 3 per 2 vectors (1.5 per vector) on C7504 and C7524, bound by
   // .L2/.S2/.C2. Total: float 11 + trip_cnt * 3, double 12 + trip_cnt * 3.
   // An odd nVecs runs one surplus vector: no read past the streams, store masked.
#pragma MUST_ITERATE(1, , 1)
   for (int32_t i = 0; i < nPairs; i++) {
      vec vInForTrunc0   = c7x::strm_eng<0, vec>::get_adv();
      vec vInForCompare0 = c7x::strm_eng<1, vec>::get_adv();
      vec vOut0          = AUDIOLIB_floor_computeFloor(vInForTrunc0, vInForCompare0);

      __vpred pred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred0, addr0, vOut0);

      vec vInForTrunc1   = c7x::strm_eng<0, vec>::get_adv();
      vec vInForCompare1 = c7x::strm_eng<1, vec>::get_adv();
      vec vOut1          = AUDIOLIB_floor_computeFloor(vInForTrunc1, vInForCompare1);

      __vpred pred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred1, addr1, vOut1);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_floor_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_floor_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floor_unroll8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_floor_PrivArgs                               *pKerPrivArgs = (AUDIOLIB_floor_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floor_unroll8_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   int32_t nVecs           = (int32_t) pKerPrivArgs->nVecs;
   int32_t nEightVecGroups = (nVecs + 7) >> 3;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   // ii = 11 per 8 vectors (1.375 per vector) on C7504 and C7524, bound by
   // .L2/.S2/.C2. Total: float 11 + trip_cnt * 11, double 13 + trip_cnt * 11.
   // Up to seven surplus vectors: no reads past the streams, stores masked.
#pragma MUST_ITERATE(1, , 1)
   for (int32_t i = 0; i < nEightVecGroups; i++) {
      vec     vInForTrunc0   = c7x::strm_eng<0, vec>::get_adv();
      vec     vInForCompare0 = c7x::strm_eng<1, vec>::get_adv();
      vec     vOut0          = AUDIOLIB_floor_computeFloor(vInForTrunc0, vInForCompare0);
      __vpred pred0          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred0, addr0, vOut0);

      vec     vInForTrunc1   = c7x::strm_eng<0, vec>::get_adv();
      vec     vInForCompare1 = c7x::strm_eng<1, vec>::get_adv();
      vec     vOut1          = AUDIOLIB_floor_computeFloor(vInForTrunc1, vInForCompare1);
      __vpred pred1          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred1, addr1, vOut1);

      vec     vInForTrunc2   = c7x::strm_eng<0, vec>::get_adv();
      vec     vInForCompare2 = c7x::strm_eng<1, vec>::get_adv();
      vec     vOut2          = AUDIOLIB_floor_computeFloor(vInForTrunc2, vInForCompare2);
      __vpred pred2          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr2          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred2, addr2, vOut2);

      vec     vInForTrunc3   = c7x::strm_eng<0, vec>::get_adv();
      vec     vInForCompare3 = c7x::strm_eng<1, vec>::get_adv();
      vec     vOut3          = AUDIOLIB_floor_computeFloor(vInForTrunc3, vInForCompare3);
      __vpred pred3          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr3          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred3, addr3, vOut3);

      vec     vInForTrunc4   = c7x::strm_eng<0, vec>::get_adv();
      vec     vInForCompare4 = c7x::strm_eng<1, vec>::get_adv();
      vec     vOut4          = AUDIOLIB_floor_computeFloor(vInForTrunc4, vInForCompare4);
      __vpred pred4          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr4          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred4, addr4, vOut4);

      vec     vInForTrunc5   = c7x::strm_eng<0, vec>::get_adv();
      vec     vInForCompare5 = c7x::strm_eng<1, vec>::get_adv();
      vec     vOut5          = AUDIOLIB_floor_computeFloor(vInForTrunc5, vInForCompare5);
      __vpred pred5          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr5          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred5, addr5, vOut5);

      vec     vInForTrunc6   = c7x::strm_eng<0, vec>::get_adv();
      vec     vInForCompare6 = c7x::strm_eng<1, vec>::get_adv();
      vec     vOut6          = AUDIOLIB_floor_computeFloor(vInForTrunc6, vInForCompare6);
      __vpred pred6          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr6          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred6, addr6, vOut6);

      vec     vInForTrunc7   = c7x::strm_eng<0, vec>::get_adv();
      vec     vInForCompare7 = c7x::strm_eng<1, vec>::get_adv();
      vec     vOut7          = AUDIOLIB_floor_computeFloor(vInForTrunc7, vInForCompare7);
      __vpred pred7          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr7          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred7, addr7, vOut7);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_floor_unroll8_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_floor_unroll8_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

void AUDIOLIB_floor_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_floor_PrivArgs *pKerPrivArgs = (AUDIOLIB_floor_PrivArgs *) handle;

   /* Same model AUDIOLIB_floor_init picks the variant with */
   uint64_t unroll2Cycles = AUDIOLIB_floor_unroll2LoopCycles(pKerPrivArgs->dataType, pKerPrivArgs->nVecs);
   uint64_t unroll8Cycles = AUDIOLIB_floor_unroll8LoopCycles(pKerPrivArgs->dataType, pKerPrivArgs->nVecs);
   uint64_t loopCycles    = (unroll8Cycles < unroll2Cycles) ? unroll8Cycles : unroll2Cycles;

   *estCycles  = loopCycles + AUDIOLIB_FLOOR_CALL_OVERHEAD_CYCLES;
   *archCycles = loopCycles;
}
