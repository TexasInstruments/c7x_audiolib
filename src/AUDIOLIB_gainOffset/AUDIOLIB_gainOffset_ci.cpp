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

   const uint64_t nPairs = (uint64_t) AUDIOLIB_ceilingDiv(pKerPrivArgs->nVecs, 2U);
   uint64_t       gainOffsetOperationCycles;

   /* C7504 software-pipeline reports: two vectors per iteration */
   if (pKerPrivArgs->dataType == (uint32_t) AUDIOLIB_INT32) {
      /* ii 3 per pair (.L/.S bound: two compares, two shifts) */
      gainOffsetOperationCycles = 5U + (3U * nPairs);
   }
   else {
      /* ii 2 per pair: one vector per cycle */
      gainOffsetOperationCycles = 7U + (2U * nPairs);
   }

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
template AUDIOLIB_STATUS AUDIOLIB_gainOffset_init_ci<int32_t>(AUDIOLIB_kernelHandle               handle,
                                                              const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                                              const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                                              const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs);

/*
 * AUDIOLIB_gainOffset_dataTypeOps<dataType>: what exec needs for one data type, its types and
 * operations (as permute<T> in AUDIOLIB_downSample_ci.cpp):
 *   vec             : vector the loop reads, computes on and stores (one SE get, one SA advance)
 *   paramType       : type of *pGain / *pOffset stored in the handle by init
 *   satParams_t     : saturation parameters of the loop
 *   loadSatParams() : loads them from the handle
 *   apply()         : computes one output vector
 *
 *   | dataType | vec        | paramType | satParams_t                |
 *   |----------|------------|-----------|----------------------------|
 *   | float    | float  x L | float     | none                       |
 *   | double   | double x L | float     | none                       |
 *   | int32_t  | int32  x L | int32_t   | linear input range, rail   |
 */

/* float and double: y = gain * x + offset */
template <typename dataType> struct AUDIOLIB_gainOffset_dataTypeOps {
   typedef typename c7x::make_full_vector<dataType>::type vec;
   typedef float                                          paramType;

   struct satParams_t {
   }; /* nothing to saturate */

   static inline void loadSatParams(const AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs, satParams_t *pSatParams)
   {
      (void) pKerPrivArgs; /* Unused parameter suppression: nothing to load for float / double */
      (void) pSatParams;
   }

   static inline vec apply(vec x, vec gain, vec offset, const satParams_t &) { return (gain * x) + offset; }
};

template <> struct AUDIOLIB_gainOffset_dataTypeOps<int32_t> {
   typedef c7x::make_full_vector<int32_t>::type  vec;
   typedef c7x::make_full_vector<uint32_t>::type uvec; /* unsigned view for the range compare */
   typedef int32_t                               paramType;

   struct satParams_t {
      vec  lo;    /* linear input range [lo, lo + width] */
      uvec width;
      vec  rail;  /* INT32_MAX (gain >= 0) or INT32_MIN; x < 0 takes the other rail */
   };

   static inline void loadSatParams(const AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs, satParams_t *pSatParams)
   {
      pSatParams->lo    = (vec) pKerPrivArgs->satLo;
      pSatParams->width = (uvec) pKerPrivArgs->satWidth;
      pSatParams->rail  = (vec) pKerPrivArgs->satRail;
   }

   static inline vec apply(vec x, vec gain, vec offset, const satParams_t &satParams)
   {
      vec     wrapped = (x * gain) + offset;
      __vpred outside = __cmp_gt_pred(c7x::reinterpret<uvec>(x - satParams.lo), satParams.width);
      return __select(outside, (x >> 31) ^ satParams.rail, wrapped);
   }
};

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainOffset_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainOffset_PrivArgs                          *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;
   typedef AUDIOLIB_gainOffset_dataTypeOps<dataType> dataTypeOps;
   typedef typename dataTypeOps::vec                vec;
   typedef typename dataTypeOps::paramType          paramType;

   const uint8_t                                                    *pBlock = pKerPrivArgs->bufPblock;
   const uint32_t                                                    nVecs  = pKerPrivArgs->nVecs;
   const uint32_t                                                    nPairs = AUDIOLIB_ceilingDiv(nVecs, 2U);

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_exec_ci nVecs %u\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(const __SE_TEMPLATE_v1 *) (pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(const __SA_TEMPLATE_v1 *) (pBlock + SE_SA0_PARAM_OFFSET);

   const dataType *restrict pInLocal = (const dataType *) pIn;
   dataType *restrict pOutLocal      = (dataType *) pOut;

   const vec gain   = (vec) *(const paramType *) pKerPrivArgs->pGain;
   const vec offset = (vec) *(const paramType *) pKerPrivArgs->pOffset;

   typename dataTypeOps::satParams_t satParams;
   dataTypeOps::loadSatParams(pKerPrivArgs, &satParams);

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   /* Two vectors per iteration (see AUDIOLIB_gainOffset_perfEst for the cycles per type) */
   /*5 + trip_cnt * 3 (int32_t)*/
   /*7 + trip_cnt * 2 ( float and double)*/
   for (uint32_t i = 0U; i < nPairs; i++) {
      vec inVec0  = c7x::strm_eng<0, vec>::get_adv();
      vec inVec1  = c7x::strm_eng<0, vec>::get_adv();

      vec outVec0 = dataTypeOps::apply(inVec0, gain, offset, satParams);
      vec outVec1 = dataTypeOps::apply(inVec1, gain, offset, satParams);

      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred0, addr0, outVec0);
      __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred1, addr1, outVec1);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_ci<int32_t>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
