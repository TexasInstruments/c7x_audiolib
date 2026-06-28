// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_mute_priv.h"
// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_mute_init_ci(AUDIOLIB_kernelHandle         handle,
                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      const AUDIOLIB_mute_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_mute_init_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS         status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs      = (AUDIOLIB_mute_PrivArgs *) handle;
   uint8_t                *pBlock            = pKerPrivArgs->bufPblock;
   uint32_t                samples           = pKerPrivArgs->samples;
   uint32_t                eleCount          = c7x::element_count_of<vec>::value;
   uint32_t                channels          = pKerPrivArgs->channels;
   uint8_t                 isInterleaved     = pKerInitArgs->isInterleaved;
   uint32_t                strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t                strideOutElements = pKerPrivArgs->strideOutElements;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   if (isInterleaved == 0) {
      if (channels == 1) {
         // /**********************************************************************/
         // /* Prepare streaming engine 0 to fetch input samples                  */
         // /**********************************************************************/
         se0Params.ICNT0   = samples;
         se0Params.ELETYPE = SE_ELETYPE;
         se0Params.VECLEN  = SE_VECLEN;
         se0Params.DIMFMT  = __SE_DIMFMT_1D;

         // /**********************************************************************/
         // /* Prepare streaming engine 1 to fetch input samples                  */
         // /**********************************************************************/
         se1Params.ICNT0   = samples;
         se1Params.ELETYPE = SE_ELETYPE;
         se1Params.VECLEN  = SE_VECLEN;
         se1Params.DIMFMT  = __SE_DIMFMT_1D;

         // /********************************************************************* */
         // /* Prepare SA0 template to store output                                */
         // /********************************************************************* */
         sa0Params.ICNT0  = samples;
         sa0Params.VECLEN = SA_VECLEN;
         sa0Params.DIMFMT = __SA_DIMFMT_1D;

         // /********************************************************************* */
         // /* Prepare SA1 template to store output                                */
         // /********************************************************************* */
         sa1Params.ICNT0  = samples;
         sa1Params.VECLEN = SA_VECLEN;
         sa1Params.DIMFMT = __SA_DIMFMT_1D;
      }
      else {
         // /*********************************************************************************/
         // /* Prepare streaming engine 0 to fetch input samples from odd number of channels */
         // /*********************************************************************************/
         se0Params.ICNT0         = eleCount;
         se0Params.ICNT1         = AUDIOLIB_ceilingDiv(channels, 2);
         se0Params.DIM1          = 2 * strideInElements;
         se0Params.DIM2          = eleCount;
         se0Params.ICNT2         = AUDIOLIB_ceilingDiv(samples, eleCount);
         se0Params.ELETYPE       = SE_ELETYPE;
         se0Params.VECLEN        = SE_VECLEN;
         se0Params.DIMFMT        = __SE_DIMFMT_3D;
         se0Params.DECDIM1       = __SE_DECDIM_DIM2;
         se0Params.DECDIM1_WIDTH = samples;

         // /*********************************************************************************/
         // /* Prepare streaming engine 1 to fetch input samples from even number of channels */
         // /*********************************************************************************/
         se1Params.ICNT0         = eleCount;
         se1Params.ICNT1         = channels % 2 == 0 ? channels / 2 : AUDIOLIB_ceilingDiv(channels, 2);
         se1Params.DIM1          = 2 * strideInElements;
         se1Params.DIM2          = eleCount;
         se1Params.ICNT2         = AUDIOLIB_ceilingDiv(samples, eleCount);
         se1Params.ELETYPE       = SE_ELETYPE;
         se1Params.VECLEN        = SE_VECLEN;
         se1Params.DIMFMT        = __SE_DIMFMT_3D;
         se1Params.DECDIM1       = __SE_DECDIM_DIM1;
         se1Params.DECDIM1_WIDTH = (channels / 2) * 2 * strideInElements;
         se1Params.DECDIM2       = __SE_DECDIM_DIM2;
         se1Params.DECDIM2_WIDTH = samples;

         // /*********************************************************************************/
         // /* Prepare Streaming address generator 0 to store output from Streaming Engine 0 */
         // /*********************************************************************************/
         sa0Params.ICNT0         = eleCount;
         sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(channels, 2);
         sa0Params.DIM1          = 2 * strideOutElements;
         sa0Params.DIM2          = eleCount;
         sa0Params.ICNT2         = AUDIOLIB_ceilingDiv(samples, eleCount);
         sa0Params.VECLEN        = SA_VECLEN;
         sa0Params.DIMFMT        = __SA_DIMFMT_3D;
         sa0Params.DECDIM1       = __SA_DECDIM_DIM2;
         sa0Params.DECDIM1_WIDTH = samples;

         // /*********************************************************************************/
         // /* Prepare Streaming address generator 1 to store output from Streaming Engine 1 */
         // /*********************************************************************************/
         sa1Params.ICNT0         = eleCount;
         sa1Params.ICNT1         = channels % 2 == 0 ? channels / 2 : AUDIOLIB_ceilingDiv(channels, 2);
         sa1Params.DIM1          = 2 * strideOutElements;
         sa1Params.DIM2          = eleCount;
         sa1Params.ICNT2         = AUDIOLIB_ceilingDiv(samples, eleCount);
         sa1Params.VECLEN        = SA_VECLEN;
         sa1Params.DIMFMT        = __SA_DIMFMT_3D;
         sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
         sa1Params.DECDIM1_WIDTH = (channels / 2) * 2 * strideOutElements;
         sa1Params.DECDIM2       = __SA_DECDIM_DIM2;
         sa1Params.DECDIM2_WIDTH = samples;
      }
   }
   else // isInterleaved == 1
   {
      // /*********************************************************************************/
      // /* Prepare streaming engine 0 to fetch input samples from odd number of samples  */
      // /*********************************************************************************/
      se0Params.ICNT0         = samples < eleCount ? samples : eleCount;
      se0Params.ICNT1         = AUDIOLIB_ceilingDiv(samples, eleCount);
      se0Params.DIM1          = 2 * eleCount;
      se0Params.DIM2          = strideInElements;
      se0Params.ICNT2         = channels;
      se0Params.VECLEN        = SE_VECLEN;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;
      se0Params.DECDIM1       = __SE_DECDIM_DIM1;
      se0Params.ELETYPE       = SE_ELETYPE;
      se0Params.DECDIM1_WIDTH = samples;

      // /*********************************************************************************/
      // /* Prepare streaming engine 1 to fetch input samples from even number of samples */
      // /*********************************************************************************/
      se1Params.ICNT0         = samples < eleCount ? samples : eleCount;
      se1Params.ICNT1         = AUDIOLIB_ceilingDiv(samples, eleCount);
      se1Params.DIM1          = 2 * eleCount;
      se1Params.DIM2          = strideInElements;
      se1Params.ICNT2         = channels;
      se1Params.VECLEN        = SE_VECLEN;
      se1Params.DIMFMT        = __SE_DIMFMT_3D;
      se1Params.ELETYPE       = SE_ELETYPE;
      se1Params.DECDIM1       = __SE_DECDIM_DIM1;
      se1Params.DECDIM1_WIDTH = samples < eleCount ? samples : samples - eleCount;

      // /*********************************************************************************/
      // /* Prepare Streaming address generator 0 to store output from Streaming Engine 0 */
      // /*********************************************************************************/
      sa0Params.ICNT0         = samples < eleCount ? samples : eleCount;
      sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(samples, eleCount);
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
      sa1Params.ICNT0         = samples < eleCount ? samples : eleCount;
      sa1Params.ICNT1         = AUDIOLIB_ceilingDiv(samples, eleCount);
      sa1Params.DIM1          = 2 * eleCount;
      sa1Params.DIM2          = strideOutElements;
      sa1Params.ICNT2         = channels;
      sa1Params.VECLEN        = SA_VECLEN;
      sa1Params.DIMFMT        = __SA_DIMFMT_3D;
      sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa1Params.DECDIM1_WIDTH = samples < eleCount ? samples : samples - eleCount;
   }

   // Store the prepared SE and SA templates in the pblock
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;

   // Pre-calculate loop bounds and pointer offsets for the execution phase.
   if (isInterleaved) // isInterleaved == 1
   {
      pKerPrivArgs->numBlocks  = channels;
      pKerPrivArgs->wBlocks    = AUDIOLIB_ceilingDiv(samples, eleCount);
      pKerPrivArgs->pInOffset  = (channels == 1) ? 0 : (samples < eleCount) ? 0 : eleCount;
      pKerPrivArgs->pOutOffset = (channels == 1) ? 0 : (samples < eleCount) ? 0 : eleCount;
   }
   else // isInterleaved == 0
   {
      pKerPrivArgs->numBlocks  = AUDIOLIB_ceilingDiv(samples, eleCount);
      pKerPrivArgs->wBlocks    = (channels == 1) ? 1 : AUDIOLIB_ceilingDiv(channels, 2);
      pKerPrivArgs->pInOffset  = (channels == 1) ? 0 : pKerPrivArgs->strideInElements;
      pKerPrivArgs->pOutOffset = (channels == 1) ? 0 : pKerPrivArgs->strideOutElements;
   }
   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_mute_init_ci<float>(AUDIOLIB_kernelHandle         handle,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                      const AUDIOLIB_mute_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
static inline void AUDIOLIB_muteBypassSmoothing(__SA_TEMPLATE_v1 sa0Params,
                                                __SA_TEMPLATE_v1 sa1Params,
                                                uint32_t         numBlocks,
                                                uint32_t         wBlocks,
                                                dataType *restrict pOutLocal,
                                                uint32_t                                              pOutOffset,
                                                const typename c7x::make_full_vector<dataType>::type &zeroVec)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   // Apply zero gain by writing zeroVec to all output channels
   // 0 + trip_cnt * 2
   for (size_t j = 0; j < numBlocks * wBlocks; j++) {
      __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(tmpSa0, outPtrSa0, zeroVec);

      __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
      vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
      __vstore_pred(tmpSa1, outPtrSa1, zeroVec);
   }

   __SA0_CLOSE();
   __SA1_CLOSE();
}

