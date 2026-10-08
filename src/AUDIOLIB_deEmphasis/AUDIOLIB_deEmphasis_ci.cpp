// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_deEmphasis_priv.h"

void AUDIOLIB_deEmphasis_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   uint64_t                      loopCycles;
   uint64_t                      overheadCycles;
   AUDIOLIB_deEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_deEmphasis_PrivArgs *) handle;

   /* Mirrors the variant selection in AUDIOLIB_deEmphasis_init. */
   if (pKerPrivArgs->isInterleave == 1U) {
      /* The last block is padded to whole with masked rows. The inner loop runs at ii = 19 per
       * 16-row block (see AUDIOLIB_deEmphasis_interleave_exec_ci). */
      const uint32_t nBlocks = pKerPrivArgs->ncols;

      loopCycles     = (uint64_t) pKerPrivArgs->nrows * (27 + (uint64_t) nBlocks * 19);
      overheadCycles = 52 + (uint64_t) pKerPrivArgs->nrows * 15;
   }
   else {
      /* ncols counts vector pairs per row; nrows is the even part of the channel count. */
      const uint64_t singleChannelLoop = 50 + (uint64_t) pKerPrivArgs->ncols * 8;

      if (pKerPrivArgs->numChannels == 1U) {
         loopCycles     = singleChannelLoop;
         overheadCycles = 61;
      }
      else {
         const uint64_t nPairs      = pKerPrivArgs->nrows / 2U;
         const uint64_t tailChannel = pKerPrivArgs->numChannels & 1U;

         loopCycles     = nPairs * (49 + (uint64_t) pKerPrivArgs->ncols * 13) + tailChannel * singleChannelLoop;
         overheadCycles = 52 + nPairs * 22 + tailChannel * 14;
      }
   }

   *estCycles  = loopCycles + overheadCycles;
   *archCycles = loopCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_deEmphasis_init_ci(AUDIOLIB_kernelHandle               handle,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                            const AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_deEmphasis_PrivArgs                          *pKerPrivArgs = (AUDIOLIB_deEmphasis_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   (void) bufParamsOut; /* Strides were already resolved by the caller. */

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_deEmphasis_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "samples %d numChannels %d strideIn %d strideOut %d eleCount %d\n", pKerPrivArgs->samples,
                         pKerPrivArgs->numChannels, pKerPrivArgs->strideIn, pKerPrivArgs->strideOut, eleCount);

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();

   __SA_TEMPLATE_v1 sa1Params  = __gen_SA_TEMPLATE_v1();
   __SE_ELETYPE     SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN      SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN  = c7x::sa_veclen<vec>::value;

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;

   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   sa1Params.VECLEN  = SA_VECLEN;

   if (pKerInitArgs->isInterleave == 1) {
      /* Channels are contiguous, so a vector holds eleCount independent filters and the loop
       * walks the sample axis in whole blocks. DECDIM on DIM2 masks the final partial channel
       * group; DECDIM on DIM1 zeroes the rows past the last sample, which the stream neither
       * fetches nor stores.
       *
       * SE0 streams the even rows and SE1 the odd rows. With a single stream the compiler
       * folds the block's last read into the add that waits on the multiply of the one
       * before it, and that ordered read chain, not the filter, bounded the loop (ii = 11).
       * Two streams keep each chain to four reads. */
      const uint32_t rowPairs =
          AUDIOLIB_ceilingDiv((uint32_t) pKerPrivArgs->samples, AUDIOLIB_DEEMPHASIS_INTERLEAVE_BLOCK) *
          (AUDIOLIB_DEEMPHASIS_INTERLEAVE_BLOCK / 2U);

      se0Params.ICNT0         = (pKerPrivArgs->numChannels < eleCount) ? pKerPrivArgs->numChannels : eleCount;
      se0Params.ICNT1         = rowPairs;
      se0Params.DIM1          = 2 * pKerPrivArgs->strideIn;
      se0Params.DECDIM1_WIDTH = pKerPrivArgs->samples * pKerPrivArgs->strideIn;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, eleCount);
      se0Params.DIM2          = eleCount;
      se0Params.DECDIM2_WIDTH = pKerPrivArgs->numChannels;
      se0Params.DECDIM2       = __SE_DECDIM_DIM2;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;

      /* Same walk one row down, so it has one row fewer before the padding starts. */
      se1Params               = se0Params;
      se1Params.DECDIM1_WIDTH = (pKerPrivArgs->samples - 1) * pKerPrivArgs->strideIn;
      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;

      sa0Params.ICNT0 = (pKerPrivArgs->numChannels < eleCount) ? pKerPrivArgs->numChannels : eleCount;
      sa0Params.ICNT1 = AUDIOLIB_ceilingDiv((uint32_t) pKerPrivArgs->samples, AUDIOLIB_DEEMPHASIS_INTERLEAVE_BLOCK) *
                        AUDIOLIB_DEEMPHASIS_INTERLEAVE_BLOCK;
      sa0Params.DIM1  = pKerPrivArgs->strideOut;
      sa0Params.DECDIM2_WIDTH = pKerPrivArgs->samples * pKerPrivArgs->strideOut;
      sa0Params.DECDIM2       = __SA_DECDIM_DIM1;
      sa0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, eleCount);
      sa0Params.DIM2          = eleCount;
      sa0Params.DECDIM1_WIDTH = pKerPrivArgs->numChannels;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM2;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;

      /* Writes one filter-state vector per channel group back into yPrev[]. */
      sa1Params.ICNT0  = pKerPrivArgs->numChannels;
      sa1Params.DIMFMT = __SA_DIMFMT_1D;

      pKerPrivArgs->nrows = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, eleCount);
      pKerPrivArgs->ncols = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, AUDIOLIB_DEEMPHASIS_INTERLEAVE_BLOCK);

      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   }
   else {
      /* Samples are contiguous, so the recursion runs along the vectorized axis and
       * is resolved by the in-vector scan in the exec function. A plain 2D walk of
       * the buffer is all the streaming engine has to do. */
      uint32_t unroll = (pKerPrivArgs->numChannels > 1) ? 2 : 1;

      se0Params.ICNT0   = ((uint32_t) pKerPrivArgs->samples < eleCount) ? pKerPrivArgs->samples : eleCount;
      se0Params.ICNT1   = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv((uint32_t) pKerPrivArgs->samples, eleCount), 2) * 2;
      se0Params.DIM1    = eleCount;
      se0Params.ICNT2   = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, unroll);
      se0Params.DIM2    = pKerPrivArgs->strideIn * unroll;
      se0Params.DIMFMT  = __SE_DIMFMT_3D;
      se0Params.DECDIM1 = __SE_DECDIM_DIM1;
      se0Params.DECDIM1_WIDTH = pKerPrivArgs->samples;
      se0Params.DECDIM2       = __SE_DECDIM_DIM2;
      se0Params.DECDIM2_WIDTH = pKerPrivArgs->numChannels * pKerPrivArgs->strideIn;

      sa0Params.ICNT0         = ((uint32_t) pKerPrivArgs->samples < eleCount) ? pKerPrivArgs->samples : eleCount;
      sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount), 2) * 2;
      sa0Params.DIM1          = eleCount;
      sa0Params.DIM2          = pKerPrivArgs->strideOut * unroll;
      sa0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, unroll);
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = pKerPrivArgs->samples;
      sa0Params.DECDIM2       = __SA_DECDIM_DIM2;
      sa0Params.DECDIM2_WIDTH = pKerPrivArgs->numChannels * pKerPrivArgs->strideOut;

      if (pKerPrivArgs->numChannels > 1) {
         se1Params.ICNT0         = ((uint32_t) pKerPrivArgs->samples < eleCount) ? pKerPrivArgs->samples : eleCount;
         se1Params.ICNT1         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount), 2) * 2;
         se1Params.DIM1          = eleCount;
         se1Params.ICNT2         = pKerPrivArgs->numChannels / 2;
         se1Params.DIM2          = pKerPrivArgs->strideIn * 2;
         se1Params.DIMFMT        = __SE_DIMFMT_3D;
         se1Params.DECDIM1       = __SE_DECDIM_DIM1;
         se1Params.DECDIM1_WIDTH = pKerPrivArgs->samples;
         se1Params.DECDIM2       = __SE_DECDIM_DIM2;
         se1Params.DECDIM2_WIDTH = (pKerPrivArgs->numChannels * pKerPrivArgs->strideIn) - pKerPrivArgs->strideIn;

         sa1Params.ICNT0         = ((uint32_t) pKerPrivArgs->samples < eleCount) ? pKerPrivArgs->samples : eleCount;
         sa1Params.ICNT1         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount), 2) * 2;
         sa1Params.DIM1          = eleCount;
         sa1Params.DIM2          = pKerPrivArgs->strideOut * 2;
         sa1Params.ICNT2         = pKerPrivArgs->numChannels / 2;
         sa1Params.DIMFMT        = __SA_DIMFMT_3D;
         sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
         sa1Params.DECDIM1_WIDTH = pKerPrivArgs->samples;
         sa1Params.DECDIM2       = __SA_DECDIM_DIM2;
         sa1Params.DECDIM2_WIDTH = (pKerPrivArgs->numChannels * pKerPrivArgs->strideOut) - pKerPrivArgs->strideOut;

         *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
         *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
      }

      pKerPrivArgs->nrows = (pKerPrivArgs->numChannels / 2) * 2;
      pKerPrivArgs->ncols = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount), 2);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_deEmphasis_init_ci<float>(AUDIOLIB_kernelHandle               handle,
                                                            const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                                            const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                                            const AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs);

