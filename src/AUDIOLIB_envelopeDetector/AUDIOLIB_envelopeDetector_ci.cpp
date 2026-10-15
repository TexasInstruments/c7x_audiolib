// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/AUDIOLIB_inlines.h"
#include "AUDIOLIB_envelopeDetector_priv.h"
#include <cmath>
#include <cstdint>

#define AUDIOLIB_ENVELOPEDETECTOR_PARAM_BASE (0x0000)
#define AUDIOLIB_ENVELOPEDETECTOR_GROUP_SE0_PARAM_OFFSET (AUDIOLIB_ENVELOPEDETECTOR_PARAM_BASE)
#define AUDIOLIB_ENVELOPEDETECTOR_GROUP_SA0_PARAM_OFFSET                                                               \
   (AUDIOLIB_ENVELOPEDETECTOR_GROUP_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SE1_PARAM_OFFSET                                                            \
   (AUDIOLIB_ENVELOPEDETECTOR_GROUP_SA0_PARAM_OFFSET + SE_PARAM_SIZE)
#define AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SA1_PARAM_OFFSET                                                            \
   (AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SE1_PARAM_OFFSET + SE_PARAM_SIZE)

/* Writes the stream templates and channel split of the group-of-four variant */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_group4_init_ci(AUDIOLIB_kernelHandle                     handle,
                                                         const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                                         const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_envelopeDetector_PrivArgs                    *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   int32_t  strideIn         = pKerPrivArgs->strideInElements;
   int32_t  strideOut        = pKerPrivArgs->strideOutElements;
   uint32_t numChannelGroups = (pKerPrivArgs->numChannels - 1U) / AUDIOLIB_ENVELOPEDETECTOR_GROUP4_CHANNELS;
   uint32_t leftoverChannels = pKerPrivArgs->numChannels - numChannelGroups * AUDIOLIB_ENVELOPEDETECTOR_GROUP4_CHANNELS;
   uint32_t leftoverRowStep1 = (leftoverChannels > 1U) ? 1U : 0U;
   uint32_t leftoverRowStep2 = (leftoverChannels == 4U) ? 2U : ((leftoverChannels == 3U) ? 1U : 0U);

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_envelopeDetector_group4_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "numSamples %d numChannels %d numChannelGroups %d leftoverChannels %d\n",
                         pKerPrivArgs->numSamples, pKerPrivArgs->numChannels, numChannelGroups, leftoverChannels);

   pKerPrivArgs->numChannelGroups     = numChannelGroups;
   pKerPrivArgs->leftoverFirstChannel = numChannelGroups * AUDIOLIB_ENVELOPEDETECTOR_GROUP4_CHANNELS;
   pKerPrivArgs->leftoverRowStep1     = leftoverRowStep1;
   pKerPrivArgs->leftoverRowStep2     = leftoverRowStep2;
   pKerPrivArgs->leftoverRowStep3     = 0U;

   /* Both parts read one element per vector and broadcast it, in the order
    * (row step 1, row step 2, sample, group) */
   __SE_TEMPLATE_v1 groupSe0Params = __gen_SE_TEMPLATE_v1();
   groupSe0Params.ELETYPE          = c7x::se_eletype<vec>::value;
   groupSe0Params.VECLEN           = c7x::se_veclen<vec>::value;
   groupSe0Params.ELEDUP           = c7x::se_eledup<dataType, vec>::value;
   groupSe0Params.ICNT0            = 1U;
   groupSe0Params.ICNT1            = 2U;
   groupSe0Params.ICNT2            = 2U;
   groupSe0Params.ICNT3            = pKerPrivArgs->numSamples;
   groupSe0Params.DIM3             = 1;

   __SA_TEMPLATE_v1 groupSa0Params = __gen_SA_TEMPLATE_v1();
   groupSa0Params.VECLEN           = __SA_VECLEN_1ELEM;
   groupSa0Params.ICNT0            = 1U;
   groupSa0Params.ICNT1            = 2U;
   groupSa0Params.ICNT2            = 2U;
   groupSa0Params.ICNT3            = pKerPrivArgs->numSamples;
   groupSa0Params.DIM3             = 1;

   __SE_TEMPLATE_v1 leftoverSe1Params = groupSe0Params;
   __SA_TEMPLATE_v1 leftoverSa1Params = groupSa0Params;

   groupSe0Params.DIMFMT = __SE_DIMFMT_5D;
   groupSe0Params.DIM1   = strideIn;
   groupSe0Params.DIM2   = 2 * strideIn;
   groupSe0Params.ICNT4  = numChannelGroups;
   groupSe0Params.DIM4   = 4 * strideIn;

   groupSa0Params.DIMFMT = __SA_DIMFMT_5D;
   groupSa0Params.DIM1   = strideOut;
   groupSa0Params.DIM2   = 2 * strideOut;
   groupSa0Params.ICNT4  = numChannelGroups;
   groupSa0Params.DIM4   = 4 * strideOut;

   if (numChannelGroups == 0U) {
      groupSe0Params.DIMFMT = __SE_DIMFMT_1D;
      groupSa0Params.DIMFMT = __SA_DIMFMT_1D;
   }

   /* Leftover channels */
   leftoverSe1Params.DIMFMT = __SE_DIMFMT_4D;
   leftoverSe1Params.DIM1   = (int32_t) leftoverRowStep1 * strideIn;
   leftoverSe1Params.DIM2   = (int32_t) leftoverRowStep2 * strideIn;
   leftoverSa1Params.DIMFMT = __SA_DIMFMT_4D;
   leftoverSa1Params.DIM1   = (int32_t) leftoverRowStep1 * strideOut;
   leftoverSa1Params.DIM2   = (int32_t) leftoverRowStep2 * strideOut;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_GROUP_SE0_PARAM_OFFSET)    = groupSe0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_GROUP_SA0_PARAM_OFFSET)    = groupSa0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SE1_PARAM_OFFSET) = leftoverSe1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SA1_PARAM_OFFSET) = leftoverSa1Params;

   return status;
}