template <typename dataType>
static inline void AUDIOLIB_unMuteBypassSmoothing(dataType *restrict pInLocal,
                                                  dataType *restrict pOutLocal,
                                                  __SE_TEMPLATE_v1 se0Params,
                                                  __SE_TEMPLATE_v1 se1Params,
                                                  __SA_TEMPLATE_v1 sa0Params,
                                                  __SA_TEMPLATE_v1 sa1Params,
                                                  uint32_t         pInOffset,
                                                  uint32_t         pOutOffset,
                                                  uint32_t         numBlocks,
                                                  uint32_t         wBlocks)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + pInOffset, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);

   // Pass the input directly to the output (gain is 1.0)
   // 1 + trip_cnt * 2
   for (size_t j = 0; j < numBlocks * wBlocks; j++) {
      vec inputSe0 = c7x::strm_eng<0, vec>::get_adv();
      vec inputSe1 = c7x::strm_eng<1, vec>::get_adv();

      __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
      vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(tmpSa0, outPtrSa0, inputSe0);

      __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
      vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
      __vstore_pred(tmpSa1, outPtrSa1, inputSe1);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteLinearFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteLinearFade_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs                                *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal                                         = (dataType *) pIn;
   dataType *restrict pOutLocal                                        = (dataType *) pOut;
   uint32_t numSamples                                                 = pKerPrivArgs->samples;
   float    targetGain                                                 = pKerPrivArgs->targetGain;
   float    currentGain                                                = pKerPrivArgs->currentGain;
   uint32_t eleCount                                                   = c7x::element_count_of<vec>::value;
   uint32_t pInOffset                                                  = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                                                 = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                                                  = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                                                    = pKerPrivArgs->wBlocks;
   int32_t  bypassSmoothing                                            = pKerPrivArgs->bypassSmoothing ||
                             (fabsf(pKerPrivArgs->currentGain - targetGain) < (AUDIOLIB_MUTE_GAIN_THRESHOLD));

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   vec currentGainVec;
   vec inputSe0, inputSe1, outputSa0, outputSa1;

   vec zeroVec        = (vec) (0.0f);
   vec initialGainVec = (vec) (currentGain);

   if (!bypassSmoothing) // When smoothing is enabled
   {
      // Load pre-calculated gain step vectors for creating the linear ramp.
      float *alphaPtr            = pKerPrivArgs->alphaCoeff;
      vec   *vecAlphaPtr         = stov_ptr(vec, (float *) &alphaPtr[0]);
      vec    gainStepVec         = *vecAlphaPtr;
      float *gainStepAdderPtr    = pKerPrivArgs->gainStepAdder;
      vec   *vecGainStepAdderPtr = stov_ptr(vec, (float *) &gainStepAdderPtr[0]);
      vec    gainStepAdderVec    = *vecGainStepAdderPtr;

      __SE0_OPEN(pInLocal, se0Params);
      __SE1_OPEN(pInLocal + pInOffset, se1Params);
      __SA0_OPEN(sa0Params);
      __SA1_OPEN(sa1Params);
      for (uint32_t i = 0; i < numBlocks; i++) {
         // Update the gain vector for this block of processing.
         currentGainVec = initialGainVec + gainStepVec;
         // Clip gain at zero to prevent it from going negative.
         currentGainVec = __max(currentGainVec, zeroVec);
         // Prepare the gain step for the *next* block iteration.
         gainStepVec = gainStepVec + gainStepAdderVec;

         // 4 + trip_cnt * 2
         for (size_t j = 0; j < wBlocks; j++) {
            inputSe0 = c7x::strm_eng<0, vec>::get_adv();
            inputSe1 = c7x::strm_eng<1, vec>::get_adv();

            outputSa0 = inputSe0 * currentGainVec;
            outputSa1 = inputSe1 * currentGainVec;

            // store outputSa0 to output buffer pOutLocal
            __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
            vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
            __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

            // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
            __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
            vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
            __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
         }
      }
      __SE0_CLOSE();
      __SE1_CLOSE();
      __SA0_CLOSE();
      __SA1_CLOSE();
      // Save the final gain value for the next execution frame
      pKerPrivArgs->currentGain = __get_vector_element(currentGainVec, (numSamples - 1) % eleCount);
   }
   else // Gain is already at or near the target, so just hard mute.
   {
      AUDIOLIB_muteBypassSmoothing<dataType>(sa0Params, sa1Params, numBlocks, wBlocks, pOutLocal, pOutOffset, zeroVec);
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_muteLinearFade_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_unMuteLinearFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_unMuteLinearFade_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;
   uint8_t                *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal          = (dataType *) pIn;
   dataType *restrict pOutLocal         = (dataType *) pOut;
   uint32_t numSamples                  = pKerPrivArgs->samples;
   float    targetGain                  = pKerPrivArgs->targetGain;
   float    currentGain                 = pKerPrivArgs->currentGain;
   uint32_t pInOffset                   = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                  = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                   = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                     = pKerPrivArgs->wBlocks;
   uint32_t eleCount                    = c7x::element_count_of<vec>::value;
   int32_t  bypassSmoothing             = pKerPrivArgs->bypassSmoothing ||
                             (fabsf(pKerPrivArgs->currentGain - targetGain) < (AUDIOLIB_MUTE_GAIN_THRESHOLD));

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   vec currentGainVec;
   vec inputSe0, inputSe1, outputSa0, outputSa1;
   vec oneVec         = (vec) (1.0f);
   vec initialGainVec = (vec) (currentGain);

   if (!bypassSmoothing) // When smoothing is enabled
   {
      float *alphaPtr            = pKerPrivArgs->alphaCoeff;
      vec   *vecAlphaPtr         = stov_ptr(vec, (float *) &alphaPtr[0]);
      vec    gainStepVec         = *vecAlphaPtr;
      float *gainStepAdderPtr    = pKerPrivArgs->gainStepAdder;
      vec   *vecGainStepAdderPtr = stov_ptr(vec, (float *) &gainStepAdderPtr[0]);
      vec    gainStepAdderVec    = *vecGainStepAdderPtr;

      __SE0_OPEN(pInLocal, se0Params);
      __SE1_OPEN(pInLocal + pInOffset, se1Params);
      __SA0_OPEN(sa0Params);
      __SA1_OPEN(sa1Params);
      for (uint32_t i = 0; i < numBlocks; i++) {
         // Update the gain vector for this block of processing.
         currentGainVec = initialGainVec + gainStepVec;
         // clip to prevent undershooting
         currentGainVec = __min(currentGainVec, oneVec);
         // Prepare the gain step for the *next* block iteration.
         gainStepVec = gainStepVec + gainStepAdderVec;

         // 4 + trip_cnt * 2
         for (size_t j = 0; j < wBlocks; j++) {
            inputSe0 = c7x::strm_eng<0, vec>::get_adv();
            inputSe1 = c7x::strm_eng<1, vec>::get_adv();

            outputSa0 = inputSe0 * currentGainVec;
            outputSa1 = inputSe1 * currentGainVec;

            // store outputSa0 to output buffer pOutLocal
            __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
            vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
            __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

            // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
            __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
            vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
            __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
         }
      }
      __SE0_CLOSE();
      __SE1_CLOSE();
      __SA0_CLOSE();
      __SA1_CLOSE();

      pKerPrivArgs->currentGain = __get_vector_element(currentGainVec, (numSamples - 1) % eleCount);
   }
   else // when smoothing is disabled or when current gain reaches target gain
   {
      AUDIOLIB_unMuteBypassSmoothing<dataType>(pInLocal, pOutLocal, se0Params, se1Params, sa0Params, sa1Params,
                                               pInOffset, pOutOffset, numBlocks, wBlocks);
   }

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_unMuteLinearFade_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteSmoothFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_muteSmoothFade_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;
   uint8_t                *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal          = (dataType *) pIn;
   dataType *restrict pOutLocal         = (dataType *) pOut;
   uint32_t numSamples                  = pKerPrivArgs->samples;
   float    targetGain                  = pKerPrivArgs->targetGain;
   float    currentGain                 = pKerPrivArgs->currentGain;
   uint32_t pInOffset                   = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                  = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                   = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                     = pKerPrivArgs->wBlocks;
   uint32_t eleCount                    = c7x::element_count_of<vec>::value;
   int32_t  bypassSmoothing             = pKerPrivArgs->bypassSmoothing ||
                             (fabsf(pKerPrivArgs->currentGain - targetGain) < (AUDIOLIB_MUTE_GAIN_THRESHOLD));

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   vec currentGainVec;
   vec inputSe0, inputSe1, outputSa0, outputSa1;

   vec zeroVec = (vec) (0.0f);

   if (!bypassSmoothing) // When smoothing is enabled
   {
      vec    initialGainVec     = (vec) (currentGain);
      vec    targetGainVec      = (vec) (targetGain);
      float  vecCoeffMultiplier = pKerPrivArgs->alphaMultiplier;
      float *alphaPtr           = pKerPrivArgs->alphaCoeff;
      vec   *vecAlphaPtr        = stov_ptr(vec, (float *) &alphaPtr[0]);
      vec    vecCoefficient     = *vecAlphaPtr;

      __SE0_OPEN(pInLocal, se0Params);
      __SE1_OPEN(pInLocal + pInOffset, se1Params);
      __SA0_OPEN(sa0Params);
      __SA1_OPEN(sa1Params);
      for (uint32_t i = 0; i < numBlocks; i++) {
         // update the initialGain vector
         currentGainVec = vecCoefficient * initialGainVec + targetGainVec * (1 - vecCoefficient);

         // update the coefficient vector
         vecCoefficient = vecCoefficient * vecCoeffMultiplier;

         // 4 + trip_cnt * 2
         for (size_t j = 0; j < wBlocks; j++) {
            inputSe0 = c7x::strm_eng<0, vec>::get_adv();
            inputSe1 = c7x::strm_eng<1, vec>::get_adv();

            // Apply gains to input samples
            outputSa0 = inputSe0 * currentGainVec;
            outputSa1 = inputSe1 * currentGainVec;

            // store outputSa0 to output buffer pOutLocal
            __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
            vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
            __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

            // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
            __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
            vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
            __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
         }
      }
      __SE0_CLOSE();
      __SE1_CLOSE();
      __SA0_CLOSE();
      __SA1_CLOSE();

      pKerPrivArgs->currentGain = __get_vector_element(currentGainVec, (numSamples - 1) % eleCount);
   }
   else // when smoothing is disabled or when current gain reaches target gain
   {
      AUDIOLIB_muteBypassSmoothing<dataType>(sa0Params, sa1Params, numBlocks, wBlocks, pOutLocal, pOutOffset, zeroVec);
   }

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_muteSmoothFade_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_unMuteSmoothFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;
   uint8_t                *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal          = (dataType *) pIn;
   dataType *restrict pOutLocal         = (dataType *) pOut;
   uint32_t numSamples                  = pKerPrivArgs->samples;
   float    targetGain                  = pKerPrivArgs->targetGain;
   float    currentGain                 = pKerPrivArgs->currentGain;
   uint32_t pInOffset                   = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                  = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                   = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                     = pKerPrivArgs->wBlocks;
   uint32_t eleCount                    = c7x::element_count_of<vec>::value;
   int32_t  bypassSmoothing             = pKerPrivArgs->bypassSmoothing ||
                             (fabsf(pKerPrivArgs->currentGain - targetGain) < (AUDIOLIB_MUTE_GAIN_THRESHOLD));

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   vec currentGainVec;
   vec inputSe0, inputSe1, outputSa0, outputSa1;

   if (!bypassSmoothing) // When smoothing is enabled
   {
      vec initialGainVec = (vec) (currentGain);
      vec targetGainVec  = (vec) (targetGain);

      // getting the Alpha update value to update the coefficient series
      float  vecCoeffMultiplier = pKerPrivArgs->alphaMultiplier;
      float *alphaPtr           = pKerPrivArgs->alphaCoeff;

      // Convert the scalar array to vector pointer
      vec *vecAlphaPtr    = stov_ptr(vec, (float *) &alphaPtr[0]);
      vec  vecCoefficient = *vecAlphaPtr;

      __SE0_OPEN(pInLocal, se0Params);
      __SE1_OPEN(pInLocal + pInOffset, se1Params);
      __SA0_OPEN(sa0Params);
      __SA1_OPEN(sa1Params);
      for (uint32_t i = 0; i < numBlocks; i++) {
         // update the initialGain vector
         currentGainVec = vecCoefficient * initialGainVec + targetGainVec * (1 - vecCoefficient);

         // update the coefficient vector
         vecCoefficient = vecCoefficient * vecCoeffMultiplier;

         // 4 + trip_cnt * 2
         for (size_t j = 0; j < wBlocks; j++) {
            inputSe0 = c7x::strm_eng<0, vec>::get_adv();
            inputSe1 = c7x::strm_eng<1, vec>::get_adv();

            // Apply gains to input samples
            outputSa0 = inputSe0 * currentGainVec;
            outputSa1 = inputSe1 * currentGainVec;

            // store outputSa0 to output buffer pOutLocal
            __vpred tmpSa0    = c7x::strm_agen<0, vec>::get_vpred();
            vec    *outPtrSa0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
            __vstore_pred(tmpSa0, outPtrSa0, outputSa0);

            // store outputSa1 to output buffer pOutLocal + pOutLocalOffset
            __vpred tmpSa1    = c7x::strm_agen<1, vec>::get_vpred();
            vec    *outPtrSa1 = c7x::strm_agen<1, vec>::get_adv(pOutLocal + pOutOffset);
            __vstore_pred(tmpSa1, outPtrSa1, outputSa1);
         }
      }
      __SE0_CLOSE();
      __SE1_CLOSE();
      __SA0_CLOSE();
      __SA1_CLOSE();
      pKerPrivArgs->currentGain = __get_vector_element(currentGainVec, (numSamples - 1) % eleCount);
   }
   else // when smoothing is disabled or when current gain reaches target gain
   {
      AUDIOLIB_unMuteBypassSmoothing<dataType>(pInLocal, pOutLocal, se0Params, se1Params, sa0Params, sa1Params,
                                               pInOffset, pOutOffset, numBlocks, wBlocks);
   }

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_unMuteSmoothFade_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_hardMute_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_hardMute_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;
   uint8_t                *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pOutLocal         = (dataType *) pOut;
   uint32_t pOutOffset                  = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                   = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                     = pKerPrivArgs->wBlocks;

   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   vec zeroVec = (vec) (0.0f);
   AUDIOLIB_muteBypassSmoothing<dataType>(sa0Params, sa1Params, numBlocks, wBlocks, pOutLocal, pOutOffset, zeroVec);

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_hardMute_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_hardUnMute_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs = (AUDIOLIB_mute_PrivArgs *) handle;
   uint8_t                *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocal          = (dataType *) pIn;
   dataType *restrict pOutLocal         = (dataType *) pOut;
   uint32_t pInOffset                   = pKerPrivArgs->pInOffset;
   uint32_t pOutOffset                  = pKerPrivArgs->pOutOffset;
   uint32_t numBlocks                   = pKerPrivArgs->numBlocks;
   uint32_t wBlocks                     = pKerPrivArgs->wBlocks;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   AUDIOLIB_unMuteBypassSmoothing<dataType>(pInLocal, pOutLocal, se0Params, se1Params, sa0Params, sa1Params, pInOffset,
                                            pOutOffset, numBlocks, wBlocks);

   return status;
}
template AUDIOLIB_STATUS
     AUDIOLIB_hardUnMute_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
void AUDIOLIB_mute_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_mute_PrivArgs *pKerPrivArgs        = (AUDIOLIB_mute_PrivArgs *) handle;
   float                   isMute              = pKerPrivArgs->setArgs.isMute;
   uint32_t                wBlocks             = pKerPrivArgs->wBlocks;
   uint8_t                 fadeType            = pKerPrivArgs->setArgs.fadeType;
   uint32_t                numBlocks           = pKerPrivArgs->numBlocks;
   int32_t                 bypassSmoothing     = pKerPrivArgs->bypassSmoothing;
   uint32_t                muteStartupCycles   = 0;
   uint32_t                muteOperationCycles = 0;
   uint32_t                muteTearDownCycles  = 0;
   uint32_t                muteOverheadCycles  = 0;

   switch (fadeType) {
   case AUDIOLIB_MUTE_FADE_TYPE_LINEAR:
      if (!bypassSmoothing) {
         muteStartupCycles   = 17 + 12 + 1;
         muteOperationCycles = (8 + (4 + wBlocks * 2)) * numBlocks;
         muteTearDownCycles  = 3 + 2 + 7;
      }
      else {
         if (isMute) {
            muteStartupCycles   = 17 + 5;
            muteOperationCycles = 2 * wBlocks * numBlocks;
            muteTearDownCycles  = 2;
         }
         else {
            muteStartupCycles   = 17 + 17 + 3;
            muteOperationCycles = 1 + (wBlocks * numBlocks) * 2;
            muteTearDownCycles  = 2;
         }
      }
      break;
   case AUDIOLIB_MUTE_FADE_TYPE_SMOOTH:
      if (!bypassSmoothing) {
         muteStartupCycles   = 17 + 12 + 7;
         muteOperationCycles = (9 + 4 + (4 + wBlocks * 2)) * numBlocks;
         muteTearDownCycles  = 3 + 2 + 7;
      }
      else {
         if (isMute) {
            muteStartupCycles   = 17 + 5;
            muteOperationCycles = 2 * wBlocks * numBlocks;
            muteTearDownCycles  = 2;
         }
         else {
            muteStartupCycles   = 17 + 17 + 3;
            muteOperationCycles = 1 + (wBlocks * numBlocks) * 2;
            muteTearDownCycles  = 2;
         }
      }
      break;
   case AUDIOLIB_MUTE_FADE_TYPE_HARD:
      if (isMute) {
         muteStartupCycles   = 14 + 2;
         muteOperationCycles = 2 * wBlocks * numBlocks;
         muteTearDownCycles  = 2;
      }
      else {
         muteStartupCycles   = 28 + 3;
         muteOperationCycles = 1 + (wBlocks * numBlocks) * 2;
         muteTearDownCycles  = 2;
      }
      break;
   }

   *archCycles += muteStartupCycles + muteOperationCycles + muteTearDownCycles;
   *estCycles += muteStartupCycles + muteOperationCycles + muteOverheadCycles + muteTearDownCycles;
}
