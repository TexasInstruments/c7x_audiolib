// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_whiteNoiseGenerator_priv.h"

// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_init_ci(AUDIOLIB_kernelHandle                        handle,
                                                     const AUDIOLIB_bufParams2D_t                *bufParamsOut,
                                                     const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_whiteNoiseGenerator_init_ci\n");
#endif
   typedef typename c7x::make_full_vector<dataType>::type vec;
   typedef typename c7x::make_full_vector<uint32_t>::type uintVec;

   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;
   AUDIOLIB_STATUS                        status       = AUDIOLIB_SUCCESS;
   uint8_t                               *pBlock       = pKerPrivArgs->bufPblock;
   int32_t                                eleCount     = c7x::element_count_of<vec>::value;
   __SA_TEMPLATE_v1                       sa0Params    = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1                       sa1Params    = __gen_SA_TEMPLATE_v1();

   // 2^-31 is a power of two, so the product is exact: (int32_t) (x ^ 2^31) * scale = (x / 2^31 - 1) * range
   pKerPrivArgs->scale = AUDIOLIB_LCG_SIGNED_NORM_FACTOR_F32 * pKerInitArgs->range;

   /* SA0 stores one channel per row: dim 0 is one vector, dim 1 the row's vectors (whole iterations),
    * dim 2 the channels. DECDIM1 starts every row with `samples` valid elements and deducts one vector
    * per dim-1 step, so lanes past the end of the row get a false predicate and are not written. */
   sa0Params.VECLEN        = c7x::sa_veclen<vec>::value;
   sa0Params.DIMFMT        = __SA_DIMFMT_3D;
   sa0Params.ICNT0         = eleCount;
   sa0Params.ICNT1         = pKerPrivArgs->numIterations * pKerPrivArgs->numVectors;
   sa0Params.DIM1          = eleCount;
   sa0Params.ICNT2         = pKerPrivArgs->numChannels;
   sa0Params.DIM2          = pKerPrivArgs->strideOut;
   sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa0Params.DECDIM1_WIDTH = pKerPrivArgs->samples;

   /* SA1 is never dereferenced; it only supplies one predicate per loop iteration. Same shape with one
    * vector per iteration: DECDIM1 leaves (numIterations - 1) vectors valid per row, so the predicate is all
    * true inside a row and all false on its last iteration. A one-iteration row has no inside iterations;
    * init then makes the iteration step equal the row jump, and SA1 keeps a full row valid. */
   int32_t validIterations = (pKerPrivArgs->numIterations > 1) ? (pKerPrivArgs->numIterations - 1) : 1;

   sa1Params.VECLEN        = c7x::sa_veclen<uintVec>::value;
   sa1Params.DIMFMT        = __SA_DIMFMT_3D;
   sa1Params.ICNT0         = eleCount;
   sa1Params.ICNT1         = pKerPrivArgs->numIterations;
   sa1Params.DIM1          = eleCount;
   sa1Params.ICNT2         = pKerPrivArgs->numChannels;
   sa1Params.DIM2          = 0;
   sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
   sa1Params.DECDIM1_WIDTH = (uint32_t) (validIterations * eleCount);

   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_whiteNoiseGenerator_init_ci<float>(AUDIOLIB_kernelHandle                        handle,
                                            const AUDIOLIB_bufParams2D_t                *bufParamsOut,
                                            const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs);