static inline float8 AUDIOLIB_deEmphasisDelay1(float8 v)
{
   return c7x::reinterpret<float8>(
       __shift_left_full(c7x::reinterpret<ulong4>(v), (uchar) AUDIOLIB_DEEMPHASIS_BITS_PER_FLOAT));
}

static inline float8 AUDIOLIB_deEmphasisDelay2(float8 v)
{
   return c7x::reinterpret<float8>(
       __shift_left_full(c7x::reinterpret<ulong4>(v), (uchar) (2U * AUDIOLIB_DEEMPHASIS_BITS_PER_FLOAT)));
}

/* Byte permute on .C (VPERM). Used for the four-lane delay, which a shift cannot do in one
 * instruction, and for broadcasting the last lane, which otherwise costs VGETW + VDUPW. Both
 * take the .L/.S units' load off the scan, which is what bounds the non-interleaved loops.
 * The direct intrinsic is used because the delay control zero-fills bytes (control bits
 * [7:6] = 01), which __permute() leaves unspecified. */
static inline float8 AUDIOLIB_deEmphasisPermute(uchar32 ctrl, float8 v)
{
   return c7x::reinterpret<float8>(__vperm_vvv(ctrl, c7x::reinterpret<uchar32>(v)));
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_interleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_deEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_deEmphasis_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_deEmphasis_interleave_exec_ci nrows %d ncols %d\n", pKerPrivArgs->nrows,
                         pKerPrivArgs->ncols);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   /* Carry weights alpha^1 .. alpha^4 inside a 4-row sub-block, and alpha^4k for the carry
    * into the end of sub-block k. All are precomputed in init. */
   vec aPow1Vec  = vec(pKerPrivArgs->alphaPow1);
   vec aPow2Vec  = vec(pKerPrivArgs->alphaPow2);
   vec aPow3Vec  = vec(pKerPrivArgs->alphaPow[2]);
   vec aPow4Vec  = vec(pKerPrivArgs->alphaPow4);
   vec aPow8Vec  = vec(pKerPrivArgs->alphaPow[AUDIOLIB_DEEMPHASIS_LANES_F32 - 1]);
   vec aPow12Vec = vec(pKerPrivArgs->alphaPow12);
   vec aPow16Vec = vec(pKerPrivArgs->alphaPow16);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pKerPrivArgs->strideIn, se1Params); // Odd rows
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   const uint32_t nrows   = pKerPrivArgs->nrows;
   const uint32_t nBlocks = pKerPrivArgs->ncols;
   float *restrict yPrev  = pKerPrivArgs->yPrev;

   /* Last real output row, from which each group's state is read back. */
   const dataType *restrict pLastRow = pOutLocal + (pKerPrivArgs->samples - 1) * pKerPrivArgs->strideOut;

   for (uint32_t i = 0; i < nrows; i++) {
      /* One filter state per lane: lane c holds y[-1] for channel i*eleCount + c. */
      vec yVec = *stov_ptr(vec, &yPrev[i * AUDIOLIB_DEEMPHASIS_LANES_F32]);

      /* ii = 19 per block of 16 vectors, 1.19 per vector, for float (C7504, from the compiled
       * .asm). Resource bound is 18 (.M/.N, 35 multiplies). Blocks of 8 run at ii = 10 and
       * blocks of 12 at ii = 15 (1.25 per vector); blocks of 20 or more run out of registers. */
      for (uint32_t j = 0; j < nBlocks; j++) {
         vec inVec0 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec1 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec2 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec3 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec4 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec5 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec6 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec7 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec8 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec9 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec10 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec11 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec12 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec13 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec14 = c7x::strm_eng<0, vec>::get_adv();
         vec inVec15 = c7x::strm_eng<1, vec>::get_adv();

         /* Scan inside each 4-row sub-block, without the carry. The pair form keeps every
          * stream read one multiply from its add, so the ordered reads do not chain. */
         vec scan0 = inVec0;
         vec scan1 = inVec1 + aPow1Vec * inVec0;
         vec p3 = inVec3 + aPow1Vec * inVec2;
         vec scan2 = inVec2 + aPow1Vec * scan1;
         vec scan3 = p3 + aPow2Vec * scan1;
         vec scan4 = inVec4;
         vec scan5 = inVec5 + aPow1Vec * inVec4;
         vec p7 = inVec7 + aPow1Vec * inVec6;
         vec scan6 = inVec6 + aPow1Vec * scan5;
         vec scan7 = p7 + aPow2Vec * scan5;
         vec scan8 = inVec8;
         vec scan9 = inVec9 + aPow1Vec * inVec8;
         vec p11 = inVec11 + aPow1Vec * inVec10;
         vec scan10 = inVec10 + aPow1Vec * scan9;
         vec scan11 = p11 + aPow2Vec * scan9;
         vec scan12 = inVec12;
         vec scan13 = inVec13 + aPow1Vec * inVec12;
         vec p15 = inVec15 + aPow1Vec * inVec14;
         vec scan14 = inVec14 + aPow1Vec * scan13;
         vec scan15 = p15 + aPow2Vec * scan13;

         /* Scan of the whole block up to the end of each sub-block, still without the carry. */
         vec end3 = scan3;
         vec end7 = scan7 + aPow4Vec * end3;
         vec end11 = scan11 + aPow4Vec * end7;
         vec end15 = scan15 + aPow4Vec * end11;

         /* Output at the end of each sub-block: its scan plus the carry-in weighted by alpha^(row+1).
          * The last one is the next carry; it depends on yVec through one multiply-add only. */
         vec out3 = end3 + aPow4Vec * yVec;
         vec out7 = end7 + aPow8Vec * yVec;
         vec out11 = end11 + aPow12Vec * yVec;
         vec out15 = end15 + aPow16Vec * yVec;

         /* Other rows: the sub-block scan plus its carry-in, the previous sub-block's last output. */
         vec out0 = scan0 + aPow1Vec * yVec;
         vec out1 = scan1 + aPow2Vec * yVec;
         vec out2 = scan2 + aPow3Vec * yVec;
         vec out4 = scan4 + aPow1Vec * out3;
         vec out5 = scan5 + aPow2Vec * out3;
         vec out6 = scan6 + aPow3Vec * out3;
         vec out8 = scan8 + aPow1Vec * out7;
         vec out9 = scan9 + aPow2Vec * out7;
         vec out10 = scan10 + aPow3Vec * out7;
         vec out12 = scan12 + aPow1Vec * out11;
         vec out13 = scan13 + aPow2Vec * out11;
         vec out14 = scan14 + aPow3Vec * out11;

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

         __vpred vpred4 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr4  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred4, addr4, out4);

         __vpred vpred5 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr5  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred5, addr5, out5);

         __vpred vpred6 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr6  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred6, addr6, out6);

         __vpred vpred7 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr7  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred7, addr7, out7);
         __vpred vpred8 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr8  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred8, addr8, out8);
         __vpred vpred9 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr9  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred9, addr9, out9);
         __vpred vpred10 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr10  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred10, addr10, out10);
         __vpred vpred11 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr11  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred11, addr11, out11);
         __vpred vpred12 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr12  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred12, addr12, out12);
         __vpred vpred13 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr13  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred13, addr13, out13);
         __vpred vpred14 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr14  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred14, addr14, out14);
         __vpred vpred15 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr15  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred15, addr15, out15);

         yVec = out15;
      }

      yVec = *stov_ptr(vec, (dataType *) &pLastRow[i * AUDIOLIB_DEEMPHASIS_LANES_F32]);
      /* Hand the tail state to the next exec() call. */
      __vpred vpredState = c7x::strm_agen<1, vec>::get_vpred();
      vec    *addrState  = c7x::strm_agen<1, vec>::get_adv(&yPrev[0]);
      __vstore_pred(vpredState, addrState, yVec);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_interleave_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_deEmphasis_nonInterleaveSingleChannel_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                       void *restrict pIn,
                                                                       void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_deEmphasis_PrivArgs                          *pKerPrivArgs = (AUDIOLIB_deEmphasis_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_deEmphasis_nonInterleaveSingleChannel_exec_ci ncols %d\n",
                         pKerPrivArgs->ncols);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   /* Scan-stage weights, the per-lane carry weights [alpha^1 .. alpha^8] and [alpha^9 .. alpha^16]
    * for the two vectors of a pair, and the carry weight across a pair (alpha^16). */
   vec a1Vec  = vec(pKerPrivArgs->alphaPow1);
   vec a2Vec  = vec(pKerPrivArgs->alphaPow2);
   vec a4Vec  = vec(pKerPrivArgs->alphaPow4);
   vec powVec = *stov_ptr(vec, &pKerPrivArgs->alphaPow[0]);
   const uchar32 delay4Ctrl = *(const uchar32 *) &pKerPrivArgs->delay4Ctrl[0];
   const uchar32 bcastCtrl  = *(const uchar32 *) &pKerPrivArgs->bcastLastCtrl[0];
   vec a8Vec  = vec(pKerPrivArgs->alphaPow[AUDIOLIB_DEEMPHASIS_LANES_F32 - 1]);
   vec pow8Vec = powVec * a8Vec; // Lane i: alpha^(i+9)

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   const uint32_t ncols    = pKerPrivArgs->ncols;
   float *restrict yPrev   = pKerPrivArgs->yPrev;
   vec            a16Vec   = a8Vec * a8Vec;
   vec            carryVec = vec(yPrev[0]);
   const uint32_t lastIdx  = pKerPrivArgs->samples - 1; // Read before the loop; its stores may alias the handle.

   /* ii = 8 cycles per pair of vectors, 4.0 per vector, for float (C7504, from the compiled
    * .asm). Resource bound is 7; register lifetimes fail ii = 7. */
   for (uint32_t j = 0; j < ncols; j++) {
      vec scanVec0 = c7x::strm_eng<0, vec>::get_adv();
      vec scanVec1 = c7x::strm_eng<0, vec>::get_adv();

      scanVec0 = scanVec0 + a1Vec * AUDIOLIB_deEmphasisDelay1(scanVec0);
      scanVec1 = scanVec1 + a1Vec * AUDIOLIB_deEmphasisDelay1(scanVec1);
      scanVec0 = scanVec0 + a2Vec * AUDIOLIB_deEmphasisDelay2(scanVec0);
      scanVec1 = scanVec1 + a2Vec * AUDIOLIB_deEmphasisDelay2(scanVec1);
      scanVec0 = scanVec0 + a4Vec * AUDIOLIB_deEmphasisPermute(delay4Ctrl, scanVec0);
      scanVec1 = scanVec1 + a4Vec * AUDIOLIB_deEmphasisPermute(delay4Ctrl, scanVec1);

      /* Extend the second scan across the first: lane i gains alpha^(i+1) times the first
       * vector's last lane, making it a 16-sample scan. Input only, so off the carried path. */
      scanVec1 = scanVec1 + powVec * AUDIOLIB_deEmphasisPermute(bcastCtrl, scanVec0);
      vec lastVec1 = AUDIOLIB_deEmphasisPermute(bcastCtrl, scanVec1);

      /* Lane i of the first vector still owes alpha^(i+1) * y[-1], and of the second
       * alpha^(i+9) * y[-1]. */
      vec outVec0   = scanVec0 + powVec * carryVec;
      vec outVec1   = scanVec1 + pow8Vec * carryVec;

      __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred0, addr0, outVec0);

      __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred1, addr1, outVec1);

      carryVec = lastVec1 + a16Vec * carryVec;
   }

   /* The carry's last lane can be padding, so the state is read back from the output. */
   yPrev[0] = (float) pOutLocal[lastIdx];

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_deEmphasis_nonInterleaveSingleChannel_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                       void *restrict pIn,
                                                                                       void *restrict pOut);

