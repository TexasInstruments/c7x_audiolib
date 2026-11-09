// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_muteNCh_priv.h"
// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNCh_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_muteNCh_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNCh_init_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS            status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs      = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                   *pBlock            = pKerPrivArgs->bufPblock;
   uint32_t                   samples           = pKerPrivArgs->samples;
   uint32_t                   eleCount          = c7x::element_count_of<vec>::value;
   uint32_t                   channels          = pKerPrivArgs->channels;
   uint8_t                    isInterleaved     = pKerInitArgs->isInterleaved;
   uint32_t                   strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t                   strideOutElements = pKerPrivArgs->strideOutElements;
   uint32_t                   numChannelBlocks  = AUDIOLIB_ceilingDiv(channels, eleCount);
   uint32_t                   vectorsPerRow     = AUDIOLIB_ceilingDiv(samples, eleCount);

   __SE_TEMPLATE_v1 seFadeDataParams = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seHardDataParams = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seHardGainParams = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saFadeDataParams = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saHardDataParams = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saGainParams     = __gen_SA_TEMPLATE_v1();

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   seFadeDataParams.ELETYPE = SE_ELETYPE;
   seFadeDataParams.VECLEN  = SE_VECLEN;
   seHardDataParams.ELETYPE = SE_ELETYPE;
   seHardDataParams.VECLEN  = SE_VECLEN;
   seHardGainParams.ELETYPE = SE_ELETYPE;
   seHardGainParams.VECLEN  = SE_VECLEN;
   saFadeDataParams.VECLEN  = SA_VECLEN;
   saHardDataParams.VECLEN  = SA_VECLEN;
   saGainParams.VECLEN      = SA_VECLEN;

   if (isInterleaved == 0) {
      // /*********************************************************************************/
      // /* Planar fades: one channel row of vectors at a time, padded to whole groups of */
      // /* four vectors. DECDIM zero-fills past the last sample and SA masks the stores. */
      // /*********************************************************************************/
      uint32_t paddedVectorsPerRow = AUDIOLIB_ceilingDiv(vectorsPerRow, AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION) *
                                     AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION;

      seFadeDataParams.ICNT0         = eleCount;
      seFadeDataParams.ICNT1         = paddedVectorsPerRow;
      seFadeDataParams.DIM1          = eleCount;
      seFadeDataParams.ICNT2         = channels;
      seFadeDataParams.DIM2          = strideInElements;
      seFadeDataParams.DECDIM1       = __SE_DECDIM_DIM1;
      seFadeDataParams.DECDIM1_WIDTH = samples;
      seFadeDataParams.DIMFMT        = __SE_DIMFMT_3D;

      saFadeDataParams.ICNT0         = eleCount;
      saFadeDataParams.ICNT1         = paddedVectorsPerRow;
      saFadeDataParams.DIM1          = eleCount;
      saFadeDataParams.ICNT2         = channels;
      saFadeDataParams.DIM2          = strideOutElements;
      saFadeDataParams.DECDIM1       = __SA_DECDIM_DIM1;
      saFadeDataParams.DECDIM1_WIDTH = samples;
      saFadeDataParams.DIMFMT        = __SA_DIMFMT_3D;

      // /*********************************************************************************/
      // /* Hard path: every row in one 2D walk; each row starts a new vector and the SE  */
      // /* zero-fills its last one. SE1 broadcasts the channel's target gain to a vector */
      // /* and repeats it for every vector of that row.                                  */
      // /*********************************************************************************/
      seHardDataParams.ICNT0  = samples;
      seHardDataParams.ICNT1  = channels;
      seHardDataParams.DIM1   = strideInElements;
      seHardDataParams.DIMFMT = __SE_DIMFMT_2D;

      saHardDataParams.ICNT0  = samples;
      saHardDataParams.ICNT1  = channels;
      saHardDataParams.DIM1   = strideOutElements;
      saHardDataParams.DIMFMT = __SA_DIMFMT_2D;

      seHardGainParams.ICNT0  = 1;
      seHardGainParams.ICNT1  = vectorsPerRow;
      seHardGainParams.DIM1   = 0;
      seHardGainParams.ICNT2  = channels;
      seHardGainParams.DIM2   = 1;
      seHardGainParams.ELEDUP = AUDIOLIB_MUTENCH_GAIN_ELEDUP;
      seHardGainParams.DIMFMT = __SE_DIMFMT_3D;

      pKerPrivArgs->numBlocks      = channels;
      pKerPrivArgs->wBlocks        = paddedVectorsPerRow / AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION;
      pKerPrivArgs->numHardVectors = channels * vectorsPerRow;
   }
   else // isInterleaved == 1
   {
      // /*********************************************************************************/
      // /* Interleaved fades: block by block, one vector of up to eleCount channels per  */
      // /* sample, rows padded to whole groups of four. DECDIM1 zero-fills the lanes past */
      // /* the last channel, DECDIM2 the padding rows past the last sample.              */
      // /*********************************************************************************/
      uint32_t paddedSamples =
          AUDIOLIB_ceilingDiv(samples, AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION) * AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION;

      seFadeDataParams.ICNT0         = eleCount;
      seFadeDataParams.ICNT1         = paddedSamples;
      seFadeDataParams.DIM1          = strideInElements;
      seFadeDataParams.ICNT2         = numChannelBlocks;
      seFadeDataParams.DIM2          = eleCount;
      seFadeDataParams.DECDIM1       = __SE_DECDIM_DIM2;
      seFadeDataParams.DECDIM1_WIDTH = channels;
      seFadeDataParams.DECDIM2       = __SE_DECDIM_DIM1;
      seFadeDataParams.DECDIM2_WIDTH = samples * strideInElements;
      seFadeDataParams.DIMFMT        = __SE_DIMFMT_3D;

      saFadeDataParams.ICNT0         = eleCount;
      saFadeDataParams.ICNT1         = paddedSamples;
      saFadeDataParams.DIM1          = strideOutElements;
      saFadeDataParams.ICNT2         = numChannelBlocks;
      saFadeDataParams.DIM2          = eleCount;
      saFadeDataParams.DECDIM1       = __SA_DECDIM_DIM2;
      saFadeDataParams.DECDIM1_WIDTH = channels;
      saFadeDataParams.DECDIM2       = __SA_DECDIM_DIM1;
      saFadeDataParams.DECDIM2_WIDTH = samples * strideOutElements;
      saFadeDataParams.DIMFMT        = __SA_DIMFMT_3D;

      // /*********************************************************************************/
      // /* Hard path: the same block walk without padding; SE1 re-reads the block's     */
      // /* target gains for every sample (the gain arrays are padded to whole vectors).  */
      // /*********************************************************************************/
      seHardDataParams.ICNT0         = eleCount;
      seHardDataParams.ICNT1         = samples;
      seHardDataParams.DIM1          = strideInElements;
      seHardDataParams.ICNT2         = numChannelBlocks;
      seHardDataParams.DIM2          = eleCount;
      seHardDataParams.DECDIM1       = __SE_DECDIM_DIM2;
      seHardDataParams.DECDIM1_WIDTH = channels;
      seHardDataParams.DIMFMT        = __SE_DIMFMT_3D;

      saHardDataParams.ICNT0         = eleCount;
      saHardDataParams.ICNT1         = samples;
      saHardDataParams.DIM1          = strideOutElements;
      saHardDataParams.ICNT2         = numChannelBlocks;
      saHardDataParams.DIM2          = eleCount;
      saHardDataParams.DECDIM1       = __SA_DECDIM_DIM2;
      saHardDataParams.DECDIM1_WIDTH = channels;
      saHardDataParams.DIMFMT        = __SA_DIMFMT_3D;

      seHardGainParams.ICNT0  = eleCount;
      seHardGainParams.ICNT1  = samples;
      seHardGainParams.DIM1   = 0;
      seHardGainParams.ICNT2  = numChannelBlocks;
      seHardGainParams.DIM2   = eleCount;
      seHardGainParams.DIMFMT = __SE_DIMFMT_3D;

      // SA2 stores each block's gain at the end of the frame
      saGainParams.ICNT0  = channels;
      saGainParams.DIMFMT = __SA_DIMFMT_1D;

      pKerPrivArgs->numBlocks      = numChannelBlocks;
      pKerPrivArgs->wBlocks        = paddedSamples / AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION;
      pKerPrivArgs->numHardVectors = numChannelBlocks * samples;
   }

   // Store the prepared SE and SA templates in the pblock
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET) = seFadeDataParams;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_HARD_DATA_OFFSET) = seHardDataParams;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_HARD_GAIN_OFFSET) = seHardGainParams;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET) = saFadeDataParams;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_HARD_DATA_OFFSET) = saHardDataParams;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_GAIN_OFFSET)      = saGainParams;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNCh_init_ci<float>(AUDIOLIB_kernelHandle            handle,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                                         const AUDIOLIB_muteNCh_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  * Each fading loop iteration handles four vectors with four independent gains
//  * (ramp0..ramp3, distance0..distance3), each advanced by four vectors' worth per
//  * iteration, so no result is needed by the next instruction of the same chain.
//  * The streams are padded to whole groups of four; padding lanes are zero on read
//  * and masked on store.
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChLinearFadeInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNChLinearFadeInterleave_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   float   *currentGain                                                = pKerPrivArgs->currentGain;
   float   *gainStepPtr                                                = pKerPrivArgs->gainStep;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   int32_t  numBlocks                                                  = (int32_t) pKerPrivArgs->numBlocks;
   int32_t  wBlocks                                                    = (int32_t) pKerPrivArgs->wBlocks;

   __SE_TEMPLATE_v1 seFadeDataParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saFadeDataParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saGainParams = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_GAIN_OFFSET);

   vec nSamplesVec = (vec) ((float) pKerPrivArgs->samples);
   vec zeroVec     = (vec) (0.0f);
   vec oneVec      = (vec) (1.0f);

   __SE0_OPEN(pIn, seFadeDataParams);
   __SA0_OPEN(saFadeDataParams);
   __SA2_OPEN(saGainParams);
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numBlocks; i++) {
      // One vector is one sample of the block's channels: ramp n is the unclamped gain of sample n
      vec gainStepVec        = *(stov_ptr(vec, &gainStepPtr[i * eleCount]));
      vec initialGainVec     = *(stov_ptr(vec, &currentGain[i * eleCount]));
      vec ramp0              = initialGainVec + gainStepVec;
      vec ramp1              = ramp0 + gainStepVec;
      vec ramp2              = ramp0 + 2.0f * gainStepVec;
      vec ramp3              = ramp0 + 3.0f * gainStepVec;
      vec fourVectorsStepVec = 4.0f * gainStepVec;

      /* ii = 4 cycles per iteration of four vectors, i.e. 1 cycle per vector (one sample of eleCount channels), for
       * C7504 and C7524 (from the compiled .asm). The four stores per iteration (one SA store per cycle) and the ramp
       * add chains both bound it at 4. */
#pragma MUST_ITERATE(1, , )
      for (int32_t j = 0; j < wBlocks; j++) {
         vec     in0     = c7x::strm_eng<0, vec>::get_adv();
         vec     in1     = c7x::strm_eng<0, vec>::get_adv();
         vec     in2     = c7x::strm_eng<0, vec>::get_adv();
         vec     in3     = c7x::strm_eng<0, vec>::get_adv();
         vec     out0    = in0 * __min(__max(ramp0, zeroVec), oneVec);
         vec     out1    = in1 * __min(__max(ramp1, zeroVec), oneVec);
         vec     out2    = in2 * __min(__max(ramp2, zeroVec), oneVec);
         vec     out3    = in3 * __min(__max(ramp3, zeroVec), oneVec);
         __vpred pred0   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred0, outPtr0, out0);
         __vpred pred1   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred1, outPtr1, out1);
         __vpred pred2   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred2, outPtr2, out2);
         __vpred pred3   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred3, outPtr3, out3);
         ramp0 += fourVectorsStepVec;
         ramp1 += fourVectorsStepVec;
         ramp2 += fourVectorsStepVec;
         ramp3 += fourVectorsStepVec;
      }
      vec finalGainVec = __min(__max(initialGainVec + nSamplesVec * gainStepVec, zeroVec), oneVec);

      __vpred predGain   = c7x::strm_agen<2, vec>::get_vpred();
      vec    *outGainPtr = c7x::strm_agen<2, vec>::get_adv(currentGain);
      __vstore_pred(predGain, outGainPtr, finalGainVec);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA2_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNChLinearFadeInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                             void *restrict pIn,
                                                                             void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChLinearFadeDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNChLinearFadeDeinterleave_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   int32_t  numBlocks                                                  = (int32_t) pKerPrivArgs->numBlocks;
   int32_t  wBlocks                                                    = (int32_t) pKerPrivArgs->wBlocks;
   uint32_t numSamples                                                 = pKerPrivArgs->samples;
   float *restrict currentGain                                         = pKerPrivArgs->currentGain;
   float *restrict gainStepPtr                                         = pKerPrivArgs->gainStep;

   __SE_TEMPLATE_v1 seFadeDataParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saFadeDataParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET);

   vec zeroVec = (vec) (0.0f);
   vec oneVec  = (vec) (1.0f);

   __SE0_OPEN(pIn, seFadeDataParams);
   __SA0_OPEN(saFadeDataParams);
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numBlocks; i++) {
      // Lane k of vector n holds sample n * eleCount + k of the channel: ramp n is the unclamped gain of vector n
      float gainStep      = gainStepPtr[i];
      vec   vectorStepVec = (vec) ((float) eleCount * gainStep);
      vec   ramp0 = (vec) (currentGain[i]) + *(stov_ptr(vec, &pKerPrivArgs->precalculatedGainVectors[i * eleCount]));
      vec   ramp1 = ramp0 + vectorStepVec;
      vec   ramp2 = ramp0 + 2.0f * vectorStepVec;
      vec   ramp3 = ramp0 + 3.0f * vectorStepVec;
      vec   fourVectorsStepVec = 4.0f * vectorStepVec;

      /* ii = 4 cycles per iteration of four vectors, i.e. 1 cycle per vector of eleCount samples, for C7504 and C7524
       * (from the compiled .asm). The four stores per iteration (one SA store per cycle) and the ramp add chains both
       * bound it at 4. */
#pragma MUST_ITERATE(1, , )
      for (int32_t j = 0; j < wBlocks; j++) {
         vec     in0     = c7x::strm_eng<0, vec>::get_adv();
         vec     in1     = c7x::strm_eng<0, vec>::get_adv();
         vec     in2     = c7x::strm_eng<0, vec>::get_adv();
         vec     in3     = c7x::strm_eng<0, vec>::get_adv();
         vec     out0    = in0 * __min(__max(ramp0, zeroVec), oneVec);
         vec     out1    = in1 * __min(__max(ramp1, zeroVec), oneVec);
         vec     out2    = in2 * __min(__max(ramp2, zeroVec), oneVec);
         vec     out3    = in3 * __min(__max(ramp3, zeroVec), oneVec);
         __vpred pred0   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred0, outPtr0, out0);
         __vpred pred1   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred1, outPtr1, out1);
         __vpred pred2   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred2, outPtr2, out2);
         __vpred pred3   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred3, outPtr3, out3);
         ramp0 += fourVectorsStepVec;
         ramp1 += fourVectorsStepVec;
         ramp2 += fourVectorsStepVec;
         ramp3 += fourVectorsStepVec;
      }
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   // Save the final gains for the next frame, eleCount channels per vector (the gain arrays are whole vectors)
   vec     nSamplesVec    = (vec) ((float) numSamples);
   int32_t numGainVectors = (int32_t) AUDIOLIB_ceilingDiv(pKerPrivArgs->channels, eleCount);
   /* ii = 4 cycles per vector of eleCount channels (C7504 and C7524, from the compiled .asm); once per frame. */
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numGainVectors; i++) {
      vec *pGainVec = stov_ptr(vec, &currentGain[i * eleCount]);
      vec  stepVec  = *(stov_ptr(vec, &gainStepPtr[i * eleCount]));
      *pGainVec     = __min(__max(*pGainVec + nSamplesVec * stepVec, zeroVec), oneVec);
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNChLinearFadeDeinterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                               void *restrict pIn,
                                                                               void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChExponentialFadeDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                    void *restrict pIn,
                                                                    void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNChExponentialFadeDeinterleave_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   int32_t numBlocks                                                   = (int32_t) pKerPrivArgs->numBlocks;
   int32_t wBlocks                                                     = (int32_t) pKerPrivArgs->wBlocks;
   float *restrict targetGain                                          = pKerPrivArgs->targetGain;
   float *restrict currentGain                                         = pKerPrivArgs->currentGain;
   float *restrict smoothDistance                                      = pKerPrivArgs->smoothDistance;
   float alphaN                                                        = pKerPrivArgs->alphaN;

   __SE_TEMPLATE_v1 seFadeDataParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saFadeDataParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET);

   // alphaCoeff holds alpha^1 .. alpha^eleCount; each further vector multiplies by alpha^eleCount
   vec   laneCoeffVec         = *(stov_ptr(vec, (float *) &pKerPrivArgs->alphaCoeff[0]));
   float vectorCoeff          = pKerPrivArgs->alphaMultiplier;
   vec   oneVectorCoeffVec    = (vec) (vectorCoeff);
   vec   twoVectorsCoeffVec   = (vec) (vectorCoeff * vectorCoeff);
   vec   threeVectorsCoeffVec = (vec) (vectorCoeff * vectorCoeff * vectorCoeff);
   vec   fourVectorsCoeffVec  = (vec) (vectorCoeff * vectorCoeff * vectorCoeff * vectorCoeff);

   __SE0_OPEN(pIn, seFadeDataParams);
   __SA0_OPEN(saFadeDataParams);
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numBlocks; i++) {
      // The gain is target + distance; the distance to the target shrinks by alpha every sample
      vec targetGainVec = (vec) (targetGain[i]);
      vec distance0     = laneCoeffVec * (vec) (smoothDistance[i]);
      vec distance1     = distance0 * oneVectorCoeffVec;
      vec distance2     = distance0 * twoVectorsCoeffVec;
      vec distance3     = distance0 * threeVectorsCoeffVec;

      /* ii = 4 cycles per iteration of four vectors, i.e. 1 cycle per vector of eleCount samples, for C7504 and C7524
       * (from the compiled .asm). Bound at 4 by the four stores and by the 4-cycle multiply of each distance chain. */
#pragma MUST_ITERATE(1, , )
      for (int32_t j = 0; j < wBlocks; j++) {
         vec     in0     = c7x::strm_eng<0, vec>::get_adv();
         vec     in1     = c7x::strm_eng<0, vec>::get_adv();
         vec     in2     = c7x::strm_eng<0, vec>::get_adv();
         vec     in3     = c7x::strm_eng<0, vec>::get_adv();
         vec     out0    = in0 * (targetGainVec + distance0);
         vec     out1    = in1 * (targetGainVec + distance1);
         vec     out2    = in2 * (targetGainVec + distance2);
         vec     out3    = in3 * (targetGainVec + distance3);
         __vpred pred0   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred0, outPtr0, out0);
         __vpred pred1   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred1, outPtr1, out1);
         __vpred pred2   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred2, outPtr2, out2);
         __vpred pred3   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred3, outPtr3, out3);
         distance0 *= fourVectorsCoeffVec;
         distance1 *= fourVectorsCoeffVec;
         distance2 *= fourVectorsCoeffVec;
         distance3 *= fourVectorsCoeffVec;
      }
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   // Save the final distances and the gains they give for the next frame, eleCount channels per vector (the arrays
   // are whole vectors)
   uint32_t eleCount       = c7x::element_count_of<vec>::value;
   vec      alphaNVec      = (vec) (alphaN);
   int32_t  numGainVectors = (int32_t) AUDIOLIB_ceilingDiv(pKerPrivArgs->channels, eleCount);
   /* ii = 3 cycles per vector of eleCount channels (C7504 and C7524, from the compiled .asm); once per frame. */
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numGainVectors; i++) {
      vec *pGainVec     = stov_ptr(vec, &currentGain[i * eleCount]);
      vec *pDistanceVec = stov_ptr(vec, &smoothDistance[i * eleCount]);
      vec  targetVec    = *(stov_ptr(vec, &targetGain[i * eleCount]));
      vec  distanceVec  = alphaNVec * (*pDistanceVec);
      *pDistanceVec     = distanceVec;
      *pGainVec         = targetVec + distanceVec;
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNChExponentialFadeDeinterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                    void *restrict pIn,
                                                                                    void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChExponentialFadeInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNChExponentialFadeInterleave_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   float   *targetGain                                                 = pKerPrivArgs->targetGain;
   float   *currentGain                                                = pKerPrivArgs->currentGain;
   float   *smoothDistance                                             = pKerPrivArgs->smoothDistance;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   int32_t  numBlocks                                                  = (int32_t) pKerPrivArgs->numBlocks;
   int32_t  wBlocks                                                    = (int32_t) pKerPrivArgs->wBlocks;

   __SE_TEMPLATE_v1 seFadeDataParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saFadeDataParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saGainParams = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_GAIN_OFFSET);

   // One vector is one sample here, so the distance to the target shrinks by alpha per vector
   float coeff                = pKerPrivArgs->smoothingCoefficient;
   vec   coeffVec             = (vec) (coeff);
   vec   twoVectorsCoeffVec   = (vec) (coeff * coeff);
   vec   threeVectorsCoeffVec = (vec) (coeff * coeff * coeff);
   vec   fourVectorsCoeffVec  = (vec) (coeff * coeff * coeff * coeff);
   vec   alphaNVec            = (vec) (pKerPrivArgs->alphaN);

   __SE0_OPEN(pIn, seFadeDataParams);
   __SA0_OPEN(saFadeDataParams);
   __SA2_OPEN(saGainParams);
   __SA3_OPEN(saGainParams);
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numBlocks; i++) {
      vec initialDistanceVec = *(stov_ptr(vec, (float *) &smoothDistance[i * eleCount]));
      vec targetGainVec      = *(stov_ptr(vec, (float *) &targetGain[i * eleCount]));
      vec distance0          = coeffVec * initialDistanceVec;
      vec distance1          = distance0 * coeffVec;
      vec distance2          = distance0 * twoVectorsCoeffVec;
      vec distance3          = distance0 * threeVectorsCoeffVec;

      /* ii = 4 cycles per iteration of four vectors, i.e. 1 cycle per vector (one sample of eleCount channels), for
       * C7504 and C7524 (from the compiled .asm). Bound at 4 by the four stores and by the 4-cycle multiply of each
       * distance chain. */
#pragma MUST_ITERATE(1, , )
      for (int32_t j = 0; j < wBlocks; j++) {
         vec     in0     = c7x::strm_eng<0, vec>::get_adv();
         vec     in1     = c7x::strm_eng<0, vec>::get_adv();
         vec     in2     = c7x::strm_eng<0, vec>::get_adv();
         vec     in3     = c7x::strm_eng<0, vec>::get_adv();
         vec     out0    = in0 * (targetGainVec + distance0);
         vec     out1    = in1 * (targetGainVec + distance1);
         vec     out2    = in2 * (targetGainVec + distance2);
         vec     out3    = in3 * (targetGainVec + distance3);
         __vpred pred0   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred0, outPtr0, out0);
         __vpred pred1   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred1, outPtr1, out1);
         __vpred pred2   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred2, outPtr2, out2);
         __vpred pred3   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred3, outPtr3, out3);
         distance0 *= fourVectorsCoeffVec;
         distance1 *= fourVectorsCoeffVec;
         distance2 *= fourVectorsCoeffVec;
         distance3 *= fourVectorsCoeffVec;
      }
      // SA2 stores the gains and SA3 the distances, one vector of channels per block
      vec     finalDistanceVec = alphaNVec * initialDistanceVec;
      __vpred predGain         = c7x::strm_agen<2, vec>::get_vpred();
      vec    *outGainPtr       = c7x::strm_agen<2, vec>::get_adv(currentGain);
      __vstore_pred(predGain, outGainPtr, targetGainVec + finalDistanceVec);
      __vpred predDistance   = c7x::strm_agen<3, vec>::get_vpred();
      vec    *outDistancePtr = c7x::strm_agen<3, vec>::get_adv(smoothDistance);
      __vstore_pred(predDistance, outDistancePtr, finalDistanceVec);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA2_CLOSE();
   __SA3_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNChExponentialFadeInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                  void *restrict pIn,
                                                                                  void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChSymmetricFadeDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNChSymmetricFadeDeinterleave_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   int32_t  numBlocks                                                  = (int32_t) pKerPrivArgs->numBlocks;
   int32_t  wBlocks                                                    = (int32_t) pKerPrivArgs->wBlocks;
   float *restrict targetGain                                          = pKerPrivArgs->targetGain;
   float *restrict currentGain                                         = pKerPrivArgs->currentGain;

   __SE_TEMPLATE_v1 seFadeDataParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saFadeDataParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET);

   vec oneVec   = (vec) (1.0f);
   vec zeroVec  = (vec) (0.0f);
   vec floorVec = (vec) (AUDIOLIB_MUTENCH_SYMMETRIC_FLOOR);

   __SE0_OPEN(pIn, seFadeDataParams);
   __SA0_OPEN(saFadeDataParams);
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numBlocks; i++) {
      // The channel's target gain (0 mutes, 1 unmutes) picks its ratio tables. Lane k of vector n holds sample
      // n * eleCount + k: ramp n is the unclamped gain of vector n, current gain times ratio^(n * eleCount + k + 1)
      uint32_t     direction           = (uint32_t) (targetGain[i] > 0.5f);
      const float *vectorRatio         = pKerPrivArgs->symmetricVectorRatio[direction];
      vec          laneRatioVec        = *(stov_ptr(vec, &pKerPrivArgs->symmetricLaneRatio[direction][0]));
      vec          ramp0               = (vec) (currentGain[i]) * laneRatioVec;
      vec          ramp1               = ramp0 * (vec) (vectorRatio[0]);
      vec          ramp2               = ramp0 * (vec) (vectorRatio[1]);
      vec          ramp3               = ramp0 * (vec) (vectorRatio[2]);
      vec          fourVectorsRatioVec = (vec) (vectorRatio[3]);

      /* ii = 4 cycles per iteration of four vectors, i.e. 1 cycle per vector of eleCount samples, for C7504 and C7524
       * (from the compiled .asm). Bound at 4 by the four stores and by the 4-cycle multiply of each ratio chain. */
#pragma MUST_ITERATE(1, , )
      for (int32_t j = 0; j < wBlocks; j++) {
         vec in0 = c7x::strm_eng<0, vec>::get_adv();
         vec in1 = c7x::strm_eng<0, vec>::get_adv();
         vec in2 = c7x::strm_eng<0, vec>::get_adv();
         vec in3 = c7x::strm_eng<0, vec>::get_adv();
         // Clamp to 1; at or below the floor the channel is fully muted and its output is exactly zero
         vec gain0 = __min(ramp0, oneVec);
         vec gain1 = __min(ramp1, oneVec);
         vec gain2 = __min(ramp2, oneVec);
         vec gain3 = __min(ramp3, oneVec);
         vec out0  = in0 * __select(__cmp_gt_pred(gain0, floorVec), gain0, zeroVec);
         vec out1  = in1 * __select(__cmp_gt_pred(gain1, floorVec), gain1, zeroVec);
         vec out2  = in2 * __select(__cmp_gt_pred(gain2, floorVec), gain2, zeroVec);
         vec out3  = in3 * __select(__cmp_gt_pred(gain3, floorVec), gain3, zeroVec);

         __vpred pred0   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred0, outPtr0, out0);

         __vpred pred1   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred1, outPtr1, out1);

         __vpred pred2   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred2, outPtr2, out2);

         __vpred pred3   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred3, outPtr3, out3);

         ramp0 *= fourVectorsRatioVec;
         ramp1 *= fourVectorsRatioVec;
         ramp2 *= fourVectorsRatioVec;
         ramp3 *= fourVectorsRatioVec;
      }
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   // Save the final gains for the next frame, eleCount channels per vector (the gain arrays are whole vectors)
   vec     upFrameRatioVec   = (vec) (pKerPrivArgs->symmetricFrameRatio[1]);
   vec     downFrameRatioVec = (vec) (pKerPrivArgs->symmetricFrameRatio[0]);
   int32_t numGainVectors    = (int32_t) AUDIOLIB_ceilingDiv(pKerPrivArgs->channels, eleCount);
   /* ii = 4 cycles per vector of eleCount channels (C7504 and C7524, from the compiled .asm); once per frame. */
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numGainVectors; i++) {
      vec *pGainVec      = stov_ptr(vec, &currentGain[i * eleCount]);
      vec  targetVec     = *(stov_ptr(vec, &targetGain[i * eleCount]));
      vec  frameRatioVec = __select(__cmp_gt_pred(targetVec, zeroVec), upFrameRatioVec, downFrameRatioVec);
      *pGainVec          = __min(__max(*pGainVec * frameRatioVec, floorVec), oneVec);
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNChSymmetricFadeDeinterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                  void *restrict pIn,
                                                                                  void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChSymmetricFadeInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNChSymmetricFadeInterleave_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   float   *targetGain                                                 = pKerPrivArgs->targetGain;
   float   *currentGain                                                = pKerPrivArgs->currentGain;
   float   *gainRatio                                                  = pKerPrivArgs->gainStep;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   int32_t  numBlocks                                                  = (int32_t) pKerPrivArgs->numBlocks;
   int32_t  wBlocks                                                    = (int32_t) pKerPrivArgs->wBlocks;

   __SE_TEMPLATE_v1 seFadeDataParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saFadeDataParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET);
   __SA_TEMPLATE_v1 saGainParams = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_GAIN_OFFSET);

   vec oneVec            = (vec) (1.0f);
   vec zeroVec           = (vec) (0.0f);
   vec floorVec          = (vec) (AUDIOLIB_MUTENCH_SYMMETRIC_FLOOR);
   vec upFrameRatioVec   = (vec) (pKerPrivArgs->symmetricFrameRatio[1]);
   vec downFrameRatioVec = (vec) (pKerPrivArgs->symmetricFrameRatio[0]);

   __SE0_OPEN(pIn, seFadeDataParams);
   __SA0_OPEN(saFadeDataParams);
   __SA2_OPEN(saGainParams);
