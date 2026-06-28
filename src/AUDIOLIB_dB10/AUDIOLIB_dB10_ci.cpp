// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/AUDIOLIB_ilut.h"
#include "../common/AUDIOLIB_permute.h"
#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_dB10_priv.h"
#include "c6x_migration.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)

const double AUDIOLIB_logTable[8] = {0.0000000000,  -0.1177830356, -0.2231435513, -0.3184537311,
                                     -0.4054651081, -0.4855078157, -0.5596157879, -0.6286086594};

static inline c7x::double_vec cmn_DIVDP_opt(c7x::double_vec a, c7x::double_vec b)
{
   c7x::double_vec Two = (c7x::double_vec)(2.0f);
   c7x::double_vec X;
   X = __recip(b);
   X = X * (Two - (b * X));
   X = X * (Two - (b * X));
   X = X * (Two - (b * X));
   X = a * X;

   return X;
}

static inline double cmn_DIVDP(double a, double b)
{
   double TWO = 2.0f;
   double X;
   X = __recip(b);
   X = X * (TWO - (b * X));
   X = X * (TWO - (b * X));
   X = X * (TWO - (b * X));
   X = a * X;

   return X;
}

void AUDIOLIB_dB10_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;
   uint32_t                dataType     = pKerPrivArgs->dataType;
   uint32_t                numSamples   = pKerPrivArgs->dim_x;
   uint32_t                numChannels  = pKerPrivArgs->dim_y;
#if defined(__C7504__)
   uint32_t inStride  = pKerPrivArgs->inStride;
   uint32_t outStride = pKerPrivArgs->outStride;
#endif

   uint64_t dB10StartupCycles   = 0;
   uint64_t dB10TeardownCycles  = 0;
   uint64_t dB10OverheadCycles  = 0;
   uint32_t nVecs               = 0;
   uint64_t dB10OperationCycles = 0;

#if defined(__C7524__)
   {
      if (dataType == AUDIOLIB_FLOAT64) // Double(inStride/64)
      {
         if (numSamples == 1 && numChannels == 1) {
            dB10OperationCycles = 90;
         }
         else if (numSamples < 32) {
            dB10StartupCycles  = 18;
            dB10TeardownCycles = 3;

            nVecs               = pKerPrivArgs->nVecs;
            dB10OperationCycles = (111 + nVecs * 28) + (31 + nVecs * 7);
         }
         else {
            dB10StartupCycles  = 22;
            dB10TeardownCycles = 4;

            nVecs               = pKerPrivArgs->nVecs;
            dB10OperationCycles = (47 + nVecs * 10) + (63 + nVecs * 14) + (31 + nVecs * 7);
         }
      }
      else if (dataType == AUDIOLIB_FLOAT32) // Float
      {
         if (numSamples == 1) {
            if (numChannels == 1) {
               dB10OperationCycles = 49;
            }
            else {
               nVecs               = numChannels * numSamples;
               dB10OperationCycles = (67 + nVecs * 12);
            }
         }
         else {
            dB10StartupCycles   = 10;
            dB10TeardownCycles  = 1;
            nVecs               = pKerPrivArgs->nVecs;
            dB10OperationCycles = (47 + nVecs * 23);
         }
      }
   }
#elif defined(__C7504__)
   {
      if (dataType == AUDIOLIB_FLOAT64) // double
      {
         if (numSamples == 1 && numChannels == 1) {
            dB10OperationCycles = 90;
         }
         else if (numSamples < 32) {
            dB10StartupCycles  = 18;
            dB10TeardownCycles = 3;

            nVecs               = pKerPrivArgs->nVecs;
            dB10OperationCycles = (111 + nVecs * 28) + (31 + nVecs * 7);
         }
         else {
            dB10StartupCycles  = 22;
            dB10TeardownCycles = 4;

            nVecs               = pKerPrivArgs->nVecs;
            dB10OperationCycles = (47 + nVecs * 10) + (63 + nVecs * 14) + (31 + nVecs * 7);
         }
      }
      if (dataType == AUDIOLIB_FLOAT32) { // Float
         nVecs = numChannels * numSamples;
         if (numSamples == 1) {
            if (numChannels == 1) {
               dB10OperationCycles = 49;
            }
            else {
               dB10OperationCycles = (67 + nVecs * 12);
            }
         }
         else if ((numSamples == inStride && numSamples == outStride) || (numSamples > 1 && numChannels == 1)) {
            dB10OperationCycles = (50 + nVecs * 20);
         }
         else {
            dB10OperationCycles = (62 + nVecs * 14);
         }
      }
   }
