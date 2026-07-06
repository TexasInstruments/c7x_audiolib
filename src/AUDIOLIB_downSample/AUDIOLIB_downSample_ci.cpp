// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/*******************************************************************************
 *
 * INCLUDES
 *
 ******************************************************************************/

#include "AUDIOLIB_downSample_priv.h"
#include <cstring>

/* Permute/select masks for the ratio-3/4/5/7/8/12 variants.  init_ci stores the
 * chosen variant's masks in bufPblock; each exec variant reads them back.
 * Layout: [perm0..2 : 32 B][sel0..5 : 8 B]; selects are 8 B (float) / 4 B
 * (double), permutes always 32 B. */

template <typename dataType>
static inline void downSampleStoreRatio3_6Masks(uint8_t *pBlock)
{
   const uint32_t selBytes = (sizeof(dataType) == 4) ? 8u : 4u;
   if (sizeof(dataType) == 4) {
      const uint8_t permute[32] = {0,  1,  2,  3,  12, 13, 14, 15, 24, 25, 26, 27, 4,  5,  6,  7,
                                   16, 17, 18, 19, 28, 29, 30, 31, 8,  9,  10, 11, 20, 21, 22, 23};
      const uint8_t sel0[8]     = {0, 1, 0, 0, 1, 0, 0, 1};
      const uint8_t sel1[8]     = {0, 0, 1, 0, 0, 1, 0, 0};
      memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(0), permute, 32);
      memcpy(pBlock + DS_MASK_SEL_OFFSET(0), sel0, selBytes);
      memcpy(pBlock + DS_MASK_SEL_OFFSET(1), sel1, selBytes);
   }
   else {
      const uint8_t permute[32] = {0,  1,  2,  3,  4,  5,  6,  7,  24, 25, 26, 27, 28, 29, 30, 31,
                                   16, 17, 18, 19, 20, 21, 22, 23, 8,  9,  10, 11, 12, 13, 14, 15};
      const uint8_t sel0[4]     = {0, 0, 1, 0};
      const uint8_t sel1[4]     = {0, 1, 0, 0};
      memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(0), permute, 32);
      memcpy(pBlock + DS_MASK_SEL_OFFSET(0), sel0, selBytes);
      memcpy(pBlock + DS_MASK_SEL_OFFSET(1), sel1, selBytes);
   }
}

template <typename dataType>
static inline void downSampleStoreRatio4Masks(uint8_t *pBlock)
{
   /* Ratio4: single identity permute, no select mask (same for float/double). */
   const uint8_t permute[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
   memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(0), permute, 32);
}

static inline void downSampleStoreRatio5Masks(uint8_t *pBlock)
{
   const uint8_t permute[32] = {0,  1,  2,  3,  20, 21, 22, 23, 8,  9,  10, 11, 28, 29, 30, 31,
                                16, 17, 18, 19, 4,  5,  6,  7,  24, 25, 26, 27, 12, 13, 14, 15};
   const uint8_t sel0[8]     = {0, 0, 1, 0, 0, 0, 0, 1};
   const uint8_t sel1[8]     = {0, 1, 0, 0, 0, 0, 1, 0};
   const uint8_t sel2[8]     = {0, 0, 0, 1, 0, 0, 0, 0};
   const uint8_t selC[8]     = {0, 1, 0, 0, 1, 0, 1, 0};
   memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(0), permute, 32);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(0), sel0, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(1), sel1, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(2), sel2, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(3), selC, 8);
}

static inline void downSampleStoreRatio7Masks(uint8_t *pBlock)
{
   const uint8_t permute[32] = {0,  1,  2,  3,  28, 29, 30, 31, 24, 25, 26, 27, 20, 21, 22, 23,
                                16, 17, 18, 19, 12, 13, 14, 15, 8,  9,  10, 11, 4,  5,  6,  7};
   const uint8_t sel0[8]     = {0, 0, 0, 0, 0, 0, 1, 0};
   const uint8_t sel1[8]     = {0, 0, 0, 0, 1, 0, 0, 0};
   const uint8_t sel2[8]     = {0, 0, 1, 0, 0, 0, 0, 0};
   const uint8_t sel3[8]     = {0, 1, 0, 0, 0, 0, 0, 0};
   const uint8_t selC1[8]    = {0, 0, 1, 1, 0, 0, 0, 0};
   const uint8_t selC2[8]    = {0, 0, 1, 1, 1, 1, 0, 0};
   memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(0), permute, 32);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(0), sel0, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(1), sel1, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(2), sel2, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(3), sel3, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(4), selC1, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(5), selC2, 8);
}

static inline void downSampleStoreRatio8Masks(uint8_t *pBlock)
{
   const uint8_t permute0[32] = {0, 1, 2, 3, 8,  9,  10, 11, 16, 17, 18, 19, 24, 25, 26, 27,
                                 4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31};
   const uint8_t permute1[32] = {4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31,
                                 0, 1, 2, 3, 8,  9,  10, 11, 16, 17, 18, 19, 24, 25, 26, 27};
   const uint8_t sel0[8]      = {0, 0, 0, 0, 1, 1, 1, 1};
   memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(0), permute0, 32);
   memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(1), permute1, 32);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(0), sel0, 8);
}