/* explicit instantiation for the different data type versions */
template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group4_init_ci<float>(AUDIOLIB_kernelHandle                     handle,
                                                const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                                const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                                const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group4_init_ci<double>(AUDIOLIB_kernelHandle                     handle,
                                                 const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                                 const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                                 const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

/* Writes the stream templates and channel split of the group-of-eight variant */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_group8_init_ci(AUDIOLIB_kernelHandle                     handle,
                                                         const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                                         const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_envelopeDetector_PrivArgs                    *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   int32_t  strideIn         = pKerPrivArgs->strideInElements;
   int32_t  strideOut        = pKerPrivArgs->strideOutElements;
   uint32_t numChannelGroups = (pKerPrivArgs->numChannels - 1U) / AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS;
   uint32_t leftoverChannels = pKerPrivArgs->numChannels - numChannelGroups * AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS;
   uint32_t leftoverRowStep1 = (leftoverChannels > 1U) ? 1U : 0U;
   uint32_t leftoverRowStep2 = (leftoverChannels >= 4U) ? 2U : ((leftoverChannels == 3U) ? 1U : 0U);
   uint32_t leftoverRowStep3 = (leftoverChannels > 4U) ? (leftoverChannels - 4U) : 0U;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_envelopeDetector_group8_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "numSamples %d numChannels %d numChannelGroups %d leftoverChannels %d\n",
                         pKerPrivArgs->numSamples, pKerPrivArgs->numChannels, numChannelGroups, leftoverChannels);

   pKerPrivArgs->numChannelGroups     = numChannelGroups;
   pKerPrivArgs->leftoverFirstChannel = numChannelGroups * AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS;
   pKerPrivArgs->leftoverRowStep1     = leftoverRowStep1;
   pKerPrivArgs->leftoverRowStep2     = leftoverRowStep2;
   pKerPrivArgs->leftoverRowStep3     = leftoverRowStep3;

   /* Both parts read one element per vector and broadcast it, in the order
    * (row step 1, row step 2, row step 3, sample, group) */
   __SE_TEMPLATE_v1 groupSe0Params = __gen_SE_TEMPLATE_v1();
   groupSe0Params.ELETYPE          = c7x::se_eletype<vec>::value;
   groupSe0Params.VECLEN           = c7x::se_veclen<vec>::value;
   groupSe0Params.ELEDUP           = c7x::se_eledup<dataType, vec>::value;
   groupSe0Params.ICNT0            = 1U;
   groupSe0Params.ICNT1            = 2U;
   groupSe0Params.ICNT2            = 2U;
   groupSe0Params.ICNT3            = 2U;
   groupSe0Params.ICNT4            = pKerPrivArgs->numSamples;
   groupSe0Params.DIM4             = 1;

   __SA_TEMPLATE_v1 groupSa0Params = __gen_SA_TEMPLATE_v1();
   groupSa0Params.VECLEN           = __SA_VECLEN_1ELEM;
   groupSa0Params.ICNT0            = 1U;
   groupSa0Params.ICNT1            = 2U;
   groupSa0Params.ICNT2            = 2U;
   groupSa0Params.ICNT3            = 2U;
   groupSa0Params.ICNT4            = pKerPrivArgs->numSamples;
   groupSa0Params.DIM4             = 1;

   __SE_TEMPLATE_v1 leftoverSe1Params = groupSe0Params;
   __SA_TEMPLATE_v1 leftoverSa1Params = groupSa0Params;

   groupSe0Params.DIMFMT = __SE_DIMFMT_6D;
   groupSe0Params.DIM1   = strideIn;
   groupSe0Params.DIM2   = 2 * strideIn;
   groupSe0Params.DIM3   = 4 * strideIn;
   groupSe0Params.ICNT5  = numChannelGroups;
   groupSe0Params.DIM5   = 8 * strideIn;
   groupSa0Params.DIMFMT = __SA_DIMFMT_6D;
   groupSa0Params.DIM1   = strideOut;
   groupSa0Params.DIM2   = 2 * strideOut;
   groupSa0Params.DIM3   = 4 * strideOut;
   groupSa0Params.ICNT5  = numChannelGroups;
   groupSa0Params.DIM5   = 8 * strideOut;

   if (numChannelGroups == 0U) {
      groupSe0Params.DIMFMT = __SE_DIMFMT_1D;
      groupSa0Params.DIMFMT = __SA_DIMFMT_1D;
   }

   /* Leftover channels */
   leftoverSe1Params.DIMFMT = __SE_DIMFMT_5D;
   leftoverSe1Params.DIM1   = (int32_t) leftoverRowStep1 * strideIn;
   leftoverSe1Params.DIM2   = (int32_t) leftoverRowStep2 * strideIn;
   leftoverSe1Params.DIM3   = (int32_t) leftoverRowStep3 * strideIn;
   leftoverSa1Params.DIMFMT = __SA_DIMFMT_5D;
   leftoverSa1Params.DIM1   = (int32_t) leftoverRowStep1 * strideOut;
   leftoverSa1Params.DIM2   = (int32_t) leftoverRowStep2 * strideOut;
   leftoverSa1Params.DIM3   = (int32_t) leftoverRowStep3 * strideOut;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_GROUP_SE0_PARAM_OFFSET)    = groupSe0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_GROUP_SA0_PARAM_OFFSET)    = groupSa0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SE1_PARAM_OFFSET) = leftoverSe1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SA1_PARAM_OFFSET) = leftoverSa1Params;

   return status;
}

