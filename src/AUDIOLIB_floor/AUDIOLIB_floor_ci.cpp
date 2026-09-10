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
 *  @brief Floor of every lane, single precision. Truncating conversion to int32
 *         and back gives the integral part; only a negative input with a fraction
 *         needs the correction. Input range precondition on
 *         @ref AUDIOLIB_floor_exec applies.
 */
static inline c7x::float_vec AUDIOLIB_floor_computeFloor(c7x::float_vec vIn)
{
   c7x::float_vec vTrunc = __int_to_float(__float_to_int_rtz(vIn));

   __vpred cmpFraction = __cmp_lt_pred(vIn, vTrunc);

   return __select(cmpFraction, vTrunc - (c7x::float_vec) 1.0f, vTrunc);
}

/*!
 *  @brief Floor of every lane, double precision. With no int64 conversion, a
 *         sign-carrying bias of 2^52 forces the sum into the binade spaced 1.0
 *         apart. Round-to-nearest overshoots by at most one, so one correction
 *         suffices. Input range precondition on @ref AUDIOLIB_floor_exec applies.
 */
static inline c7x::double_vec AUDIOLIB_floor_computeFloor(c7x::double_vec vIn)
{
   c7x::double_vec vRoundBias    = (c7x::double_vec) AUDIOLIB_FLOOR_ROUND_BIAS_DOUBLE;
   c7x::double_vec vRoundBiasNeg = (c7x::double_vec)(-AUDIOLIB_FLOOR_ROUND_BIAS_DOUBLE);

   __vpred         cmpNegative = __cmp_lt_pred(vIn, (c7x::double_vec) 0.0);
   c7x::double_vec vBias       = __select(cmpNegative, vRoundBiasNeg, vRoundBias);
   c7x::double_vec vRounded    = (vIn + vBias) - vBias;

   __vpred cmpOvershoot = __cmp_gt_pred(vRounded, vIn);

   return __select(cmpOvershoot, vRounded - (c7x::double_vec) 1.0, vRounded);
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

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   // Software pipeline, trip_cnt = nPairs, resource bound (carried dep 2).
   //   float  : ii = 4, 9 + trip_cnt * 4   (short buffers only)
   //   double : ii = 4, 12 + trip_cnt * 4
   // nPairs rounds up; the surplus half reads inside the SE ICNT extent and
   // its store is masked by the SA predicate.
#pragma MUST_ITERATE(1, , 1)
   for (int32_t i = 0; i < nPairs; i++) {
      vec vIn0  = c7x::strm_eng<0, vec>::get_adv();
      vec vOut0 = AUDIOLIB_floor_computeFloor(vIn0);

      __vpred pred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred0, addr0, vOut0);

      vec vIn1  = c7x::strm_eng<0, vec>::get_adv();
      vec vOut1 = AUDIOLIB_floor_computeFloor(vIn1);

      __vpred pred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred1, addr1, vOut1);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_floor_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_floor_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief Execution variant unrolled by four, single precision only. Four
 *         vectors of six operations fill the four issue slots exactly, giving
 *         1.5 cycles per vector against 2.0 for @ref AUDIOLIB_floor_exec_ci. The
 *         longer drain costs more on short buffers, so @ref AUDIOLIB_floor_init
 *         chooses by size. Double precision is already at its floor at two.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floor_exec_ci_unroll4(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_floor_PrivArgs                               *pKerPrivArgs = (AUDIOLIB_floor_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floor_exec_ci_unroll4 nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   int32_t nVecs  = (int32_t) pKerPrivArgs->nVecs;
   int32_t nQuads = (nVecs + 3) >> 2;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   // Software pipeline, trip_cnt = nQuads, 100% slot use (carried dep 4).
   //   float : ii = 6, 13 + trip_cnt * 6
   // nQuads rounds up; up to three surplus vectors read inside the SE ICNT
   // extent and their stores are masked by the SA predicate.
#pragma MUST_ITERATE(1, , 1)
   for (int32_t i = 0; i < nQuads; i++) {
      vec vIn0  = c7x::strm_eng<0, vec>::get_adv();
      vec vOut0 = AUDIOLIB_floor_computeFloor(vIn0);

      __vpred pred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred0, addr0, vOut0);

      vec vIn1  = c7x::strm_eng<0, vec>::get_adv();
      vec vOut1 = AUDIOLIB_floor_computeFloor(vIn1);

      __vpred pred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred1, addr1, vOut1);

      vec vIn2  = c7x::strm_eng<0, vec>::get_adv();
      vec vOut2 = AUDIOLIB_floor_computeFloor(vIn2);

      __vpred pred2 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred2, addr2, vOut2);

      vec vIn3  = c7x::strm_eng<0, vec>::get_adv();
      vec vOut3 = AUDIOLIB_floor_computeFloor(vIn3);

      __vpred pred3 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred3, addr3, vOut3);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_floor_exec_ci_unroll4<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

void AUDIOLIB_floor_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   uint64_t                 loopCycles;
   uint32_t                 nGroups;
   AUDIOLIB_floor_PrivArgs *pKerPrivArgs = (AUDIOLIB_floor_PrivArgs *) handle;

   /* Mirrors the variant selection in AUDIOLIB_floor_init */
   if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
      nGroups    = AUDIOLIB_ceilingDiv(pKerPrivArgs->nVecs, 2u);
      loopCycles = AUDIOLIB_FLOOR_DP_PIPE_UP_CYCLES + nGroups * AUDIOLIB_FLOOR_DP_CYCLES_PER_VEC_PAIR;
   }
   else if (pKerPrivArgs->nVecs >= AUDIOLIB_FLOOR_SP_UNROLL4_MIN_VECS) {
      nGroups    = AUDIOLIB_ceilingDiv(pKerPrivArgs->nVecs, 4u);
      loopCycles = AUDIOLIB_FLOOR_SP_UNROLL4_PIPE_UP_CYCLES + nGroups * AUDIOLIB_FLOOR_SP_UNROLL4_CYCLES_PER_VEC_QUAD;
   }
   else {
      nGroups    = AUDIOLIB_ceilingDiv(pKerPrivArgs->nVecs, 2u);
      loopCycles = AUDIOLIB_FLOOR_SP_PIPE_UP_CYCLES + nGroups * AUDIOLIB_FLOOR_SP_CYCLES_PER_VEC_PAIR;
   }

   *estCycles  = loopCycles + AUDIOLIB_FLOOR_CALL_OVERHEAD_CYCLES;
   *archCycles = loopCycles;
}
