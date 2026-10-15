// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_preEmphasis_priv.h"

void AUDIOLIB_preEmphasis_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   uint64_t                       loopCycles;
   uint64_t                       overheadCycles;
   AUDIOLIB_preEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_preEmphasis_PrivArgs *) handle;

   if (pKerPrivArgs->isInterleave == 1U) {
      loopCycles     = (uint64_t) pKerPrivArgs->nrows * (7 + (pKerPrivArgs->ncols - 1U) * 1);
      overheadCycles = 51 + (uint64_t) pKerPrivArgs->nrows * 25;
   }
   else if (pKerPrivArgs->samples <= AUDIOLIB_PREEMPHASIS_LANES_F32) {
      /* Single-vector variant: one pipelined loop over the rows, no inner loop. */
      loopCycles =(16 + (uint64_t) pKerPrivArgs->nrows * 2);
      overheadCycles = 46;
   }
   else {
      loopCycles     = (uint64_t) pKerPrivArgs->nrows * (7 + (pKerPrivArgs->ncols - 1U) * 1);
      overheadCycles = 45 + (uint64_t) pKerPrivArgs->nrows * 32;
   }

   *estCycles  = loopCycles + overheadCycles;
   *archCycles = loopCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_preEmphasis_init_ci(AUDIOLIB_kernelHandle                handle,
                                             const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                             const AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                             const AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_preEmphasis_PrivArgs                         *pKerPrivArgs = (AUDIOLIB_preEmphasis_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_preEmphasis_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "samples %d numChannels %d strideIn %d strideOut %d eleCount %d\n", pKerPrivArgs->samples,
                         pKerPrivArgs->numChannels, pKerPrivArgs->strideIn, pKerPrivArgs->strideOut, eleCount);

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;

   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;

   // For input buffer without stride
   if (pKerInitArgs->isInterleave == 1) {
      __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();
      sa1Params.VECLEN           = SA_VECLEN;

      se0Params.ICNT0         = pKerPrivArgs->numChannels < eleCount ? pKerPrivArgs->numChannels : eleCount;
      se0Params.ICNT1         = pKerPrivArgs->samples;
      se0Params.DIM1          = pKerPrivArgs->strideIn;
      se0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, eleCount);
      se0Params.DIM2          = eleCount;
      se0Params.DECDIM1_WIDTH = pKerPrivArgs->numChannels;
      se0Params.DECDIM1       = __SE_DECDIM_DIM2;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;

      se1Params.ICNT0         = pKerPrivArgs->numChannels < eleCount ? pKerPrivArgs->numChannels : eleCount;
      se1Params.ICNT1         = pKerPrivArgs->samples - 1;
      se1Params.DIM1          = pKerPrivArgs->strideIn;
      se1Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, eleCount);
      se1Params.DIM2          = eleCount;
      se1Params.DECDIM1_WIDTH = pKerPrivArgs->numChannels;
      se1Params.DECDIM1       = __SE_DECDIM_DIM2;
      se1Params.DIMFMT        = __SE_DIMFMT_3D;

      sa0Params.ICNT0         = pKerPrivArgs->numChannels < eleCount ? pKerPrivArgs->numChannels : eleCount;
      sa0Params.ICNT1         = pKerPrivArgs->samples;
      sa0Params.DIM1          = pKerPrivArgs->strideOut;
      sa0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, eleCount);
      sa0Params.DIM2          = eleCount;
      sa0Params.DECDIM1_WIDTH = pKerPrivArgs->numChannels;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM2;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;

      sa1Params.ICNT0  = pKerPrivArgs->numChannels;
      sa1Params.DIMFMT = __SA_DIMFMT_1D;

      pKerPrivArgs->nrows = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, eleCount);
      pKerPrivArgs->ncols = pKerPrivArgs->samples;

      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   }
   // For input buffer with stride
   else {
      se0Params.ICNT0  = pKerPrivArgs->samples;
      se0Params.ICNT1  = pKerPrivArgs->numChannels;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      if ((uint32_t) pKerPrivArgs->samples > eleCount) {
         const int32_t histElems = pKerPrivArgs->samples - (int32_t) AUDIOLIB_PREEMPHASIS_NONINTERLEAVE_HISTORY_OFFSET;
         se1Params.ICNT0         = (histElems < (int32_t) eleCount) ? (uint32_t) histElems : eleCount;
         se1Params.ICNT1 = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples - AUDIOLIB_PREEMPHASIS_LANES_F32, eleCount);
         se1Params.DIM1  = eleCount;
         se1Params.ICNT2 = pKerPrivArgs->numChannels;
         se1Params.DIM2  = pKerPrivArgs->strideIn;
         se1Params.DECDIM1_WIDTH = histElems;
         se1Params.DECDIM1       = __SE_DECDIM_DIM1;
         se1Params.DIMFMT        = __SE_DIMFMT_3D;
      }

      sa0Params.ICNT0  = pKerPrivArgs->samples;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->numChannels;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      pKerPrivArgs->nrows = pKerPrivArgs->numChannels;
      pKerPrivArgs->ncols = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_preEmphasis_init_ci<float>(AUDIOLIB_kernelHandle                handle,
                                                             const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                                             const AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                                             const AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_interleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_preEmphasis_PrivArgs *pKerPrivArgs = (AUDIOLIB_preEmphasis_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;
   uint8_t *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter %s nrows %d ncols %d\n", __FUNCTION__, pKerPrivArgs->nrows, pKerPrivArgs->ncols);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);

   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;
   vec alphaVec                 = vec(pKerPrivArgs->alpha);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal, se1Params); // Fetch the previous sample history vector using Streaming Engine 1 (SE1)
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   const uint32_t nrows  = pKerPrivArgs->nrows;
   const uint32_t ncols  = pKerPrivArgs->ncols;
   float *restrict xPrev = pKerPrivArgs->xPrev;

   vec inVec;

   for (uint32_t i = 0; i < nrows; i++) {
      // Load the initial historical state (xPrev) for the first vec of this row
      vec prevVec0 = *stov_ptr(vec, &xPrev[i * eleCount]);
      // Fetch the first input vector chunk using Streaming Engine 0 (SE0)
      inVec = c7x::strm_eng<0, vec>::get_adv();

      vec     outVec0 = inVec - alphaVec * prevVec0;
      __vpred vpred0  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred0, addr0, outVec0);

      /* ii = 1 */
      for (uint32_t j = 1; j < ncols; j++) {
         vec prevVec = c7x::strm_eng<1, vec>::get_adv();
         inVec       = c7x::strm_eng<0, vec>::get_adv();

         vec     outVec = inVec - alphaVec * prevVec;
         __vpred vpred  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred, addr, outVec);
      }

      // Save the tail state of the current frame back to xPrev for the next iteration/buffer call
      __vpred vpred = c7x::strm_agen<1, vec>::get_vpred();
      vec    *addr  = c7x::strm_agen<1, vec>::get_adv(&xPrev[0]);
      __vstore_pred(vpred, addr, inVec);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();
   __SE1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_interleave_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_nonInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_preEmphasis_PrivArgs                         *pKerPrivArgs = (AUDIOLIB_preEmphasis_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter %s nrows %d ncols %d\n", __FUNCTION__, pKerPrivArgs->nrows, pKerPrivArgs->ncols);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;
   vec alphaVec                 = vec(pKerPrivArgs->alpha);
   /* Loop bounds and the state pointer are read once: xPrev lives inside the handle, so
    * without local copies every state store would force the handle fields to be re-read. */
   const uint32_t nrows           = pKerPrivArgs->nrows;
   const uint32_t ncols           = pKerPrivArgs->ncols;
   const int32_t  strideIn        = pKerPrivArgs->strideIn;
   const dataType *restrict pLast = pInLocal + pKerPrivArgs->samples - 1;
   float *restrict xPrev          = pKerPrivArgs->xPrev;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + AUDIOLIB_PREEMPHASIS_NONINTERLEAVE_HISTORY_OFFSET, se1Params);
   __SA0_OPEN(sa0Params);

   vec inVec;
   for (uint32_t i = 0; i < nrows; i++) {
      inVec = c7x::strm_eng<0, vec>::get_adv();

      vec prevVec0 = c7x::reinterpret<vec>(__shift_left_merge(c7x::reinterpret<c7x::char_vec>(inVec), __as_char4(xPrev[i])));

      vec     outVec0 = inVec - alphaVec * prevVec0;
      __vpred vpred0  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred0, addr0, outVec0);

      /* ii = 1 */
      for (uint32_t j = 1; j < ncols; j++) {
         vec prevVec = c7x::strm_eng<1, vec>::get_adv();
         inVec       = c7x::strm_eng<0, vec>::get_adv();

         vec     outVec = inVec - alphaVec * prevVec;
         __vpred vpred  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred, addr, outVec);
      }
      xPrev[i] = pLast[i * strideIn];
   }

   __SE0_CLOSE();
   __SE1_CLOSE();

   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_preEmphasis_nonInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_preEmphasis_nonInterleaveSingleVec_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                    void *restrict pIn,
                                                                    void *restrict pOut)
{

   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_preEmphasis_PrivArgs                         *pKerPrivArgs = (AUDIOLIB_preEmphasis_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter %s nrows %d ncols %d\n", __FUNCTION__, pKerPrivArgs->nrows, pKerPrivArgs->ncols);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;
   vec alphaVec                 = vec(pKerPrivArgs->alpha);

   const uint32_t nrows           = pKerPrivArgs->nrows;
   const int32_t  strideIn        = pKerPrivArgs->strideIn;
   const dataType *restrict pLast = pInLocal + pKerPrivArgs->samples - 1;
   float *restrict xPrev          = pKerPrivArgs->xPrev;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   vec inVec;
   /* ii = 2  */
   for (uint32_t i = 0; i < nrows; i++) {
      inVec = c7x::strm_eng<0, vec>::get_adv();

      vec prevVec0 = c7x::reinterpret<vec>(__shift_left_merge(c7x::reinterpret<char32>(inVec), __as_char4(xPrev[i])));

      vec     outVec0 = inVec - alphaVec * prevVec0;
      __vpred vpred0  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr0   = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred0, addr0, outVec0);

      xPrev[i] = pLast[i * strideIn];
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_preEmphasis_nonInterleaveSingleVec_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                    void *restrict pIn,
                                                                                    void *restrict pOut);