/* explicit instantiation for the different data type versions */
template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group8_init_ci<float>(AUDIOLIB_kernelHandle                     handle,
                                                const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                                const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                                const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group8_init_ci<double>(AUDIOLIB_kernelHandle                     handle,
                                                 const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                                 const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                                 const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

/* One envelope step for float: folded to envelope + c * (magnitude - envelope) so that each step
 * needs one VMPYDP (.N unit only) and no retention select */
#pragma FUNC_ALWAYS_INLINE
static inline c7x::float_vec AUDIOLIB_envelopeDetector_step(c7x::float_vec envelopeVec,
                                                            c7x::float_vec magnitudeVec,
                                                            c7x::float_vec attackCoefficientVec,
                                                            c7x::float_vec releaseCoefficientVec)
{
   __vpred        isAttack    = __cmp_lt_pred(envelopeVec, magnitudeVec);
   c7x::float_vec coefficient = __select(isAttack, attackCoefficientVec, releaseCoefficientVec);
   return envelopeVec + coefficient * (magnitudeVec - envelopeVec);
}

/* One envelope step for double, folded to envelope + c * (magnitude - envelope) so that each step
 * needs one VMPYDP (.N unit only) and no retention select */
#pragma FUNC_ALWAYS_INLINE
static inline c7x::double_vec AUDIOLIB_envelopeDetector_step(c7x::double_vec envelopeVec,
                                                             c7x::double_vec magnitudeVec,
                                                             c7x::double_vec attackCoefficientVec,
                                                             c7x::double_vec releaseCoefficientVec)
{
   __vpred         isAttack    = __cmp_lt_pred(envelopeVec, magnitudeVec);
   c7x::double_vec coefficient = __select(isAttack, attackCoefficientVec, releaseCoefficientVec);
   return envelopeVec + coefficient * (magnitudeVec - envelopeVec);
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group4_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_envelopeDetector_PrivArgs                    *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_envelopeDetector_group4_exec_ci\n");

   __SE_TEMPLATE_v1 groupSe0Params =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_GROUP_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 groupSa0Params =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_GROUP_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 leftoverSe1Params =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 leftoverSa1Params =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SA1_PARAM_OFFSET);

   uint32_t numSamples       = pKerPrivArgs->numSamples;
   uint32_t numChannelGroups = pKerPrivArgs->numChannelGroups;
   uint32_t leftover0        = pKerPrivArgs->leftoverFirstChannel;
   uint32_t leftover1        = leftover0 + pKerPrivArgs->leftoverRowStep1;
   uint32_t leftover2        = leftover0 + pKerPrivArgs->leftoverRowStep2;
   uint32_t leftover3        = leftover1 + pKerPrivArgs->leftoverRowStep2;
   float   *pEnvelopeStates  = pKerPrivArgs->envelopeStates;

   dataType *restrict pInLocal          = (dataType *) pIn;
   dataType *restrict pOutLocal         = (dataType *) pOut;
   dataType *restrict pLeftoverInLocal  = pInLocal + leftover0 * pKerPrivArgs->strideInElements;
   dataType *restrict pLeftoverOutLocal = pOutLocal + leftover0 * pKerPrivArgs->strideOutElements;

   vec attackCoefficientVec  = vec((dataType) pKerPrivArgs->attackCoefficient);
   vec releaseCoefficientVec = vec((dataType) pKerPrivArgs->releaseCoefficient);

   __SE0_OPEN(pInLocal, groupSe0Params);
   __SA0_OPEN(groupSa0Params);
   __SE1_OPEN(pLeftoverInLocal, leftoverSe1Params);
   __SA1_OPEN(leftoverSa1Params);

   /* Full groups of four channels */
   for (uint32_t group = 0; group < numChannelGroups; group++) {
      uint32_t firstChannel = group * AUDIOLIB_ENVELOPEDETECTOR_GROUP4_CHANNELS;
      vec      envelopeVec0 = vec((dataType) pEnvelopeStates[firstChannel]);
      vec      envelopeVec1 = vec((dataType) pEnvelopeStates[firstChannel + 1U]);
      vec      envelopeVec2 = vec((dataType) pEnvelopeStates[firstChannel + 2U]);
      vec      envelopeVec3 = vec((dataType) pEnvelopeStates[firstChannel + 3U]);

#pragma MUST_ITERATE(1, , 1)
      for (uint32_t sample = 0; sample < numSamples; sample++) {
         vec magnitudeVec0 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec0 =
             AUDIOLIB_envelopeDetector_step(envelopeVec0, magnitudeVec0, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred0  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred0, pStore0, envelopeVec0);

         vec magnitudeVec1 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec1 =
             AUDIOLIB_envelopeDetector_step(envelopeVec1, magnitudeVec1, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred1  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred1, pStore1, envelopeVec1);

         vec magnitudeVec2 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec2 =
             AUDIOLIB_envelopeDetector_step(envelopeVec2, magnitudeVec2, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred2  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred2, pStore2, envelopeVec2);

         vec magnitudeVec3 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec3 =
             AUDIOLIB_envelopeDetector_step(envelopeVec3, magnitudeVec3, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred3  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred3, pStore3, envelopeVec3);
      }

      pEnvelopeStates[firstChannel]      = (float) __get_vector_element(envelopeVec0, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 1U] = (float) __get_vector_element(envelopeVec1, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 2U] = (float) __get_vector_element(envelopeVec2, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 3U] = (float) __get_vector_element(envelopeVec3, (uint8_t) 0);
   }

   /* One to four leftover channels, as one more group of four */
   vec envelopeVec0 = vec((dataType) pEnvelopeStates[leftover0]);
   vec envelopeVec1 = vec((dataType) pEnvelopeStates[leftover1]);
   vec envelopeVec2 = vec((dataType) pEnvelopeStates[leftover2]);
   vec envelopeVec3 = vec((dataType) pEnvelopeStates[leftover3]);

#pragma MUST_ITERATE(1, , 1)
   for (uint32_t sample = 0; sample < numSamples; sample++) {
      vec magnitudeVec0 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec0 =
          AUDIOLIB_envelopeDetector_step(envelopeVec0, magnitudeVec0, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred0  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore0 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred0, pStore0, envelopeVec0);

      vec magnitudeVec1 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec1 =
          AUDIOLIB_envelopeDetector_step(envelopeVec1, magnitudeVec1, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred1  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore1 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred1, pStore1, envelopeVec1);

      vec magnitudeVec2 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec2 =
          AUDIOLIB_envelopeDetector_step(envelopeVec2, magnitudeVec2, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred2  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore2 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred2, pStore2, envelopeVec2);

      vec magnitudeVec3 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec3 =
          AUDIOLIB_envelopeDetector_step(envelopeVec3, magnitudeVec3, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred3  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore3 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred3, pStore3, envelopeVec3);
   }

   /* Repeated rows store identical states */
   pEnvelopeStates[leftover0] = (float) __get_vector_element(envelopeVec0, (uint8_t) 0);
   pEnvelopeStates[leftover1] = (float) __get_vector_element(envelopeVec1, (uint8_t) 0);
   pEnvelopeStates[leftover2] = (float) __get_vector_element(envelopeVec2, (uint8_t) 0);
   pEnvelopeStates[leftover3] = (float) __get_vector_element(envelopeVec3, (uint8_t) 0);

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();
   __SA1_CLOSE();

   return status;
}

/* explicit instantiation for the different data type versions */
template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group4_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group4_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_envelopeDetector_PrivArgs                    *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_envelopeDetector_group8_exec_ci\n");

   __SE_TEMPLATE_v1 groupSe0Params =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_GROUP_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 groupSa0Params =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_GROUP_SA0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 leftoverSe1Params =
       *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 leftoverSa1Params =
       *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + AUDIOLIB_ENVELOPEDETECTOR_LEFTOVER_SA1_PARAM_OFFSET);

   uint32_t numSamples       = pKerPrivArgs->numSamples;
   uint32_t numChannelGroups = pKerPrivArgs->numChannelGroups;
   uint32_t leftover0        = pKerPrivArgs->leftoverFirstChannel;
   uint32_t leftover1        = leftover0 + pKerPrivArgs->leftoverRowStep1;
   uint32_t leftover2        = leftover0 + pKerPrivArgs->leftoverRowStep2;
   uint32_t leftover3        = leftover1 + pKerPrivArgs->leftoverRowStep2;
   uint32_t leftover4        = leftover0 + pKerPrivArgs->leftoverRowStep3;
   uint32_t leftover5        = leftover1 + pKerPrivArgs->leftoverRowStep3;
   uint32_t leftover6        = leftover2 + pKerPrivArgs->leftoverRowStep3;
   uint32_t leftover7        = leftover3 + pKerPrivArgs->leftoverRowStep3;
   float   *pEnvelopeStates  = pKerPrivArgs->envelopeStates;

   dataType *restrict pInLocal          = (dataType *) pIn;
   dataType *restrict pOutLocal         = (dataType *) pOut;
   dataType *restrict pLeftoverInLocal  = pInLocal + leftover0 * pKerPrivArgs->strideInElements;
   dataType *restrict pLeftoverOutLocal = pOutLocal + leftover0 * pKerPrivArgs->strideOutElements;

   vec attackCoefficientVec  = vec((dataType) pKerPrivArgs->attackCoefficient);
   vec releaseCoefficientVec = vec((dataType) pKerPrivArgs->releaseCoefficient);

   __SE0_OPEN(pInLocal, groupSe0Params);
   __SA0_OPEN(groupSa0Params);
   __SE1_OPEN(pLeftoverInLocal, leftoverSe1Params);
   __SA1_OPEN(leftoverSa1Params);

   /* Full groups of eight channels */
   for (uint32_t group = 0; group < numChannelGroups; group++) {
      uint32_t firstChannel = group * AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS;
      vec      envelopeVec0 = vec((dataType) pEnvelopeStates[firstChannel]);
      vec      envelopeVec1 = vec((dataType) pEnvelopeStates[firstChannel + 1U]);
      vec      envelopeVec2 = vec((dataType) pEnvelopeStates[firstChannel + 2U]);
      vec      envelopeVec3 = vec((dataType) pEnvelopeStates[firstChannel + 3U]);
      vec      envelopeVec4 = vec((dataType) pEnvelopeStates[firstChannel + 4U]);
      vec      envelopeVec5 = vec((dataType) pEnvelopeStates[firstChannel + 5U]);
      vec      envelopeVec6 = vec((dataType) pEnvelopeStates[firstChannel + 6U]);
      vec      envelopeVec7 = vec((dataType) pEnvelopeStates[firstChannel + 7U]);

#pragma MUST_ITERATE(1, , 1)
      for (uint32_t sample = 0; sample < numSamples; sample++) {
         vec magnitudeVec0 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec0 =
             AUDIOLIB_envelopeDetector_step(envelopeVec0, magnitudeVec0, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred0  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore0 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred0, pStore0, envelopeVec0);

         vec magnitudeVec1 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec1 =
             AUDIOLIB_envelopeDetector_step(envelopeVec1, magnitudeVec1, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred1  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore1 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred1, pStore1, envelopeVec1);

         vec magnitudeVec2 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec2 =
             AUDIOLIB_envelopeDetector_step(envelopeVec2, magnitudeVec2, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred2  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore2 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred2, pStore2, envelopeVec2);

         vec magnitudeVec3 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec3 =
             AUDIOLIB_envelopeDetector_step(envelopeVec3, magnitudeVec3, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred3  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore3 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred3, pStore3, envelopeVec3);

         vec magnitudeVec4 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec4 =
             AUDIOLIB_envelopeDetector_step(envelopeVec4, magnitudeVec4, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred4  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore4 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred4, pStore4, envelopeVec4);

         vec magnitudeVec5 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec5 =
             AUDIOLIB_envelopeDetector_step(envelopeVec5, magnitudeVec5, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred5  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore5 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred5, pStore5, envelopeVec5);

         vec magnitudeVec6 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec6 =
             AUDIOLIB_envelopeDetector_step(envelopeVec6, magnitudeVec6, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred6  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore6 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred6, pStore6, envelopeVec6);

         vec magnitudeVec7 = __abs(c7x::strm_eng<0, vec>::get_adv());
         envelopeVec7 =
             AUDIOLIB_envelopeDetector_step(envelopeVec7, magnitudeVec7, attackCoefficientVec, releaseCoefficientVec);
         __vpred vpred7  = c7x::strm_agen<0, vec>::get_vpred();
         vec    *pStore7 = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred7, pStore7, envelopeVec7);
      }

      pEnvelopeStates[firstChannel]      = (float) __get_vector_element(envelopeVec0, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 1U] = (float) __get_vector_element(envelopeVec1, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 2U] = (float) __get_vector_element(envelopeVec2, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 3U] = (float) __get_vector_element(envelopeVec3, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 4U] = (float) __get_vector_element(envelopeVec4, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 5U] = (float) __get_vector_element(envelopeVec5, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 6U] = (float) __get_vector_element(envelopeVec6, (uint8_t) 0);
      pEnvelopeStates[firstChannel + 7U] = (float) __get_vector_element(envelopeVec7, (uint8_t) 0);
   }

   /* One to eight leftover channels, as one more group of eight */
   vec envelopeVec0 = vec((dataType) pEnvelopeStates[leftover0]);
   vec envelopeVec1 = vec((dataType) pEnvelopeStates[leftover1]);
   vec envelopeVec2 = vec((dataType) pEnvelopeStates[leftover2]);
   vec envelopeVec3 = vec((dataType) pEnvelopeStates[leftover3]);
   vec envelopeVec4 = vec((dataType) pEnvelopeStates[leftover4]);
   vec envelopeVec5 = vec((dataType) pEnvelopeStates[leftover5]);
   vec envelopeVec6 = vec((dataType) pEnvelopeStates[leftover6]);
   vec envelopeVec7 = vec((dataType) pEnvelopeStates[leftover7]);

