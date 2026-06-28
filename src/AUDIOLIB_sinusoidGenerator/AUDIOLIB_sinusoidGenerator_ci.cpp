// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_sinusoidGenerator_priv.h"
#include "c6x_migration.h"
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_init_ci(AUDIOLIB_kernelHandle                      handle,
                                                   const AUDIOLIB_bufParams1D_t              *bufParamsOut,
                                                   const AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_sinusoidGenerator_PrivArgs                   *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   int32_t                                                eleCount     = c7x::element_count_of<vec>::value;
   __SA_TEMPLATE_v1                                       sa0Params    = __gen_SA_TEMPLATE_v1();
   __SA_VECLEN                                            SA_VECLEN    = c7x::sa_veclen<vec>::value;
   pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(pKerPrivArgs->numSamples, eleCount);

   // configure SA0 to store output samples
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_1D;
   sa0Params.ICNT0  = pKerPrivArgs->numSamples;

   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_sinusoidGenerator_init_ci<float>(AUDIOLIB_kernelHandle                      handle,
                                          const AUDIOLIB_bufParams1D_t              *bufParamsOut,
                                          const AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs);

static inline float sin_scalar_ci(float a)
{
   float InvPI = 0.318309886183791f;
   float One   = 1.0f;
   float MAX   = 1048576.0f;
   float Zero  = 0.0f;
   float s1    = -1.666665668e-1f;
   float s2    = 8.333025139e-3f;
   float s3    = -1.980741872e-4f;
   float s4    = 2.601903036e-6f;
   float C1    = 3.140625f;
   float C2    = 9.67653589793e-4f;
   int   N     = 0;

   float Sign = 0.0f, X = 0.0f, Y = 0.0f, Z = 0.0f, F = 0.0f, G = 0.0f, R = 0.0f;

   Sign = One;
   Y    = a;

   if (_fabsf(Y) > MAX) {
      Y = Zero;
   }

   X = Y * InvPI; /* X = Y * (1/PI)  */
   N = _spint(X); /* N = integer part of X  */
   Z = (float) N;

   if ((N % 2) != 0) {
      Sign = -Sign; /* Quadrant 3 or 4 */
   }

   F = (Y - (Z * C1)) - (Z * C2);
   G = F * F;
   R = ((((((s4 * G) + s3) * G) + s2) * G) + s1) * G;

   return ((F + (F * R)) * Sign);
}
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_scalar_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{
   AUDIOLIB_STATUS                      status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_sinusoidGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_sinusoidGenerator_exec_cn");
   dataType *restrict pOutLocal = (dataType *) pOut;
   dataType TWO_PIF             = 6.28318530717958647692;

   uint32_t numSamples                   = pKerPrivArgs->numSamples;
   dataType phase                        = pKerPrivArgs->phase;
   dataType phaseInc                     = pKerPrivArgs->phaseInc;
   dataType phaseIncTarget               = pKerPrivArgs->phaseIncTarget;
   dataType smoothingCoefficient         = pKerPrivArgs->smoothingCoefficient;
   dataType oneMinusSmoothingCoefficient = pKerPrivArgs->oneMinusSmoothingCoefficient;

   // 59 + trip_cnt * 7
   for (uint32_t i = 0; i < numSamples; i++) {
      pOutLocal[i] = sin_scalar_ci(phase);
      phase += phaseInc;

      if (phase >= TWO_PIF) {
         phase -= TWO_PIF;
      }
      phaseInc = phaseInc * oneMinusSmoothingCoefficient + phaseIncTarget * smoothingCoefficient;
   }

   pKerPrivArgs->phase    = phase;
   pKerPrivArgs->phaseInc = phaseInc;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_scalar_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                          void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_vector_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{
   AUDIOLIB_STATUS                      status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_sinusoidGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;
   dataType *restrict pOutLocal                      = (dataType *) pOut;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_sinusoidGenerator_exec_cn");

   typedef typename c7x::make_full_vector<dataType>::type       vec;
   typedef typename c7x::make_full_vector<c7x::ulong_vec>::type ulongVec;
   typedef typename c7x::make_full_vector<c7x::int_vec>::type   vec_type;

   vec      X, Z, F, G, R, phaseRamp;
   vec_type N;
   __vpred  stateResult;

   uint8_t         *pBlock             = pKerPrivArgs->bufPblock;
   __SA_TEMPLATE_v1 sa0Params          = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   dataType         phase              = pKerPrivArgs->phase;
   dataType         phaseInc           = pKerPrivArgs->phaseInc;
   dataType         phaseIncTarget     = pKerPrivArgs->phaseIncTarget;
   uint32_t         nVecs              = pKerPrivArgs->nVecs;
   uint32_t         lastSampleIdx      = pKerPrivArgs->lastSampleIdx;
   float            vecCoeffMultiplier = pKerPrivArgs->alphaMultiplier;
   float           *alphaPtr           = pKerPrivArgs->alphaCoeff;
   vec_type         intOne             = (vec_type) 1;
   vec              vecCoefficient     = *(stov_ptr(vec, (float *) &alphaPtr[0]));
   vec              phaseVec           = (vec) phase;
   vec              phaseIncVec        = (vec) phaseInc;
   vec              phaseIncStart      = (vec) phaseInc;
   vec              phaseIncTargetVec  = (vec) phaseIncTarget;
   vec              TWO_PIF            = (vec) (2 * 3.14159265358979323846);
   vec              InvPI              = (vec) 0.318309886183791;
   vec              MAX                = (vec) 1048576.0;
   vec              Zero               = (vec) 0.0;
   vec              s1                 = (vec) -1.666665668e-1;
   vec              s2                 = (vec) 8.333025139e-3;
   vec              s3                 = (vec) -1.980741872e-4;
   vec              s4                 = (vec) 2.601903036e-6;
   vec              C1                 = (vec) 3.140625;
   vec              C2                 = (vec) 9.67653589793e-4;
   vec              negativeOne        = (vec) -1;
   vec              sign               = (vec) 1.0;
   vec              diff               = (phaseIncStart - phaseIncTargetVec);

   // Check if smoothing is active (only need to check one element)
   bool smoothingActive = (__get_vector_element(diff, 0) != 0.0f && !pKerPrivArgs->bypassSmoothing);
   if (smoothingActive) {
      __SA0_OPEN(sa0Params);
      // 89 + trip_cnt * 17
      for (uint32_t i = 0; i < nVecs; i++) {
         phaseIncVec    = phaseIncTargetVec + diff * vecCoefficient;
         vecCoefficient = vecCoefficient * vecCoeffMultiplier;
         vec phaseIncVecA =
             c7x::reinterpret<vec>(__shift_left_full(c7x::reinterpret<ulongVec>(phaseIncVec), uchar(32)));
         vec phaseIncVecB =
             c7x::reinterpret<vec>(__shift_left_full(c7x::reinterpret<ulongVec>(phaseIncVec), uchar(64)));
         vec phaseIncVecC =
             c7x::reinterpret<vec>(__shift_left_full(c7x::reinterpret<ulongVec>(phaseIncVecB), uchar(32)));
         vec phaseIncVecD =
             c7x::reinterpret<vec>(__shift_left_full(c7x::reinterpret<ulongVec>(phaseIncVecB), uchar(64)));
         vec phaseIncVecE =
             c7x::reinterpret<vec>(__shift_left_full(c7x::reinterpret<ulongVec>(phaseIncVecD), uchar(32)));
         vec phaseIncVecF =
             c7x::reinterpret<vec>(__shift_left_full(c7x::reinterpret<ulongVec>(phaseIncVecD), uchar(64)));
         vec phaseIncVecG =
             c7x::reinterpret<vec>(__shift_left_full(c7x::reinterpret<ulongVec>(phaseIncVecF), uchar(32)));
         vec cumulativeSum =
             phaseIncVecA + phaseIncVecB + phaseIncVecC + phaseIncVecD + phaseIncVecE + phaseIncVecF + phaseIncVecG;
         phaseRamp            = phaseVec + cumulativeSum;
         phaseVec             = __get_vector_element(phaseRamp, 7) + __get_vector_element(phaseIncVec, 7);
         stateResult          = __cmp_ge_pred(phaseVec, TWO_PIF);
         phaseVec             = __select(stateResult, (phaseVec - TWO_PIF), phaseVec);
         __vpred cmp_gt       = __cmp_lt_pred((vec) MAX, __abs(phaseRamp));
         phaseRamp            = __select(cmp_gt, Zero, phaseRamp);
         __vpred result       = __cmp_ge_pred(phaseRamp, vec(TWO_PIF));
         phaseRamp            = __select(result, (phaseRamp - TWO_PIF), phaseRamp);
         X                    = phaseRamp * InvPI;
         N                    = __float_to_int(X);
         Z                    = c7x::convert<vec>(N);
         vec_type andN        = N & intOne;
         vec      convertandN = c7x::convert<vec>(andN);
         __vpred  cmpMod      = __cmp_le_pred(convertandN, Zero);
         vec      signVec     = __select(cmpMod, sign, negativeOne);
         F                    = (phaseRamp - (Z * C1)) - (Z * C2);
         G                    = F * F;
         R                    = ((((((s4 * G) + s3) * G) + s2) * G) + s1) * G;
         vec     output       = ((F + (F * R)) * signVec);
         __vpred tmp          = c7x::strm_agen<0, vec>::get_vpred();
         vec    *VB1          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmp, VB1, output);
      }
      __SA0_CLOSE();
      // saving the state for the next frame of execution
      float lastProcessedPhaseInc        = __get_vector_element(phaseIncVec, lastSampleIdx);
      float oneMinusSmoothingCoefficient = pKerPrivArgs->oneMinusSmoothingCoefficient;
      pKerPrivArgs->phaseInc = phaseIncTarget + (lastProcessedPhaseInc - phaseIncTarget) * oneMinusSmoothingCoefficient;
      vec lastPhase          = __get_vector_element(phaseRamp, lastSampleIdx);
      vec lastInc            = __get_vector_element(phaseIncVec, lastSampleIdx);
      vec nextPhase          = lastPhase + lastInc;
      stateResult            = __cmp_ge_pred(nextPhase, TWO_PIF);
      nextPhase              = __select(stateResult, (nextPhase - TWO_PIF), nextPhase);
      pKerPrivArgs->phase    = __get_vector_element(nextPhase, 0);
   }
   else {
      vec    vecWidth = (vec) c7x::element_count_of<vec>::value;
      float *rampPtr  = pKerPrivArgs->ramp;
      vec    vecRamp  = *stov_ptr(vec, (float *) &rampPtr[0]);
      phaseIncVec     = phaseIncTargetVec;

      __SA0_OPEN(sa0Params);
      // 62 + trip_cnt * 10
      for (uint32_t i = 0; i < nVecs; i++) {
         phaseRamp            = phaseVec + (vecRamp * phaseIncVec);
         phaseVec             = phaseVec + (phaseIncVec * vecWidth);
         stateResult          = __cmp_ge_pred(phaseVec, TWO_PIF);
         phaseVec             = __select(stateResult, (phaseVec - TWO_PIF), phaseVec);
         __vpred cmp_gt       = __cmp_lt_pred((vec) MAX, __abs(phaseRamp));
         phaseRamp            = __select(cmp_gt, Zero, phaseRamp);
         __vpred result       = __cmp_ge_pred(phaseRamp, vec(TWO_PIF));
         phaseRamp            = __select(result, (phaseRamp - TWO_PIF), phaseRamp);
         X                    = phaseRamp * InvPI;
         N                    = __float_to_int(X);
         Z                    = c7x::convert<vec>(N);
         vec_type andN        = N & intOne;
         vec      convertandN = c7x::convert<vec>(andN);
         __vpred  cmpMod      = __cmp_le_pred(convertandN, Zero);
         vec      signVec     = __select(cmpMod, sign, negativeOne);
         F                    = (phaseRamp - (Z * C1)) - (Z * C2);
         G                    = F * F;
         R                    = ((((((s4 * G) + s3) * G) + s2) * G) + s1) * G;
         vec     output       = ((F + (F * R)) * signVec);
         __vpred tmp          = c7x::strm_agen<0, vec>::get_vpred();
         vec    *VB1          = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmp, VB1, output);
      }
      __SA0_CLOSE();

      // saving the state for the next frame
      vec lastPhase          = __get_vector_element(phaseRamp, lastSampleIdx);
      vec lastInc            = __get_vector_element(phaseIncVec, lastSampleIdx);
      vec nextPhase          = lastPhase + lastInc;
      stateResult            = __cmp_ge_pred(nextPhase, TWO_PIF);
      nextPhase              = __select(stateResult, (nextPhase - TWO_PIF), nextPhase);
      pKerPrivArgs->phase    = __get_vector_element(nextPhase, 0);
      pKerPrivArgs->phaseInc = phaseIncTarget;
   }
   return status;
}
template AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_vector_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                          void *restrict pOut);

