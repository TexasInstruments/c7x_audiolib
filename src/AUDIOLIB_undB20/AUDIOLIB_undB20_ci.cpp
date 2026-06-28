// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/AUDIOLIB_ilut.h"
#include "../common/AUDIOLIB_permute.h"
#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_undB20_priv.h"
#include "c6x_migration.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)

#define ELEMENT_COUNT(x) c7x::element_count_of<x>::value

const double AUDIOLIB_kTable[4] = {
    1.000000000, /* 2^(0/4) */
    1.189207115, /* 2^(1/4) */
    1.414213562, /* 2^(2/4) */
    1.681792831  /* 2^(3/4) */
};

const double AUDIOLIB_jTable[4] = {
    1.000000000, /* 2^(0/16) */
    1.044273782, /* 2^(1/16) */
    1.090507733, /* 2^(2/16) */
    1.138788635  /* 2^(3/16) */
};

void AUDIOLIB_undB20_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_undB20_PrivArgs *pKerPrivArgs = (AUDIOLIB_undB20_PrivArgs *) handle;
   uint32_t                  dataType     = pKerPrivArgs->dataType;
   uint32_t                  numSamples   = pKerPrivArgs->dim_x;
   uint32_t                  numChannels  = pKerPrivArgs->dim_y;
#if defined(__C7504__)
   uint32_t inStride  = pKerPrivArgs->inStride;
   uint32_t outStride = pKerPrivArgs->outStride;
#endif

   uint64_t undB20StartupCycles   = 0;
   uint64_t undB20TeardownCycles  = 0;
   uint64_t undB20OverheadCycles  = 0;
   uint32_t nVecs                 = 0;
   uint64_t undB20OperationCycles = 0;

#if defined(__C7524__)
   {
      if (dataType == AUDIOLIB_FLOAT32) {
         if (numSamples == 1) {
            if (numChannels == 1) {
               undB20OperationCycles = 55;
            }
            else {
               nVecs                 = numChannels * numSamples;
               undB20OperationCycles = (67 + nVecs * 9);
            }
         }
         else {
            undB20StartupCycles  = 25;
            undB20TeardownCycles = 3;

            nVecs                 = pKerPrivArgs->nVecs;
            undB20OperationCycles = (61 + nVecs * 14) + (8 + nVecs * 3);
         }
      }
   }
#elif defined(__C7504__)
   {
      if (dataType == AUDIOLIB_FLOAT32) {
         nVecs = numChannels * numSamples;
         if (numSamples == 1) {
            if (numChannels == 1) {
               undB20OperationCycles = 55;
            }
            else {
               undB20OperationCycles = (67 + nVecs * 9);
            }
         }
         else if ((numSamples == inStride && numSamples == outStride) || (numSamples > 1 && numChannels == 1)) {
            undB20OperationCycles = (64 + nVecs * 9);
         }
         else {
            undB20OperationCycles = (73 + nVecs * 9);
         }
      }
   }
#endif

   undB20OverheadCycles = undB20StartupCycles + undB20TeardownCycles;
   *estCycles           = undB20OperationCycles + undB20OverheadCycles;
   *archCycles          = undB20OperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_undB20_init_ci(AUDIOLIB_kernelHandle           handle,
                                        const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                        const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                        const AUDIOLIB_undB20_InitArgs *pKerInitArgs)
{

#if defined(__C7524__)
   AUDIOLIB_ILUTInit();
#endif
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_undB20_PrivArgs *pKerPrivArgs = (AUDIOLIB_undB20_PrivArgs *) handle;

   // derive c7x vector type from template typename
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;
#if defined(__C7524__)
   uint32_t eleCount = c7x::element_count_of<vec>::value;
#endif
   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE;
   __SE_VECLEN  SE_VECLEN;
   __SA_VECLEN  SA_VECLEN;

   // assign SE and SA params based on vector type
   SE_VECLEN  = c7x::se_veclen<vec>::value;
   SA_VECLEN  = c7x::sa_veclen<vec>::value;
   SE_ELETYPE = c7x::se_eletype<vec>::value;

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;

   // For input buffer without stride
   if (pKerPrivArgs->dim_x == pKerPrivArgs->inStride && pKerPrivArgs->dim_x == pKerPrivArgs->inStride) {
      uint32_t blockSize = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;
      se0Params.ICNT0    = blockSize;
      se0Params.DIMFMT   = __SE_DIMFMT_1D;

      sa0Params.ICNT0  = blockSize;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;
#if defined(__C7524__)
      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(blockSize, eleCount);

#elif defined(__C7504__)
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32)
         pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;

#endif
   }
   // For input buffer with stride
   else {
      se0Params.ICNT0  = pKerPrivArgs->dim_x;
      se0Params.ICNT1  = pKerPrivArgs->dim_y;
      se0Params.DIM1   = pKerPrivArgs->inStride;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = pKerPrivArgs->dim_x;
      sa0Params.ICNT1  = pKerPrivArgs->dim_y;
      sa0Params.DIM1   = pKerPrivArgs->outStride;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

#if defined(__C7524__)
      pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);