#pragma MUST_ITERATE(1, , 1)
   for (uint32_t sample = 0; sample < numSamples; sample++) {
      vec magnitudeVec0 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec0 =
          AUDIOLIB_envelopeDetector_step(envelopeVec0, magnitudeVec0, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred0  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore0 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred0, pStore0, envelopeVec0);

      vec magnitudeVec1 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec1 =
          AUDIOLIB_envelopeDetector_step(envelopeVec1, magnitudeVec1, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred1  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore1 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred1, pStore1, envelopeVec1);

      vec magnitudeVec2 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec2 =
          AUDIOLIB_envelopeDetector_step(envelopeVec2, magnitudeVec2, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred2  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore2 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred2, pStore2, envelopeVec2);

      vec magnitudeVec3 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec3 =
          AUDIOLIB_envelopeDetector_step(envelopeVec3, magnitudeVec3, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred3  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore3 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred3, pStore3, envelopeVec3);

      vec magnitudeVec4 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec4 =
          AUDIOLIB_envelopeDetector_step(envelopeVec4, magnitudeVec4, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred4  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore4 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred4, pStore4, envelopeVec4);

      vec magnitudeVec5 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec5 =
          AUDIOLIB_envelopeDetector_step(envelopeVec5, magnitudeVec5, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred5  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore5 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred5, pStore5, envelopeVec5);

      vec magnitudeVec6 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec6 =
          AUDIOLIB_envelopeDetector_step(envelopeVec6, magnitudeVec6, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred6  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore6 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred6, pStore6, envelopeVec6);

      vec magnitudeVec7 = __abs(c7x::strm_eng<1, vec>::get_adv());
      envelopeVec7 =
          AUDIOLIB_envelopeDetector_step(envelopeVec7, magnitudeVec7, attackCoefficientVec, releaseCoefficientVec);
      __vpred vpred7  = c7x::strm_agen<1, vec>::get_vpred();
      vec    *pStore7 = c7x::strm_agen<1, vec>::get_adv(pLeftoverOutLocal);
      __vstore_pred(vpred7, pStore7, envelopeVec7);
   }

   /* Repeated rows store identical states */
   pEnvelopeStates[leftover0] = (float) __get_vector_element(envelopeVec0, (uint8_t) 0);
   pEnvelopeStates[leftover1] = (float) __get_vector_element(envelopeVec1, (uint8_t) 0);
   pEnvelopeStates[leftover2] = (float) __get_vector_element(envelopeVec2, (uint8_t) 0);
   pEnvelopeStates[leftover3] = (float) __get_vector_element(envelopeVec3, (uint8_t) 0);
   pEnvelopeStates[leftover4] = (float) __get_vector_element(envelopeVec4, (uint8_t) 0);
   pEnvelopeStates[leftover5] = (float) __get_vector_element(envelopeVec5, (uint8_t) 0);
   pEnvelopeStates[leftover6] = (float) __get_vector_element(envelopeVec6, (uint8_t) 0);
   pEnvelopeStates[leftover7] = (float) __get_vector_element(envelopeVec7, (uint8_t) 0);

   __SE0_CLOSE();
   __SA0_CLOSE();
   __SE1_CLOSE();
   __SA1_CLOSE();

   return status;
}

