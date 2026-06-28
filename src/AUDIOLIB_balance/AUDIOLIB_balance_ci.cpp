// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_balance_priv.h"

// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_balance_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_balance_InitArgs *pKerInitArgs)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_balance_init_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS            status            = AUDIOLIB_SUCCESS;
   AUDIOLIB_balance_PrivArgs *pKerPrivArgs      = (AUDIOLIB_balance_PrivArgs *) handle;
   uint8_t                   *pBlock            = pKerPrivArgs->bufPblock;
   uint32_t                   samples           = pKerPrivArgs->samples;
   uint32_t                   channels          = pKerPrivArgs->channels;
   uint32_t                   strideInElements  = pKerPrivArgs->strideInElements;
   uint32_t                   strideOutElements = pKerPrivArgs->strideOutElements;
   int32_t                    eleCount          = c7x::element_count_of<vec>::value;
   pKerPrivArgs->nVecs                          = AUDIOLIB_ceilingDiv(samples, eleCount);
   __SE_TEMPLATE_v1 se0Params                   = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params                   = __gen_SA_TEMPLATE_v1();
   __SE_VECLEN      SE_VECLEN                   = c7x::se_veclen<vec>::value;
   __SA_VECLEN      SA_VECLEN                   = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE     SE_ELETYPE                  = c7x::se_eletype<vec>::value;

#if AUDIOLIB_DEBUGPRINT
   printf("AUDIOLIB_DEBUGPRINT SE_VECLEN: %d, SA_VECLEN: %d, SE_ELETYPE: %d\n", SE_VECLEN, SA_VECLEN, SE_ELETYPE);
