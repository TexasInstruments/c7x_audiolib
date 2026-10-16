// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/AUDIOLIB_inlines.h"
#include "AUDIOLIB_rmsSmoothed_priv.h"

/* Vectors read from each streaming engine per iteration of the accumulation loop */
#define AUDIOLIB_RMSSMOOTHED_UNROLL (4)

// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/

void AUDIOLIB_rmsSmoothed_perfEst(AUDIOLIB_kernelHandle                handle,
                                  uint64_t                            *archCycles,
                                  uint64_t                            *estCycles,
                                  uint32_t                             data_type,
                                  const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs)
{
   AUDIOLIB_rmsSmoothed_PrivArgs *pKerPrivArgs = (AUDIOLIB_rmsSmoothed_PrivArgs *) handle;
   uint64_t                       nVecs        = pKerPrivArgs->nVecs;

   /* For loading the SE params and opening both streams */
   uint64_t rmsSmoothedStartupCycles   = 30;
   uint64_t rmsSmoothedOperationCycles = 0;
   uint64_t rmsSmoothedTeardownCycles  = 0;

   /* Accumulation loop: ii = 4 for float and 8 for double (double multiplies take twice the .M/.N time);
    */
   if (data_type == AUDIOLIB_FLOAT32) {
      rmsSmoothedOperationCycles = 12 + nVecs * 4;
      rmsSmoothedTeardownCycles  = 101;
   }
   else if (data_type == AUDIOLIB_FLOAT64) {
      rmsSmoothedOperationCycles = 16 + nVecs * 8;
      rmsSmoothedTeardownCycles  = 94;
   }

   *archCycles = rmsSmoothedOperationCycles;
   *estCycles  = rmsSmoothedStartupCycles + rmsSmoothedOperationCycles + rmsSmoothedTeardownCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_init_ci(AUDIOLIB_kernelHandle                handle,
                                             const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                             const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                             const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_rmsSmoothed_PrivArgs *pKerPrivArgs = (AUDIOLIB_rmsSmoothed_PrivArgs *) handle;
   uint8_t                       *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                       eleCount     = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_rmsSmoothed_init_ci \n");

   uint32_t blockSize        = pKerPrivArgs->blockSize;
   uint32_t numChannels      = pKerPrivArgs->numChannels;
   uint32_t strideInElements = pKerPrivArgs->strideInElements;

   /* Contiguous channels are streamed as one row of blockSize * numChannels samples */
   uint32_t isContiguous = (strideInElements == blockSize);
   uint32_t samples      = isContiguous ? (blockSize * numChannels) : blockSize;
   uint32_t nRows        = isContiguous ? 1 : numChannels;
   uint32_t iter         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(samples, eleCount), 2);

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;

   /* SE0 reads the even vectors and SE1 the odd vectors of every row, so each reads half of the row's vectors */
   if (isContiguous) {
      se0Params.ICNT0         = eleCount;
      se0Params.ICNT1         = iter;
      se0Params.DIM1          = 2 * eleCount;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.DECDIM1_WIDTH = blockSize * numChannels;
      se0Params.ELETYPE       = SE_ELETYPE;
      se0Params.VECLEN        = SE_VECLEN;
      se0Params.DIMFMT        = __SE_DIMFMT_2D;
   }
   else {
      se0Params.ICNT0         = eleCount;
      se0Params.ICNT1         = iter;
      se0Params.DIM1          = 2 * eleCount;
      se0Params.ICNT2         = numChannels;
      se0Params.DIM2          = strideInElements;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.DECDIM1_WIDTH = blockSize;
      se0Params.ELETYPE       = SE_ELETYPE;
      se0Params.VECLEN        = SE_VECLEN;
   }

   /* Reads beyond the end of a stream return zero vectors, so the loop count is only rounded up */
   pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(iter * nRows, AUDIOLIB_RMSSMOOTHED_UNROLL);

   se1Params = se0Params;
   if (samples > eleCount) {
      /* SE1 starts one vector into each row, so it has eleCount fewer valid samples per row */
      se1Params.DECDIM1_WIDTH   = samples - eleCount;
      pKerPrivArgs->offset      = eleCount;
      pKerPrivArgs->energyScale = pKerPrivArgs->invNumSamples;
   }
   else {
      /* Rows fit in one vector: SE1 re-reads SE0's stream and the doubled sum is halved */
      pKerPrivArgs->offset      = 0;
      pKerPrivArgs->energyScale = pKerPrivArgs->invNumSamples * 0.5f;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_init_ci<float>(AUDIOLIB_kernelHandle                handle,
                                                             const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                                             const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                                             const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs);
template AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_init_ci<double>(AUDIOLIB_kernelHandle                handle,
                                                              const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                                              const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                                              const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs);

// Fast square root
template <typename dataType> static inline dataType fast_sqrt(dataType x)
{
   dataType y = __recip_sqrt(x);
   // Two Newton-Raphson iterations for improved accuracy
   y = y * ((dataType) 1.5 - (x * (dataType) 0.5 * y * y));
   y = y * ((dataType) 1.5 - (x * (dataType) 0.5 * y * y));
   return x * y;
}

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_rmsSmoothed_PrivArgs *pKerPrivArgs = (AUDIOLIB_rmsSmoothed_PrivArgs *) handle;
   uint8_t                       *pBlock       = pKerPrivArgs->bufPblock;
   int32_t                        nVecs        = (int32_t) pKerPrivArgs->nVecs;
   uint32_t                       offset       = pKerPrivArgs->offset;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_rmsSmoothed_exec_ci");

   dataType *restrict pInLocalSrc  = (dataType *) pIn;
   dataType *restrict pOutLocalOut = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);

   __SE0_OPEN(pInLocalSrc, se0Params);
   __SE1_OPEN(pInLocalSrc + offset, se1Params);

   vec acc0 = (vec) 0;
   vec acc1 = (vec) 0;
   vec acc2 = (vec) 0;
   vec acc3 = (vec) 0;

#pragma MUST_ITERATE(1, , 1)
   for (int32_t i = 0; i < nVecs; i++) {
      vec vecInA0 = c7x::strm_eng<0, vec>::get_adv();
      vec vecInB0 = c7x::strm_eng<1, vec>::get_adv();
      vec vecInA1 = c7x::strm_eng<0, vec>::get_adv();
      vec vecInB1 = c7x::strm_eng<1, vec>::get_adv();
      vec vecInA2 = c7x::strm_eng<0, vec>::get_adv();
      vec vecInB2 = c7x::strm_eng<1, vec>::get_adv();
      vec vecInA3 = c7x::strm_eng<0, vec>::get_adv();
      vec vecInB3 = c7x::strm_eng<1, vec>::get_adv();

      acc0 += vecInA0 * vecInA0 + vecInB0 * vecInB0;
      acc1 += vecInA1 * vecInA1 + vecInB1 * vecInB1;
      acc2 += vecInA2 * vecInA2 + vecInB2 * vecInB2;
      acc3 += vecInA3 * vecInA3 + vecInB3 * vecInB3;
   }

   __SE0_CLOSE();
   __SE1_CLOSE();

   acc0 = (acc0 + acc1) + (acc2 + acc3);

   dataType energy = 0;
   c7x_horizontal_add(acc0, &energy);
   energy = energy * (dataType) pKerPrivArgs->energyScale;

   dataType smoothingCoefficient = (dataType) pKerPrivArgs->smoothingCoefficient;
   dataType y = (smoothingCoefficient * pKerPrivArgs->smoothedEnergy) + ((1 - smoothingCoefficient) * energy);

   pKerPrivArgs->smoothedEnergy = y;

   /* fast_sqrt(0) is NaN (rsqrt(0) = inf), so silent input is mapped to 0 */
   pOutLocalOut[0] = (y > (dataType) 0) ? fast_sqrt(y) : (dataType) 0;

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
