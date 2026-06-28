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

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params = __gen_SA_TEMPLATE_v1();

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   if (isInterleaved == 0) {
      // /*********************************************************************************/
      // /* Prepare streaming engine 0 to fetch input samples from odd number of channels */
      // /*********************************************************************************/
      se0Params.ICNT0         = eleCount;
      se0Params.ICNT1         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(samples, eleCount), 2);
      se0Params.DIM1          = 2 * eleCount;
      se0Params.DIM2          = strideInElements;
      se0Params.ICNT2         = channels;
      se0Params.ELETYPE       = SE_ELETYPE;
      se0Params.VECLEN        = SE_VECLEN;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.DECDIM1_WIDTH = samples;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      // /*********************************************************************************/
      // /* Prepare streaming engine 1 to fetch input samples from even number of channels */
      // /*********************************************************************************/
      se1Params.ICNT0   = eleCount;
      se1Params.ICNT1   = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(samples, eleCount), 2);
      se1Params.DIM1    = 2 * eleCount;
      se1Params.DIM2    = strideInElements;
      se1Params.ICNT2   = channels;
      se1Params.ELETYPE = SE_ELETYPE;
      se1Params.VECLEN  = SE_VECLEN;
      se1Params.DIMFMT  = __SE_DIMFMT_3D;

      se1Params.DECDIM1       = __SE_DECDIM_DIM1;
      se1Params.DECDIM1_WIDTH = samples < eleCount ? samples : samples - eleCount;

      // /*********************************************************************************/
      // /* Prepare Streaming address generator 0 to store output from Streaming Engine 0 */
      // /*********************************************************************************/
      sa0Params.ICNT0         = eleCount;
      sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(samples, eleCount), 2);
      sa0Params.DIM1          = 2 * eleCount;
      sa0Params.DIM2          = strideOutElements;
      sa0Params.ICNT2         = channels;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.VECLEN        = SA_VECLEN;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = samples;

      // /*********************************************************************************/
      // /* Prepare Streaming address generator 1 to store output from Streaming Engine 1 */
      // /*********************************************************************************/
      sa1Params.ICNT0         = eleCount;
      sa1Params.ICNT1         = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(samples, eleCount), 2);
      sa1Params.DIM1          = 2 * eleCount;
      sa1Params.DIM2          = strideOutElements;
      sa1Params.ICNT2         = channels;
      sa1Params.DIMFMT        = __SA_DIMFMT_3D;
      sa1Params.VECLEN        = SA_VECLEN;
      sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa1Params.DECDIM1_WIDTH = samples < eleCount ? samples : samples - eleCount;
   }
   else // isInterleaved == 1
   {
      // /*********************************************************************************/
      // /* Prepare streaming engine 0 to fetch input samples from odd number of samples  */
      // /*********************************************************************************/

      se0Params.ICNT0         = eleCount;
      se0Params.ICNT1         = AUDIOLIB_ceilingDiv(samples, 2);
      se0Params.DIM1          = 2 * strideInElements;
      se0Params.DIM2          = eleCount;
      se0Params.ICNT2         = AUDIOLIB_ceilingDiv(channels, eleCount);
      se0Params.VECLEN        = SE_VECLEN;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.ELETYPE       = SE_ELETYPE;
      se0Params.DECDIM1_WIDTH = channels;
      se0Params.DECDIM1       = __SE_DECDIM_DIM2;

      // /*********************************************************************************/
      // /* Prepare streaming engine 1 to fetch input samples from even number of samples */
      // /*********************************************************************************/
      se1Params.ICNT0         = eleCount;
      se1Params.ICNT1         = samples % 2 == 0 ? samples / 2 : AUDIOLIB_ceilingDiv(samples, 2);
      se1Params.DIM1          = 2 * strideInElements;
      se1Params.DIM2          = eleCount;
      se1Params.ICNT2         = AUDIOLIB_ceilingDiv(channels, eleCount);
      se1Params.VECLEN        = SE_VECLEN;
      se1Params.DIMFMT        = __SE_DIMFMT_3D;
      se1Params.ELETYPE       = SE_ELETYPE;
      se1Params.DECDIM1       = __SE_DECDIM_DIM1;
      se1Params.DECDIM1_WIDTH = (samples / 2) * 2 * strideInElements;
      se1Params.DECDIM2_WIDTH = channels;
      se1Params.DECDIM2       = __SE_DECDIM_DIM2;

      // /*********************************************************************************/
      // /* Prepare Streaming address generator 0 to store output from Streaming Engine 0 */
      // /*********************************************************************************/
      sa0Params.ICNT0         = eleCount;
      sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(samples, 2);
      sa0Params.DIM1          = 2 * strideOutElements;
      sa0Params.DIM2          = eleCount;
      sa0Params.ICNT2         = AUDIOLIB_ceilingDiv(channels, eleCount);
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.VECLEN        = SA_VECLEN;
      sa0Params.DECDIM1_WIDTH = channels;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM2;

      // /*********************************************************************************/
      // /* Prepare Streaming address generator 1 to store output from Streaming Engine 1 */
      // /*********************************************************************************/

      sa1Params.ICNT0         = eleCount;
      sa1Params.ICNT1         = samples % 2 == 0 ? samples / 2 : AUDIOLIB_ceilingDiv(samples, 2);
      sa1Params.DIM1          = 2 * strideOutElements;
      sa1Params.DIM2          = eleCount;
      sa1Params.ICNT2         = AUDIOLIB_ceilingDiv(channels, eleCount);
      sa1Params.DIMFMT        = __SA_DIMFMT_3D;
      sa1Params.VECLEN        = SA_VECLEN;
      sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa1Params.DECDIM1_WIDTH = (samples / 2) * 2 * strideOutElements;
      sa1Params.DECDIM2       = __SA_DECDIM_DIM2;
      sa1Params.DECDIM2_WIDTH = channels;

      sa2Params.ICNT0  = channels;
      sa2Params.DIMFMT = __SA_DIMFMT_1D;
      sa2Params.VECLEN = SA_VECLEN;
   }

   // Store the prepared SE and SA templates in the pblock
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;

   // Pre-calculate loop bounds and pointer offsets for the execution phase.
   if (isInterleaved) // isInterleaved == 1
   {
      pKerPrivArgs->numBlocks  = AUDIOLIB_ceilingDiv(channels, eleCount); // outerloop count
      pKerPrivArgs->wBlocks    = AUDIOLIB_ceilingDiv(samples, 2);         // inner loopCount
      pKerPrivArgs->pInOffset  = pKerPrivArgs->strideInElements;
      pKerPrivArgs->pOutOffset = pKerPrivArgs->strideOutElements;
   }
   else // isInterleaved == 0
   {
      pKerPrivArgs->numBlocks  = channels;                                                       // outer loop count
      pKerPrivArgs->wBlocks    = AUDIOLIB_ceilingDiv(AUDIOLIB_ceilingDiv(samples, eleCount), 2); // inner loop count
      pKerPrivArgs->pInOffset  = samples < eleCount ? 0 : eleCount;
      pKerPrivArgs->pOutOffset = samples < eleCount ? 0 : eleCount;
   }
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
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   float   *currentGain                                                = pKerPrivArgs->currentGain;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   uint32_t pInOffset                                                  = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                                                 = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                                                  = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                                                    = pKerPrivArgs->wBlocks;
   float   *gainStepPtr                                                = pKerPrivArgs->gainStep;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);

   vec currentGainVec1, currentGainVec2, currentGainVec3, currentGainVec4;
   vec inputSe0, inputSe1, outputSa0, outputSa1;
   vec input2Se0, input2Se1, output1Sa0, output1Sa1;

   vec nSamplesVec = (vec) ((float) pKerPrivArgs->samples);
   vec gainStepVec;
   vec initialGainVec;
   vec zeroVec = (vec) (0.0f);
   vec oneVec  = (vec) (1.0f);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pInOffset, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   __SA2_OPEN(sa2Params);
   for (uint32_t i = 0; i < numBlocks; i++) {
      gainStepVec           = *(stov_ptr(vec, (float *) &gainStepPtr[i * eleCount]));
      initialGainVec        = *(stov_ptr(vec, (float *) &currentGain[i * eleCount]));
      vec processingGainVec = initialGainVec;
      vec gainStepVec2      = 2.0f * gainStepVec;
      vec gainStepVec3      = 3.0f * gainStepVec;
      vec gainStepVec4      = 4.0f * gainStepVec;

      // 12 + wBlocks * 5
      for (size_t j = 0; j < wBlocks; j += 2) {
         currentGainVec1 = processingGainVec + gainStepVec;
         currentGainVec1 = __min(__max(currentGainVec1, zeroVec), oneVec);
         currentGainVec2 = processingGainVec + gainStepVec2;
         currentGainVec2 = __min(__max(currentGainVec2, zeroVec), oneVec);
         currentGainVec3 = processingGainVec + gainStepVec3;
         currentGainVec3 = __min(__max(currentGainVec3, zeroVec), oneVec);
         currentGainVec4 = processingGainVec + gainStepVec4;
         currentGainVec4 = __min(__max(currentGainVec4, zeroVec), oneVec);
         processingGainVec += 4 * gainStepVec;

         inputSe0  = c7x::strm_eng<0, vec>::get_adv();
         inputSe1  = c7x::strm_eng<1, vec>::get_adv();
         input2Se0 = c7x::strm_eng<0, vec>::get_adv();
         input2Se1 = c7x::strm_eng<1, vec>::get_adv();

         outputSa0  = inputSe0 * currentGainVec1;
         outputSa1  = inputSe1 * currentGainVec2;
         output1Sa0 = input2Se0 * currentGainVec3;
         output1Sa1 = input2Se1 * currentGainVec4;

         // store outputSa0 to output buffer pOutLocal
         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

         // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
         __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmpSa1, outPtrSa1, outputSa1);

         // store outputSa0 to output buffer pOutLocal
         __vpred tmp2Sa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *out2PtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmp2Sa0, out2PtrSa0, output1Sa0);

         // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
         __vpred tmp2Sa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *out2PtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmp2Sa1, out2PtrSa1, output1Sa1);
      }
      vec finalGainVec = initialGainVec + nSamplesVec * gainStepVec;

      // Clamp the final vector to ensure gain is between [0.0, 1.0]
      finalGainVec = __min(__max(finalGainVec, zeroVec), oneVec);

      __vpred tmpSa2    = c7x::strm_agen<2, vec>::get_vpred();
      vec    *outPtrSa2 = c7x::strm_agen<2, vec>::get_adv(currentGain);
      __vstore_pred(tmpSa2, outPtrSa2, finalGainVec);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();
   __SA2_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNChLinearFadeInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                             void *restrict pIn,
                                                                             void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChLinearFadeUnrolledDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                       void *restrict pIn,
                                                                       void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteNChLinearFadeUnrolledDeinterleave_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount     = c7x::element_count_of<vec>::value;
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   uint32_t pInOffset                                                  = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                                                 = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                                                  = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                                                    = pKerPrivArgs->wBlocks;
   uint32_t numSamples                                                 = pKerPrivArgs->samples;
   float   *currentGain                                                = pKerPrivArgs->currentGain;
   float   *gainStepPtr                                                = pKerPrivArgs->gainStep;
   float    gainStep                                                   = 0;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   vec currentGainVec1, currentGainVec2, currentGainVec3, currentGainVec4;
   vec inputSe0, inputSe1, outputSa0, outputSa1;
   vec input2Se0, input2Se1, output2Sa0, output2Sa1;
   vec gainStepVec;
   vec gainStepAdderVec, nextGainStepVecMid;
   vec initialGainVec;
   vec zeroVec = (vec) (0.0f);
   vec oneVec  = (vec) (1.0f);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pInOffset, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint32_t i = 0; i < numBlocks; i++) {
      gainStep            = gainStepPtr[i];
      gainStepVec         = *(stov_ptr(vec, &pKerPrivArgs->precalculatedGainVectors[i * eleCount]));
      gainStepAdderVec    = (vec) (8 * gainStep);
      initialGainVec      = (vec) (currentGain[i]);
      vec nextGainStepVec = gainStepVec;
      vec totalStep       = 4.0f * gainStepAdderVec;

      // 16 + trip_cnt * 5
      for (size_t j = 0; j < wBlocks; j += 2) {
         currentGainVec1    = initialGainVec + nextGainStepVec;
         currentGainVec2    = initialGainVec + nextGainStepVec + gainStepAdderVec;
         nextGainStepVecMid = nextGainStepVec + 2 * gainStepAdderVec;
         currentGainVec3    = initialGainVec + nextGainStepVecMid;
         currentGainVec4    = initialGainVec + nextGainStepVecMid + gainStepAdderVec;
         nextGainStepVec += totalStep;

         currentGainVec1 = __min(__max(currentGainVec1, zeroVec), oneVec);
         currentGainVec2 = __min(__max(currentGainVec2, zeroVec), oneVec);
         currentGainVec3 = __min(__max(currentGainVec3, zeroVec), oneVec);
         currentGainVec4 = __min(__max(currentGainVec4, zeroVec), oneVec);

         inputSe0  = c7x::strm_eng<0, vec>::get_adv();
         inputSe1  = c7x::strm_eng<1, vec>::get_adv();
         input2Se0 = c7x::strm_eng<0, vec>::get_adv();
         input2Se1 = c7x::strm_eng<1, vec>::get_adv();

         outputSa0  = inputSe0 * currentGainVec1;
         outputSa1  = inputSe1 * currentGainVec2;
         output2Sa0 = input2Se0 * currentGainVec3;
         output2Sa1 = input2Se1 * currentGainVec4;

         // store outputSa0 to output buffer pOutLocal
         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

         // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
         __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmpSa1, outPtrSa1, outputSa1);

         // store outputSa0 to output buffer pOutLocal
         __vpred tmp2Sa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *out2PtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmp2Sa0, out2PtrSa0, output2Sa0);

         // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
         __vpred tmp2Sa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *out2PtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmp2Sa1, out2PtrSa1, output2Sa1);
      }
      float finalGain    = currentGain[i] + (float) numSamples * gainStep;
      vec   finalGainVec = __min(__max((vec) (finalGain), zeroVec), oneVec);
      currentGain[i]     = __get_vector_element(finalGainVec, (int8_t) 0);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS AUDIOLIB_muteNChLinearFadeUnrolledDeinterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
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
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   uint32_t pInOffset                                                  = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                                                 = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                                                  = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                                                    = pKerPrivArgs->wBlocks;
   uint32_t numSamples                                                 = pKerPrivArgs->samples;
   float   *currentGain                                                = pKerPrivArgs->currentGain;
   float   *gainStepPtr                                                = pKerPrivArgs->gainStep;
   float    gainStep                                                   = 0;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   vec currentGainVec1, currentGainVec2;
   vec inputSe0, inputSe1, outputSa0, outputSa1;
   vec gainStepVec;
   vec gainStepAdderVec;
   vec initialGainVec;
   vec zeroVec = (vec) (0.0f);
   vec oneVec  = (vec) (1.0f);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pInOffset, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint32_t i = 0; i < numBlocks; i++) {
      gainStep            = gainStepPtr[i];
      gainStepVec         = *(stov_ptr(vec, &pKerPrivArgs->precalculatedGainVectors[i * eleCount]));
      gainStepAdderVec    = (vec) (8 * gainStep);
      initialGainVec      = (vec) (currentGain[i]);
      vec nextGainStepVec = gainStepVec;

      // 18 + trip_cnt * 3
      for (size_t j = 0; j < wBlocks; j++) {
         currentGainVec1 = initialGainVec + nextGainStepVec;
         currentGainVec2 = initialGainVec + nextGainStepVec + gainStepAdderVec;
         nextGainStepVec = nextGainStepVec + 2 * gainStepAdderVec;
         currentGainVec1 = __min(__max(currentGainVec1, zeroVec), oneVec);
         currentGainVec2 = __min(__max(currentGainVec2, zeroVec), oneVec);

         inputSe0 = c7x::strm_eng<0, vec>::get_adv();
         inputSe1 = c7x::strm_eng<1, vec>::get_adv();

         outputSa0 = inputSe0 * currentGainVec1;
         outputSa1 = inputSe1 * currentGainVec2;

         // store outputSa0 to output buffer pOutLocal
         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

         // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
         __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
      }
      // Save the final gain value for the next execution frame
      float finalGain    = currentGain[i] + (float) numSamples * gainStep;
      vec   finalGainVec = __min(__max((vec) (finalGain), zeroVec), oneVec);
      currentGain[i]     = __get_vector_element(finalGainVec, (int8_t) 0);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

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
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   uint32_t pInOffset                                                  = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                                                 = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                                                  = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                                                    = pKerPrivArgs->wBlocks;
   float   *targetGain                                                 = pKerPrivArgs->targetGain;
   float   *currentGain                                                = pKerPrivArgs->currentGain;
   float    alphaN                                                     = pKerPrivArgs->alphaN;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   vec currentGainVec1, currentGainVec2;
   vec inputSe0, inputSe1, outputSa0, outputSa1;
   vec initialGainVec, targetGainVec;

   float vecCoeffMultiplier = pKerPrivArgs->alphaMultiplier;
   vec   coeffSquaredVec    = vecCoeffMultiplier * vecCoeffMultiplier;

   float *alphaPtr  = pKerPrivArgs->alphaCoeff;
   vec    alphaNVec = (vec) (alphaN);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pInOffset, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint32_t i = 0; i < numBlocks; i++) {
      initialGainVec = (vec) (currentGain[i]);
      targetGainVec  = (vec) (targetGain[i]);
      vec coeffA     = *(stov_ptr(vec, (float *) &alphaPtr[0]));
      vec coeffB     = coeffA * vecCoeffMultiplier;
      // 12 + trip_cnt * 4
      for (size_t j = 0; j < wBlocks; j++) {

         currentGainVec1 = coeffA * initialGainVec + (1.0f - coeffA) * targetGainVec;
         currentGainVec2 = coeffB * initialGainVec + (1.0f - coeffB) * targetGainVec;
         coeffA          = coeffA * coeffSquaredVec;
         coeffB          = coeffB * coeffSquaredVec;

         inputSe0 = c7x::strm_eng<0, vec>::get_adv();
         inputSe1 = c7x::strm_eng<1, vec>::get_adv();

         outputSa0 = inputSe0 * currentGainVec1;
         outputSa1 = inputSe1 * currentGainVec2;

         // store outputSa0 to output buffer pOutLocal
         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

         // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
         __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
      }
      // Save the final gain value for the next execution frame
      vec finalGainVec = alphaNVec * (initialGainVec - targetGainVec) + targetGainVec;
      currentGain[i]   = __get_vector_element(finalGainVec, (int8_t) 0);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

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
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   float   *targetGain                                                 = pKerPrivArgs->targetGain;
   float   *currentGain                                                = pKerPrivArgs->currentGain;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   uint32_t pInOffset                                                  = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                                                 = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                                                  = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                                                    = pKerPrivArgs->wBlocks;
   float    alphaN                                                     = pKerPrivArgs->alphaN;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);

   vec currentGainVec1, currentGainVec2;
   vec inputSe0, inputSe1, outputSa0, outputSa1;

   vec    initialGainVec, targetGainVec;
   vec    vecCoefficient;
   float *alphaPtr  = pKerPrivArgs->alphaCoeff;
   vec    alphaNVec = (vec) (alphaN);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pInOffset, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   __SA2_OPEN(sa2Params);
   for (uint32_t i = 0; i < numBlocks; i++) {
      initialGainVec         = *(stov_ptr(vec, (float *) &currentGain[i * eleCount]));
      targetGainVec          = *(stov_ptr(vec, (float *) &targetGain[i * eleCount]));
      vecCoefficient         = *(stov_ptr(vec, (float *) &alphaPtr[0]));
      vec coeffBufA          = vecCoefficient;
      vec coeffBufB          = coeffBufA * vecCoefficient;
      vec coeffMultiplierVec = vecCoefficient * vecCoefficient;

      // 12 + trip_cnt * 4
      for (size_t j = 0; j < wBlocks; j++) {
         currentGainVec1 = coeffBufA * initialGainVec + (1.0f - coeffBufA) * targetGainVec;
         currentGainVec2 = coeffBufB * initialGainVec + (1.0f - coeffBufB) * targetGainVec;

         inputSe0 = c7x::strm_eng<0, vec>::get_adv();
         inputSe1 = c7x::strm_eng<1, vec>::get_adv();

         outputSa0 = inputSe0 * currentGainVec1;
         outputSa1 = inputSe1 * currentGainVec2;

         coeffBufA = coeffBufA * coeffMultiplierVec;
         coeffBufB = coeffBufB * coeffMultiplierVec;

         // store outputSa0 to output buffer pOutLocal
         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

         // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
         __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
      }
      vec     finalGainVec = alphaNVec * (initialGainVec - targetGainVec) + targetGainVec;
      __vpred tmpSa2       = c7x::strm_agen<2, vec>::get_vpred();
      vec    *outPtrSa2    = c7x::strm_agen<2, vec>::get_adv(currentGain);
      __vstore_pred(tmpSa2, outPtrSa2, finalGainVec);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();
   __SA2_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_muteNChExponentialFadeInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                                  void *restrict pIn,
                                                                                  void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_hardMuteInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_hardMuteInterleave_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   float   *targetGain                                                 = pKerPrivArgs->targetGain;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   uint32_t pInOffset                                                  = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                                                 = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                                                  = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                                                    = pKerPrivArgs->wBlocks;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pInOffset, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   for (uint32_t ch = 0; ch < numBlocks; ch++) {
      vec targetGainVec = *(stov_ptr(vec, (float *) &targetGain[ch * eleCount]));

      // 4 + trip_cnt * 2
      for (size_t j = 0; j < wBlocks; j++) {
         vec inputSe0 = c7x::strm_eng<0, vec>::get_adv();
         vec inputSe1 = c7x::strm_eng<1, vec>::get_adv();

         vec outputSa0 = inputSe0 * targetGainVec;
         vec outputSa1 = inputSe1 * targetGainVec;

         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

         __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
      }
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_hardMuteInterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_hardMuteDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_hardMuteDeinterleave_exec_ci\n");
#endif

   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint8_t                   *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal             = (dataType *) pIn;
   dataType *restrict pOutLocal            = (dataType *) pOut;
   float   *targetGain                     = pKerPrivArgs->targetGain;
   uint32_t pInOffset                      = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                     = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                      = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                        = pKerPrivArgs->wBlocks;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pInOffset, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   for (uint32_t ch = 0; ch < numBlocks; ch++) {
      vec targetGainVec = (vec) (targetGain[ch]);

      // 4 + trip_cnt * 2
      for (size_t j = 0; j < wBlocks; j++) {
         vec inputSe0 = c7x::strm_eng<0, vec>::get_adv();
         vec inputSe1 = c7x::strm_eng<1, vec>::get_adv();

         vec outputSa0 = inputSe0 * targetGainVec;
         vec outputSa1 = inputSe1 * targetGainVec;

         __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

         __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
         __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
      }
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_hardMuteDeinterleave_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

void AUDIOLIB_muteNCh_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_muteNCh_PrivArgs *pKerPrivArgs           = (AUDIOLIB_muteNCh_PrivArgs *) handle;
   uint32_t                   wBlocks                = pKerPrivArgs->wBlocks;
   uint8_t                    fadeType               = pKerPrivArgs->setArgs.fadeType;
   float                      fadeTime               = pKerPrivArgs->setArgs.fadeTime;
   uint32_t                   numBlocks              = pKerPrivArgs->numBlocks;
   int32_t                    isInterleave           = pKerPrivArgs->initArgs.isInterleaved;
   const bool                 useUnrolled            = (pKerPrivArgs->samples % 8 == 0);
   uint32_t                   muteNChStartupCycles   = 0;
   uint32_t                   muteNChOperationCycles = 0;
   uint32_t                   muteNChTearDownCycles  = 0;
   uint32_t                   muteNChOverheadCycles  = 0;

   if (fadeTime == 0.0f) {
      fadeType = AUDIOLIB_MUTE_FADE_TYPE_HARD;
   }

   switch (fadeType) {
   case AUDIOLIB_MUTE_FADE_TYPE_LINEAR:
      if (isInterleave) {
         muteNChStartupCycles   = 36 + 1;
         muteNChOperationCycles = (13 + 1 + 12 + (wBlocks / 2) * 5 + 13) * numBlocks;
         muteNChTearDownCycles  = 3;
      }
      else {
         if (useUnrolled) {
            muteNChStartupCycles   = 25 + 1;
            muteNChOperationCycles = (16 + 3 + 16 + (wBlocks / 2) * 5 + 21) * numBlocks;
            muteNChTearDownCycles  = 6;
         }
         else {
            muteNChStartupCycles   = 25 + 2;
            muteNChOperationCycles = (14 + 4 + 18 + wBlocks * 3 + 21) * numBlocks;
            muteNChTearDownCycles  = 6;
         }
      }
      break;
   case AUDIOLIB_MUTE_FADE_TYPE_SMOOTH:
      if (!isInterleave) {
         muteNChStartupCycles   = 27 + 2;
         muteNChOperationCycles = (10 + 1 + 12 + wBlocks * 4 + 19) * numBlocks;
         muteNChTearDownCycles  = 2;
      }
      else {
         muteNChStartupCycles   = 36 + 2;
         muteNChOperationCycles = (10 + 1 + 12 + wBlocks * 4 + 14) * numBlocks;
         muteNChTearDownCycles  = 3;
      }
      break;
   case AUDIOLIB_MUTE_FADE_TYPE_HARD:
      if (isInterleave) {
         muteNChStartupCycles   = 29 + 1;
         muteNChOperationCycles = (8 + 4 + 4 + wBlocks * 2 + 3) * numBlocks;
         muteNChTearDownCycles  = 2;
      }
      else {
         muteNChStartupCycles   = 27 + 1;
         muteNChOperationCycles = (7 + 4 + 4 + wBlocks * 2 + 3) * numBlocks;
         muteNChTearDownCycles  = 2;
      }
      break;
   }

   *archCycles += muteNChStartupCycles + muteNChOperationCycles + muteNChTearDownCycles;
   *estCycles += muteNChStartupCycles + muteNChOperationCycles + muteNChOverheadCycles + muteNChTearDownCycles;
}
