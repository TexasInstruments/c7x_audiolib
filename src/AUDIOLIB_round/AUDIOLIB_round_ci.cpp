// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/*******************************************************************************
 *
 * INCLUDES
 *
 ******************************************************************************/
#include "AUDIOLIB_round_priv.h"

void AUDIOLIB_round_perfEst(AUDIOLIB_kernelHandle handle,
                            uint64_t             *archCycles,
                            uint64_t             *estCycles,
                            uint32_t              inDataType,
                            uint32_t              outDataType)
{
   const AUDIOLIB_round_PrivArgs *pKerPrivArgs = (const AUDIOLIB_round_PrivArgs *) handle;

   /* AUDIOLIB_round_init_ci stores the modelled cycles of the loop it picked. */
   *archCycles = pKerPrivArgs->loopCycles;
   *estCycles  = pKerPrivArgs->loopCycles + AUDIOLIB_ROUND_CALL_OVERHEAD_CYCLES;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_round_init_ci(AUDIOLIB_kernelHandle          handle,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                       const AUDIOLIB_round_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_round_PrivArgs                               *pKerPrivArgs = (AUDIOLIB_round_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   se0Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;
   se0Params.ELETYPE = SE_ELETYPE;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_round_init_ci samples %d channels %d strideIn %d strideOut %d eleCount %d\n",
                         pKerPrivArgs->samples, pKerPrivArgs->channels, pKerPrivArgs->strideIn,
                         pKerPrivArgs->strideOut, eleCount);

   if ((pKerPrivArgs->strideIn == pKerPrivArgs->samples) && (pKerPrivArgs->strideOut == pKerPrivArgs->samples)) {
      /* Gap free on both sides, so the frame is one contiguous stream. */
      se0Params.ICNT0  = pKerPrivArgs->channels * pKerPrivArgs->samples;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0  = pKerPrivArgs->channels * pKerPrivArgs->samples;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;

      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples * pKerPrivArgs->channels, eleCount);
   }
   else {
      /* Row pitch differs from row length, so stream row by row. */
      se0Params.ICNT0  = pKerPrivArgs->samples;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.ICNT1  = pKerPrivArgs->channels;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = pKerPrivArgs->samples;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->channels;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      pKerPrivArgs->nVecs = pKerPrivArgs->channels * AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   /* The loop over groups of 16 vectors is faster per vector but pads the
    * frame up to a multiple of 16; the single vector loop runs the exact count.
    * Pick the one with fewer modelled loop cycles for this frame. */
   uint64_t pipeUpCycles1  = (sizeof(dataType) == sizeof(float)) ? AUDIOLIB_ROUND_SP_PIPE_UP_CYCLES_1
                                                                  : AUDIOLIB_ROUND_DP_PIPE_UP_CYCLES_1;
   uint64_t pipeUpCycles16 = (sizeof(dataType) == sizeof(float)) ? AUDIOLIB_ROUND_SP_PIPE_UP_CYCLES_16
                                                                  : AUDIOLIB_ROUND_DP_PIPE_UP_CYCLES_16;
   uint64_t loopCycles1    = pipeUpCycles1 + (uint64_t) pKerPrivArgs->nVecs * AUDIOLIB_ROUND_CYCLES_PER_VEC;
   uint64_t loopCycles16   = pipeUpCycles16 + (uint64_t) AUDIOLIB_ceilingDiv(pKerPrivArgs->nVecs, 16U) *
                                                AUDIOLIB_ROUND_CYCLES_PER_16_VECS;

   if (loopCycles16 < loopCycles1) {
      pKerPrivArgs->execute    = AUDIOLIB_round_exec_ci<dataType, 16U>;
      pKerPrivArgs->loopCycles = loopCycles16;
   }
   else {
      pKerPrivArgs->execute    = AUDIOLIB_round_exec_ci<dataType, 1U>;
      pKerPrivArgs->loopCycles = loopCycles1;
   }

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting AUDIOLIB_round_init_ci");

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_round_init_ci<float>(AUDIOLIB_kernelHandle          handle,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                       const AUDIOLIB_round_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_round_init_ci<double>(AUDIOLIB_kernelHandle          handle,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                        const AUDIOLIB_round_InitArgs *pKerInitArgs);

template <typename dataType, uint32_t vecsPerIter>
AUDIOLIB_STATUS AUDIOLIB_round_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_round_PrivArgs *pKerPrivArgs = (AUDIOLIB_round_PrivArgs *) handle;
   uint8_t                 *pBlock       = pKerPrivArgs->bufPblock;
   int32_t                  nIter        = (int32_t) AUDIOLIB_ceilingDiv(pKerPrivArgs->nVecs, vecsPerIter);

   typedef typename c7x::make_full_vector<dataType>::type vec;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_round_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   vec vecNoFractionBound = (vec) AUDIOLIB_roundNoFractionBound<dataType>();

   /* The bit pattern of -0.0 is the sign bit alone, for float and double
    * lanes alike; its complement keeps the magnitude bits. */
   c7x::uint_vec vecSignMask      = c7x::reinterpret<c7x::uint_vec>((vec) -0.0);
   c7x::uint_vec vecMagnitudeMask = ~vecSignMask;

   /* SE1 streams the same input again for the sign, so no register has to
    * hold the input across the add / subtract chain. */
   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   /* ii = 2 cycles per vector for vecsPerIter 1 and ii = 29 cycles per 16
    * vectors (1.8125 per vector) for vecsPerIter 16, for float and double on
    * C7504 and C7524 (from the compiled .asm). Bound by .C2/.L2/.M2/.S2: every
    * vector needs 7 vector ops on these 4 units, so the resource bound is 1.75
    * per vector (vecsPerIter 1: 7 ops on whole cycles gives 2; vecsPerIter 16:
    * bound 28, the scheduler finds 29). Surplus vectors of the last group of 16
    * read zeros past the streams and their stores are masked by the SA. */
#pragma MUST_ITERATE(1, , 1)
   for (int32_t iter = 0; iter < nIter; iter++) {
      /* Fully unrolls the group of 1 or 16 vectors; the pragma takes a literal only. */
#pragma UNROLL(16)
      for (int32_t vecIdx = 0; vecIdx < (int32_t) vecsPerIter; vecIdx++) {
         vec vecIn     = c7x::strm_eng<0, vec>::get_adv();
         vec vecInSign = c7x::strm_eng<1, vec>::get_adv();

         /* Round the magnitude to nearest, ties to even: adding 2^mantissaBits
          * pushes the fraction out, subtracting it returns the integer. */
         vec vecInAbs   = c7x::reinterpret<vec>(c7x::reinterpret<c7x::uint_vec>(vecIn) & vecMagnitudeMask);
         vec vecNearest = (vecInAbs + vecNoFractionBound) - vecNoFractionBound;

         /* At or above the bound every value is already integral (as are inf
          * and NaN), and the add would carry into the next binade. */
         __vpred cmpHasFraction = __cmp_lt_pred(vecInAbs, vecNoFractionBound);
         vec     vecMagnitude   = __select(cmpHasFraction, vecNearest, vecInAbs);

         /* Put the input sign back, which also keeps -0.0 for inputs in (-0.5, -0]. */
         c7x::uint_vec vecSign = c7x::reinterpret<c7x::uint_vec>(vecInSign) & vecSignMask;
         vec           vecOut  = c7x::reinterpret<vec>(c7x::reinterpret<c7x::uint_vec>(vecMagnitude) | vecSign);

         __vpred predStore = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore    = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(predStore, pStore, vecOut);
      }
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting AUDIOLIB_round_exec_ci");

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_round_exec_ci<float, 1U>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_round_exec_ci<float, 16U>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_round_exec_ci<double, 1U>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_round_exec_ci<double, 16U>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
