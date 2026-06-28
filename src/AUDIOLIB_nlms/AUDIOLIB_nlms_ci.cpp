// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_nlms_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE2_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE3_PARAM_OFFSET (SE_SE2_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE4_PARAM_OFFSET (SE_SE3_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE5_PARAM_OFFSET (SE_SE4_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE6_PARAM_OFFSET (SE_SE5_PARAM_OFFSET + SE_PARAM_SIZE)

#define SE_SA0_PARAM_OFFSET (SE_SE6_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA2_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA3_PARAM_OFFSET (SE_SA2_PARAM_OFFSET + SA_PARAM_SIZE)

void AUDIOLIB_nlms_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_nlms_PrivArgs *pKerPrivArgs = (AUDIOLIB_nlms_PrivArgs *) handle;
   const uint32_t          numSamples   = pKerPrivArgs->totalSamples;
   const uint32_t          numChannels  = pKerPrivArgs->dim_y;

   uint64_t nlmsStartupCycles   = 0;
   uint64_t nlmsTeardownCycles  = 0;
   uint64_t nlmsOperationCycles = 0;
   uint64_t nlmsOverheadCycles  = 0;

   uint32_t vecLen = 8;

   const uint32_t writeIdx            = pKerPrivArgs->writeIdx;
   const uint32_t circBuffSize        = pKerPrivArgs->circBuffSize;
   const uint32_t nTilesSampleLength  = pKerPrivArgs->nTilesSampleLength;
   const uint32_t nTilesFilterLength  = pKerPrivArgs->nTilesFilterLength;
   const uint32_t nBlocksFilterLength = pKerPrivArgs->nBlocksFilterLength;
   const uint32_t nBlocksSampleLength = pKerPrivArgs->nBlocksSampleLength;

   uint32_t loopWriteCnt1 = (circBuffSize - writeIdx) <= numSamples ? (circBuffSize - writeIdx) : numSamples;
   uint32_t loopWriteCnt2 = numSamples - loopWriteCnt1;
   uint32_t loopCnt       = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt1, vecLen);

   nlmsStartupCycles += 13;
   nlmsTeardownCycles += 3;
   nlmsOperationCycles += 1 + loopCnt;

   loopCnt = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt2, vecLen);
   if (loopCnt > 0) {
      nlmsStartupCycles += 13;
      nlmsTeardownCycles += 3;
      nlmsOperationCycles += 1 + loopCnt;
   }

   // FIR
   nlmsStartupCycles += 22;
   nlmsTeardownCycles += 7;
   nlmsOperationCycles += (9 + 7 + nBlocksFilterLength * 4 + 32 + 2) * nTilesSampleLength;

   // IIR
   nlmsStartupCycles += 22;
   nlmsTeardownCycles += 7;
   nlmsOperationCycles += (5 + 7 + nBlocksSampleLength * 4 + 11 + 2) * nTilesFilterLength;

   // inverse block copy 2d
   nlmsStartupCycles += 23;
   nlmsTeardownCycles += 16;
   nlmsOperationCycles += (3 + 3 + nTilesFilterLength * 1);

   *archCycles += nlmsStartupCycles + nlmsOperationCycles + nlmsTeardownCycles;
   *estCycles += nlmsStartupCycles + nlmsOperationCycles + nlmsOverheadCycles + nlmsTeardownCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_nlms_init_ci(AUDIOLIB_kernelHandle         handle,
                                      AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                      const AUDIOLIB_nlms_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS         status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_nlms_PrivArgs *pKerPrivArgs = (AUDIOLIB_nlms_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   const uint32_t numChannels         = pKerPrivArgs->dim_y;
   const uint32_t totalSamples        = pKerPrivArgs->totalSamples;
   const uint32_t filterLength        = pKerPrivArgs->filterLength;
   const int32_t  strideInElements    = pKerPrivArgs->strideInElements;
   const int32_t  strideOutElements   = pKerPrivArgs->strideOutElements;
   const int32_t  strideStateElements = pKerPrivArgs->strideStateElements;
   uint8_t       *pBlock              = pKerPrivArgs->bufPblock;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se4Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se5Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se6Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa3Params = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELEDUP  SE_ELEDUP  = c7x::se_eledup<dataType, vec>::value;

   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;
   se3Params.ELETYPE = SE_ELETYPE;
   se3Params.VECLEN  = SE_VECLEN;
   se4Params.ELETYPE = SE_ELETYPE;
   se4Params.VECLEN  = SE_VECLEN;
   se5Params.ELETYPE = SE_ELETYPE;
   se5Params.VECLEN  = SE_VECLEN;
   se6Params.ELETYPE = SE_ELETYPE;
   se6Params.VECLEN  = SE_VECLEN;

   sa0Params.VECLEN = SA_VECLEN;
   sa1Params.VECLEN = SA_VECLEN;
   sa2Params.VECLEN = SA_VECLEN;
   sa3Params.VECLEN = SA_VECLEN;

   // Parameters for data copying from Input buffer to state Buffer
   se0Params.ICNT0  = totalSamples;
   se0Params.ICNT1  = numChannels;
   se0Params.DIM1   = strideInElements;
   se0Params.DIMFMT = __SE_DIMFMT_2D;

   sa0Params.ICNT0  = totalSamples;
   sa0Params.ICNT1  = numChannels;
   sa0Params.DIM1   = strideStateElements;
   sa0Params.DIMFMT = __SA_DIMFMT_2D;

   uint32_t cbSize       = 512;
   uint32_t encCbSize    = 0;
   uint32_t totBlockSize = strideStateElements * AUDIOLIB_sizeof(bufParamsIn->data_type);

   while (totBlockSize > cbSize) {
      encCbSize++;
      cbSize *= 2;
   }

   // for FIR Convolution
   // for extracting the input samples
   se1Params.ICNT0  = eleCount;
   se1Params.ICNT1  = filterLength;
   se1Params.DIM1   = 1;
   se1Params.ICNT2  = AUDIOLIB_ceilingDiv(totalSamples, eleCount);
   se1Params.DIM2   = eleCount;
   se1Params.ICNT3  = numChannels;
   se1Params.DIM3   = strideStateElements;
   se1Params.CBK0   = encCbSize;
   se1Params.AM0    = __SE_AM_CIRC_CBK0;
   se1Params.AM1    = __SE_AM_CIRC_CBK0;
   se1Params.DIMFMT = __SE_DIMFMT_4D;

   // for extracting the coefficients
   se2Params.ELEDUP = SE_ELEDUP;
   se2Params.ICNT0  = filterLength;
   se2Params.ICNT1  = AUDIOLIB_ceilingDiv(totalSamples, eleCount);
   se2Params.DIM1   = 0;
   se2Params.ICNT2  = numChannels;
   se2Params.DIM2   = strideInElements;
   se2Params.DIR    = __SE_DIR_DEC;
   se2Params.DIMFMT = __SE_DIMFMT_3D;

   // for storing the output samples, fetching the desired samples
   sa1Params.ICNT0  = totalSamples;
   sa1Params.ICNT1  = numChannels;
   sa1Params.DIM1   = strideOutElements;
   sa1Params.DIMFMT = __SA_DIMFMT_2D;

   // for storing the step factor values
   sa3Params.ICNT0  = totalSamples;
   sa3Params.DIM1   = strideInElements;
   sa3Params.ICNT1  = numChannels;
   sa3Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->nTilesSampleLength  = AUDIOLIB_ceilingDiv(totalSamples, eleCount) * numChannels;
   pKerPrivArgs->nBlocksFilterLength = AUDIOLIB_ceilingDiv(filterLength, UNROLL_FACTOR);

   // for coefficient updation
   // for extracting the input samples from the circular state buffer
   se3Params.ICNT0  = filterLength < eleCount ? filterLength : eleCount;
   se3Params.DIM1   = 1;
   se3Params.ICNT1  = totalSamples;
   se3Params.ICNT2  = AUDIOLIB_ceilingDiv(filterLength, eleCount);
   se3Params.DIM2   = filterLength < eleCount ? filterLength : eleCount;
   se3Params.ICNT3  = numChannels;
   se3Params.DIM3   = strideStateElements;
   se3Params.CBK0   = encCbSize;
   se3Params.AM0    = __SE_AM_CIRC_CBK0;
   se3Params.AM1    = __SE_AM_CIRC_CBK0;
   se3Params.DIMFMT = __SE_DIMFMT_4D;

   // for extracting the step factor variables
   se4Params.ELEDUP = SE_ELEDUP;
   se4Params.ICNT0  = totalSamples;
   se4Params.ICNT1  = AUDIOLIB_ceilingDiv(filterLength, eleCount);
   se4Params.DIM1   = 0;
   se4Params.ICNT2  = numChannels;
   se4Params.DIM2   = strideInElements;
   se4Params.DIMFMT = __SE_DIMFMT_3D;

   // for copying the updated coeff in chAccum to coeff buffer
   // for extracting the accumulated coeff samples
   se5Params.ICNT0  = filterLength;
   se5Params.DIM1   = strideInElements;
   se5Params.ICNT1  = numChannels;
   se5Params.DIR    = __SE_DIR_DEC;
   se5Params.DIMFMT = __SE_DIMFMT_2D;

   // for extracting the coefficient samples
   se6Params.ICNT0  = filterLength;
   se6Params.DIM1   = strideInElements;
   se6Params.ICNT1  = numChannels;
   se6Params.DIMFMT = __SE_DIMFMT_2D;

   // for storing the updated coeff samples to the output buffer
   sa2Params.ICNT0  = filterLength;
   sa2Params.DIM1   = strideInElements;
   sa2Params.ICNT1  = numChannels;
   sa2Params.DIMFMT = __SA_DIMFMT_2D;

   pKerPrivArgs->nTilesFilterLength  = (AUDIOLIB_ceilingDiv(filterLength, eleCount)) * numChannels;
   pKerPrivArgs->nBlocksSampleLength = (AUDIOLIB_ceilingDiv(totalSamples, UNROLL_FACTOR));

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = se4Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET) = se5Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE6_PARAM_OFFSET) = se6Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET) = sa3Params;
   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_nlms_init_ci<float>(AUDIOLIB_kernelHandle         handle,
                                                      AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                                      AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                                      const AUDIOLIB_nlms_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_nlms_exec_ci(AUDIOLIB_kernelHandle handle,
                                      void *restrict pIn,
                                      void *restrict pInRef,
                                      void *restrict pStateBuffer,
                                      void *restrict pScratchBuffer,
                                      void *restrict pCoefficients,
                                      void *restrict pOut)
{

   AUDIOLIB_STATUS                                        status = AUDIOLIB_SUCCESS;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_nlms_PrivArgs *pKerPrivArgs = (AUDIOLIB_nlms_PrivArgs *) handle;
   uint32_t                eleCount     = c7x::element_count_of<vec>::value;
   uint8_t                *pBlock       = pKerPrivArgs->bufPblock;

   __SE_TEMPLATE_v1 se1Params    = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params    = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params    = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se4Params    = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se5Params    = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE5_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se6Params    = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE6_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params    = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params    = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa3Params    = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se0ParamCirc = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0ParamCirc = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   const float    stepSize            = pKerPrivArgs->stepSize;
   const uint32_t numSamples          = pKerPrivArgs->totalSamples;
   const uint32_t numChannels         = pKerPrivArgs->dim_y;
   const uint32_t filterLength        = pKerPrivArgs->filterLength;
   const float    regularization      = pKerPrivArgs->regularization;
   const int32_t  strideInElements    = pKerPrivArgs->strideInElements;
   const uint32_t nTilesSampleLength  = pKerPrivArgs->nTilesSampleLength;
   const uint32_t nTilesFilterLength  = pKerPrivArgs->nTilesFilterLength;
   const uint32_t nBlocksFilterLength = pKerPrivArgs->nBlocksFilterLength;
   const uint32_t nBlocksSampleLength = pKerPrivArgs->nBlocksSampleLength;

   dataType *restrict pInLocal           = (dataType *) pIn;
   dataType *restrict pOutLocal          = (dataType *) pOut;
   dataType *restrict pRef               = (dataType *) pInRef;
   dataType *restrict pState             = (dataType *) pStateBuffer;
   dataType *restrict pScratch           = (dataType *) pScratchBuffer;
   dataType *restrict chAccumLocal       = pScratch + numChannels * strideInElements;
   dataType *restrict pCoefficientsLocal = (dataType *) pCoefficients;
   uint32_t circBuffMask                 = pKerPrivArgs->circBuffMask;
   uint32_t circBuffSize                 = pKerPrivArgs->circBuffSize;

   uint32_t readIdx, writeIdx, loopWriteCnt1, loopWriteCnt2, loopReadCnt1, loopReadCnt2;
   uint32_t nVecs = 0;

   readIdx  = pKerPrivArgs->readIdx;
   writeIdx = pKerPrivArgs->writeIdx;

   loopWriteCnt1 = (circBuffSize - writeIdx) <= numSamples ? (circBuffSize - writeIdx) : numSamples;
   loopWriteCnt2 = numSamples - loopWriteCnt1;

   loopReadCnt1 = (circBuffSize - readIdx) <= numSamples ? (circBuffSize - readIdx) : numSamples;
   loopReadCnt2 = numSamples - loopReadCnt1;

   nVecs              = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt1, eleCount);
   se0ParamCirc.ICNT0 = loopWriteCnt1;
   sa0ParamCirc.ICNT0 = loopWriteCnt1;

   __SE0_OPEN(pInLocal, se0ParamCirc);
   __SA0_OPEN(sa0ParamCirc);
   for (uint32_t counter = 0; counter < nVecs; counter++) {
      vec     inVec      = c7x::strm_eng<0, vec>::get_adv();
      __vpred vpredState = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addrState  = c7x::strm_agen<0, vec>::get_adv(&pState[writeIdx]);
      __vstore_pred(vpredState, addrState, inVec);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   writeIdx = (writeIdx + loopWriteCnt1) & circBuffMask;

   nVecs              = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt2, eleCount);
   se0ParamCirc.ICNT0 = loopWriteCnt2;
   sa0ParamCirc.ICNT0 = loopWriteCnt2;

   if (loopWriteCnt2 > 0) {
      __SE0_OPEN(&pInLocal[loopWriteCnt1], se0ParamCirc);
      __SA0_OPEN(sa0ParamCirc);
      for (uint32_t counter = 0; counter < nVecs; counter++) {
         vec     inVec      = c7x::strm_eng<0, vec>::get_adv();
         __vpred vpredState = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addrState  = c7x::strm_agen<0, vec>::get_adv(pState);
         __vstore_pred(vpredState, addrState, inVec);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
      writeIdx = loopWriteCnt2;
   }

   __SE0_OPEN(&pState[readIdx + 1], se1Params);
   __SE1_OPEN(&pCoefficientsLocal[(filterLength)], se2Params);
   __SA0_OPEN(sa1Params);
   __SA1_OPEN(sa1Params);
   __SA2_OPEN(sa3Params);
   // FIR Convolution
   for (uint32_t j = 0; j < nTilesSampleLength; j++) {
      vec vOut     = (vec) (0);
      vec vOut1    = (vec) (0);
      vec vOut2    = (vec) (0);
      vec vOut3    = (vec) (0);
      vec vOut4    = (vec) (0);
      vec vEnergy  = (vec) (0);
      vec vEnergy1 = (vec) (0);
      vec vEnergy2 = (vec) (0);
      vec vEnergy3 = (vec) (0);
      vec vEnergy4 = (vec) (0);

      // 7 + trip_cnt * 4
      for (uint32_t k = 0; k < nBlocksFilterLength; k++) {
         vec vState1 = c7x::strm_eng<0, vec>::get_adv();
         vec vCoeff1 = c7x::strm_eng<1, vec>::get_adv();
         vec vState2 = c7x::strm_eng<0, vec>::get_adv();
         vec vCoeff2 = c7x::strm_eng<1, vec>::get_adv();
         vec vState3 = c7x::strm_eng<0, vec>::get_adv();
         vec vCoeff3 = c7x::strm_eng<1, vec>::get_adv();
         vec vState4 = c7x::strm_eng<0, vec>::get_adv();
         vec vCoeff4 = c7x::strm_eng<1, vec>::get_adv();

         vOut1 += vState1 * vCoeff1;
         vEnergy1 += vState1 * vState1;
         vOut2 += vState2 * vCoeff2;
         vEnergy2 += vState2 * vState2;
         vOut3 += vState3 * vCoeff3;
         vEnergy3 += vState3 * vState3;
         vOut4 += vState4 * vCoeff4;
         vEnergy4 += vState4 * vState4;
      }

      vOut             = vOut1 + vOut2 + vOut3 + vOut4;
      vEnergy          = vEnergy1 + vEnergy2 + vEnergy3 + vEnergy4;
      __vpred vpredOut = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addrOut  = c7x::strm_agen<0, vec>::get_adv(&pOutLocal[0]);
      __vstore_pred(vpredOut, addrOut, vOut);
      vpredOut          = c7x::strm_agen<1, vec>::get_vpred();
      vec *addrRef      = c7x::strm_agen<1, vec>::get_adv(&pRef[0]);
      vec  vRef         = __vload_pred(vpredOut, addrRef);
      vec  vErr         = vRef - vOut;
      vEnergy           = __max(vEnergy, (vec) 1e-6) + (vec) regularization;
      vec vRecipEnergy  = __recip(vEnergy);
      vec vNRCorrection = (vec) 2.0f - vEnergy * vRecipEnergy; // newton-raphson correction
      vRecipEnergy      = vRecipEnergy * vNRCorrection;
      vec vStepSize     = (vec) (2.0f * stepSize);
      vec vMu           = (vStepSize * vErr) * vRecipEnergy;
      vpredOut          = c7x::strm_agen<2, vec>::get_vpred();
      vec *addrScratch  = c7x::strm_agen<2, vec>::get_adv(&pScratch[0]);
      __vstore_pred(vpredOut, addrScratch, vMu);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();
   __SA2_CLOSE();

   __SE0_OPEN(&pState[readIdx + 1], se3Params);
   __SE1_OPEN(&pScratch[0], se4Params);
   __SA0_OPEN(sa2Params);
   for (uint32_t counter = 0; counter < nTilesFilterLength; counter++) {
      vec acc  = (vec) 0;
      vec acc1 = (vec) 0;
      vec acc2 = (vec) 0;
      vec acc3 = (vec) 0;
      vec acc4 = (vec) 0;

      // 7 + trip_cnt * 4
      for (uint32_t k = 0; k < nBlocksSampleLength; k++) {
         vec inVec          = c7x::strm_eng<0, vec>::get_adv();
         vec stepFactorVec  = c7x::strm_eng<1, vec>::get_adv();
         vec inVec2         = c7x::strm_eng<0, vec>::get_adv();
         vec stepFactorVec2 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec3         = c7x::strm_eng<0, vec>::get_adv();
         vec stepFactorVec3 = c7x::strm_eng<1, vec>::get_adv();
         vec inVec4         = c7x::strm_eng<0, vec>::get_adv();
         vec stepFactorVec4 = c7x::strm_eng<1, vec>::get_adv();
         acc1 += inVec * stepFactorVec;
         acc2 += inVec2 * stepFactorVec2;
         acc3 += inVec3 * stepFactorVec3;
         acc4 += inVec4 * stepFactorVec4;
      }

      acc                = acc1 + acc2 + acc3 + acc4;
      __vpred vpredAccum = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addrAccum  = c7x::strm_agen<0, vec>::get_adv(&chAccumLocal[0]);
      __vstore_pred(vpredAccum, addrAccum, acc);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   // copying the accum values in reverse to the coefficent buffer
   vec coeffVec = (vec) 0;
   __SE0_OPEN(pCoefficientsLocal, se6Params);
   __SE1_OPEN(&chAccumLocal[filterLength], se5Params);
   __SA0_OPEN(sa2Params);
   // 3 + trip_cnt * 1
   for (uint32_t counter = 0; counter < nTilesFilterLength; counter++) {
      vec accVec         = c7x::strm_eng<1, vec>::get_adv();
      coeffVec           = c7x::strm_eng<0, vec>::get_adv();
      coeffVec           = coeffVec + accVec;
      __vpred vpredCoeff = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addrCoeff  = c7x::strm_agen<0, vec>::get_adv(pCoefficientsLocal);
      __vstore_pred(vpredCoeff, addrCoeff, coeffVec);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   readIdx                = ((readIdx + numSamples) >= circBuffSize) ? loopReadCnt2 : (readIdx + numSamples);
   pKerPrivArgs->readIdx  = readIdx;
   pKerPrivArgs->writeIdx = writeIdx;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_nlms_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pInRef,
                                                      void *restrict pStateBuffer,
                                                      void *restrict pScratchBuffer,
                                                      void *restrict pCoefficients,
                                                      void *restrict pOut);