/* explicit instantiation for the different data type versions */
template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group8_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group8_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @brief Modelled cycles of @ref AUDIOLIB_envelopeDetector_group4_exec_ci. */
uint64_t AUDIOLIB_envelopeDetector_group4Cycles(uint32_t dataType, uint32_t numSamples, uint32_t numChannels)
{
   uint64_t numChannelGroups = (uint64_t) (numChannels - 1U) / AUDIOLIB_ENVELOPEDETECTOR_GROUP4_CHANNELS;
   uint64_t numLoops         = numChannelGroups + 1U;
   uint64_t cycles           = 0U;

   if (dataType == AUDIOLIB_FLOAT64) {
      cycles = AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP4_CALL_CYCLES + AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP4_LEFTOVER_CYCLES +
               (numChannelGroups * AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP4_GROUP_CYCLES) +
               (numLoops * numSamples * AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP4_CYCLES_PER_SAMPLE);
   }
   else {
      cycles = AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP4_CALL_CYCLES + AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP4_LEFTOVER_CYCLES +
               (numChannelGroups * AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP4_GROUP_CYCLES) +
               (numLoops * numSamples * AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP4_CYCLES_PER_SAMPLE);
   }

   return cycles;
}

/** @brief Modelled cycles of @ref AUDIOLIB_envelopeDetector_group8_exec_ci. */
uint64_t AUDIOLIB_envelopeDetector_group8Cycles(uint32_t dataType, uint32_t numSamples, uint32_t numChannels)
{
   uint64_t numChannelGroups = (uint64_t) (numChannels - 1U) / AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS;
   uint64_t numLoops         = numChannelGroups + 1U;
   uint64_t cycles           = 0U;

   if (dataType == AUDIOLIB_FLOAT64) {
      cycles = AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP8_CALL_CYCLES + AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP8_LEFTOVER_CYCLES +
               (numChannelGroups * AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP8_GROUP_CYCLES) +
               (numLoops * numSamples * AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP8_CYCLES_PER_SAMPLE);
   }
   else {
      cycles = AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP8_CALL_CYCLES + AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP8_LEFTOVER_CYCLES +
               (numChannelGroups * AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP8_GROUP_CYCLES) +
               (numLoops * numSamples * AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP8_CYCLES_PER_SAMPLE);
   }

   return cycles;
}

void AUDIOLIB_envelopeDetector_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_envelopeDetector_PrivArgs *pKerPrivArgs = (AUDIOLIB_envelopeDetector_PrivArgs *) handle;
   uint32_t                            dataType     = (uint32_t) pKerPrivArgs->dataType;
   uint64_t                            cycles       = 0U;

   /* Model of the variant init selected */
   if (pKerPrivArgs->channelsPerGroup == AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS) {
      cycles = AUDIOLIB_envelopeDetector_group8Cycles(dataType, pKerPrivArgs->numSamples, pKerPrivArgs->numChannels);
   }
   else {
      cycles = AUDIOLIB_envelopeDetector_group4Cycles(dataType, pKerPrivArgs->numSamples, pKerPrivArgs->numChannels);
   }

   *estCycles  = cycles;
   *archCycles = cycles;
}