#endif

   dB10OverheadCycles = dB10StartupCycles + dB10TeardownCycles;
   *estCycles         = dB10OperationCycles + dB10OverheadCycles;
   *archCycles        = dB10OperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_dB10_init_ci(AUDIOLIB_kernelHandle         handle,
                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      const AUDIOLIB_dB10_InitArgs *pKerInitArgs)
{
#if defined(__C7524__)
   AUDIOLIB_ILUTInit();
#endif

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;

   // derive c7x vector type from template typename
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

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
   if (pKerPrivArgs->dim_x == pKerPrivArgs->inStride && pKerPrivArgs->dim_x == pKerPrivArgs->outStride) {

      uint32_t blockSize = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;

      se0Params.ICNT0  = blockSize;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0  = blockSize;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;
#if defined(__C7524__)
      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(blockSize, eleCount);
#elif defined(__C7504__)
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32)
         pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;
      else
         pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(blockSize, eleCount);

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
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32)
         pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;
      else
         pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
#endif
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_dB10_init_ci<float>(AUDIOLIB_kernelHandle         handle,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                      const AUDIOLIB_dB10_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_dB10_init_ci<double>(AUDIOLIB_kernelHandle         handle,
                                                       const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                       const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                       const AUDIOLIB_dB10_InitArgs *pKerInitArgs);

inline double divdpMod_dB10dpi(double a, double b) { return cmn_DIVDP(a, b); }

template <typename dataType> static inline dataType AUDIOLIB_dB10_scalar_ci(dataType a);

template <> inline double AUDIOLIB_dB10_scalar_ci(double a)
{
   double       Half   = 0.5;
   double       MAXe   = 1.7976931348623157e+308;
   double       srHalf = 0.70710678118654752440; /* sqrt(0.5) */
   double       MINe   = 2.2250738585072014e-308;
   double       a0     = -0.64124943423745581147e+2;
   double       a1     = 0.16383943563021534222e+2;
   double       a2     = -0.78956112887491257267e+0;
   double       b0     = -0.76949932108494879777e+3;
   double       b1     = 0.31203222091924532844e+3;
   double       b2     = -0.35667977739034646171e+2; /* Note b3 = 1.0 */
   double       c1     = 0.693359375;                /*  355/512      */
   double       c2     = -2.121944400546905827679e-4;
   double       c10e   = 0.43429448190325182765; /* log (base 10) of e */
   double       W, X, Y, Z;
   double       zn, zd;
   double       Rz, Sa, Bd, Cn, Da;
   int          N, exp_;
   unsigned int upper;

   /* get unbiased exponent */
   Y    = a;
   exp_ = (int) _extu(_hi(Y), 1u, 21u);
   N    = exp_ - 1022;

   /* force DP exp = 1022 if not zero */
   upper = _clr(_hi(Y), 20u, 31u);
   upper = 0x3fe00000u | upper;
   Z     = _itod(upper, _lo(Y));

   if (exp_ == 0) {
      Z = 0.0;
   }

   if (Z > srHalf) {
      zn = (Z - Half) - Half;
      zd = (Z * Half) + Half;
   }
   else {
      zn = Z - Half;
      zd = (zn * Half) + Half;
      N  = N - 1;
   }

   X  = divdpMod_dB10dpi(zn, zd);
   W  = X * X;
   Bd = ((((W + b2) * W) + b1) * W) + b0;
   Cn = (((W * a2) + a1) * W) + a0;
   Rz = W * divdpMod_dB10dpi(Cn, Bd);
   Sa = X + (X * Rz);
   Cn = (double) N;
   Da = ((Cn * c2) + Sa) + (Cn * c1);
   Da = c10e * Da;

   if (Y < MINe) {
      Da = CONST_DOUBLE_MIN_DB10; // out[in0 <= 0.0] = np.uint64(0xFFF0000000000000)
   }
   if (Y > MAXe) {
      Da = CONST_MAX_DB10;
   }

   return (10 * Da);
}

template <> inline float AUDIOLIB_dB10_scalar_ci(float a)
{
   double       ln2  = 0.693147180559945f;
   double       base = 0.4342944819033f;
   float        c1   = -0.2302894f;
   float        c2   = 0.1908169f;
   float        c3   = -0.2505905f;
   float        c4   = 0.3333164f;
   float        c5   = -0.5000002f;
   float        MAXe = 3.402823466E+38f;
   float        pol, r1, r2, r3, r4, res;
   double       dr, frcpax, rcp, T;
   unsigned int T_index;
   int          N;

   /* r = x * frcpa(x) -1 */
   rcp    = _rcpdp((double) a);
   frcpax = _itod(_clr(_hi(rcp), 0u, 16u), 0u);
   dr     = (frcpax * (double) a) - 1.0;

   /* Polynomial p(r) that approximates ln(1+r) - r */
   r1 = (float) dr;
   r2 = r1 * r1;
   r3 = r1 * r2;
   r4 = r2 * r2;

   pol = (c5 * r2) + ((c4 * r3) + ((((c2 * r1) + c3) + (c1 * r2)) * r4));
   pol *= (float) base;

   /* Reconstruction: result = T + r + p(r) */
   N       = (int) _extu(_hi(frcpax), 1u, 21u) - 1023;
   T_index = _extu(_hi(frcpax), 12u, 29u);

   T = (AUDIOLIB_logTable[T_index] - (ln2 * (double) N)) * base;

   res = (float) ((dr * base) + T) + pol;

   if (a <= 0.0f) {
      res = CONST_FLOAT_MIN_DB10; // out[in0 <= 0.0] = np.uint32(0xFF800000)
   }
   if (a > MAXe) {
      res = CONST_MAX_DB10;
   }

   return (10 * res);
}

#if defined(__C7504__)
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_dB10_scaler_sp_1xN_exec_ci(AUDIOLIB_kernelHandle handle,
                                                    void *restrict pIn,
                                                    void *restrict pOut) // AUDIOLIB_dB10_scaler_sp_1xN_exec_ci
{
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;

   float *restrict pSrc = (float *) pIn;
   float *restrict pDst = (float *) pOut;

   for (uint32_t idx = 0; idx < pKerPrivArgs->nVecs; idx++) {
      pDst[idx] = AUDIOLIB_dB10_scalar_ci<dataType>(pSrc[idx]);
   }
   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_dB10_scaler_sp_1xN_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_dB10_scaler_sp_MxN_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;

   uint32_t inStride  = pKerPrivArgs->inStride;
   uint32_t outStride = pKerPrivArgs->outStride;

   float *restrict pSrc = (float *) pIn;
   float *restrict pDst = (float *) pOut;

   uint32_t idx       = 0;
   uint32_t inOffset  = 0;
   uint32_t outOffset = 0;

   for (uint32_t i = 0; i < pKerPrivArgs->nVecs; i++) {
      pDst[outOffset + idx] = AUDIOLIB_dB10_scalar_ci<dataType>(pSrc[inOffset + idx]);
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
AUDIOLIB_dB10_scaler_sp_MxN_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

#elif defined(__C7524__)
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_dB10_vector_sp_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;
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
   /* Create and assign values for constants employed on dB10 computation */
   /***********************************************************************/

   vec C1, C2, C3, C4, C5, eMax, outVecMin, outVecMax;
   c7x::double_vec ln2, base;
   c7x::float_vec basef, db;
   c7x::uint_vec zero;
   zero = (c7x::uint_vec) 0;

   ln2 = (c7x::double_vec) 0.693147180559945;
   base = (c7x::double_vec) 0.4342944819033f;
   basef = (vec) 0.434294f;
   C1 = (vec) -0.2302894f;
   C2 = (vec) 0.1908169f;
   C3 = (vec) -0.2505905f;
   C4 = (vec) 0.3333164f;
   C5 = (vec) -0.5000002f;
   eMax = (vec) 3.402823466e+38f;
   outVecMin = (vec) 0xFF800000u;
   db = (vec) 10.0f;
   outVecMax = (vec) CONST_MAX_DB10;

   // compute loop to perform vector dB10
   for (size_t i = 0; i < numBlocks; i++) {
      vec inVec = c7x::strm_eng<0, vec>::get_adv();

      /**********************************************************************/
      /* Create variables employed on dB10 computation                      */
      /**********************************************************************/
      vec pol, r1, r2, r3, r4;
      c7x::double_vec inVecVals_odd, inVecVals_even, inVecVals_oddReciprocal, inVecVals_evenReciprocal,
          inVecReciprocalApprox_8_15, inVecReciprocalApprox_0_7, inVecVals_8_15, inVecVals_0_7, rVals_0_7, rVals_8_15,
          TVals_8_15, TVals_0_7, NVals_odd, NVals_even, NVals_0_7, NVals_8_15, outVec_8_15, outVec_0_7;
      c7x::uint_vec inVecReciprocal_32_63, inVecReciprocalClr_32_63, inVecReciprocalApprox_32_63, indexT;
      c7x::int_vec N;
      vec outVec;

      /**********************************************************************/
      /* Calculate Taylor series approximation for dB10                     */
      /**********************************************************************/

      // Split vectors to compute r with double precision
      inVecVals_odd = __high_float_to_double(inVec);
      inVecVals_even = __low_float_to_double(inVec);
      inVecVals_oddReciprocal = __recip(inVecVals_odd);
      inVecVals_evenReciprocal = __recip(inVecVals_even);

      // Create floating point reciprocal approximation
      // Upper 32 bits of all inVec reciprocal values
      inVecReciprocal_32_63 = c7x::reinterpret<c7x::uint_vec>(
          __permute_odd_odd_int(AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(inVecVals_oddReciprocal),
                                c7x::as_uchar_vec(inVecVals_evenReciprocal)));
      // Clear bits 0-16 inclusive
      inVecReciprocalClr_32_63 = inVecReciprocal_32_63 & 0xFFFE0000u;

      // Concatenate cleared bit reciprocal with zero bits
      inVecReciprocalApprox_8_15 = c7x::reinterpret<c7x::double_vec>(__permute_high_high(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(inVecReciprocalClr_32_63), c7x::as_uchar_vec(zero)));

      inVecReciprocalApprox_0_7 = c7x::reinterpret<c7x::double_vec>(__permute_low_low(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(inVecReciprocalClr_32_63), c7x::as_uchar_vec(zero)));

      // Split inVec into two vectors with double precision
      inVecVals_0_7 = c7x::reinterpret<c7x::double_vec>(__permute_low_low(
          AUDIOLIB_vperm_data_dp_interweave_0_63, c7x::as_uchar_vec(inVecVals_odd), c7x::as_uchar_vec(inVecVals_even)));
      inVecVals_8_15 = c7x::reinterpret<c7x::double_vec>(__permute_high_high(
          AUDIOLIB_vperm_data_dp_interweave_0_63, c7x::as_uchar_vec(inVecVals_odd), c7x::as_uchar_vec(inVecVals_even)));

      // Calculate r in double precision
      rVals_0_7 = (inVecReciprocalApprox_0_7 * inVecVals_0_7) - 1.0;
      rVals_8_15 = (inVecReciprocalApprox_8_15 * inVecVals_8_15) - 1.0;

      // Convert r to float, compute r to the power of 2, 3, 4
      r1 = c7x::reinterpret<vec>(__permute_even_even_int(AUDIOLIB_vperm_data_0_63,
                                                         c7x::as_uchar_vec(__double_to_float(rVals_8_15)),
                                                         c7x::as_uchar_vec(__double_to_float(rVals_0_7))));
      r2 = r1 * r1;
      r3 = r1 * r2;
      r4 = r2 * r2;

      // Compute Taylor series polynomial
      pol = (C5 * r2) + ((C4 * r3) + ((((C2 * r1) + C3) + (C1 * r2)) * r4));
      pol = pol * basef;

      /**********************************************************************/
      /* Calculate N                                                        */
      /**********************************************************************/

      // Upper 32 bits of all inVec reciprocal approximation values
      inVecReciprocalApprox_32_63 = c7x::reinterpret<c7x::uint_vec>(
          __permute_odd_odd_int(AUDIOLIB_vperm_data_0_63, c7x::as_uchar_vec(inVecReciprocalApprox_8_15),
                                c7x::as_uchar_vec(inVecReciprocalApprox_0_7)));

      N = c7x::convert<c7x::int_vec>(((inVecReciprocalApprox_32_63 << 1) >> 21) - 1023);

      // Covert N to double precision for later calculation with LUT values
      NVals_odd = __high_int_to_double(N);
      NVals_even = __low_int_to_double(N);
      NVals_0_7 = c7x::reinterpret<c7x::double_vec>(__permute_low_low(
          AUDIOLIB_vperm_data_dp_interweave_0_63, c7x::as_uchar_vec(NVals_odd), c7x::as_uchar_vec(NVals_even)));
      NVals_8_15 = c7x::reinterpret<c7x::double_vec>(__permute_high_high(
          AUDIOLIB_vperm_data_dp_interweave_0_63, c7x::as_uchar_vec(NVals_odd), c7x::as_uchar_vec(NVals_even)));

      /**********************************************************************/
      /* Determine LUT values                                               */
      /**********************************************************************/

      // Calculate LUT index
      indexT = (((inVecReciprocalApprox_32_63 << 12) >> 29) + 8);

      // Read from LUT or ILUT and reconstruct double values split into two vectors

      c7x::uint_vec upperBitsIndexT = AUDIOLIB_ILUTReadUpperBits(indexT);
      c7x::uint_vec lowerBitsIndexT = AUDIOLIB_ILUTReadLowerBits(indexT);

      // Read from LUT and reconstruct double values split into two vectors
      TVals_8_15 = c7x::reinterpret<c7x::double_vec>(__permute_high_high(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(upperBitsIndexT), c7x::as_uchar_vec(lowerBitsIndexT)));
      TVals_0_7 = c7x::reinterpret<c7x::double_vec>(__permute_low_low(
          AUDIOLIB_vperm_data_interweave_0_63, c7x::as_uchar_vec(upperBitsIndexT), c7x::as_uchar_vec(lowerBitsIndexT)));

      // Calculate an adjusted T
      TVals_8_15 = (TVals_8_15 - (ln2 * NVals_8_15)) * base;
      TVals_0_7 = (TVals_0_7 - (ln2 * NVals_0_7)) * base;

      /**********************************************************************/
      /* Calculate output with adjusted LUT and Taylor series values        */
      /**********************************************************************/

      // Add LUT values and Taylor series values
      // TODO: Multiple by base
      outVec_0_7 = (rVals_0_7 * base) + TVals_0_7;
      outVec_8_15 = (rVals_8_15 * base) + TVals_8_15;

      // Combine output vector into one floating point result
      outVec = c7x::reinterpret<vec>(__permute_even_even_int(AUDIOLIB_vperm_data_0_63,
                                                             c7x::as_uchar_vec(__double_to_float(outVec_8_15)),
                                                             c7x::as_uchar_vec(__double_to_float(outVec_0_7))));

      outVec = outVec + pol;
      // outVec.print();

      /*****************************************************************/
      /* Bounds checking                                                    */
      /**********************************************************************/

      __vpred cmp_min = __cmp_le_pred(inVec, c7x::convert<vec>(zero));
      outVec = __select(cmp_min, outVecMin, outVec);

      __vpred cmp_max = __cmp_lt_pred(eMax, inVec);
      outVec = __select(cmp_max, outVecMax, outVec);

      outVec = outVec * db;

      __vpred tmp = c7x::strm_agen<0, vec>::get_vpred();
      vec *addr = c7x::strm_agen<0, vec>::get_adv(pDst);
      __vstore_pred(tmp, addr, outVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_dB10_vector_sp_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
#endif

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_dB10_vector_dp_split1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;

   uint8_t *pBlock      = pKerPrivArgs->bufPblock;
   uint32_t numChannels = pKerPrivArgs->dim_y;
   uint32_t numSamples  = pKerPrivArgs->dim_x;

   double *restrict pSrc = (double *) pIn;
   double *restrict pDst = (double *) pOut;

   // variables
   size_t numBlocks    = 0; // compute loop's iteration count
   size_t remNumBlocks = 0; // when numBlocks is not a multiple of SIMD width

   // derive c7x vector type from template typename
   typedef typename c7x::make_full_vector<double>::type vec;

   __SE_TEMPLATE_v1 se0Params;
   __SA_TEMPLATE_v1 sa0Params;

   se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   // calculate compute loop's iteration counter
   numBlocks    = pKerPrivArgs->nVecs;
   remNumBlocks = numSamples % c7x::element_count_of<vec>::value;
   if (remNumBlocks) {
      numBlocks += numChannels;
   }
   // open SE0, SE1, and SA0 for reading and writing operands

   __SE0_OPEN(pSrc, se0Params);
   __SA0_OPEN(sa0Params);

   /***********************************************************************/
   /* Create and assign values for constants employed on dB10 computation */
   /***********************************************************************/

   vec Half, MAXe, srHalf, Half_sq, MINe, a0, a1, a2, b0, b1, b2, c1, c2, c10e, W, X, Y, Z, zn, zd, Rz, Sa, Bd, Cn, Da,
       cons;

   Half    = (vec) 0.5;
   Half_sq = (vec) 0.5 * 0.5;
   MAXe    = (vec) 1.7976931348623157e+308;
   srHalf  = (vec) 0.70710678118654752440; /* sqrt(0.5) */
   MINe    = (vec) 2.2250738585072014e-308;
   a0      = (vec) -0.64124943423745581147e+2;
   a1      = (vec) 0.16383943563021534222e+2;
   a2      = (vec) -0.78956112887491257267e+0;
   b0      = (vec) -0.76949932108494879777e+3;
   b1      = (vec) 0.31203222091924532844e+3;
   b2      = (vec) -0.35667977739034646171e+2; /* Note b3 = 1.0 */
   c1      = (vec) 0.693359375;                /*  355/512      */
   c2      = (vec) -2.121944400546905827679e-4;
   c10e    = (vec) 0.43429448190325182765; /* log (base 10) of e */
   cons    = (vec) CONST_DOUBLE_MIN_DB10;

   c7x::long_vec long_zero_vec   = (c7x::long_vec) 0;
   vec           double_zero_vec = (vec) 0.0;
   vec           outMAX          = (vec) (CONST_MAX_DB10);

   // compute loop to perform vector dB10
   for (size_t i = 0; i < numBlocks; i++) {

      vec a = c7x::strm_eng<0, vec>::get_adv();

      Y                  = a;
      c7x::long_vec exp_ = c7x::as_long_vec((c7x::as_ulong_vec(Y) << 1) >> 53);

      c7x::ulong_vec upper = c7x::as_ulong_vec(Y) & (0x000FFFFF00000000u); // Extract the upper 20 bits of Mantissa
      upper                = 0x3FE0000000000000u | upper;

      Z            = c7x::as_double_vec((0x00000000FFFFFFFFu & c7x::as_ulong_vec(Y)) | upper);
      __vpred cmp1 = __cmp_eq_pred(exp_, long_zero_vec);

      Z = __select(cmp1, double_zero_vec, Z); //(exp,1,0)

      vec z_minus_half = Z - Half;
      vec z_mul_half   = (Z * Half) + Half;

      __vpred cmp2 = __cmp_lt_pred(srHalf, Z);

      zn = __select(cmp2, (z_minus_half - Half), z_minus_half);
      zd = __select(cmp2, z_mul_half, (z_mul_half - Half_sq));

      X  = cmn_DIVDP_opt(zn, zd);
      W  = X * X;
      Bd = ((((W + b2) * W) + b1) * W) + b0;
      Cn = (((W * a2) + a1) * W) + a0;
      Rz = W * cmn_DIVDP_opt(Cn, Bd);
      Sa = X + (X * Rz);

      __vpred tmp  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr = c7x::strm_agen<0, vec>::get_adv(pDst);
      __vstore_pred(tmp, addr, Sa);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   // open SE0, SE1, and SA0 for reading and writing operands
   __SE0_OPEN(pSrc, se0Params);
   se0Params.DIM1 = pKerPrivArgs->outStride;
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pDst, se0Params);

   for (size_t i = 0; i < numBlocks; i++) {

      vec a = c7x::strm_eng<0, vec>::get_adv();
      Sa    = c7x::strm_eng<1, vec>::get_adv();

      Y                  = a;
      c7x::long_vec exp_ = c7x::as_long_vec((c7x::as_ulong_vec(Y) << 1) >> 53);
      c7x::long_vec N    = exp_ - 1022;

      c7x::ulong_vec upper = c7x::as_ulong_vec(Y) & (0x000FFFFF00000000u);
      upper                = 0x3FE0000000000000u | upper;

      Z = c7x::as_double_vec((0x00000000FFFFFFFFu & c7x::as_ulong_vec(Y)) | upper);

      __vpred cmp1 = __cmp_eq_pred(exp_, long_zero_vec);
      Z            = __select(cmp1, double_zero_vec, Z);

      __vpred cmp2 = __cmp_lt_pred(srHalf, Z);
      N            = __select(cmp2, N, (N - 1));
      Cn           = __low_int_to_double(c7x::as_int_vec(N));
      Da           = ((Cn * c2) + Sa) + (Cn * c1);
      Da           = c10e * Da;

      /**********************************************************************/
      /* Bounds checking                                                    */
      /**********************************************************************/

      __vpred cmp_min = __cmp_lt_pred(Y, MINe);
      Da              = __select(cmp_min, cons, Da);

      __vpred cmp_max = __cmp_lt_pred(MAXe, Y);
      Da              = __select(cmp_max, outMAX, Da);

      __vpred tmp  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr = c7x::strm_agen<0, vec>::get_adv(pDst);
      __vstore_pred(tmp, addr, 10 * Da);
   }
   __SE1_CLOSE();
   __SE0_CLOSE();
   __SA0_CLOSE();
   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_dB10_vector_dp_split1_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_dB10_vector_dp_split2_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;

   uint8_t *pBlock     = pKerPrivArgs->bufPblock;
   uint32_t numChannel = pKerPrivArgs->dim_y;
   uint32_t numSamples = pKerPrivArgs->dim_x;

   double *restrict pSrc = (double *) pIn;
   double *restrict pDst = (double *) pOut;

   // variables
   size_t numBlocks    = 0; // compute loop's iteration count
   size_t remNumBlocks = 0; // when numBlocks is not a multiple of SIMD width

   // derive c7x vector type from template typename
   typedef typename c7x::make_full_vector<double>::type vec;

   __SE_TEMPLATE_v1 se0Params;
   __SA_TEMPLATE_v1 sa0Params;

   se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   // calculate compute loop's iteration counter
   numBlocks    = pKerPrivArgs->nVecs;
   remNumBlocks = numSamples % c7x::element_count_of<vec>::value;

   if (remNumBlocks) {
      numBlocks += numChannel;
   }

   // open SE0, SE1, and SA0 for reading and writing operands
   __SE0_OPEN(pSrc, se0Params);
   __SA0_OPEN(sa0Params);

   /***********************************************************************/
   /* Create and assign values for constants employed on dB10 computation */
   /***********************************************************************/

   vec Half, MAXe, srHalf, Half_sq, MINe, a0, a1, a2, b0, b1, b2, c1, c2, c10e, W, X, Y, Z, zn, zd, Rz, Sa, Bd, Cn, Da,
       cons;

   Half    = (vec) 0.5;
   Half_sq = (vec) 0.5 * 0.5;
   MAXe    = (vec) 1.7976931348623157e+308;
   srHalf  = (vec) 0.70710678118654752440; /* sqrt(0.5) */
   MINe    = (vec) 2.2250738585072014e-308;
   a0      = (vec) -0.64124943423745581147e+2;
   a1      = (vec) 0.16383943563021534222e+2;
   a2      = (vec) -0.78956112887491257267e+0;
   b0      = (vec) -0.76949932108494879777e+3;
   b1      = (vec) 0.31203222091924532844e+3;
   b2      = (vec) -0.35667977739034646171e+2; /* Note b3 = 1.0 */
   c1      = (vec) 0.693359375;                /*  355/512      */
   c2      = (vec) -2.121944400546905827679e-4;
   c10e    = (vec) 0.43429448190325182765; /* log (base 10) of e */
   cons    = (vec) CONST_DOUBLE_MIN_DB10;

   c7x::long_vec long_zero_vec   = (c7x::long_vec) 0;
   vec           double_zero_vec = (vec) 0.0;
   vec           outMAX          = (vec) (CONST_MAX_DB10);

   // compute loop to perform vector dB10
   for (size_t i = 0; i < numBlocks; i++) {

      vec a = c7x::strm_eng<0, vec>::get_adv();

      Y                  = a;
      c7x::long_vec exp_ = c7x::as_long_vec((c7x::as_ulong_vec(Y) << 1) >> 53);

      c7x::ulong_vec upper = c7x::as_ulong_vec(Y) & (0x000FFFFF00000000u);
      upper                = 0x3FE0000000000000u | upper;

      Z = c7x::as_double_vec((0x00000000FFFFFFFFu & c7x::as_ulong_vec(Y)) | upper);

      __vpred cmp1 = __cmp_eq_pred(exp_, long_zero_vec);
      Z            = __select(cmp1, double_zero_vec, Z);

      vec     z_minus_half = Z - Half;
      vec     z_mul_half   = (Z * Half) + Half;
      __vpred cmp2         = __cmp_lt_pred(srHalf, Z);
      zn                   = __select(cmp2, (z_minus_half - Half), z_minus_half);
      zd                   = __select(cmp2, z_mul_half, (z_mul_half - Half_sq));

      X = cmn_DIVDP_opt(zn, zd);

      __vpred tmp  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr = c7x::strm_agen<0, vec>::get_adv(pDst);
      __vstore_pred(tmp, addr, X);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   // open SE0, SE1, and SA0 for reading and writing operands
   se0Params.DIM1 = pKerPrivArgs->outStride;
   __SE0_OPEN(pDst, se0Params);
   __SA0_OPEN(sa0Params);

   for (size_t i = 0; i < numBlocks; i++) {

      X = c7x::strm_eng<0, vec>::get_adv();

      W  = X * X;
      Bd = ((((W + b2) * W) + b1) * W) + b0;
      Cn = (((W * a2) + a1) * W) + a0;
      Rz = W * cmn_DIVDP_opt(Cn, Bd);
      Sa = X + (X * Rz);

      __vpred tmp  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr = c7x::strm_agen<0, vec>::get_adv(pDst);
      __vstore_pred(tmp, addr, Sa);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   // open SE0, SE1, and SA0 for reading and writing operands
   __SE1_OPEN(pDst, se0Params);
   se0Params.DIM1 = pKerPrivArgs->inStride;
   __SE0_OPEN(pSrc, se0Params);
   __SA0_OPEN(sa0Params);

   for (size_t i = 0; i < numBlocks; i++) {

      vec a = c7x::strm_eng<0, vec>::get_adv();
      Sa    = c7x::strm_eng<1, vec>::get_adv();

      Y                  = a;
      c7x::long_vec exp_ = c7x::as_long_vec((c7x::as_ulong_vec(Y) << 1) >> 53);
      c7x::long_vec N    = exp_ - 1022;

      c7x::ulong_vec upper = c7x::as_ulong_vec(Y) & (0x000FFFFF00000000u);
      upper                = 0x3FE0000000000000u | upper;

      Z = c7x::as_double_vec((0x00000000FFFFFFFFu & c7x::as_ulong_vec(Y)) | upper);

      __vpred cmp1 = __cmp_eq_pred(exp_, long_zero_vec);
      Z            = __select(cmp1, double_zero_vec, Z);

      __vpred cmp2 = __cmp_lt_pred(srHalf, Z);

      N = __select(cmp2, N, (N - 1));

      Cn = __low_int_to_double(c7x::as_int_vec(N));
      Da = ((Cn * c2) + Sa) + (Cn * c1);
      Da = c10e * Da;

      /**********************************************************************/
      /* Bounds checking                                                    */
      /**********************************************************************/

      __vpred cmp_min = __cmp_lt_pred(Y, MINe);
      Da              = __select(cmp_min, cons, Da);

      __vpred cmp_max = __cmp_lt_pred(MAXe, Y);
      Da              = __select(cmp_max, outMAX, Da);

      __vpred tmp  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr = c7x::strm_agen<0, vec>::get_adv(pDst);
      __vstore_pred(tmp, addr, 10 * Da);
   }
   __SE1_CLOSE();
   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_dB10_vector_dp_split2_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_dB10_scaler_1x1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut) // 1x1
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   dataType *restrict pSrc = (dataType *) pIn;
   dataType *restrict pDst = (dataType *) pOut;

   pDst[0] = AUDIOLIB_dB10_scalar_ci<dataType>(pSrc[0]);

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_dB10_scaler_1x1_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_dB10_scaler_1x1_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_dB10_scaler_Mx1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_dB10_PrivArgs *pKerPrivArgs = (AUDIOLIB_dB10_PrivArgs *) handle;

   uint32_t numChannel = pKerPrivArgs->dim_y;
   uint32_t inStride   = pKerPrivArgs->inStride;
   uint32_t outStride  = pKerPrivArgs->outStride;

   dataType *restrict pSrc = (dataType *) pIn;
   dataType *restrict pDst = (dataType *) pOut;

   uint32_t inOffset  = 0;
   uint32_t outOffset = 0;

   for (uint32_t idx = 0; idx < numChannel; idx++) {
      pDst[outOffset] = AUDIOLIB_dB10_scalar_ci<dataType>(pSrc[inOffset]);
      outOffset += outStride;
      inOffset += inStride;
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_dB10_scaler_Mx1_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
