// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/AUDIOLIB_inlines.h"
#include "AUDIOLIB_agc_priv.h"
#include "c6x_migration.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE2_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA0_PARAM_OFFSET (SE_SE2_PARAM_OFFSET + SE_PARAM_SIZE)

void AUDIOLIB_agc_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   uint64_t                   agcStartupCycles   = 0;
   uint64_t                   agcControlCycles   = 0;
   uint64_t                   agcTeardownCycles  = 0;
   uint64_t                   agcOperationCycles = 0;
   uint64_t                   agcOverheadCycles  = 0;
   uint64_t                   rowCycles              = 0;
   AUDIOLIB_agc_PrivArgs *pKerPrivArgs           = (AUDIOLIB_agc_PrivArgs *) handle;

   if (pKerPrivArgs->gainComputed != 0U) {
      agcControlCycles = AUDIOLIB_AGC_ACTIVE_CONTROL_CYCLES;
   }
   else {
      agcControlCycles = AUDIOLIB_AGC_INACTIVE_CONTROL_CYCLES;
   }

   uint64_t nIterIn        = pKerPrivArgs->nIterIn;
   uint64_t nIterPerRowOut = pKerPrivArgs->nIterPerRowOut;

   if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
      agcOperationCycles =
          AUDIOLIB_AGC_DP_ENERGY_LOOP_OVERHEAD_CYCLES + (AUDIOLIB_AGC_DP_ENERGY_LOOP_II * nIterIn);
      agcStartupCycles = AUDIOLIB_AGC_DP_ENTRY_CYCLES + AUDIOLIB_AGC_DP_ENERGY_TO_GAIN_CYCLES +
                             AUDIOLIB_AGC_DP_OUTPUT_SETUP_CYCLES;
      agcTeardownCycles = AUDIOLIB_AGC_DP_TEARDOWN_CYCLES;
      rowCycles             = AUDIOLIB_AGC_DP_ROW_LOOP_CONTROL_CYCLES + AUDIOLIB_AGC_DP_OUTPUT_LOOP_PROLOG_CYCLES +
                  (AUDIOLIB_AGC_DP_OUTPUT_LOOP_II * nIterPerRowOut) + AUDIOLIB_AGC_DP_OUTPUT_LOOP_EPILOG_CYCLES;
   }
   else {
      agcOperationCycles =
          AUDIOLIB_AGC_SP_ENERGY_LOOP_OVERHEAD_CYCLES + (AUDIOLIB_AGC_SP_ENERGY_LOOP_II * nIterIn);
      agcStartupCycles = AUDIOLIB_AGC_SP_ENTRY_CYCLES + AUDIOLIB_AGC_SP_ENERGY_TO_GAIN_CYCLES +
                             AUDIOLIB_AGC_SP_OUTPUT_SETUP_CYCLES;
      agcTeardownCycles = AUDIOLIB_AGC_SP_TEARDOWN_CYCLES;
      rowCycles             = AUDIOLIB_AGC_SP_ROW_LOOP_CONTROL_CYCLES + AUDIOLIB_AGC_SP_OUTPUT_LOOP_PROLOG_CYCLES +
                  (AUDIOLIB_AGC_SP_OUTPUT_LOOP_II * nIterPerRowOut) + AUDIOLIB_AGC_SP_OUTPUT_LOOP_EPILOG_CYCLES;
   }
   agcOperationCycles += rowCycles * pKerPrivArgs->nRowsOut;

   agcOverheadCycles = agcStartupCycles + agcControlCycles + agcTeardownCycles;
   *estCycles            = agcOperationCycles + agcOverheadCycles;
   *archCycles           = agcOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_agc_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_agc_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_agc_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_agc_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_agc_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideIn %d eleCount %d\n", pKerPrivArgs->dim_x, pKerPrivArgs->dim_y,
                         pKerPrivArgs->strideIn, eleCount);

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   se0Params.ELETYPE = c7x::se_eletype<vec>::value;
   se0Params.VECLEN  = c7x::se_veclen<vec>::value;
   se1Params.ELETYPE = c7x::se_eletype<vec>::value;
   se1Params.VECLEN  = c7x::se_veclen<vec>::value;
   se2Params.ELETYPE = c7x::se_eletype<vec>::value;
   se2Params.VECLEN  = c7x::se_veclen<vec>::value;
   sa0Params.VECLEN  = c7x::sa_veclen<vec>::value;

   uint32_t dimX        = pKerPrivArgs->dim_x;
   uint32_t dimY        = pKerPrivArgs->dim_y;
   int32_t  strideIn    = pKerPrivArgs->strideIn;
   int32_t  strideOut   = pKerPrivArgs->strideOut;
   uint32_t unroll      = AUDIOLIB_AGC_UNROLL;
   uint32_t blockSize   = dimX * dimY;
   uint32_t nVecsPerRow = AUDIOLIB_ceilingDiv(dimX, eleCount);
   bool     isPackedIn  = (dimY == 1U) || ((int32_t) dimX == strideIn);
   bool     isPackedOut = (dimY == 1U) || ((int32_t) dimX == strideOut);

   /* Energy pass: SE0 reads the first half of the block and SE1 the second half, unroll vectors from each
    * per iteration. Both streams are padded with zero vectors up to nIterIn * unroll. */
   if (isPackedIn) {
      /* One row of nIterIn * unroll vectors per stream, split at sample halfSize */
      uint32_t nIterIn  = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(blockSize, eleCount), 2U * unroll);
      uint32_t halfSize = nIterIn * unroll * eleCount;
      uint32_t rowLen1  = (blockSize > halfSize) ? (blockSize - halfSize) : 0U;

      se0Params.ICNT0         = eleCount;
      se0Params.ICNT1         = nIterIn * unroll;
      se0Params.DIM1          = (int32_t) eleCount;
      se0Params.ICNT2         = 1U;
      se0Params.DIM2          = halfSize;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.DECDIM1_WIDTH = (rowLen1 > 0U) ? halfSize : blockSize;
      se0Params.DECDIM2       = __SE_DECDIM_DIM2;
      se0Params.DECDIM2_WIDTH = halfSize;

      se1Params.ICNT0         = eleCount;
      se1Params.ICNT1         = nIterIn * unroll;
      se1Params.DIM1          = (int32_t) eleCount;
      se1Params.ICNT2         = 1U;
      se1Params.DIM2          = halfSize;
      se1Params.DIMFMT        = __SE_DIMFMT_3D;
      se1Params.DECDIM1       = __SE_DECDIM_DIM1;
      se1Params.DECDIM1_WIDTH = rowLen1;
      se1Params.DECDIM2       = __SE_DECDIM_DIM2;
      se1Params.DECDIM2_WIDTH = halfSize;

      pKerPrivArgs->nIterIn = nIterIn;
      /* With nothing left for SE1 it streams only fill values; keep its base inside the buffer */
      pKerPrivArgs->in1Offset = (rowLen1 > 0U) ? (int32_t) halfSize : 0;
   }
   else {
      /* SE0 reads rows [0, rows0), SE1 rows [rows0, dim_y) */
      uint32_t rows0     = AUDIOLIB_ceilingDiv(dimY, 2U);
      uint32_t rows1     = dimY - rows0;
      uint32_t nIterIn   = AUDIOLIB_ceilingDiv(rows0 * nVecsPerRow, unroll);
      uint32_t rowsTotal = AUDIOLIB_ceilingDiv(nIterIn * unroll, nVecsPerRow);

      se0Params.ICNT0         = eleCount;
      se0Params.ICNT1         = nVecsPerRow;
      se0Params.DIM1          = (int32_t) eleCount;
      se0Params.ICNT2         = rowsTotal;
      se0Params.DIM2          = strideIn;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.DECDIM1_WIDTH = dimX;
      se0Params.DECDIM2       = __SE_DECDIM_DIM2;
      se0Params.DECDIM2_WIDTH = rows0 * (uint32_t) strideIn;

      se1Params.ICNT0         = eleCount;
      se1Params.ICNT1         = nVecsPerRow;
      se1Params.DIM1          = (int32_t) eleCount;
      se1Params.ICNT2         = rowsTotal;
      se1Params.DIM2          = strideIn;
      se1Params.DIMFMT        = __SE_DIMFMT_3D;
      se1Params.DECDIM1       = __SE_DECDIM_DIM1;
      se1Params.DECDIM1_WIDTH = dimX;
      se1Params.DECDIM2       = __SE_DECDIM_DIM2;
      se1Params.DECDIM2_WIDTH = rows1 * (uint32_t) strideIn;

      pKerPrivArgs->nIterIn   = nIterIn;
      pKerPrivArgs->in1Offset = (int32_t) rows0 * strideIn;
   }

   /* Output pass: SE2 (opened on SE0) and SA0 split the samples into identical vectors, padded up to a whole
    * number of unrolled iterations; SE2 zero-fills and SA0 predicates off the padding. */
   if (isPackedIn && isPackedOut) {
      uint32_t nIterOut = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(blockSize, eleCount), unroll);
      uint32_t rowSize  = nIterOut * unroll * eleCount;

      se2Params.ICNT0         = eleCount;
      se2Params.ICNT1         = nIterOut * unroll;
      se2Params.DIM1          = (int32_t) eleCount;
      se2Params.ICNT2         = 1U;
      se2Params.DIM2          = rowSize;
      se2Params.DIMFMT        = __SE_DIMFMT_3D;
      se2Params.DECDIM1       = __SE_DECDIM_DIM1;
      se2Params.DECDIM1_WIDTH = blockSize;
      se2Params.DECDIM2       = __SE_DECDIM_DIM2;
      se2Params.DECDIM2_WIDTH = rowSize;

      sa0Params.ICNT0         = eleCount;
      sa0Params.ICNT1         = nIterOut * unroll;
      sa0Params.DIM1          = (int32_t) eleCount;
      sa0Params.ICNT2         = 1U;
      sa0Params.DIM2          = rowSize;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = blockSize;
      sa0Params.DECDIM2       = __SA_DECDIM_DIM2;
      sa0Params.DECDIM2_WIDTH = rowSize;

      pKerPrivArgs->nRowsOut       = 1U;
      pKerPrivArgs->nIterPerRowOut = nIterOut;
   }
   else if ((dimX % eleCount) == 0U) {
      /* Rows hold whole vectors only, so the coefficient chain runs on seamlessly from one row into the
       * next and the rows are walked as a single row, padded with zero rows */
      uint32_t nIterOut  = AUDIOLIB_ceilingDiv(dimY * nVecsPerRow, unroll);
      uint32_t rowsTotal = AUDIOLIB_ceilingDiv(nIterOut * unroll, nVecsPerRow);

      se2Params.ICNT0         = eleCount;
      se2Params.ICNT1         = nVecsPerRow;
      se2Params.DIM1          = (int32_t) eleCount;
      se2Params.ICNT2         = rowsTotal;
      se2Params.DIM2          = strideIn;
      se2Params.DIMFMT        = __SE_DIMFMT_3D;
      se2Params.DECDIM1       = __SE_DECDIM_DIM1;
      se2Params.DECDIM1_WIDTH = dimX;
      se2Params.DECDIM2       = __SE_DECDIM_DIM2;
      se2Params.DECDIM2_WIDTH = dimY * (uint32_t) strideIn;

      sa0Params.ICNT0         = eleCount;
      sa0Params.ICNT1         = nVecsPerRow;
      sa0Params.DIM1          = (int32_t) eleCount;
      sa0Params.ICNT2         = rowsTotal;
      sa0Params.DIM2          = strideOut;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = dimX;
      sa0Params.DECDIM2       = __SA_DECDIM_DIM2;
      sa0Params.DECDIM2_WIDTH = dimY * (uint32_t) strideOut;

      pKerPrivArgs->nRowsOut       = 1U;
      pKerPrivArgs->nIterPerRowOut = nIterOut;
   }
   else {
      /* The tail of each row is padded, so the coefficients restart at decay^(m * dim_x) per row */
      uint32_t nIterPerRow = AUDIOLIB_ceilingDiv(nVecsPerRow, unroll);

      se2Params.ICNT0         = eleCount;
      se2Params.ICNT1         = nIterPerRow * unroll;
      se2Params.DIM1          = (int32_t) eleCount;
      se2Params.ICNT2         = dimY;
      se2Params.DIM2          = strideIn;
      se2Params.DIMFMT        = __SE_DIMFMT_3D;
      se2Params.DECDIM1       = __SE_DECDIM_DIM1;
      se2Params.DECDIM1_WIDTH = dimX;
      se2Params.DECDIM2       = __SE_DECDIM_DIM2;
      se2Params.DECDIM2_WIDTH = dimY * (uint32_t) strideIn;

      sa0Params.ICNT0         = eleCount;
      sa0Params.ICNT1         = nIterPerRow * unroll;
      sa0Params.DIM1          = (int32_t) eleCount;
      sa0Params.ICNT2         = dimY;
      sa0Params.DIM2          = strideOut;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = dimX;
      sa0Params.DECDIM2       = __SA_DECDIM_DIM2;
      sa0Params.DECDIM2_WIDTH = dimY * (uint32_t) strideOut;

      pKerPrivArgs->nRowsOut       = dimY;
      pKerPrivArgs->nIterPerRowOut = nIterPerRow;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_agc_init_ci<float>(AUDIOLIB_kernelHandle            handle,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                                         const AUDIOLIB_agc_InitArgs *pKerInitArgs);
template AUDIOLIB_STATUS AUDIOLIB_agc_init_ci<double>(AUDIOLIB_kernelHandle            handle,
                                                          const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                                          const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                                          const AUDIOLIB_agc_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_agc_set_ci(AUDIOLIB_kernelHandle handle, const AUDIOLIB_agc_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_agc_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_agc_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   /* decayPow is written as AUDIOLIB_AGC_UNROLL * eleCount values of dataType */
   static_assert((AUDIOLIB_AGC_UNROLL * c7x::element_count_of<vec>::value * sizeof(dataType)) <=
                     sizeof(((AUDIOLIB_agc_PrivArgs *) 0)->decayPow),
                 "AUDIOLIB_agc: decayPow too small for AUDIOLIB_AGC_UNROLL vectors of dataType");

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_agc_set_ci \n");

   /* Lane i of output vector j (j < AUDIOLIB_AGC_UNROLL) is decay^(j * eleCount + i + 1). Powers are
    * evaluated in double and rounded once to the kernel data type. */
   dataType *decayPow = (dataType *) pKerPrivArgs->decayPow;
   double    decay    = (double) (1.0f - pKerPrivArgs->smoothingCoeffSample);
   for (uint32_t i = 0; i < (AUDIOLIB_AGC_UNROLL * eleCount); i++) {
      decayPow[i] = (dataType) pow(decay, (double) (i + 1U));
   }

   /* Coefficient steps by one unrolled iteration and by one row */
   *(dataType *) &pKerPrivArgs->decayPowUnroll = (dataType) pow(decay, (double) (AUDIOLIB_AGC_UNROLL * eleCount));
   *(dataType *) &pKerPrivArgs->decayPowRow    = (dataType) pow(decay, (double) pKerPrivArgs->dim_x);

   /* Target gain in the linear domain: level >= threshold  <=>  energy >= 10^(threshold/10), and
    * 10^(((targetLevel - 10 log10(E)) * s) / 20) = 10^(targetLevel * s / 20) * E^(-s/2) */
   pKerPrivArgs->thrEnergy = powf(10.0f, pKerSetArgs->activationThreshold * 0.1f);
   pKerPrivArgs->gainScale = powf(10.0f, (pKerSetArgs->targetLevel * pKerPrivArgs->oneOverSlope) * 0.05f);
   pKerPrivArgs->gainExp   = -0.5f * pKerPrivArgs->oneOverSlope;
   pKerPrivArgs->gainMin   = powf(10.0f, -pKerSetArgs->maxAttenuation * 0.05f);
   pKerPrivArgs->gainMax   = powf(10.0f, pKerSetArgs->maxGain * 0.05f);

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_agc_set_ci<float>(AUDIOLIB_kernelHandle           handle,
                                                        const AUDIOLIB_agc_SetArgs *pKerSetArgs);
template AUDIOLIB_STATUS AUDIOLIB_agc_set_ci<double>(AUDIOLIB_kernelHandle           handle,
                                                         const AUDIOLIB_agc_SetArgs *pKerSetArgs);

/* log2(x) */
static inline float AUDIOLIB_agc_log2_ci(float x)
{
   uint32_t bits = _ftoi(x);
   int32_t  e    = (int32_t) ((bits >> 23U) & 0xFFU) - 127;
   uint32_t mant = bits & 0x007FFFFFU;

   /* Mantissa above sqrt(2): halve it and bump the exponent */
   if (mant > 0x003504F3U) {
      mant |= 0x3F000000U;
      e++;
   }
   else {
      mant |= 0x3F800000U;
   }

   float t = _itof(mant) - 1.0f;
   float p = -0.14620529f;
   p       = (p * t) + 0.23421262f;
   p       = (p * t) - 0.24882235f;
   p       = (p * t) + 0.28707531f;
   p       = (p * t) - 0.36024192f;
   p       = (p * t) + 0.48092404f;
   p       = (p * t) - 0.72135276f;
   p       = (p * t) + 1.44269490f;

   return (float) e + (t * p);
}

/* 2^y for y in [-126, 127] */
static inline float AUDIOLIB_agc_exp2_ci(float y)
{
   y = (y < -126.0f) ? -126.0f : y;
   y = (y > 127.0f) ? 127.0f : y;

   int32_t n = (int32_t) y;
   n         = ((float) n > y) ? (n - 1) : n;

   float f = y - (float) n;
   float q = 0.0018951079f;
   q       = (q * f) + 0.0089462195f;
   q       = (q * f) + 0.0558632687f;
   q       = (q * f) + 0.2401407808f;
   q       = (q * f) + 0.6931546330f;
   q       = (q * f) + 0.9999998808f;

   return q * _itof((uint32_t) (n + 127) << 23U);
}

/* Block-rate update of the smoothed energy and the target gain in the linear domain:
 *   level >= threshold        <=>  E >= 10^(threshold/10)
 *   10^(gainDB/20)            =   gainScale * E^gainExp
 *   clamp in dB               <=>  clamp to [gainMin, gainMax]
 * which replaces log10f + powf by one inline log2/exp2 pair. */
static inline void AUDIOLIB_agc_updateTargetGain_ci(AUDIOLIB_agc_PrivArgs *pKerPrivArgs, float energySum)
{
   const AUDIOLIB_agc_SetArgs *pPrm = &pKerPrivArgs->setArgs;

   float blockEnergy    = energySum * pKerPrivArgs->oneOverNumSamples;
   float energy         = (pKerPrivArgs->energy * (1.0f - pKerPrivArgs->smoothingCoeffBlock)) +
                          (blockEnergy * pKerPrivArgs->smoothingCoeffBlock);
   pKerPrivArgs->energy = energy;

   /* Zero energy is -inf dB, which is always below the activation threshold */
   uint32_t active = ((energy > 0.0f) && (energy >= pKerPrivArgs->thrEnergy)) ? 1U : 0U;

   pKerPrivArgs->gainComputed = active;

   if (active != 0U) {
      float targetGain =
          pKerPrivArgs->gainScale * AUDIOLIB_agc_exp2_ci(pKerPrivArgs->gainExp * AUDIOLIB_agc_log2_ci(energy));
      targetGain = (targetGain > pKerPrivArgs->gainMax) ? pKerPrivArgs->gainMax : targetGain;
      targetGain = (targetGain < pKerPrivArgs->gainMin) ? pKerPrivArgs->gainMin : targetGain;

      pKerPrivArgs->targetGain = targetGain;
   }
   else if (pPrm->enableRecovery != AUDIOLIB_AGC_RECOVERY_DISABLE) {
      float targetGain = pKerPrivArgs->targetGain;
      if (targetGain > AUDIOLIB_AGC_UNITY_GAIN) {
         targetGain *= pKerPrivArgs->recoveryRateDown;
         targetGain = (targetGain < AUDIOLIB_AGC_UNITY_GAIN) ? AUDIOLIB_AGC_UNITY_GAIN : targetGain;
      }
      else if (targetGain < AUDIOLIB_AGC_UNITY_GAIN) {
         targetGain *= pKerPrivArgs->recoveryRateUp;
         targetGain = (targetGain > AUDIOLIB_AGC_UNITY_GAIN) ? AUDIOLIB_AGC_UNITY_GAIN : targetGain;
      }
      else {
         /* Already at 0 dB */
      }
      pKerPrivArgs->targetGain = targetGain;
   }
   else {
      /* Hold the target gain */
   }
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_agc_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_agc_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_agc_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_agc_exec_ci nIterIn %d nRowsOut %d nIterPerRowOut %d\n",
                         pKerPrivArgs->nIterIn, pKerPrivArgs->nRowsOut, pKerPrivArgs->nIterPerRowOut);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   /**********************************************************************/
   /* Sum of squares over the whole input block. SE0 reads the first    */
   /* half and SE1 the second half; both zero-fill row tails and pad    */
   /* up to whole iterations, so padded lanes contribute nothing.       */
   /**********************************************************************/
   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pKerPrivArgs->in1Offset, se1Params);

   uint32_t nIterIn = pKerPrivArgs->nIterIn;
   vec      acc0    = (vec) (0);
   vec      acc1    = (vec) (0);
   vec      acc2    = (vec) (0);
   vec      acc3    = (vec) (0);
   vec      acc4    = (vec) (0);
   vec      acc5    = (vec) (0);
   vec      acc6    = (vec) (0);
   vec      acc7    = (vec) (0);

#pragma MUST_ITERATE(1, , )
   for (uint32_t i = 0; i < nIterIn; i++) {
      vec in0 = c7x::strm_eng<0, vec>::get_adv();
      vec in1 = c7x::strm_eng<1, vec>::get_adv();
      vec in2 = c7x::strm_eng<0, vec>::get_adv();
      vec in3 = c7x::strm_eng<1, vec>::get_adv();
      vec in4 = c7x::strm_eng<0, vec>::get_adv();
      vec in5 = c7x::strm_eng<1, vec>::get_adv();
      vec in6 = c7x::strm_eng<0, vec>::get_adv();
      vec in7 = c7x::strm_eng<1, vec>::get_adv();
      acc0 += in0 * in0;
      acc1 += in1 * in1;
      acc2 += in2 * in2;
      acc3 += in3 * in3;
      acc4 += in4 * in4;
      acc5 += in5 * in5;
      acc6 += in6 * in6;
      acc7 += in7 * in7;
   }

   __SE0_CLOSE();
   __SE1_CLOSE();

   __SE0_OPEN(pInLocal, se2Params);
   __SA0_OPEN(sa0Params);

   /* Horizontal sum across the lanes */
   dataType energySum;
   c7x_horizontal_add(((acc0 + acc1) + (acc2 + acc3)) + ((acc4 + acc5) + (acc6 + acc7)), &energySum);

   AUDIOLIB_agc_updateTargetGain_ci(pKerPrivArgs, (float) energySum);

   /**********************************************************************/
   /* Apply per-sample gain smoothing and multiply by input. The gain    */
   /* runs continuously over all M * N samples, k = m * N + n:           */
   /*   g[k] = T + (g[-1] - T) * decay^(k+1)                             */
   /* SE0 zero-fills and SA0 predicates off the padding vectors.         */
   /**********************************************************************/
   dataType targetGain  = (dataType) pKerPrivArgs->targetGain;
   dataType currentGain = (dataType) pKerPrivArgs->currentGain;

   uint32_t  eleCount   = c7x::element_count_of<vec>::value;
   dataType *pDecayPow  = (dataType *) pKerPrivArgs->decayPow;
   vec       targetVec  = (vec) (targetGain);
   vec       stepVec4   = (vec) (*(dataType *) &pKerPrivArgs->decayPowUnroll);
   vec       rowStepVec = (vec) (*(dataType *) &pKerPrivArgs->decayPowRow);

   /* Chain seeds of row 0: (g[-1] - T) * decay^(j * eleCount + i + 1) for vectors j = 0..3 */
   vec diffVec  = (vec) (currentGain - targetGain);
   vec rowDVec0 = diffVec * *(stov_ptr(vec, pDecayPow));
   vec rowDVec1 = diffVec * *(stov_ptr(vec, pDecayPow + eleCount));
   vec rowDVec2 = diffVec * *(stov_ptr(vec, pDecayPow + (2U * eleCount)));
   vec rowDVec3 = diffVec * *(stov_ptr(vec, pDecayPow + (3U * eleCount)));

   uint32_t nRowsOut       = pKerPrivArgs->nRowsOut;
   uint32_t nIterPerRowOut = pKerPrivArgs->nIterPerRowOut;

#pragma MUST_ITERATE(1, , )
   for (uint32_t m = 0; m < nRowsOut; m++) {
      vec dVec0 = rowDVec0;
      vec dVec1 = rowDVec1;
      vec dVec2 = rowDVec2;
      vec dVec3 = rowDVec3;

#pragma MUST_ITERATE(1, , )
      for (uint32_t i = 0; i < nIterPerRowOut; i++) {
         vec in0 = c7x::strm_eng<0, vec>::get_adv();
         vec in1 = c7x::strm_eng<0, vec>::get_adv();
         vec in2 = c7x::strm_eng<0, vec>::get_adv();
         vec in3 = c7x::strm_eng<0, vec>::get_adv();

         vec out0 = in0 * (targetVec + dVec0);
         vec out1 = in1 * (targetVec + dVec1);
         vec out2 = in2 * (targetVec + dVec2);
         vec out3 = in3 * (targetVec + dVec3);

         dVec0 = dVec0 * stepVec4;
         dVec1 = dVec1 * stepVec4;
         dVec2 = dVec2 * stepVec4;
         dVec3 = dVec3 * stepVec4;

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred0, addr0, out0);

         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred1, addr1, out1);

         __vpred vpred2 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr2  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred2, addr2, out2);

         __vpred vpred3 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr3  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred3, addr3, out3);
      }

      /* Next row starts dim_x samples later along the gain trajectory */
      rowDVec0 = rowDVec0 * rowStepVec;
      rowDVec1 = rowDVec1 * rowStepVec;
      rowDVec2 = rowDVec2 * rowStepVec;
      rowDVec3 = rowDVec3 * rowStepVec;
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   /* Gain after the last of the dim_x * dim_y samples */
   pKerPrivArgs->currentGain =
       (float) (targetGain + ((currentGain - targetGain) * (dataType) pKerPrivArgs->blockDecay));

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_agc_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_agc_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