#elif defined(__C7504__)
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;
      }
#endif
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_undB20_init_ci<float>(AUDIOLIB_kernelHandle           handle,
                                                        const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                                        const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                                        const AUDIOLIB_undB20_InitArgs *pKerInitArgs);

template <typename dataType> static inline dataType AUDIOLIB_undB20_scalar_ci(dataType a);

template <> inline float AUDIOLIB_undB20_scalar_ci(float a)
{
   float        log2_base_x16 = 16.0f * 3.321928095f;
   float        Halfe         = 0.5f;
   float        LnMine        = -87.33654475f;
   float        LnMaxe        = 88.72283905f;
   float        Maxe          = 3.402823466E+38f;
   float        c0            = 0.1667361910f;
   float        c1            = 0.4999999651f;
   float        c2            = 0.9999998881f;
   float        P1            = 0.04331970214844f;
   float        P2            = 1.99663646e-6f;
   float        k10e          = 2.302585093f;
   float        pol, r, r2, r3, res, Ye;
   unsigned int Ttemp, J, K;
   float        Nf;
   int          N;
   double       dT;

   a  = a * 0.05f;
   Ye = k10e * a;

   /* Get N such that |N - x*16/ln(2)| is minimized */
   Nf = (a * log2_base_x16) + Halfe;
   N  = (int) Nf; /* Cast from intermediate variable to appease MISRA */

   if ((a * log2_base_x16) < -Halfe) {
      N--;
   }

   /* Argument reduction, r, and polynomial approximation pol(r) */
   r  = (Ye - (P1 * (float) N)) - (P2 * (float) N);
   r2 = r * r;
   r3 = r * r2;

   pol = (r * c2) + ((r3 * c0) + (r2 * c1));

   /* Get index for ktable and jtable */
   K  = _extu((unsigned int) N, 28u, 30u);
   J  = (unsigned int) N & 0x3u;
   dT = AUDIOLIB_kTable[K] * AUDIOLIB_jTable[J];

   /* Scale exponent to adjust for 2^M */
   Ttemp = _hi(dT) + (((unsigned int) N >> 4) << 20);
   dT    = _itod(Ttemp, _lo(dT));

   res = (float) (dT * (1.0 + (double) pol));

   /* Early exit for small a */
   if (_extu(_ftoi(Ye), 1u, 24u) < 114u) {
      res = 1.0f + Ye;
   }

   /* < LnMin returns 0 */
   if (Ye < LnMine) {
      res = 0.0f;
   }

   /* > LnMax returns MAX */
   if (Ye > LnMaxe) {
      res = Maxe;
   }

   return (res);
}