#pragma MUST_ITERATE(1, , )
   for (int32_t i = 0; i < numBlocks; i++) {
      // One vector is one sample of the block's channels; each lane has its channel's ratio (up or down)
      vec ratioVec            = *(stov_ptr(vec, &gainRatio[i * eleCount]));
      vec initialGainVec      = *(stov_ptr(vec, &currentGain[i * eleCount]));
      vec targetGainVec       = *(stov_ptr(vec, &targetGain[i * eleCount]));
      vec twoRatioVec         = ratioVec * ratioVec;
      vec fourVectorsRatioVec = twoRatioVec * twoRatioVec;
      vec ramp0               = initialGainVec * ratioVec;
      vec ramp1               = initialGainVec * twoRatioVec;
      vec ramp2               = ramp1 * ratioVec;
      vec ramp3               = initialGainVec * fourVectorsRatioVec;

      /* ii = 4 cycles per iteration of four vectors, i.e. 1 cycle per vector (one sample of eleCount channels), for
       * C7504 and C7524 (from the compiled .asm). Bound at 4 by the four stores and by the 4-cycle multiply of each
       * ratio chain. */
#pragma MUST_ITERATE(1, , )
      for (int32_t j = 0; j < wBlocks; j++) {
         vec in0 = c7x::strm_eng<0, vec>::get_adv();
         vec in1 = c7x::strm_eng<0, vec>::get_adv();
         vec in2 = c7x::strm_eng<0, vec>::get_adv();
         vec in3 = c7x::strm_eng<0, vec>::get_adv();
         // Clamp to 1; at or below the floor the channel is fully muted and its output is exactly zero
         vec gain0 = __min(ramp0, oneVec);
         vec gain1 = __min(ramp1, oneVec);
         vec gain2 = __min(ramp2, oneVec);
         vec gain3 = __min(ramp3, oneVec);
         vec out0  = in0 * __select(__cmp_gt_pred(gain0, floorVec), gain0, zeroVec);
         vec out1  = in1 * __select(__cmp_gt_pred(gain1, floorVec), gain1, zeroVec);
         vec out2  = in2 * __select(__cmp_gt_pred(gain2, floorVec), gain2, zeroVec);
         vec out3  = in3 * __select(__cmp_gt_pred(gain3, floorVec), gain3, zeroVec);

         __vpred pred0   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred0, outPtr0, out0);

         __vpred pred1   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred1, outPtr1, out1);

         __vpred pred2   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred2, outPtr2, out2);

         __vpred pred3   = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtr3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(pred3, outPtr3, out3);

         ramp0 *= fourVectorsRatioVec;
         ramp1 *= fourVectorsRatioVec;
         ramp2 *= fourVectorsRatioVec;
         ramp3 *= fourVectorsRatioVec;
      }
      vec frameRatioVec = __select(__cmp_gt_pred(targetGainVec, zeroVec), upFrameRatioVec, downFrameRatioVec);
      vec finalGainVec  = __min(__max(initialGainVec * frameRatioVec, floorVec), oneVec);

      __vpred predGain   = c7x::strm_agen<2, vec>::get_vpred();
      vec    *outGainPtr = c7x::strm_agen<2, vec>::get_adv(currentGain);
      __vstore_pred(predGain, outGainPtr, finalGainVec);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();
   __SA2_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNChSymmetricFadeInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                void *restrict pIn,
                                                                                void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChHard_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNChHard_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   int32_t numVectors                                                  = (int32_t) pKerPrivArgs->numHardVectors;

   __SE_TEMPLATE_v1 seHardDataParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_HARD_DATA_OFFSET);
   __SE_TEMPLATE_v1 seHardGainParams =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SE_HARD_GAIN_OFFSET);
   __SA_TEMPLATE_v1 saHardDataParams =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_MUTENCH_SA_HARD_DATA_OFFSET);

   // SE1 delivers, for every data vector, the target gain of its channels: one loop covers the whole frame
   __SE0_OPEN(pIn, seHardDataParams);
   __SE1_OPEN(pKerPrivArgs->targetGain, seHardGainParams);
   __SA0_OPEN(saHardDataParams);

   /* ii = 1 cycle per vector for C7504 and C7524 (from the compiled .asm): one SA store per cycle. */