#endif

   if (channels == 1) {
      // /**********************************************************************/
      // /* Prepare streaming engine 0 to fetch input samples                        */
      // /**********************************************************************/
      se0Params.ICNT0   = samples;
      se0Params.ELETYPE = SE_ELETYPE;
      se0Params.VECLEN  = SE_VECLEN;
      se0Params.DIMFMT  = __SE_DIMFMT_1D;

      // /**********************************************************************/
      // /* Prepare SA template to store output                                */
      // /**********************************************************************/
      sa0Params.ICNT0  = samples;
      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;
   }
   else {
      // /**********************************************************************/
      // /* Prepare streaming engine 0 to fetch input samples                        */
      // /**********************************************************************/
      se0Params.ICNT0   = eleCount;
      se0Params.ICNT1   = channels;
      se0Params.DIM1    = strideInElements;
      se0Params.DIM2    = eleCount;
      se0Params.ICNT2   = pKerPrivArgs->nVecs;
      se0Params.ELETYPE = SE_ELETYPE;
      se0Params.VECLEN  = SE_VECLEN;
      se0Params.DIMFMT  = __SE_DIMFMT_3D;

      // /**********************************************************************/
      // /* Prepare SA template to store output                                */
      // /**********************************************************************/
      sa0Params.ICNT0  = eleCount;
      sa0Params.ICNT1  = channels;
      sa0Params.DIM1   = strideOutElements;
      sa0Params.DIM2   = eleCount;
      sa0Params.ICNT2  = pKerPrivArgs->nVecs;
      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.DIMFMT = __SA_DIMFMT_3D;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_balance_init_ci<float>(AUDIOLIB_kernelHandle            handle,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                                         const AUDIOLIB_balance_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_balance_exec_ci(AUDIOLIB_kernelHandle handle,
                                         void *restrict pInL,
                                         void *restrict pInR,
                                         void *restrict pOutL,
                                         void *restrict pOutR)
{

#if AUDIOLIB_DEBUGPRINT
   printf("Enter AUDIOLIB_balance_exec_ci\n");
#endif

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_balance_PrivArgs *pKerPrivArgs = (AUDIOLIB_balance_PrivArgs *) handle;
   uint8_t                   *pBlock       = pKerPrivArgs->bufPblock;
   dataType *restrict pInLocalLeft         = (dataType *) pInL;
   dataType *restrict pInLocalRight        = (dataType *) pInR;
   dataType *restrict pOutLocalLeft        = (dataType *) pOutL;
   dataType *restrict pOutLocalRight       = (dataType *) pOutR;
   uint32_t channels                       = pKerPrivArgs->channels;
   float    targetGainL                    = pKerPrivArgs->targetGainL;
   float    targetGainR                    = pKerPrivArgs->targetGainR;
   int32_t  nVecs                          = pKerPrivArgs->nVecs;

   pKerPrivArgs->currentGainL = pKerPrivArgs->smoothingCoefficient * pKerPrivArgs->currentGainL +
                                pKerPrivArgs->targetGainL * (1 - pKerPrivArgs->smoothingCoefficient);

   pKerPrivArgs->currentGainR = pKerPrivArgs->smoothingCoefficient * pKerPrivArgs->currentGainR +
                                pKerPrivArgs->targetGainR * (1 - pKerPrivArgs->smoothingCoefficient);

   int32_t bypassSmoothing = pKerPrivArgs->bypassSmoothing ||
                             (fabsf(pKerPrivArgs->currentGainL - targetGainL) < (AUDIOLIB_BALANCE__GAIN_THRESHOLD) &&
                              fabsf(pKerPrivArgs->currentGainR - targetGainR) < (AUDIOLIB_BALANCE__GAIN_THRESHOLD));

   vec vecTargetGainL = (vec) (targetGainL);
   vec vecTargetGainR = (vec) (targetGainR);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   vec inputL, inputR, outputL, outputR;

   if (!bypassSmoothing) // When smoothing is enabled
   {
      // getting the Alpha update value to update the coefficient series
      float  vecCoeffMultiplier = pKerPrivArgs->alphaMultiplier;
      float *alphaPtr           = pKerPrivArgs->alphaCoeff;

      // Convert the scalar array to vector pointer
      vec *vecAlphaPtr    = stov_ptr(vec, (float *) &alphaPtr[0]);
      vec  vecCoefficient = *vecAlphaPtr;

      // duplicate initial current gains and target gains to a vector
      vec vecInitGainL = (vec) (pKerPrivArgs->currentGainL);
      vec vecInitGainR = (vec) (pKerPrivArgs->currentGainR);

      vec vecCurrentGainL, vecCurrentGainR;

      __SE0_OPEN(pInLocalLeft, se0Params);
      __SE1_OPEN(pInLocalRight, se0Params);
      __SA0_OPEN(sa0Params);
      __SA1_OPEN(sa0Params);
      if (channels == 1) {
         for (int i = 0; i < nVecs; i++) // est cyc = 12 + trip_cnt * 4
         {
            // use vecCoefficient to compute currentgains
            vecCurrentGainL = vecCoefficient * vecInitGainL + vecTargetGainL * (1 - vecCoefficient);
            vecCurrentGainR = vecCoefficient * vecInitGainR + vecTargetGainR * (1 - vecCoefficient);
            vecCoefficient  = vecCoefficient * vecCoeffMultiplier;

            // fetch input samples for left channel using SE0
            inputL = c7x::strm_eng<0, vec>::get_adv();
            inputR = c7x::strm_eng<1, vec>::get_adv();

            // Apply gains to inputL
            outputL = inputL * vecCurrentGainL;
            outputR = inputR * vecCurrentGainR;

            // store outputL to output buffer pOutLocalLeft
            __vpred tmpL    = c7x::strm_agen<0, vec>::get_vpred();
            vec    *outPtrL = c7x::strm_agen<0, vec>::get_adv(pOutLocalLeft);
            __vstore_pred(tmpL, outPtrL, outputL);
            // store outputR to output buffer pOutLocalRight
            __vpred tmpR    = c7x::strm_agen<1, vec>::get_vpred();
            vec    *outPtrR = c7x::strm_agen<1, vec>::get_adv(pOutLocalRight);
            __vstore_pred(tmpR, outPtrR, outputR);
         }
      }

      else if (channels == 2) {
         for (int i = 0; i < nVecs; i++) // est cyc = 14 + trip_cnt * 5
         {
            // use vecCoefficient to compute currentgains
            vecCurrentGainL = vecCoefficient * vecInitGainL + vecTargetGainL * (1 - vecCoefficient);
            vecCurrentGainR = vecCoefficient * vecInitGainR + vecTargetGainR * (1 - vecCoefficient);
            vecCoefficient  = vecCoefficient * vecCoeffMultiplier;

            // Apply the gains to all the channels
            for (size_t j = 0; j < 2; j++) {
               // fetch input samples for left channel using SE0
               inputL = c7x::strm_eng<0, vec>::get_adv();

               // fetch input samples for right channel using SE1
               inputR = c7x::strm_eng<1, vec>::get_adv();

               // Apply gains to inputL and inputR
               outputL = inputL * vecCurrentGainL;
               outputR = inputR * vecCurrentGainR;

               // store outputL to output buffer pOutLocalLeft
               __vpred tmpL    = c7x::strm_agen<0, vec>::get_vpred();
               vec    *outPtrL = c7x::strm_agen<0, vec>::get_adv(pOutLocalLeft);
               __vstore_pred(tmpL, outPtrL, outputL);

               // store outputR to output buffer pOutLocalRight
               __vpred tmpR    = c7x::strm_agen<1, vec>::get_vpred();
               vec    *outPtrR = c7x::strm_agen<1, vec>::get_adv(pOutLocalRight);
               __vstore_pred(tmpR, outPtrR, outputR);
            }
         }
      }
      else if (channels == 4) {
         __vpred tmpL, tmpR;
         vec    *outPtrL, *outPtrR;
         for (int i = 0; i < nVecs; i++) // est cyc = 15 + trip_cnt * 8
         {
            // use vecCoefficient to compute currentgains
            vecCurrentGainL = vecCoefficient * vecInitGainL + vecTargetGainL * (1 - vecCoefficient);
            vecCurrentGainR = vecCoefficient * vecInitGainR + vecTargetGainR * (1 - vecCoefficient);
            vecCoefficient  = vecCoefficient * vecCoeffMultiplier;

            // fetch input samples for left channel using SE0
            inputL = c7x::strm_eng<0, vec>::get_adv();

            // fetch input samples for right channel using SE1
            inputR = c7x::strm_eng<1, vec>::get_adv();

            // Apply gains to inputL and inputR
            outputL = inputL * vecCurrentGainL;
            outputR = inputR * vecCurrentGainR;

            // store outputL to output buffer pOutLocalLeft
            tmpL    = c7x::strm_agen<0, vec>::get_vpred();
            outPtrL = c7x::strm_agen<0, vec>::get_adv(pOutLocalLeft);
            __vstore_pred(tmpL, outPtrL, outputL);

            // store outputR to output buffer pOutLocalRight
            tmpR    = c7x::strm_agen<1, vec>::get_vpred();
            outPtrR = c7x::strm_agen<1, vec>::get_adv(pOutLocalRight);
            __vstore_pred(tmpR, outPtrR, outputR);

            inputL = c7x::strm_eng<0, vec>::get_adv();

            inputR = c7x::strm_eng<1, vec>::get_adv();

            outputL = inputL * vecCurrentGainL;
            outputR = inputR * vecCurrentGainR;

            tmpL    = c7x::strm_agen<0, vec>::get_vpred();
            outPtrL = c7x::strm_agen<0, vec>::get_adv(pOutLocalLeft);
            __vstore_pred(tmpL, outPtrL, outputL);

            tmpR    = c7x::strm_agen<1, vec>::get_vpred();
            outPtrR = c7x::strm_agen<1, vec>::get_adv(pOutLocalRight);
            __vstore_pred(tmpR, outPtrR, outputR);

            inputL = c7x::strm_eng<0, vec>::get_adv();

            inputR = c7x::strm_eng<1, vec>::get_adv();

            outputL = inputL * vecCurrentGainL;
            outputR = inputR * vecCurrentGainR;

            tmpL    = c7x::strm_agen<0, vec>::get_vpred();
            outPtrL = c7x::strm_agen<0, vec>::get_adv(pOutLocalLeft);
            __vstore_pred(tmpL, outPtrL, outputL);

            tmpR    = c7x::strm_agen<1, vec>::get_vpred();
            outPtrR = c7x::strm_agen<1, vec>::get_adv(pOutLocalRight);
            __vstore_pred(tmpR, outPtrR, outputR);

            inputL = c7x::strm_eng<0, vec>::get_adv();

            inputR = c7x::strm_eng<1, vec>::get_adv();

            outputL = inputL * vecCurrentGainL;
            outputR = inputR * vecCurrentGainR;

            tmpL    = c7x::strm_agen<0, vec>::get_vpred();
            outPtrL = c7x::strm_agen<0, vec>::get_adv(pOutLocalLeft);
            __vstore_pred(tmpL, outPtrL, outputL);

            tmpR    = c7x::strm_agen<1, vec>::get_vpred();
            outPtrR = c7x::strm_agen<1, vec>::get_adv(pOutLocalRight);
            __vstore_pred(tmpR, outPtrR, outputR);
         }
      }
      else {
         for (int32_t i = 0; i < nVecs; i++) {
            // use vecCoefficient to compute currentgains (5 cycles)
            vecCurrentGainL = vecCoefficient * vecInitGainL + vecTargetGainL * (1 - vecCoefficient);
            vecCurrentGainR = vecCoefficient * vecInitGainR + vecTargetGainR * (1 - vecCoefficient);
            vecCoefficient  = vecCoefficient * vecCoeffMultiplier;

            // Apply the gains to all the channels
            for (size_t j = 0; j < channels; j++) // est cyc = 4 + channels * 2
            {
               // fetch input samples for left channel using SE0
               inputL = c7x::strm_eng<0, vec>::get_adv();

               // fetch input samples for right channel using SE1
               inputR = c7x::strm_eng<1, vec>::get_adv();

               // Apply gains to inputL and inputR
               outputL = inputL * vecCurrentGainL;
               outputR = inputR * vecCurrentGainR;

               // store outputL to output buffer pOutLocalLeft
               __vpred tmpL    = c7x::strm_agen<0, vec>::get_vpred();
               vec    *outPtrL = c7x::strm_agen<0, vec>::get_adv(pOutLocalLeft);
               __vstore_pred(tmpL, outPtrL, outputL);

               // store outputR to output buffer pOutLocalRight
               __vpred tmpR    = c7x::strm_agen<1, vec>::get_vpred();
               vec    *outPtrR = c7x::strm_agen<1, vec>::get_adv(pOutLocalRight);
               __vstore_pred(tmpR, outPtrR, outputR);
            }
         }
      }
      __SE0_CLOSE();
      __SE1_CLOSE();
      __SA0_CLOSE();
      __SA1_CLOSE();

      pKerPrivArgs->currentGainL = __get_vector_element(vecCurrentGainL, 7);
      pKerPrivArgs->currentGainR = __get_vector_element(vecCurrentGainR, 7);
   }
   else // when smoothing is disabled or when current gain reaches target gain
   {
      __SE0_OPEN(pInLocalLeft, se0Params);
      __SE1_OPEN(pInLocalRight, se0Params);
      __SA0_OPEN(sa0Params);
      __SA1_OPEN(sa0Params);
      // Apply the gains to all the channels
      for (size_t j = 0; j < nVecs * channels; j++) {
         // fetch input samples for left channel using SE0
         inputL = c7x::strm_eng<0, vec>::get_adv();

         // fetch input samples for right channel using SE1
         inputR = c7x::strm_eng<1, vec>::get_adv();

         // Apply gains to inputL and inputR
         outputL = inputL * vecTargetGainL;
         outputR = inputR * vecTargetGainR;

         // store outputL to output buffer pOutLocalLeft
         __vpred tmpL    = c7x::strm_agen<0, vec>::get_vpred();
         vec    *outPtrL = c7x::strm_agen<0, vec>::get_adv(pOutLocalLeft);
         __vstore_pred(tmpL, outPtrL, outputL);

         // store outputR to output buffer pOutLocalRight
         __vpred tmpR    = c7x::strm_agen<1, vec>::get_vpred();
         vec    *outPtrR = c7x::strm_agen<1, vec>::get_adv(pOutLocalRight);
         __vstore_pred(tmpR, outPtrR, outputR);
      }
      __SE0_CLOSE();
      __SE1_CLOSE();
      __SA0_CLOSE();
      __SA1_CLOSE();
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_balance_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pInL,
                                                         void *restrict pInR,
                                                         void *restrict pOutL,
                                                         void *restrict pOutR);

void AUDIOLIB_balance_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   typedef typename c7x::make_full_vector<c7x::float_vec>::type vec;
   AUDIOLIB_balance_PrivArgs                                   *pKerPrivArgs = (AUDIOLIB_balance_PrivArgs *) handle;
   uint32_t                                                     samples      = pKerPrivArgs->samples;
   uint32_t                                                     channels     = pKerPrivArgs->channels;
   int32_t                                                      eleCount     = c7x::element_count_of<vec>::value;
   int32_t                                                      nVecs        = AUDIOLIB_ceilingDiv(samples, eleCount);
   int32_t                                                      bypassSmoothing        = pKerPrivArgs->bypassSmoothing;
   uint32_t                                                     balanceStartupCycles   = 18 + 17 + 10;
   uint32_t                                                     balanceOperationCycles = 0;
   uint32_t                                                     balanceTearDownCycles  = 0;
   uint32_t                                                     balanceOverheadCycles  = 0;
   if (!bypassSmoothing) {
      if (channels == 1) {
         balanceStartupCycles += 12; // SE/SA Open
         balanceOperationCycles = 12 + nVecs * 4;
         balanceTearDownCycles  = 10; // SE/SA Close
      }
      else if (channels == 2) {
         balanceStartupCycles += 12; // SE/SA Open
         balanceOperationCycles = 14 + (nVecs * 5);
         balanceTearDownCycles  = 10; // SE/SA Close
      }
      else if (channels == 4) {
         balanceStartupCycles += 12; // SE/SA Open
         balanceOperationCycles = 15 + (nVecs * 8);
         balanceTearDownCycles  = 10; // SE/SA Close
      }
      else {
         balanceStartupCycles += 12; // SE/SA Open
         balanceOperationCycles = (16 + (4 + channels * 2)) * nVecs;
         balanceTearDownCycles  = 10; // SE/SA Close
      }
   }
   else {
      balanceStartupCycles += 17; // SE/SA Open
      balanceOperationCycles = 4 + (channels * nVecs) * 2;
      balanceTearDownCycles  = 8; // SE/SA Close
   }
   *archCycles += balanceStartupCycles + balanceOperationCycles + balanceTearDownCycles;
   *estCycles += balanceStartupCycles + balanceOperationCycles + balanceOverheadCycles + balanceTearDownCycles;
}