#if defined(__C7504__)
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_undB20_scaler_sp_1xN_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_undB20_PrivArgs *pKerPrivArgs = (AUDIOLIB_undB20_PrivArgs *) handle;
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;

   float *restrict pSrc = (float *) pIn;
   float *restrict pDst = (float *) pOut;

   for (uint32_t idx = 0; idx < pKerPrivArgs->nVecs; idx++) {
      pDst[idx] = AUDIOLIB_undB20_scalar_ci<dataType>(pSrc[idx]);
   }
   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_undB20_scaler_sp_1xN_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_undB20_scaler_sp_MxN_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_undB20_PrivArgs *pKerPrivArgs = (AUDIOLIB_undB20_PrivArgs *) handle;
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;

   uint32_t inStride  = pKerPrivArgs->inStride;
   uint32_t outStride = pKerPrivArgs->outStride;

   float *restrict pSrc = (float *) pIn;
   float *restrict pDst = (float *) pOut;

   uint32_t idx       = 0;
   uint32_t inOffset  = 0;
   uint32_t outOffset = 0;

   for (uint32_t i = 0; i < pKerPrivArgs->nVecs; i++) {
      pDst[outOffset + idx] = AUDIOLIB_undB20_scalar_ci<dataType>(pSrc[inOffset + idx]);
      idx++;

      if (idx == pKerPrivArgs->dim_x) {
         idx = 0;
         inOffset += inStride;
         outOffset += outStride;
      }
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_undB20_scaler_sp_MxN_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

#elif defined(__C7524__)

template <typename dataType>
static inline void AUDIOLIB_undB20_cond(AUDIOLIB_kernelHandle handle,
                                        __SE_TEMPLATE_v1 *restrict se0Params,
                                        __SA_TEMPLATE_v1 *restrict sa0Params,
                                        dataType *restrict pIn,
                                        dataType *restrict pOut)
{
   AUDIOLIB_undB20_PrivArgs *pKerPrivArgs = (AUDIOLIB_undB20_PrivArgs *) handle;
   uint32_t numChannel = pKerPrivArgs->dim_y;
   uint32_t numSamples = pKerPrivArgs->dim_x;

   float *restrict pSrc = pIn;
   float *restrict pDst = pOut;

   // variables
   size_t numBlocks = 0;    // compute loop's iteration count
   size_t remNumBlocks = 0; // when numBlocks is not a multiple of SIMD width

   // derive c7x vector type from template typename
   typedef typename c7x::make_full_vector<float>::type vec;

   // calculate compute loop's iteration counter
   numBlocks = pKerPrivArgs->nVecs;
   remNumBlocks = (numSamples) % c7x::element_count_of<vec>::value;
   if (remNumBlocks) {
      numBlocks += numChannel;
   }

   // open SE0, SE1, and SA0 for reading and writing operands

   __SE1_OPEN(pSrc, *se0Params);
   se0Params->DIM1 = pKerPrivArgs->outStride;
   __SE0_OPEN(pDst, *se0Params);
   __SA0_OPEN(*sa0Params);

   /**********************************************************************/
   /* Create and assign values for constants employed on pow computation */
   /**********************************************************************/

   vec k10e, LnMin, LnMax, Max, zero;
   c7x::uint_vec inVec_min;

   k10e = (vec) 2.302585093f;
   LnMin = (vec) -87.33654475f;
   LnMax = (vec) 88.72283905f;
   Max = (vec) 3.402823466E+38f;
   zero = (vec) 0.0f;
   inVec_min = (c7x::uint_vec) 114u;

   // compute loop to perform vector pow
   for (size_t i = 0; i < numBlocks; i++) {
      vec outVec = c7x::strm_eng<0, vec>::get_adv();
      vec inVec = c7x::strm_eng<1, vec>::get_adv();

      /**********************************************************************/
      /* Create variables employed on exp computation                       */
      /**********************************************************************/
      vec Ye;
      c7x::uint_vec inVec_small;

      /**********************************************************************/
      /* Bounds checking                                                    */
      /**********************************************************************/
      Ye = k10e * inVec;
      // Early exit for small input
      inVec_small = ((c7x::as_uint_vec(Ye) << 1U) >> 24U);
      __vpred cmp_inVec = __cmp_gt_pred(inVec_min, inVec_small);
      outVec = __select(cmp_inVec, 1.0f + Ye, outVec);

      // < LnMin returns 0
      __vpred cmp_min = __cmp_lt_pred(Ye, LnMin);

      outVec = __select(cmp_min, zero, outVec);
      // > LnMax returns MAX
      __vpred cmp_max = __cmp_lt_pred(LnMax, Ye);
      outVec = __select(cmp_max, Max, outVec);

      __vpred tmp = c7x::strm_agen<0, vec>::get_vpred();
      vec *addr = c7x::strm_agen<0, vec>::get_adv(pDst);
      __vstore_pred(tmp, addr, outVec);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_undB20_vector_sp_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_undB20_PrivArgs *pKerPrivArgs = (AUDIOLIB_undB20_PrivArgs *) handle;
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   uint8_t *pBlock = pKerPrivArgs->bufPblock;
   uint32_t numChannel = pKerPrivArgs->dim_y;
   uint32_t numSamples = pKerPrivArgs->dim_x;

   float *restrict pSrc = (float *) pIn;
   float *restrict pDst = (float *) pOut;

   // variables
   size_t numBlocks = 0;    // compute loop's iteration count
   size_t remNumBlocks = 0; // when numBlocks is not a multiple of SIMD width

   // derive c7x vector type from template typename
   typedef typename c7x::make_full_vector<float>::type vec;

   // Compile-time decision: float_vec => int_vec and double_vec=> long_vec
   typedef
       typename std::conditional<ELEMENT_COUNT(c7x::float_vec) == ELEMENT_COUNT(vec), c7x::int_vec, c7x::long_vec>::type
           vec_type;

   typedef typename std::conditional<ELEMENT_COUNT(c7x::float_vec) == ELEMENT_COUNT(vec), c7x::uint_vec,
                                     c7x::ulong_vec>::type uvec_type;

   __SE_TEMPLATE_v1 se0Params;
   __SA_TEMPLATE_v1 sa0Params;

   se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   // calculate compute loop's iteration counter

   numBlocks = pKerPrivArgs->nVecs;
   remNumBlocks = (numSamples) % c7x::element_count_of<vec>::value;
   if (remNumBlocks) {
      numBlocks += numChannel;
   }

   // open SE & SA
   __SE0_OPEN(pSrc, se0Params);
   __SA0_OPEN(sa0Params);

   /***********************************************************************/
   /* Create and assign values for constants employed on undB20 computation */
   /***********************************************************************/

   vec log2_base_x16, half, negativeHalf, C0, C1, C2, P1, P2, k10e, undb10;
   uvec_type mask;

   log2_base_x16 = (vec) (16.0f * 3.321928095f);
   half = (vec) 0.5f;
   negativeHalf = (vec) -0.5f;
   C0 = (vec) 0.1667361910f;
   C1 = (vec) 0.4999999651f;
   C2 = (vec) 0.9999998881f;
   P1 = (vec) 0.04331970214844f;
   P2 = (vec) 1.99663646e-6f;
   k10e = (vec) 2.302585093f;
   mask = (uvec_type) 0x3u;
   undb10 = (vec) 0.05f;

   // compute loop to perform vector exp10
   for (size_t i = 0; i < numBlocks; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      inVec = inVec * undb10;

      /**********************************************************************/
      /* Create variables employed on exp10 computation                       */
      /**********************************************************************/

      vec pol, r, r2, r3, outVec, Nf, absNf, Ye;
      uvec_type J, K, uN, dTAdjusted_32_63, dT_32_63, dT_0_31;
      vec_type N, minusN;
      c7x::double_vec KVals_8_15, KVals_0_7, JVals_8_15, JVals_0_7, dTVals_8_15, dTVals_0_7, pol_0_7, pol_8_15,
          outVec_0_7, outVec_8_15;
      c7x::uint_vec upperBitsK, lowerBitsK, upperBitsJ, lowerBitsJ;

      Ye = k10e * inVec;

      // Get N such that |N - inVec*16/ln(2)| is minimized
      Nf = inVec * log2_base_x16;
      absNf = Nf + half;
      N = c7x::convert<vec_type>(absNf);
      minusN = N - 1;

      //    N--;
      __vpred cmp_N = __cmp_lt_pred(Nf, negativeHalf);
      N = __select(cmp_N, minusN, N);

      /**********************************************************************/
      /* Calculate Taylor series approximation for exp10                      */
      /**********************************************************************/
      r = (Ye - (P1 * __int_to_float(N))) - (P2 * __int_to_float(N));
      // Taylor series approximation
      r2 = r * r;
      r3 = r2 * r;
      pol = (r * C2) + ((r3 * C0) + (r2 * C1));

      /**********************************************************************/
      /* Get index of LUT and 2^M values                                    */
      /**********************************************************************/

      // Create vectors of LUT indices
      uN = c7x::convert<uvec_type>(N);
      K = ((uN << 28u) >> 30) + AUDIOLIB_KTABLE_OFFSET;
      J = (uN & mask) + AUDIOLIB_JTABLE_OFFSET;

      // Read values from LUT and convert and store as doubles in split vectors

      upperBitsK = AUDIOLIB_ILUTReadUpperBits(K);
      lowerBitsK = AUDIOLIB_ILUTReadLowerBits(K);
      upperBitsJ = AUDIOLIB_ILUTReadUpperBits(J);
      lowerBitsJ = AUDIOLIB_ILUTReadLowerBits(J);

      KVals_8_15 = c7x::reinterpret<c7x::double_vec>(__permute_high_high(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(upperBitsK), c7x::as_uchar_vec(lowerBitsK)));
      KVals_0_7 = c7x::reinterpret<c7x::double_vec>(__permute_low_low(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(upperBitsK), c7x::as_uchar_vec(lowerBitsK)));
      JVals_8_15 = c7x::reinterpret<c7x::double_vec>(__permute_high_high(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(upperBitsJ), c7x::as_uchar_vec(lowerBitsJ)));
      JVals_0_7 = c7x::reinterpret<c7x::double_vec>(__permute_low_low(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(upperBitsJ), c7x::as_uchar_vec(lowerBitsJ)));

      // Multiply LUT values
      dTVals_8_15 = KVals_8_15 * JVals_8_15;
      dTVals_0_7 = KVals_0_7 * JVals_0_7;

      /**********************************************************************/
      /* Scale exponent to adjust for 2^M                                   */
      /**********************************************************************/

      // Upper 32 bits of all dT values and lower 32 bits of all dT values
      dT_32_63 = c7x::as_uint_vec(__permute_odd_odd_int(AUDIOLIB_vperm_data_0_63, c7x::as_uchar_vec(dTVals_8_15),
                                                        c7x::as_uchar_vec(dTVals_0_7)));
      dT_0_31 = c7x::as_uint_vec(__permute_even_even_int(AUDIOLIB_vperm_data_0_63, c7x::as_uchar_vec(dTVals_8_15),
                                                         c7x::as_uchar_vec(dTVals_0_7)));

      uN = (uN >> 4) << 20;
      dTAdjusted_32_63 = dT_32_63 + uN;

      // Concatenate the adjusted upper 32 bits of dT values to the lower 32 bits, convert dT to float
      dTVals_8_15 = c7x::reinterpret<c7x::double_vec>(__permute_high_high(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(dTAdjusted_32_63), c7x::as_uchar_vec(dT_0_31)));
      dTVals_0_7 = c7x::reinterpret<c7x::double_vec>(__permute_low_low(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(dTAdjusted_32_63), c7x::as_uchar_vec(dT_0_31)));

      // TODO: adjust calculation so that DT and POL are doubles then out is float

      pol_0_7 = c7x::reinterpret<c7x::double_vec>(__permute_low_low(AUDIOLIB_vperm_data_dp_interweave_0_63,
                                                                    c7x::as_uchar_vec(__high_float_to_double(pol)),
                                                                    c7x::as_uchar_vec(__low_float_to_double(pol))));
      pol_8_15 = c7x::reinterpret<c7x::double_vec>(__permute_high_high(AUDIOLIB_vperm_data_dp_interweave_0_63,
                                                                       c7x::as_uchar_vec(__high_float_to_double(pol)),
                                                                       c7x::as_uchar_vec(__low_float_to_double(pol))));

      outVec_0_7 = dTVals_0_7 * (1.0f + pol_0_7);
      outVec_8_15 = dTVals_8_15 * (1.0f + pol_8_15);

      outVec = c7x::reinterpret<vec>(__permute_even_even_int(AUDIOLIB_vperm_data_0_63,
                                                             c7x::as_uchar_vec(__double_to_float(outVec_8_15)),
                                                             c7x::as_uchar_vec(__double_to_float(outVec_0_7))));

      __vpred tmp = c7x::strm_agen<0, vec>::get_vpred();
      vec *addr = c7x::strm_agen<0, vec>::get_adv(pDst);
      __vstore_pred(tmp, addr, outVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_undB20_cond(handle, &se0Params, &sa0Params, pSrc, pDst);

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_undB20_vector_sp_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
#endif

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_undB20_scaler_1x1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   dataType *restrict pSrc = (dataType *) pIn;
   dataType *restrict pDst = (dataType *) pOut;

   pDst[0] = AUDIOLIB_undB20_scalar_ci<dataType>(pSrc[0]);

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_undB20_scaler_1x1_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_undB20_scaler_Mx1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_undB20_PrivArgs *pKerPrivArgs = (AUDIOLIB_undB20_PrivArgs *) handle;

   uint32_t numChannel = pKerPrivArgs->dim_y;
   uint32_t inStride   = pKerPrivArgs->inStride;
   uint32_t outStride  = pKerPrivArgs->outStride;

   dataType *restrict pSrc = (dataType *) pIn;
   dataType *restrict pDst = (dataType *) pOut;

   uint32_t inOffset  = 0;
   uint32_t outOffset = 0;

   for (uint32_t idx = 0; idx < numChannel; idx++) {
      pDst[outOffset] = AUDIOLIB_undB20_scalar_ci<dataType>(pSrc[inOffset]);
      outOffset += outStride;
      inOffset += inStride;
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_undB20_scaler_Mx1_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