static inline void downSampleStoreRatio12Masks(uint8_t *pBlock)
{
   const uint8_t permute0[32] = {0, 1, 2, 3, 12, 13, 14, 15, 24, 25, 26, 27, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0};
   const uint8_t permute1[32] = {0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 4, 5, 6, 7,
                                 16, 17, 18, 19, 28, 29, 30, 31, 0, 0, 0, 0, 0, 0, 0, 0};
   const uint8_t permute2[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 8, 9, 10, 11, 20, 21, 22, 23};
   const uint8_t sel0[8]      = {0, 0, 0, 1, 1, 1, 0, 0};
   const uint8_t sel1[8]      = {0, 0, 0, 0, 0, 0, 1, 1};
   memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(0), permute0, 32);
   memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(1), permute1, 32);
   memcpy(pBlock + DS_MASK_PERMUTE_OFFSET(2), permute2, 32);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(0), sel0, 8);
   memcpy(pBlock + DS_MASK_SEL_OFFSET(1), sel1, 8);
}

/* Read masks back from bufPblock */
static inline uchar32 downSampleLoadPermute(const uint8_t *pBlock, uint32_t i)
{
   uchar32 v;
   memcpy(&v, pBlock + DS_MASK_PERMUTE_OFFSET(i), sizeof(v));
   return v;
}

template <typename dataType>
static inline typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type
downSampleLoadSelect(const uint8_t *pBlock, uint32_t i)
{
   typedef typename std::conditional<sizeof(dataType) == 4, __uchar8, __uchar4>::type v_stage_tp;
   typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type   v_bool_tp;
   v_stage_tp staging;
   memcpy(&staging, pBlock + DS_MASK_SEL_OFFSET(i), sizeof(staging));
   return *(v_bool_tp *) &staging;
}

template <typename T> struct permute;
template <> struct permute<float> {
   static inline uchar32 apply(uchar32 mask, uchar32 vec1, uchar32 vec0)
   {
      return __permute_even_even_int(mask, vec1, vec0);
   }
};

template <> struct permute<double> {
   static inline uchar32 apply(uchar32 mask, uchar32 vec1, uchar32 vec0)
   {
      return __permute_even_even_long(mask, vec1, vec0);
   }
};