// Converts one vector of top-bit-flipped LCG outputs to noise in [-range, +range] and stores it through SA0.
template <typename dataType, typename uintVec, typename floatVec>
static inline void
AUDIOLIB_whiteNoiseGenerator_storeNoise(uintVec centredOut, floatVec scaleVec, dataType *restrict pOutLocal)
{
   floatVec  output = __int_to_float(c7x::as_int_vec(centredOut)) * scaleVec;
   __vpred   tmp    = c7x::strm_agen<0, floatVec>::get_vpred();
   floatVec *VB1    = c7x::strm_agen<0, floatVec>::get_adv(pOutLocal);
   __vstore_pred(tmp, VB1, output);
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_unroll5_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_whiteNoiseGenerator_unroll5_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<uint32_t>::type uintVec;
   typedef typename c7x::make_full_vector<dataType>::type floatVec;

   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;

   AUDIOLIB_STATUS status           = AUDIOLIB_SUCCESS;
   uint8_t        *pBlock           = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal     = (dataType *) pOut;
   __SA_TEMPLATE_v1 sa0Params       = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params       = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   int32_t          totalIterations = pKerPrivArgs->totalIterations;
   uintVec         *pStates         = stov_ptr(uintVec, pKerPrivArgs->states);
   const uint32_t  *pVecMult        = pKerPrivArgs->vectorMultiplier;
   const uint32_t  *pVecAdd         = pKerPrivArgs->vectorIncrement;

   uintVec  iterMult    = (uintVec) (pKerPrivArgs->iterationMultiplier);
   uintVec  iterAdd     = (uintVec) (pKerPrivArgs->iterationIncrement);
   uintVec  rowJumpMult = (uintVec) (pKerPrivArgs->rowJumpMultiplier);
   uintVec  rowJumpAdd  = (uintVec) (pKerPrivArgs->rowJumpIncrement);
   floatVec scaleVec    = (floatVec) (pKerPrivArgs->scale);

   /* The state vectors hold the LCG outputs with the top bit flipped: (int32_t) (x ^ 2^31) = x - 2^31 is the
    * output centred on zero, so one convert and one multiply give the noise. The flipped states follow the
    * same jumps: (x + 2^31) * A + C = (x * A + C) + 2^31 (mod 2^32), because every multiplier A = a^k is odd.
    * State vector v starts (v - 1) * STATE_INDEX steps after the first. */
   uintVec state1 = *pStates ^ (uintVec) (AUDIOLIB_LCG_SIGN_FLIP);
   uintVec state2 = state1 * (uintVec) (pVecMult[1]) + (uintVec) (pVecAdd[1]);
   uintVec state3 = state1 * (uintVec) (pVecMult[2]) + (uintVec) (pVecAdd[2]);
   uintVec state4 = state1 * (uintVec) (pVecMult[3]) + (uintVec) (pVecAdd[3]);
   uintVec state5 = state1 * (uintVec) (pVecMult[4]) + (uintVec) (pVecAdd[4]);

   // channel 0 continues `samples` steps on in the next call; done here, so nothing waits for the loop
   *pStates = *pStates * (uintVec) (pKerPrivArgs->jumpHeadMultiplierNextFrame) +
              (uintVec) (pKerPrivArgs->jumpHeadIncNextFrame);

   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   /* ii = 5 cycles per 5 vectors (40 samples), 1 per vector, for float (C7504 and C7524, from the compiled .asm).
    * Bound by the .M/.N units (two multiplies per vector, VMPYWW and VMPYSP, on the two vector multiply units) and
    * the .D2 store port (one VST8W per vector); the loop-carried multiply-add of each state (4 + 1) is also 5.
    * The pipeline fills and drains once per call (12 cycles). */
   for (int32_t i = 0; i < totalIterations; i++) {
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state1, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state2, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state3, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state4, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state5, scaleVec, pOutLocal);

      // true inside a row, false on its last iteration: there, jump to the next channel's first outputs
      __vpred inRow = c7x::strm_agen<1, uintVec>::get_vpred();
      (void) c7x::strm_agen<1, uintVec>::get_adv(pOutLocal);
      uintVec stepMult = __select(inRow, iterMult, rowJumpMult);
      uintVec stepAdd  = __select(inRow, iterAdd, rowJumpAdd);

      state1 = state1 * stepMult + stepAdd;
      state2 = state2 * stepMult + stepAdd;
      state3 = state3 * stepMult + stepAdd;
      state4 = state4 * stepMult + stepAdd;
      state5 = state5 * stepMult + stepAdd;
   }

   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_unroll5_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                             void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_unroll8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_whiteNoiseGenerator_unroll8_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<uint32_t>::type uintVec;
   typedef typename c7x::make_full_vector<dataType>::type floatVec;

   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;

   AUDIOLIB_STATUS status           = AUDIOLIB_SUCCESS;
   uint8_t        *pBlock           = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal     = (dataType *) pOut;
   __SA_TEMPLATE_v1 sa0Params       = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params       = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   int32_t          totalIterations = pKerPrivArgs->totalIterations;
   uintVec         *pStates         = stov_ptr(uintVec, pKerPrivArgs->states);
   const uint32_t  *pVecMult        = pKerPrivArgs->vectorMultiplier;
   const uint32_t  *pVecAdd         = pKerPrivArgs->vectorIncrement;

   uintVec  iterMult    = (uintVec) (pKerPrivArgs->iterationMultiplier);
   uintVec  iterAdd     = (uintVec) (pKerPrivArgs->iterationIncrement);
   uintVec  rowJumpMult = (uintVec) (pKerPrivArgs->rowJumpMultiplier);
   uintVec  rowJumpAdd  = (uintVec) (pKerPrivArgs->rowJumpIncrement);
   floatVec scaleVec    = (floatVec) (pKerPrivArgs->scale);

   /* The state vectors hold the LCG outputs with the top bit flipped: (int32_t) (x ^ 2^31) = x - 2^31 is the
    * output centred on zero, so one convert and one multiply give the noise. The flipped states follow the
    * same jumps: (x + 2^31) * A + C = (x * A + C) + 2^31 (mod 2^32), because every multiplier A = a^k is odd.
    * State vector v starts (v - 1) * STATE_INDEX steps after the first. */
   uintVec state1 = *pStates ^ (uintVec) (AUDIOLIB_LCG_SIGN_FLIP);
   uintVec state2 = state1 * (uintVec) (pVecMult[1]) + (uintVec) (pVecAdd[1]);
   uintVec state3 = state1 * (uintVec) (pVecMult[2]) + (uintVec) (pVecAdd[2]);
   uintVec state4 = state1 * (uintVec) (pVecMult[3]) + (uintVec) (pVecAdd[3]);
   uintVec state5 = state1 * (uintVec) (pVecMult[4]) + (uintVec) (pVecAdd[4]);
   uintVec state6 = state1 * (uintVec) (pVecMult[5]) + (uintVec) (pVecAdd[5]);
   uintVec state7 = state1 * (uintVec) (pVecMult[6]) + (uintVec) (pVecAdd[6]);
   uintVec state8 = state1 * (uintVec) (pVecMult[7]) + (uintVec) (pVecAdd[7]);

   // channel 0 continues `samples` steps on in the next call; done here, so nothing waits for the loop
   *pStates = *pStates * (uintVec) (pKerPrivArgs->jumpHeadMultiplierNextFrame) +
              (uintVec) (pKerPrivArgs->jumpHeadIncNextFrame);

   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   /* ii = 8 cycles per 8 vectors (64 samples), 1 per vector, for float (C7504 and C7524, from the compiled .asm).
    * Same bound as the 5-vector loop: .M/.N units and the .D2 store port. The pipeline fills and drains once per
    * call (12 cycles). */
   for (int32_t i = 0; i < totalIterations; i++) {
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state1, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state2, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state3, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state4, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state5, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state6, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state7, scaleVec, pOutLocal);
      AUDIOLIB_whiteNoiseGenerator_storeNoise(state8, scaleVec, pOutLocal);

      // true inside a row, false on its last iteration: there, jump to the next channel's first outputs
      __vpred inRow = c7x::strm_agen<1, uintVec>::get_vpred();
      (void) c7x::strm_agen<1, uintVec>::get_adv(pOutLocal);
      uintVec stepMult = __select(inRow, iterMult, rowJumpMult);
      uintVec stepAdd  = __select(inRow, iterAdd, rowJumpAdd);

      state1 = state1 * stepMult + stepAdd;
      state2 = state2 * stepMult + stepAdd;
      state3 = state3 * stepMult + stepAdd;
      state4 = state4 * stepMult + stepAdd;
      state5 = state5 * stepMult + stepAdd;
      state6 = state6 * stepMult + stepAdd;
      state7 = state7 * stepMult + stepAdd;
      state8 = state8 * stepMult + stepAdd;
   }

   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_unroll8_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                             void *restrict pOut);

void AUDIOLIB_whiteNoiseGenerator_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;

   // archCycles: the loop alone (fill and drain plus one cycle per vector); estCycles: the same model init used
   *archCycles += AUDIOLIB_WHITENOISEGENERATOR_PIPE_UP_CYCLES + (uint64_t) pKerPrivArgs->totalIterations *
                                                                    (uint64_t) pKerPrivArgs->numVectors *
                                                                    AUDIOLIB_WHITENOISEGENERATOR_CYCLES_PER_VECTOR;
   *estCycles += AUDIOLIB_whiteNoiseGenerator_execCycles(pKerPrivArgs->numChannels, pKerPrivArgs->samples,
                                                         pKerPrivArgs->numVectors);
}