void AUDIOLIB_sinusoidGenerator_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   uint64_t                             sinusoidGeneratorStartupCycles   = 0;
   uint64_t                             sinusoidGeneratorTeardownCycles  = 0;
   uint64_t                             sinusoidGeneratorOperationCycles = 0;
   uint64_t                             sinusoidGeneratorOverheadCycles  = 0;
   uint32_t                             nVecs;
   AUDIOLIB_sinusoidGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_sinusoidGenerator_PrivArgs *) handle;

   nVecs = pKerPrivArgs->nVecs;

   if (!pKerPrivArgs->initArgs.executionMode) {
      sinusoidGeneratorStartupCycles += 9 + 7;
      sinusoidGeneratorOperationCycles = 59 + pKerPrivArgs->numSamples * 7;
      sinusoidGeneratorTeardownCycles += 2;
   }
   else {
      if (pKerPrivArgs->bypassSmoothing || (pKerPrivArgs->phaseInc == pKerPrivArgs->phaseIncTarget)) {
         sinusoidGeneratorStartupCycles += 22 + 7 + 7 + 3;
         sinusoidGeneratorOperationCycles = 62 + nVecs * 10;
         sinusoidGeneratorTeardownCycles += 1 + 3 + 20;
      }
      else {
         sinusoidGeneratorStartupCycles += 22 + 7 + 1;
         sinusoidGeneratorOperationCycles = 89 + nVecs * 17;
         sinusoidGeneratorTeardownCycles += 3 + 21;
      }
   }
   sinusoidGeneratorOverheadCycles = sinusoidGeneratorStartupCycles + sinusoidGeneratorTeardownCycles;
   *estCycles                      = sinusoidGeneratorOperationCycles + sinusoidGeneratorOverheadCycles;
   *archCycles                     = sinusoidGeneratorOperationCycles;
}