#pragma MUST_ITERATE(1, , )
   for (int32_t j = 0; j < numVectors; j++) {
      vec in   = c7x::strm_eng<0, vec>::get_adv();
      vec gain = c7x::strm_eng<1, vec>::get_adv();
      vec out  = in * gain;

      __vpred pred   = c7x::strm_agen<0, vec>::get_vpred();
      vec    *outPtr = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(pred, outPtr, out);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_muteNChHard_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

void AUDIOLIB_muteNCh_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   const uint64_t             numBlocks    = pKerPrivArgs->numBlocks;
   const uint64_t             wBlocks      = pKerPrivArgs->wBlocks;
   const uint64_t numGainVectors           = AUDIOLIB_ceilingDiv(pKerPrivArgs->channels, (uint32_t) ALPHA_COEFF_INDEX);
   const uint32_t fadeType                 = pKerPrivArgs->setArgs.fadeType;
   uint64_t       loopCycles;
   uint64_t       overheadCycles;

   /* Mirrors AUDIOLIB_muteNCh_set and AUDIOLIB_muteNCh_exec: the hard path for a hard change and once a linear or
    * symmetric fade has converged, otherwise the fading variant of the layout. */
   const bool isHard = (pKerPrivArgs->bypassSmoothing != 0) || (fadeType == AUDIOLIB_MUTE_FADE_TYPE_HARD) ||
                       ((fadeType != AUDIOLIB_MUTE_FADE_TYPE_SMOOTH) && (pKerPrivArgs->remainingFadeSamples == 0U));
   const bool isSmooth    = (fadeType == AUDIOLIB_MUTE_FADE_TYPE_SMOOTH);
   const bool isSymmetric = (fadeType == AUDIOLIB_MUTE_FADE_TYPE_SYMMETRIC);

   if (isHard) {
      loopCycles = AUDIOLIB_MUTENCH_HARD_PIPE_UP_CYCLES +
                   (uint64_t) pKerPrivArgs->numHardVectors * AUDIOLIB_MUTENCH_HARD_CYCLES_PER_VECTOR;
      overheadCycles = AUDIOLIB_MUTENCH_HARD_CALL_OVERHEAD_CYCLES;
   }
   else if (isSymmetric && pKerPrivArgs->initArgs.isInterleaved) {
      loopCycles = numBlocks *
                   (AUDIOLIB_MUTENCH_SYMMETRIC_PIPE_UP_CYCLES + wBlocks * AUDIOLIB_MUTENCH_FADE_CYCLES_PER_ITERATION);
      overheadCycles = AUDIOLIB_MUTENCH_SYMMETRIC_INTERLEAVE_CALL_OVERHEAD_CYCLES +
                       numBlocks * AUDIOLIB_MUTENCH_SYMMETRIC_INTERLEAVE_ROW_OVERHEAD_CYCLES;
   }
   else if (isSymmetric) {
      loopCycles = numBlocks * (AUDIOLIB_MUTENCH_SYMMETRIC_PIPE_UP_CYCLES +
                                wBlocks * AUDIOLIB_MUTENCH_FADE_CYCLES_PER_ITERATION) +
                   AUDIOLIB_MUTENCH_SYMMETRIC_GAIN_UPDATE_PIPE_UP_CYCLES +
                   numGainVectors * AUDIOLIB_MUTENCH_SYMMETRIC_GAIN_UPDATE_CYCLES_PER_VECTOR;
      overheadCycles = AUDIOLIB_MUTENCH_SYMMETRIC_PLANAR_CALL_OVERHEAD_CYCLES +
                       numBlocks * AUDIOLIB_MUTENCH_SYMMETRIC_PLANAR_ROW_OVERHEAD_CYCLES;
   }
   else if (isSmooth && pKerPrivArgs->initArgs.isInterleaved) {
      loopCycles =
          numBlocks * (AUDIOLIB_MUTENCH_SMOOTH_PIPE_UP_CYCLES + wBlocks * AUDIOLIB_MUTENCH_FADE_CYCLES_PER_ITERATION);
      overheadCycles = AUDIOLIB_MUTENCH_SMOOTH_INTERLEAVE_CALL_OVERHEAD_CYCLES +
                       numBlocks * AUDIOLIB_MUTENCH_SMOOTH_INTERLEAVE_ROW_OVERHEAD_CYCLES;
   }
   else if (isSmooth) {
      loopCycles =
          numBlocks * (AUDIOLIB_MUTENCH_SMOOTH_PIPE_UP_CYCLES + wBlocks * AUDIOLIB_MUTENCH_FADE_CYCLES_PER_ITERATION) +
          AUDIOLIB_MUTENCH_SMOOTH_GAIN_UPDATE_PIPE_UP_CYCLES +
          numGainVectors * AUDIOLIB_MUTENCH_SMOOTH_GAIN_UPDATE_CYCLES_PER_VECTOR;
      overheadCycles = AUDIOLIB_MUTENCH_SMOOTH_PLANAR_CALL_OVERHEAD_CYCLES +
                       numBlocks * AUDIOLIB_MUTENCH_SMOOTH_PLANAR_ROW_OVERHEAD_CYCLES;
   }
   else if (pKerPrivArgs->initArgs.isInterleaved) {
      loopCycles =
          numBlocks * (AUDIOLIB_MUTENCH_LINEAR_PIPE_UP_CYCLES + wBlocks * AUDIOLIB_MUTENCH_FADE_CYCLES_PER_ITERATION);
      overheadCycles = AUDIOLIB_MUTENCH_LINEAR_INTERLEAVE_CALL_OVERHEAD_CYCLES +
                       numBlocks * AUDIOLIB_MUTENCH_LINEAR_INTERLEAVE_ROW_OVERHEAD_CYCLES;
   }
   else {
      loopCycles =
          numBlocks * (AUDIOLIB_MUTENCH_LINEAR_PIPE_UP_CYCLES + wBlocks * AUDIOLIB_MUTENCH_FADE_CYCLES_PER_ITERATION) +
          AUDIOLIB_MUTENCH_LINEAR_GAIN_UPDATE_PIPE_UP_CYCLES +
          numGainVectors * AUDIOLIB_MUTENCH_LINEAR_GAIN_UPDATE_CYCLES_PER_VECTOR;
      overheadCycles = AUDIOLIB_MUTENCH_LINEAR_PLANAR_CALL_OVERHEAD_CYCLES +
                       numBlocks * AUDIOLIB_MUTENCH_LINEAR_PLANAR_ROW_OVERHEAD_CYCLES;
   }

   *archCycles = loopCycles;
   *estCycles  = loopCycles + overheadCycles;
}