void AUDIOLIB_downSample_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;

   uint64_t downSampleStartupCycles   = 0;
   uint64_t downSampleTeardownCycles  = 0;
   uint64_t downSampleOperationCycles = 0;
   uint64_t downSampleOverheadCycles  = 0;
   uint32_t nVecs                     = 0;
   nVecs                              = pKerPrivArgs->nVecs;

   if (pKerPrivArgs->dim_x == 2 && pKerPrivArgs->usesRatio2Path) {
      downSampleStartupCycles   = 28 + 3;
      downSampleOperationCycles = 1 + nVecs * 2 * 2;
      downSampleTeardownCycles  = 2 + 1 + 2;
   }
   else if (pKerPrivArgs->usesRatio2Path || (pKerPrivArgs->isInterleave == 0 && pKerPrivArgs->downSampleRatio == 2)) {
      downSampleStartupCycles   = 28 + 3;
      downSampleOperationCycles = 1 + nVecs * 2 * 2;
      downSampleTeardownCycles  = 2 + 1 + 2;
   }
   else if (pKerPrivArgs->isInterleave == 0) {
      if (pKerPrivArgs->downSampleRatio == 3 ||
          (pKerPrivArgs->downSampleRatio == 6 && pKerPrivArgs->dataType == AUDIOLIB_FLOAT32)) {
         downSampleStartupCycles   = 28 + 3;
         downSampleOperationCycles = 8 + nVecs * 3;
         downSampleTeardownCycles  = 2 + 1 + 2;
      }
      else if (pKerPrivArgs->downSampleRatio == 4) {
         downSampleStartupCycles   = 30 + 4;
         downSampleOperationCycles = 4 + nVecs * 1 * 2;
         downSampleTeardownCycles  = 1 + 1 + 2;
      }
      else if (pKerPrivArgs->downSampleRatio == 5 && pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         downSampleStartupCycles   = 28 + 8;
         downSampleOperationCycles = 8 + nVecs * 8;
         downSampleTeardownCycles  = 8 + 1 + 2;
      }
      else if (pKerPrivArgs->downSampleRatio == 7 && pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         downSampleStartupCycles   = 28 + 2;
         downSampleOperationCycles = 8 + nVecs * 6;
         downSampleTeardownCycles  = 6 + 6 + 1 + 2;
      }
      else if (pKerPrivArgs->downSampleRatio == 8 && pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         downSampleStartupCycles   = 30 + 3;
         downSampleOperationCycles = 5 + nVecs * 2 * 2;
         downSampleTeardownCycles  = 1 + 2 + 1 + 2;
      }
      else if (pKerPrivArgs->downSampleRatio == 12 && pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         downSampleStartupCycles   = 31 + 2;
         downSampleOperationCycles = 5 + nVecs * 3 * 2;
         downSampleTeardownCycles  = 2 + 3 + 1 + 2;
      }
      else {
         downSampleStartupCycles   = 26 + 3;
         downSampleOperationCycles = 1 + nVecs * 1;
         downSampleTeardownCycles  = 3;
      }
   }
   else {
      downSampleStartupCycles   = 26 + 3;
      downSampleOperationCycles = 1 + nVecs * 1;
      downSampleTeardownCycles  = 3;
   }

   downSampleOverheadCycles += downSampleStartupCycles + downSampleTeardownCycles;
   *estCycles  = downSampleOperationCycles + downSampleOverheadCycles;
   *archCycles = downSampleOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_downSample_init_ci(AUDIOLIB_kernelHandle               handle,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                            const AUDIOLIB_downSample_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs                          *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSample_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d dim_x %d dim_y %d\n", pKerPrivArgs->dim_x,
                         pKerPrivArgs->dim_y, pKerPrivArgs->strideIn, eleCount, pKerPrivArgs->dim_x,
                         pKerPrivArgs->dim_y);

   uint32_t blockSizeWithPhase;
   uint32_t channel;
   uint32_t downSampleRatio;
   uint32_t outBlockSize;
   uint32_t eleFetch;
   uint32_t itrCount;
   uint32_t blockSample;

   if (pKerPrivArgs->isInterleave == 1) {
      blockSample        = pKerPrivArgs->dim_y;
      blockSizeWithPhase = pKerPrivArgs->dim_y - pKerPrivArgs->phase;
      channel            = pKerPrivArgs->dim_x;
      downSampleRatio    = pKerPrivArgs->downSampleRatio;
      outBlockSize       = AUDIOLIB_ceilingDiv(blockSizeWithPhase, downSampleRatio);
      eleFetch           = channel > eleCount ? eleCount : channel;
      itrCount           = AUDIOLIB_ceilingDiv(channel, eleCount);
   }
   else {
      blockSizeWithPhase = pKerPrivArgs->dim_x - pKerPrivArgs->phase;
      channel            = pKerPrivArgs->dim_y;
      downSampleRatio    = pKerPrivArgs->downSampleRatio;
      outBlockSize       = AUDIOLIB_ceilingDiv(blockSizeWithPhase, downSampleRatio);
      eleFetch           = eleCount < outBlockSize ? eleCount : outBlockSize;
      itrCount           = AUDIOLIB_ceilingDiv(outBlockSize, eleCount);
      blockSample        = pKerPrivArgs->dim_x;

      pKerPrivArgs->nVecs = channel * AUDIOLIB_ceilingDiv(outBlockSize, eleCount);
   }

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/

   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;

   /**********************************************************************/
   /* Prepare SE/SA template                     */
   /**********************************************************************/
   __SE_TRANSPOSE transposeMode = (sizeof(dataType) == 4) ? __SE_TRANSPOSE_32BIT : __SE_TRANSPOSE_64BIT;

   if ((int32_t) pKerPrivArgs->isInterleave == 1 && pKerPrivArgs->usesRatio2Path == 0) { // Interleave cases

      pKerPrivArgs->nVecs         = outBlockSize * AUDIOLIB_ceilingDiv(channel, eleCount);
      pKerPrivArgs->startingIndex = pKerPrivArgs->phase * pKerPrivArgs->strideIn;

      se0Params.ICNT0         = eleFetch;
      se0Params.DIM1          = pKerPrivArgs->strideIn * downSampleRatio;
      se0Params.ICNT1         = outBlockSize; // outBlockSize;
      se0Params.DIM2          = eleCount;
      se0Params.ICNT2         = itrCount;
      se0Params.DECDIM1       = __SE_DECDIM_DIM2;
      se0Params.DECDIM1_WIDTH = channel;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;

      sa0Params.ICNT0         = eleFetch;
      sa0Params.DIM1          = pKerPrivArgs->strideOut;
      sa0Params.ICNT1         = outBlockSize;
      sa0Params.DIM2          = eleCount;
      sa0Params.ICNT2         = itrCount;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM2;
      sa0Params.DECDIM1_WIDTH = channel;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;

      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   }
   else if ((downSampleRatio == 2 && pKerPrivArgs->isInterleave == 0) || pKerPrivArgs->usesRatio2Path == 1) {

      __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
      __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();
      uint32_t         roundOne  = blockSample / AUDIOLIB_NUM_STREAMING_ENGINES;
      uint32_t         se0Count  = AUDIOLIB_ceilingDiv(roundOne, 2) * 2;
      uint32_t         se1Count;
      uint32_t         outBlockSizeSa0;
      uint32_t         outBlockSizeSa1;

      if (pKerPrivArgs->usesRatio2Path == 1 && pKerPrivArgs->dim_x == 2) {
         typedef typename c7x::make_full_vector<double>::type doubleVec;

         eleCount                       = c7x::element_count_of<doubleVec>::value;
         SE_ELETYPE                     = c7x::se_eletype<doubleVec>::value;
         SE_VECLEN                      = c7x::se_veclen<doubleVec>::value;
         channel                        = 1;
         se0Params.ELETYPE              = SE_ELETYPE;
         se0Params.VECLEN               = SE_VECLEN;
         sa0Params.VECLEN               = SA_VECLEN;
         pKerPrivArgs->startingIndexSE1 = se0Count * 2;
         pKerPrivArgs->startingIndex    = pKerPrivArgs->phase * pKerPrivArgs->dim_x;
         se1Count                       = blockSample - se0Count - pKerInitArgs->phase;
         outBlockSizeSa0                = AUDIOLIB_ceilingDiv(se0Count, 2) * 2; // double to float values
         outBlockSizeSa1                = AUDIOLIB_ceilingDiv(se1Count, 2) * 2;
         pKerPrivArgs->outSa1           = outBlockSizeSa0;
         pKerPrivArgs->nVecs            = AUDIOLIB_ceilingDiv(outBlockSizeSa0, eleCount) * channel;
      }
      else {
         pKerPrivArgs->startingIndex    = pKerPrivArgs->phase;
         se1Count                       = blockSample - se0Count - pKerInitArgs->phase;
         pKerPrivArgs->startingIndexSE1 = se0Count;
         outBlockSizeSa0                = AUDIOLIB_ceilingDiv(se0Count, 2);
         outBlockSizeSa1                = AUDIOLIB_ceilingDiv(se1Count, 2);
         pKerPrivArgs->outSa1           = outBlockSizeSa0;
         pKerPrivArgs->nVecs            = AUDIOLIB_ceilingDiv(outBlockSizeSa0, eleCount) * channel;
      }
      se1Params.ELETYPE = SE_ELETYPE;
      se1Params.VECLEN  = SE_VECLEN;
      sa1Params.VECLEN  = SA_VECLEN;

      se0Params.ICNT0  = AUDIOLIB_ceilingDiv(se0Count, 2) * 2;
      se0Params.DECIM  = __SE_DECIM_2;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.ICNT1  = channel;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      se1Params.ICNT0  = AUDIOLIB_ceilingDiv(se1Count, 2) * 2;
      se1Params.DECIM  = __SE_DECIM_2;
      se1Params.DIM1   = pKerPrivArgs->strideIn;
      se1Params.ICNT1  = channel;
      se1Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = outBlockSizeSa0;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = channel;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      sa1Params.ICNT0  = outBlockSizeSa1;
      sa1Params.DIM1   = pKerPrivArgs->strideOut;
      sa1Params.ICNT1  = channel;
      sa1Params.DIMFMT = __SA_DIMFMT_2D;

      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   }
   else {
      pKerPrivArgs->startingIndex = pKerPrivArgs->phase;

      if (downSampleRatio == 3 || (downSampleRatio == 5 && pKerPrivArgs->dataType == AUDIOLIB_FLOAT32)) {
         __SE_TEMPLATE_v1 se1Params     = __gen_SE_TEMPLATE_v1();
         se1Params.ELETYPE              = SE_ELETYPE;
         se1Params.VECLEN               = SE_VECLEN;
         pKerPrivArgs->startingIndexSE1 = eleCount;
         uint32_t numSEIterations       = AUDIOLIB_ceilingDiv(blockSizeWithPhase, eleCount);

         se0Params.ICNT0         = eleCount;
         se0Params.DIM1          = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES;
         se0Params.ICNT1         = downSampleRatio;
         se0Params.ICNT2         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(numSEIterations, 2), downSampleRatio);
         se0Params.DIM2          = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES * downSampleRatio;
         se0Params.ICNT3         = channel;
         se0Params.DIM3          = pKerPrivArgs->strideIn;
         se0Params.DECDIM1       = __SE_DECDIM_DIM2;
         se0Params.DECDIM1_WIDTH = blockSizeWithPhase;
         se0Params.DIMFMT        = __SE_DIMFMT_4D;

         se1Params.ICNT0         = eleCount;
         se1Params.DIM1          = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES;
         se1Params.ICNT1         = downSampleRatio;
         se1Params.ICNT2         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(numSEIterations, 2), downSampleRatio);
         se1Params.DIM2          = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES * downSampleRatio;
         se1Params.ICNT3         = channel;
         se1Params.DIM3          = pKerPrivArgs->strideIn;
         se1Params.DECDIM1       = __SE_DECDIM_DIM2;
         se1Params.DECDIM1_WIDTH = blockSizeWithPhase - eleCount;
         se1Params.DIMFMT        = __SE_DIMFMT_4D;

         uint32_t numSAIterations = AUDIOLIB_ceilingDiv(outBlockSize, eleCount);

         sa0Params.ICNT0         = eleCount;
         sa0Params.DIM1          = eleCount;
         sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(numSAIterations, AUDIOLIB_NUM_STREAMING_ENGINES) * AUDIOLIB_NUM_STREAMING_ENGINES;
         sa0Params.DIM2          = pKerPrivArgs->strideOut;
         sa0Params.ICNT2         = channel;
         sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
         sa0Params.DECDIM1_WIDTH = outBlockSize;
         sa0Params.DIMFMT        = __SA_DIMFMT_3D;

         pKerPrivArgs->nVecs = channel * AUDIOLIB_ceilingDiv(numSAIterations, 2);

         if (downSampleRatio == 3) {
            downSampleStoreRatio3_6Masks<dataType>(pBlock);
         }
         else { // downSampleRatio == 5 (float only)
            downSampleStoreRatio5Masks(pBlock);
         }

         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
      }
      else if (downSampleRatio == 7 && pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {

         __SE_TEMPLATE_v1 se1Params     = __gen_SE_TEMPLATE_v1();
         se1Params.ELETYPE              = SE_ELETYPE;
         se1Params.VECLEN               = SE_VECLEN;
         pKerPrivArgs->startingIndexSE1 = eleCount;
         uint32_t numSEIterations       = AUDIOLIB_ceilingDiv(blockSizeWithPhase, eleCount);
         uint32_t reps                  = AUDIOLIB_ceilingDiv(numSEIterations, downSampleRatio);

         se0Params.ICNT0         = eleCount;
         se0Params.DIM1          = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES;
         se0Params.ICNT1         = AUDIOLIB_ceilingDiv(downSampleRatio, 2);
         se0Params.ICNT2         = reps;
         se0Params.DIM2          = eleCount * downSampleRatio;
         se0Params.ICNT3         = channel;
         se0Params.DIM3          = pKerPrivArgs->strideIn;
         se0Params.DECDIM1       = __SE_DECDIM_DIM2;
         se0Params.DECDIM1_WIDTH = blockSizeWithPhase;
         se0Params.DIMFMT        = __SE_DIMFMT_4D;

         se1Params.ICNT0         = eleCount;
         se1Params.DIM1          = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES;
         se1Params.ICNT1         = downSampleRatio / 2;
         se1Params.ICNT2         = reps;
         se1Params.DIM2          = eleCount * downSampleRatio;
         se1Params.ICNT3         = channel;
         se1Params.DIM3          = pKerPrivArgs->strideIn;
         se1Params.DECDIM1       = __SE_DECDIM_DIM2;
         se1Params.DECDIM1_WIDTH = blockSizeWithPhase - (eleCount);
         se1Params.DIMFMT        = __SE_DIMFMT_4D;

         uint32_t numSAIterations = AUDIOLIB_ceilingDiv(outBlockSize, eleCount);

         sa0Params.ICNT0         = eleCount;
         sa0Params.DIM1          = eleCount;
         sa0Params.ICNT1         = numSAIterations;
         sa0Params.DIM2          = pKerPrivArgs->strideOut;
         sa0Params.ICNT2         = channel;
         sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
         sa0Params.DECDIM1_WIDTH = outBlockSize;
         sa0Params.DIMFMT        = __SA_DIMFMT_3D;

         pKerPrivArgs->nVecs = channel * numSAIterations;

         downSampleStoreRatio7Masks(pBlock);

         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
      }
      else if (downSampleRatio == 4 || ((downSampleRatio == 6 || downSampleRatio == 8 || downSampleRatio == 12) &&
                                        pKerPrivArgs->dataType == AUDIOLIB_FLOAT32)) {
         __SE_TEMPLATE_v1 se1Params     = __gen_SE_TEMPLATE_v1();
         se1Params.ELETYPE              = SE_ELETYPE;
         se1Params.VECLEN               = SE_VECLEN;
         uint32_t numSEIterations       = AUDIOLIB_ceilingDiv(blockSizeWithPhase, eleCount * 2); // eleCount * DECIM2
         pKerPrivArgs->startingIndexSE1 = eleCount * 2; // eleCount * DECIM2

         uint32_t repeatCount;

         switch (downSampleRatio) {
         case 8:
            repeatCount = 2;
            break;
         case 6:
         case 12:
            repeatCount = 3;
            break;
         default:
            repeatCount = 1;
            break;
         }
         se0Params.ICNT0         = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES;
         se0Params.DECIM         = __SE_DECIM_2;
         se0Params.ICNT1         = repeatCount;
         se0Params.DIM1          = (eleCount * 2 * AUDIOLIB_NUM_STREAMING_ENGINES); // eleCount * Decim2 * 2SE
         se0Params.DIM2          = eleCount * 2 * AUDIOLIB_NUM_STREAMING_ENGINES * repeatCount;
         se0Params.ICNT2         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(numSEIterations, AUDIOLIB_NUM_STREAMING_ENGINES), repeatCount);
         se0Params.DIM3          = pKerPrivArgs->strideIn;
         se0Params.ICNT3         = channel;
         se0Params.DECDIM1       = __SE_DECDIM_DIM2;
         se0Params.DECDIM1_WIDTH = blockSizeWithPhase;
         se0Params.DIMFMT        = __SE_DIMFMT_4D;

         se1Params.ICNT0                = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES;
         se1Params.DECIM                = __SE_DECIM_2;
         se1Params.ICNT1                = repeatCount;
         se1Params.DIM1                 = (eleCount * 2 * AUDIOLIB_NUM_STREAMING_ENGINES); // eleCount * Decim2 * 2SE
         se1Params.DIM2                 = eleCount * 2 * AUDIOLIB_NUM_STREAMING_ENGINES * repeatCount;;
         se1Params.ICNT2                = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(numSEIterations, AUDIOLIB_NUM_STREAMING_ENGINES), repeatCount);
         se1Params.DIM3                 = pKerPrivArgs->strideIn;
         se1Params.ICNT3                = channel;
         se1Params.DECDIM1              = __SE_DECDIM_DIM2;
         se1Params.DECDIM1_WIDTH        = blockSizeWithPhase - eleCount * 2;
         se1Params.DIMFMT               = __SE_DIMFMT_4D;
         pKerPrivArgs->startingIndexSE1 = eleCount * AUDIOLIB_NUM_STREAMING_ENGINES;

         if (downSampleRatio == 6) {
            // Two SA write in a loop
            uint32_t numSAIterations = AUDIOLIB_ceilingDiv(outBlockSize, eleCount);

            sa0Params.ICNT0         = eleCount;
            sa0Params.DIM1          = eleCount;
            sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(numSAIterations, AUDIOLIB_NUM_STREAMING_ENGINES) * AUDIOLIB_NUM_STREAMING_ENGINES;
            sa0Params.DIM2          = pKerPrivArgs->strideOut;
            sa0Params.ICNT2         = channel;
            sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
            sa0Params.DECDIM1_WIDTH = outBlockSize;
            sa0Params.DIMFMT        = __SA_DIMFMT_3D;
         }
         else {
            // One SA write in a loop
            sa0Params.ICNT0  = outBlockSize;
            sa0Params.DIM1   = pKerPrivArgs->strideOut;
            sa0Params.ICNT1  = channel;
            sa0Params.DIMFMT = __SA_DIMFMT_2D;
         }
         pKerPrivArgs->nVecs = channel * AUDIOLIB_ceilingDiv(outBlockSize, eleCount);

         switch (downSampleRatio) {
         case 4:
            downSampleStoreRatio4Masks<dataType>(pBlock);
            break;
         case 6:
            downSampleStoreRatio3_6Masks<dataType>(pBlock);
            break;
         case 8:
            downSampleStoreRatio8Masks(pBlock);
            break;
         case 12:
            downSampleStoreRatio12Masks(pBlock);
            break;
         default:
            break;
         }

         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
      }
      else { // DeInterleave Format
         pKerPrivArgs->nVecs = channel * AUDIOLIB_ceilingDiv(outBlockSize, eleCount);

         se0Params.TRANSPOSE = transposeMode;
         se0Params.ICNT0     = 1;
         se0Params.DIM1      = downSampleRatio;
         se0Params.ICNT1     = eleFetch;
         se0Params.DIM2      = eleCount * downSampleRatio;
         se0Params.ICNT2     = itrCount;
         se0Params.DIM3      = pKerPrivArgs->strideIn;
         se0Params.ICNT3     = channel;
         se0Params.DIMFMT    = __SE_DIMFMT_4D;

         sa0Params.ICNT0  = outBlockSize;
         sa0Params.DIM1   = pKerPrivArgs->strideOut;
         sa0Params.ICNT1  = channel;
         sa0Params.DIMFMT = __SA_DIMFMT_2D;

         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
      }
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_downSample_init_ci<float>(AUDIOLIB_kernelHandle               handle,
                                                            const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                                            const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                                            const AUDIOLIB_downSample_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_downSample_init_ci<double>(AUDIOLIB_kernelHandle               handle,
                                                             const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                                             const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                                             const AUDIOLIB_downSample_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_downSampleRatio2_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleRatio2_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   dataType *restrict pInLocal   = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal  = (dataType *) pOut;
   dataType *restrict pInLocal0  = pInLocal + pKerPrivArgs->startingIndexSE1;
   dataType *restrict pOutLocal0 = pOutLocal + pKerPrivArgs->outSa1;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal0, se1Params);
   __SA1_OPEN(sa1Params);

   for (uint32_t counter = 0; counter < nVecs; counter++) {
      vec downSampleVec0 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec1 = c7x::strm_eng<1, vec>::get_adv();

      __vpred vpred  = c7x::strm_agen<0, vec>::get_vpred();
      __vpred vpred0 = c7x::strm_agen<1, vec>::get_vpred();
      vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      vec    *addr0  = c7x::strm_agen<1, vec>::get_adv(pOutLocal0);
      __vstore_pred(vpred, addr, downSampleVec0);
      __vstore_pred(vpred0, addr0, downSampleVec1);
   }

   __SE1_CLOSE();
   __SA1_CLOSE();
   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio2_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio2_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio2TwoCh_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleRatio2TwoCh_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   dataType *restrict pInLocal   = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal  = (dataType *) pOut;
   dataType *restrict pInLocal0  = pInLocal + pKerPrivArgs->startingIndexSE1;
   dataType *restrict pOutLocal0 = pOutLocal + pKerPrivArgs->outSa1;

   // Two interleaved channels of float are fetched two-at-a-time as a double-wide
   // element and stored back as float8.
   typedef typename c7x::make_full_vector<double>::type twoFetch_vec;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal0, se1Params);
   __SA1_OPEN(sa1Params);

   for (uint32_t counter = 0; counter < nVecs; counter++) {
      twoFetch_vec downSampleWideVec0 = c7x::strm_eng<0, twoFetch_vec>::get_adv();
      twoFetch_vec downSampleWideVec1 = c7x::strm_eng<1, twoFetch_vec>::get_adv();
      float8       downSampleVec0     = __as_float8(downSampleWideVec0);
      float8       downSampleVec1     = __as_float8(downSampleWideVec1);

      __vpred vpred  = c7x::strm_agen<0, float8>::get_vpred();
      __vpred vpred0 = c7x::strm_agen<1, float8>::get_vpred();
      float8 *addr   = c7x::strm_agen<0, float8>::get_adv(pOutLocal);
      float8 *addr0  = c7x::strm_agen<1, float8>::get_adv(pOutLocal0);
      __vstore_pred(vpred, addr, downSampleVec0);
      __vstore_pred(vpred0, addr0, downSampleVec1);
   }

   __SE1_CLOSE();
   __SA1_CLOSE();
   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio2TwoCh_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio3_6_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleRatio3_6_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);

   typedef typename c7x::make_full_vector<dataType>::type vec;

   dataType *restrict pInLocal  = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal = (dataType *) pOut;
   dataType *restrict pInLocal0 = pInLocal + pKerPrivArgs->startingIndexSE1;

   typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;

   v_bool_tp vpMask0 = downSampleLoadSelect<dataType>(pBlock, 0);
   v_bool_tp vpMask1 = downSampleLoadSelect<dataType>(pBlock, 1);
   uchar32   vPMask  = downSampleLoadPermute(pBlock, 0);

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal0, se1Params);

   for (uint32_t counter = 0; counter < nVecs; counter++) {
      vec downSampleVec0 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec1 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec2 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec3 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec4 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec5 = c7x::strm_eng<1, vec>::get_adv();

      vec selectVec01 = __select(vpMask0, downSampleVec1, downSampleVec0);
      vec selectVec11 = __select(vpMask0, downSampleVec4, downSampleVec3);
      vec selectVec02 = __select(vpMask1, downSampleVec2, selectVec01);
      vec selectVec12 = __select(vpMask1, downSampleVec5, selectVec11);
      vec permuteVec0 = c7x::reinterpret<vec>(__permute(vPMask, __as_uchar32(selectVec02)));
      vec permuteVec1 = c7x::reinterpret<vec>(__permute(vPMask, __as_uchar32(selectVec12)));

      __vpred vpred;
      vec    *addr;
      vpred = c7x::strm_agen<0, vec>::get_vpred();
      addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, permuteVec0);
      vpred = c7x::strm_agen<0, vec>::get_vpred();
      addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, permuteVec1);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio3_6_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio3_6_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_downSampleRatio5_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleRatio5_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);

   typedef typename c7x::make_full_vector<dataType>::type vec;

   dataType *restrict pInLocal  = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal = (dataType *) pOut;
   dataType *restrict pInLocal0 = pInLocal + pKerPrivArgs->startingIndexSE1;

   typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;

   v_bool_tp vpMask0 = downSampleLoadSelect<dataType>(pBlock, 0);
   v_bool_tp vpMask1 = downSampleLoadSelect<dataType>(pBlock, 1);
   v_bool_tp vpMask2 = downSampleLoadSelect<dataType>(pBlock, 2);
   v_bool_tp vpMaskC = downSampleLoadSelect<dataType>(pBlock, 3);
   uchar32   vPMask  = downSampleLoadPermute(pBlock, 0);

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal0, se1Params);
   for (uint32_t counter = 0; counter < nVecs; counter++) {

      vec downSampleVec0 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec1 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec2 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec3 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec4 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec5 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec6 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec7 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec8 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec9 = c7x::strm_eng<1, vec>::get_adv();

      vec selectVec01 = __select(vpMask0, downSampleVec1, downSampleVec0);
      vec selectVec02 = __select(vpMask1, downSampleVec3, downSampleVec2);
      vec selectCVec1 = __select(vpMaskC, selectVec02, selectVec01);
      vec selectVec03 = __select(vpMask2, downSampleVec4, selectCVec1);
      vec selectVec11 = __select(vpMask0, downSampleVec6, downSampleVec5);
      vec selectVec12 = __select(vpMask1, downSampleVec8, downSampleVec7);
      vec selectCVec2 = __select(vpMaskC, selectVec12, selectVec11);
      vec selectVec13 = __select(vpMask2, downSampleVec9, selectCVec2);
      vec permuteVec0 = c7x::reinterpret<vec>(__permute(vPMask, __as_uchar32(selectVec03)));
      vec permuteVec1 = c7x::reinterpret<vec>(__permute(vPMask, __as_uchar32(selectVec13)));

      __vpred vpred;
      vec    *addr;
      vpred = c7x::strm_agen<0, vec>::get_vpred();
      addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, permuteVec0);
      vpred = c7x::strm_agen<0, vec>::get_vpred();
      addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, permuteVec1);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio5_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_downSampleRatio7_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleRatio7_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);

   typedef typename c7x::make_full_vector<dataType>::type vec;

   dataType *restrict pInLocal  = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal = (dataType *) pOut;
   dataType *restrict pInLocal0 = pInLocal + pKerPrivArgs->startingIndexSE1;

   typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;

   v_bool_tp vpMask0  = downSampleLoadSelect<dataType>(pBlock, 0);
   v_bool_tp vpMask1  = downSampleLoadSelect<dataType>(pBlock, 1);
   v_bool_tp vpMask2  = downSampleLoadSelect<dataType>(pBlock, 2);
   v_bool_tp vpMask3  = downSampleLoadSelect<dataType>(pBlock, 3);
   v_bool_tp vpMaskC1 = downSampleLoadSelect<dataType>(pBlock, 4);
   v_bool_tp vpMaskC2 = downSampleLoadSelect<dataType>(pBlock, 5);

   uchar32 vPMask = downSampleLoadPermute(pBlock, 0);

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal0, se1Params);
   for (uint32_t counter = 0; counter < nVecs; counter++) {
      vec downSampleVec0 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec1 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec2 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec3 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec4 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec5 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec6 = c7x::strm_eng<0, vec>::get_adv();

      vec selectVec01  = __select(vpMask0, downSampleVec1, downSampleVec0);
      vec selectVec02  = __select(vpMask1, downSampleVec3, downSampleVec2);
      vec selectVec03  = __select(vpMask2, downSampleVec5, downSampleVec4);
      vec selectVec04  = __select(vpMask3, downSampleVec6, selectVec01);
      vec selectVecC01 = __select(vpMaskC1, selectVec03, selectVec02);
      vec selectVecC02 = __select(vpMaskC2, selectVecC01, selectVec04);
      vec permuteVec0  = c7x::reinterpret<vec>(__permute(vPMask, __as_uchar32(selectVecC02)));

      __vpred vpred;
      vec    *addr;
      vpred = c7x::strm_agen<0, vec>::get_vpred();
      addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, permuteVec0);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio7_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_downSampleRatio4_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleRatio4_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);

   typedef typename c7x::make_full_vector<dataType>::type vec;

   dataType *restrict pInLocal  = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal = (dataType *) pOut;
   dataType *restrict pInLocal0 = pInLocal + pKerPrivArgs->startingIndexSE1;

   uchar32 permMask = downSampleLoadPermute(pBlock, 0);

   using permuteHelper = permute<dataType>;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal0, se1Params);

   for (uint32_t counter = 0; counter < nVecs; counter++) {
      vec downSampleVec0 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec1 = c7x::strm_eng<1, vec>::get_adv();

      vec resultVec;
      resultVec = c7x::reinterpret<vec>(
          permuteHelper::apply(permMask, __as_uchar32(downSampleVec1), __as_uchar32(downSampleVec0)));

      __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, resultVec);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio4_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio4_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_downSampleRatio8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleRatio8_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);

   typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;
   typedef typename c7x::make_full_vector<dataType>::type                           vec;

   dataType *restrict pInLocal  = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal = (dataType *) pOut;
   dataType *restrict pInLocal0 = pInLocal + pKerPrivArgs->startingIndexSE1;

   uchar32   permMask0 = downSampleLoadPermute(pBlock, 0);
   uchar32   permMask1 = downSampleLoadPermute(pBlock, 1);
   v_bool_tp sel0      = downSampleLoadSelect<dataType>(pBlock, 0);

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal0, se1Params);

   for (uint32_t counter = 0; counter < nVecs; counter++) {
      vec downSampleVec0 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec1 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec2 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec3 = c7x::strm_eng<1, vec>::get_adv();

      vec permuteVec0 = c7x::reinterpret<vec>(
          __permute_even_even_int(permMask0, __as_uchar32(downSampleVec1), __as_uchar32(downSampleVec0)));
      vec permuteVec1 = c7x::reinterpret<vec>(
          __permute_even_even_int(permMask1, __as_uchar32(downSampleVec3), __as_uchar32(downSampleVec2)));
      vec resultVec;
      resultVec = __select(sel0, permuteVec1, permuteVec0);

      __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, resultVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio8_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio12_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleRatio12_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);

   typedef typename std::conditional<sizeof(dataType) == 4, __bool8, __bool4>::type v_bool_tp;
   typedef typename c7x::make_full_vector<dataType>::type                           vec;

   dataType *restrict pInLocal  = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal = (dataType *) pOut;
   dataType *restrict pInLocal0 = pInLocal + pKerPrivArgs->startingIndexSE1;

   uchar32 permMask0 = downSampleLoadPermute(pBlock, 0);
   uchar32 permMask1 = downSampleLoadPermute(pBlock, 1);
   uchar32 permMask2 = downSampleLoadPermute(pBlock, 2);

   v_bool_tp sel0 = downSampleLoadSelect<dataType>(pBlock, 0);
   v_bool_tp sel1 = downSampleLoadSelect<dataType>(pBlock, 1);

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal0, se1Params);

   for (uint32_t counter = 0; counter < nVecs; counter++) {
      vec downSampleVec0 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec1 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec2 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec3 = c7x::strm_eng<1, vec>::get_adv();
      vec downSampleVec4 = c7x::strm_eng<0, vec>::get_adv();
      vec downSampleVec5 = c7x::strm_eng<1, vec>::get_adv();

      vec permuteVec0 = c7x::reinterpret<vec>(
          __permute_even_even_int(permMask0, __as_uchar32(downSampleVec1), __as_uchar32(downSampleVec0)));
      vec permuteVec1 = c7x::reinterpret<vec>(
          __permute_even_even_int(permMask1, __as_uchar32(downSampleVec3), __as_uchar32(downSampleVec2)));
      vec permuteVec2 = c7x::reinterpret<vec>(
          __permute_even_even_int(permMask2, __as_uchar32(downSampleVec5), __as_uchar32(downSampleVec4)));
      vec combinePer1_2 = __select(sel0, permuteVec1, permuteVec0);
      vec resultVec     = __select(sel1, permuteVec2, combinePer1_2);

      __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, resultVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio12_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleGeneric_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_downSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_downSample_PrivArgs *) handle;
   uint8_t                      *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                      nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_downSampleGeneric_exec_ci nVecs %d\n", nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn + pKerPrivArgs->startingIndex;
   dataType *restrict pOutLocal = (dataType *) pOut;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t counter = 0; counter < nVecs; counter++) {
      vec     downSampleVec0 = c7x::strm_eng<0, vec>::get_adv();
      __vpred vpred          = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr           = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, downSampleVec0);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_downSampleGeneric_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_downSampleGeneric_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