template <typename dataType, bool hasTailChannel>
AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_nonInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_deEmphasis_PrivArgs                          *pKerPrivArgs = (AUDIOLIB_deEmphasis_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_deEmphasis_nonInterleave_exec_ci nrows %d ncols %d\n", pKerPrivArgs->nrows,
                         pKerPrivArgs->ncols);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   dataType *restrict pInLocal   = (dataType *) pIn;
   dataType *restrict pOutLocal  = (dataType *) pOut;
   dataType *restrict pOutLocal1 = ((dataType *) pOut) + pKerPrivArgs->strideOut;

   /* Scan-stage weights, the per-lane carry weights [alpha^1 .. alpha^8] and [alpha^9 .. alpha^16]
    * for the two vectors of a pair, and the carry weight across a pair (alpha^16). */
   vec      a1Vec        = vec(pKerPrivArgs->alphaPow1);
   vec      a2Vec        = vec(pKerPrivArgs->alphaPow2);
   vec      a4Vec        = vec(pKerPrivArgs->alphaPow4);
   vec      powVec       = *stov_ptr(vec, &pKerPrivArgs->alphaPow[0]);
   const uchar32 delay4Ctrl = *(const uchar32 *) &pKerPrivArgs->delay4Ctrl[0];
   const uchar32 bcastCtrl  = *(const uchar32 *) &pKerPrivArgs->bcastLastCtrl[0];
   vec      a8Vec        = vec(pKerPrivArgs->alphaPow[AUDIOLIB_DEEMPHASIS_LANES_F32 - 1]);
   vec      a16Vec       = vec(pKerPrivArgs->alphaPow16);
   vec      pow8Vec      = powVec * a8Vec; // Lane i: alpha^(i+9)
   uint32_t strideOut    = pKerPrivArgs->strideOut;
   uint32_t sample       = pKerPrivArgs->samples - 1;
   float *restrict yPrev = &pKerPrivArgs->yPrev[0];
   const uint32_t nrows  = pKerPrivArgs->nrows;
   const uint32_t ncols  = pKerPrivArgs->ncols;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   __SE1_OPEN(pInLocal + pKerPrivArgs->strideIn, se1Params);
   __SA1_OPEN(sa1Params);
   uint32_t i = 0;
   for (i = 0; i < nrows; i += 2) {
      /* Carry-in for the first vector of this channel is y[-1] from the last call. */
      vec carryVecCh0U0 = vec(yPrev[i]);
      vec carryVecCh1U0 = vec(yPrev[i + 1]);

      /* ii = 13 cycles per 2 channels x 2 vectors, 3.25 per vector, for float (C7504, from the
       * compiled .asm). Resource bound is 12 (.L/.S/.C); register lifetimes fail ii = 12. */
      for (uint32_t j = 0; j < ncols; j++) {
         vec scanVecCh0U0 = c7x::strm_eng<0, vec>::get_adv();
         vec scanVecCh0U1 = c7x::strm_eng<0, vec>::get_adv();

         vec scanVecCh1U0 = c7x::strm_eng<1, vec>::get_adv();
         vec scanVecCh1U1 = c7x::strm_eng<1, vec>::get_adv();

         scanVecCh0U0 = scanVecCh0U0 + a1Vec * AUDIOLIB_deEmphasisDelay1(scanVecCh0U0);
         scanVecCh0U1 = scanVecCh0U1 + a1Vec * AUDIOLIB_deEmphasisDelay1(scanVecCh0U1);
         scanVecCh0U0 = scanVecCh0U0 + a2Vec * AUDIOLIB_deEmphasisDelay2(scanVecCh0U0);
         scanVecCh0U1 = scanVecCh0U1 + a2Vec * AUDIOLIB_deEmphasisDelay2(scanVecCh0U1);
         scanVecCh0U0 = scanVecCh0U0 + a4Vec * AUDIOLIB_deEmphasisPermute(delay4Ctrl, scanVecCh0U0);
         scanVecCh0U1 = scanVecCh0U1 + a4Vec * AUDIOLIB_deEmphasisPermute(delay4Ctrl, scanVecCh0U1);

         scanVecCh1U0 = scanVecCh1U0 + a1Vec * AUDIOLIB_deEmphasisDelay1(scanVecCh1U0);
         scanVecCh1U1 = scanVecCh1U1 + a1Vec * AUDIOLIB_deEmphasisDelay1(scanVecCh1U1);
         scanVecCh1U0 = scanVecCh1U0 + a2Vec * AUDIOLIB_deEmphasisDelay2(scanVecCh1U0);
         scanVecCh1U1 = scanVecCh1U1 + a2Vec * AUDIOLIB_deEmphasisDelay2(scanVecCh1U1);
         scanVecCh1U0 = scanVecCh1U0 + a4Vec * AUDIOLIB_deEmphasisPermute(delay4Ctrl, scanVecCh1U0);
         scanVecCh1U1 = scanVecCh1U1 + a4Vec * AUDIOLIB_deEmphasisPermute(delay4Ctrl, scanVecCh1U1);

         /* Extend the second scan across the first: lane i gains alpha^(i+1) times the first
          * vector's last lane, making it a 16-sample scan. Input only, so off the carried path. */
         scanVecCh0U1 = scanVecCh0U1 + powVec * AUDIOLIB_deEmphasisPermute(bcastCtrl, scanVecCh0U0);
         scanVecCh1U1 = scanVecCh1U1 + powVec * AUDIOLIB_deEmphasisPermute(bcastCtrl, scanVecCh1U0);

         /* Lane i of the first vector still owes alpha^(i+1) * y[-1], and of the second
          * alpha^(i+9) * y[-1]. */
         vec outVecCh0U0   = scanVecCh0U0 + powVec * carryVecCh0U0;
         vec outVecCh0U1   = scanVecCh0U1 + pow8Vec * carryVecCh0U0;

         vec outVecCh1U0   = scanVecCh1U0 + powVec * carryVecCh1U0;
         vec outVecCh1U1   = scanVecCh1U1 + pow8Vec * carryVecCh1U0;

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred0, addr0, outVecCh0U0);

         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred1, addr1, outVecCh0U1);

         __vpred vpred2 = c7x::strm_agen<1, vec>::get_vpred();
         vec    *addr2  = c7x::strm_agen<1, vec>::get_adv(pOutLocal1);
         __vstore_pred(vpred2, addr2, outVecCh1U0);

         __vpred vpred3 = c7x::strm_agen<1, vec>::get_vpred();
         vec    *addr3  = c7x::strm_agen<1, vec>::get_adv(pOutLocal1);
         __vstore_pred(vpred3, addr3, outVecCh1U1);

         /* The second output's last lane is y at the end of the pair, so the carry is its broadcast.
          * This puts the broadcast on the carried path; with two channels it stays under ii. */
         carryVecCh0U0 = AUDIOLIB_deEmphasisPermute(bcastCtrl, outVecCh0U1);
         carryVecCh1U0 = AUDIOLIB_deEmphasisPermute(bcastCtrl, outVecCh1U1);
      }
      /* The carry's last lane can be padding when the row does not fill its last vector,
       * so the state for the next call is read back from the output. */
      yPrev[i]     = (float) pOutLocal[i * strideOut + sample];
      yPrev[i + 1] = (float) pOutLocal[(i + 1) * strideOut + sample];
   }

   /* Channel left over by an odd count, on SE0/SA0. hasTailChannel is a template constant
    * (0 or 1), so this runs once for odd counts and is removed by the compiler for even ones. */
   for (uint32_t tail = 0U; tail != (uint32_t) hasTailChannel; tail++) {
      vec            carryVec = vec(yPrev[i]);
      const uint32_t lastIdx  = i * strideOut + sample;

      /* ii = 8 cycles per pair of vectors, 4.0 per vector, for float */
      for (uint32_t j = 0; j < ncols; j++) {
         vec scanVec0 = c7x::strm_eng<0, vec>::get_adv();
         vec scanVec1 = c7x::strm_eng<0, vec>::get_adv();

         scanVec0 = scanVec0 + a1Vec * AUDIOLIB_deEmphasisDelay1(scanVec0);
         scanVec1 = scanVec1 + a1Vec * AUDIOLIB_deEmphasisDelay1(scanVec1);
         scanVec0 = scanVec0 + a2Vec * AUDIOLIB_deEmphasisDelay2(scanVec0);
         scanVec1 = scanVec1 + a2Vec * AUDIOLIB_deEmphasisDelay2(scanVec1);
         scanVec0 = scanVec0 + a4Vec * AUDIOLIB_deEmphasisPermute(delay4Ctrl, scanVec0);
         scanVec1 = scanVec1 + a4Vec * AUDIOLIB_deEmphasisPermute(delay4Ctrl, scanVec1);

         /* Extend the second scan across the first: lane i gains alpha^(i+1) times the first
          * vector's last lane, making it a 16-sample scan. Input only, so off the carried path. */
         scanVec1 = scanVec1 + powVec * AUDIOLIB_deEmphasisPermute(bcastCtrl, scanVec0);
         vec lastVec1 = AUDIOLIB_deEmphasisPermute(bcastCtrl, scanVec1);

         /* Lane i of the first vector still owes alpha^(i+1) * y[-1], and of the second
          * alpha^(i+9) * y[-1]. */
         vec outVec0   = scanVec0 + powVec * carryVec;
         vec outVec1   = scanVec1 + pow8Vec * carryVec;

         __vpred vpred0 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr0  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred0, addr0, outVec0);

         __vpred vpred1 = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr1  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred1, addr1, outVec1);

         carryVec = lastVec1 + a16Vec * carryVec;
      }

      yPrev[i] = (float) pOutLocal[lastIdx];
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();
   __SA1_CLOSE();
   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_deEmphasis_nonInterleave_exec_ci<float, false>(AUDIOLIB_kernelHandle handle,
                                                                                 void *restrict pIn,
                                                                                 void *restrict pOut);
template AUDIOLIB_STATUS AUDIOLIB_deEmphasis_nonInterleave_exec_ci<float, true>(AUDIOLIB_kernelHandle handle,
                                                                                void *restrict pIn,
                                                                                void *restrict pOut);
