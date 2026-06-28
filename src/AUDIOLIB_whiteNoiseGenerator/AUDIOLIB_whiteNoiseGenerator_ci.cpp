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
                                                     const AUDIOLIB_bufParams1D_t                *bufParamsOut,
                                                     const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_whiteNoiseGenerator_init_ci\n");
#endif
   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;
   AUDIOLIB_STATUS                        status       = AUDIOLIB_SUCCESS;
   uint8_t                               *pBlock       = pKerPrivArgs->bufPblock;
   int32_t                                eleCount     = c7x::element_count_of<vec>::value;
   __SA_TEMPLATE_v1                       sa0Params    = __gen_SA_TEMPLATE_v1();
   __SA_VECLEN                            SA_VECLEN    = c7x::sa_veclen<vec>::value;
   pKerPrivArgs->nVecs     = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount * AUDIOLIB_UNROLL_FACTOR);
   pKerPrivArgs->twoXRange = 2 * pKerInitArgs->range;
   // configure SA0 to store output samples
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_1D;
   sa0Params.ICNT0  = pKerPrivArgs->samples;

   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_whiteNoiseGenerator_init_ci<float>(AUDIOLIB_kernelHandle                        handle,
                                            const AUDIOLIB_bufParams1D_t                *bufParamsOut,
                                            const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_whiteNoiseGenerator_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<uint32_t>::type uintVec;
   typedef typename c7x::make_full_vector<dataType>::type floatVec;
   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;

   AUDIOLIB_STATUS status        = AUDIOLIB_SUCCESS;
   uint8_t        *pBlock        = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal  = (dataType *) pOut;
   __SA_TEMPLATE_v1 sa0Params    = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   uint32_t        *statePtr     = pKerPrivArgs->states;
   uintVec          vecState     = *(stov_ptr(uintVec, (uint32_t *) &statePtr[0]));
   uintVec          vecStateNext = *(stov_ptr(uintVec, (uint32_t *) &statePtr[0]));
   uintVec          jumpMult     = (uintVec) (pKerPrivArgs->jumpHeadMultiplierNextFrame);
   uintVec          jumpAdd      = (uintVec) (pKerPrivArgs->jumpHeadIncNextFrame);
   uint32_t         nVecs        = pKerPrivArgs->nVecs;
   uintVec          aVec         = (uintVec) (AUDIOLIB_LCG_MULTIPLIER);
   uintVec          cVec         = (uintVec) (AUDIOLIB_LCG_INCREMENT);
   uintVec          a8           = (uintVec) (AUDIOLIB_LCG_A_JUMP8);
   uintVec          c8           = (uintVec) (AUDIOLIB_LCG_C_JUMP8);
   uintVec          a32          = (uintVec) (AUDIOLIB_LCG_A_JUMP32);
   uintVec          c32          = (uintVec) (AUDIOLIB_LCG_C_JUMP32);
   floatVec         scaleFacVec  = (floatVec) (AUDIOLIB_LCG_NORM_FACTOR_F32);
   floatVec         rangeVec     = (floatVec) (pKerPrivArgs->range);
   floatVec         twoRangeVec  = (floatVec) (pKerPrivArgs->twoXRange);
   uintVec          vecState2    = vecState * a8 + c8;
   uintVec          vecState3    = vecState2 * a8 + c8;
   uintVec          vecState4    = vecState3 * a8 + c8;

   __SA0_OPEN(sa0Params);
   // 16 + trip_cnt * 8
   for (uint32_t i = 0; i < nVecs; i++) {
      uintVec stateVecOut1 = vecState * aVec + cVec;
      uintVec stateVecOut2 = vecState2 * aVec + cVec;
      uintVec stateVecOut3 = vecState3 * aVec + cVec;
      uintVec stateVecOut4 = vecState4 * aVec + cVec;

      // Advance each by +32 samples
      vecState  = vecState * a32 + c32;
      vecState2 = vecState2 * a32 + c32;
      vecState3 = vecState3 * a32 + c32;
      vecState4 = vecState4 * a32 + c32;

      floatVec  stateFloatVecOut1 = __int_to_float(stateVecOut1);
      floatVec  normalised1       = stateFloatVecOut1 * scaleFacVec;
      floatVec  output1           = (twoRangeVec * normalised1) - rangeVec;
      __vpred   tmp               = c7x::strm_agen<0, floatVec>::get_vpred();
      floatVec *VB1               = c7x::strm_agen<0, floatVec>::get_adv(pOutLocal);
      __vstore_pred(tmp, VB1, output1);

      floatVec stateFloatVecOut2 = __int_to_float(stateVecOut2);
      floatVec normalised2       = stateFloatVecOut2 * scaleFacVec;
      floatVec output2           = (twoRangeVec * normalised2) - rangeVec;
      tmp                        = c7x::strm_agen<0, floatVec>::get_vpred();
      VB1                        = c7x::strm_agen<0, floatVec>::get_adv(pOutLocal);
      __vstore_pred(tmp, VB1, output2);

      floatVec stateFloatVecOut3 = __int_to_float(stateVecOut3);
      floatVec normalised3       = stateFloatVecOut3 * scaleFacVec;
      floatVec output3           = (twoRangeVec * normalised3) - rangeVec;
      tmp                        = c7x::strm_agen<0, floatVec>::get_vpred();
      VB1                        = c7x::strm_agen<0, floatVec>::get_adv(pOutLocal);
      __vstore_pred(tmp, VB1, output3);

      floatVec stateFloatVecOut4 = __int_to_float(stateVecOut4);
      floatVec normalised4       = stateFloatVecOut4 * scaleFacVec;
      floatVec output4           = (twoRangeVec * normalised4) - rangeVec;
      tmp                        = c7x::strm_agen<0, floatVec>::get_vpred();
      VB1                        = c7x::strm_agen<0, floatVec>::get_adv(pOutLocal);
      __vstore_pred(tmp, VB1, output4);
   }
   __SA0_CLOSE();

   vecStateNext = jumpMult * vecStateNext + jumpAdd;

   // saving the state for next frame
   *(vtos_ptr(uintVec, pKerPrivArgs->states)) = vecStateNext;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pOut);

void AUDIOLIB_whiteNoiseGenerator_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_whiteNoiseGenerator_PrivArgs *pKerPrivArgs = (AUDIOLIB_whiteNoiseGenerator_PrivArgs *) handle;
   int32_t                                nVecs        = pKerPrivArgs->nVecs;
   uint32_t                               whiteNoiseGeneratorStartupCycles   = 21 + 3;
   uint32_t                               whiteNoiseGeneratorOperationCycles = 0;
   uint32_t                               whiteNoiseGeneratorTearDownCycles  = 13;
   uint32_t                               whiteNoiseGeneratorOverheadCycles  = 0;

   whiteNoiseGeneratorOperationCycles = 16 + nVecs * 8;

   *archCycles +=
       whiteNoiseGeneratorStartupCycles + whiteNoiseGeneratorOperationCycles + whiteNoiseGeneratorTearDownCycles;
   *estCycles += whiteNoiseGeneratorStartupCycles + whiteNoiseGeneratorOperationCycles +
                 whiteNoiseGeneratorOverheadCycles + whiteNoiseGeneratorTearDownCycles;
}
