// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_delay_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE2_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE3_PARAM_OFFSET (SE_SE2_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE4_PARAM_OFFSET (SE_SE3_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA0_PARAM_OFFSET (SE_SE4_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA2_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA3_PARAM_OFFSET (SE_SA2_PARAM_OFFSET + SA_PARAM_SIZE)

void AUDIOLIB_delay_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles, uint64_t dataType)
{
   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   uint64_t delayStartupCycles   = 0;
   uint64_t delayTeardownCycles  = 0;
   uint64_t delayOperationCycles = 0;
   uint64_t delayOverheadCycles  = 0;

   uint32_t mode        = pKerPrivArgs->mode;
   uint32_t interleave  = pKerPrivArgs->interleave;
   uint32_t numSamples  = pKerPrivArgs->numSamples;
   uint32_t numChannels = pKerPrivArgs->numChannels;
   uint32_t delaySize   = pKerPrivArgs->delaySize;

   uint32_t vecLen  = 0;
   uint32_t loopCnt = 0;

   if (dataType == AUDIOLIB_FLOAT32) {
      vecLen = 8;
   }
   else if (dataType == AUDIOLIB_FLOAT64) {
      vecLen = 4;
   }

   if (interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      if (mode == 0) {
         if (delaySize <= numSamples) {
            loopCnt = numChannels * AUDIOLIB_ceilingDiv(delaySize, vecLen);
            delayStartupCycles += 25 + 7;
            delayTeardownCycles += 3;
            delayOperationCycles += 1 + loopCnt * 2;

            loopCnt = numChannels * AUDIOLIB_ceilingDiv(numSamples - delaySize, vecLen);
            if (loopCnt > 0) {
               delayStartupCycles += 26;
               delayTeardownCycles += 3;
               delayOperationCycles += 1 + loopCnt;
            }
         }
         else {
            loopCnt = numChannels * AUDIOLIB_ceilingDiv(numSamples, vecLen);
            delayStartupCycles += 23 + 26;
            delayTeardownCycles += 3 + 3;
            delayOperationCycles += 2 + 2 * loopCnt;

            loopCnt = numChannels * AUDIOLIB_ceilingDiv(delaySize - numSamples, vecLen);
            delayStartupCycles += 18;
            delayTeardownCycles += 3;
            delayOperationCycles += 1 + loopCnt;
         }

         delayOverheadCycles += 50;
      }
      else if (mode == 1) {
         uint32_t writeIdx      = pKerPrivArgs->writeIdx;
         uint32_t delayBuffSize = pKerPrivArgs->delayBuffSize;

         uint32_t loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
         uint32_t loopWriteCnt2 = numSamples - loopWriteCnt1;

         loopCnt = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt1, vecLen);
         delayStartupCycles += 35;
         delayTeardownCycles += 3;
         delayOperationCycles += 1 + loopCnt;

         loopCnt = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt2, vecLen);
         if (loopCnt > 0) {
            delayStartupCycles += 25;
            delayTeardownCycles += 3;
            delayOperationCycles += 1 + loopCnt;
         }

         loopCnt = numChannels * AUDIOLIB_ceilingDiv(numSamples, vecLen);
         delayStartupCycles += 23;
         delayTeardownCycles += 6;
         delayOperationCycles += 1 + loopCnt;

         delayOverheadCycles += 50;
      }

      loopCnt = numSamples * AUDIOLIB_ceilingDiv(numChannels, vecLen);
      delayStartupCycles += 24;
      delayTeardownCycles += 6;
      delayOperationCycles += 1 + loopCnt;
   }
   else {
      if (mode == 0) {
         if (delaySize <= numSamples) {
            loopCnt = numChannels * AUDIOLIB_ceilingDiv(delaySize, vecLen);
            delayStartupCycles += 24;
            delayTeardownCycles += 3;
            delayOperationCycles += 1 + loopCnt * 2;

            loopCnt = numChannels * AUDIOLIB_ceilingDiv(numSamples - delaySize, vecLen);
            if (loopCnt > 0) {
               delayStartupCycles += 13;
               delayTeardownCycles += 3;
               delayOperationCycles += 1 + loopCnt;
            }
         }
         else {
            loopCnt = numChannels * AUDIOLIB_ceilingDiv(numSamples, vecLen);
            delayStartupCycles += 13 + 13;
            delayTeardownCycles += 3 + 3;
            delayOperationCycles += 2 + 2 * loopCnt;

            loopCnt = numChannels * AUDIOLIB_ceilingDiv(delaySize - numSamples, vecLen);
            delayStartupCycles += 13;
            delayTeardownCycles += 3;
            delayOperationCycles += 1 + loopCnt;
         }

         delayOverheadCycles += 50;
      }
      else if (mode == 1) {
         uint32_t writeIdx      = pKerPrivArgs->writeIdx;
         uint32_t delayBuffSize = pKerPrivArgs->delayBuffSize;

         uint32_t loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
         uint32_t loopWriteCnt2 = numSamples - loopWriteCnt1;

         loopCnt = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt1, vecLen);
         delayStartupCycles += 13;
         delayTeardownCycles += 3;
         delayOperationCycles += 1 + loopCnt;

         loopCnt = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt2, vecLen);
         if (loopCnt > 0) {
            delayStartupCycles += 13;
            delayTeardownCycles += 3;
            delayOperationCycles += 1 + loopCnt;
         }

         loopCnt = numChannels * AUDIOLIB_ceilingDiv(numSamples, vecLen);
         delayStartupCycles += 13;
         delayTeardownCycles += 3;
         delayOperationCycles += 1 + loopCnt;

         delayOverheadCycles += 50;
      }
   }

   delayOverheadCycles += delayStartupCycles + delayTeardownCycles;
   *estCycles  = delayOperationCycles + delayOverheadCycles;
   *archCycles = delayOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delay_interleave_init_ci(AUDIOLIB_kernelHandle          handle,
                                                  const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                  const AUDIOLIB_bufParams2D_t  *bufParamsDelay,
                                                  const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                  const AUDIOLIB_delay_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delay_interleave_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "numSamples: %d delaySize: %d numChannels: %d interleave: %d eleCount: %d\n",
                         pKerPrivArgs->numSamples, pKerPrivArgs->delaySize, pKerPrivArgs->numChannels,
                         pKerPrivArgs->interleave, eleCount);

   uint32_t mode                = pKerPrivArgs->mode;
   uint32_t delaySize           = pKerPrivArgs->delaySize;
   uint32_t numSamples          = pKerPrivArgs->numSamples;
   uint32_t numChannels         = pKerPrivArgs->numChannels;
   int32_t  strideInElements    = pKerPrivArgs->strideInElements;
   int32_t  strideOutElements   = pKerPrivArgs->strideOutElements;
   int32_t  strideDelayElements = pKerPrivArgs->strideDelayElements;
   uint8_t *pBlock              = pKerPrivArgs->bufPblock;

   __SE_TEMPLATE_v1 se0Params           = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params           = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params           = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se3Params           = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seParamDeinterleave = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params           = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params           = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params           = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saParamDeinterleave = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   se0Params.ELETYPE           = SE_ELETYPE;
   se0Params.VECLEN            = SE_VECLEN;
   se1Params.ELETYPE           = SE_ELETYPE;
   se1Params.VECLEN            = SE_VECLEN;
   se2Params.ELETYPE           = SE_ELETYPE;
   se2Params.VECLEN            = SE_VECLEN;
   se3Params.ELETYPE           = SE_ELETYPE;
   se3Params.VECLEN            = SE_VECLEN;
   seParamDeinterleave.ELETYPE = SE_ELETYPE;
   seParamDeinterleave.VECLEN  = SE_VECLEN;

   sa0Params.VECLEN           = SA_VECLEN;
   sa1Params.VECLEN           = SA_VECLEN;
   sa2Params.VECLEN           = SA_VECLEN;
   saParamDeinterleave.VECLEN = SA_VECLEN;

   if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
      se1Params.TRANSPOSE           = __SE_TRANSPOSE_32BIT;
      se2Params.TRANSPOSE           = __SE_TRANSPOSE_32BIT;
      seParamDeinterleave.TRANSPOSE = __SE_TRANSPOSE_32BIT;
   }
   else {
      se1Params.TRANSPOSE           = __SE_TRANSPOSE_64BIT;
      se2Params.TRANSPOSE           = __SE_TRANSPOSE_64BIT;
      seParamDeinterleave.TRANSPOSE = __SE_TRANSPOSE_64BIT;
   }

   seParamDeinterleave.ICNT0  = 1;
   seParamDeinterleave.ICNT1  = eleCount;
   seParamDeinterleave.DIM1   = numSamples;
   seParamDeinterleave.ICNT2  = AUDIOLIB_ceilingDiv(numChannels, eleCount);
   seParamDeinterleave.DIM2   = eleCount * numSamples;
   seParamDeinterleave.ICNT3  = numSamples;
   seParamDeinterleave.DIM3   = 1;
   seParamDeinterleave.DIMFMT = __SE_DIMFMT_4D;

   saParamDeinterleave.ICNT0  = numChannels;
   saParamDeinterleave.ICNT1  = numSamples;
   saParamDeinterleave.DIM1   = strideOutElements;
   saParamDeinterleave.DIMFMT = __SA_DIMFMT_2D;

   // Linear Delay Parameters
   if (mode == 0) {
      if (delaySize <= numSamples) {
         // Parameters for data copying from Delay buffer to Output Buffer
         pKerPrivArgs->nVecsDelaySize = numChannels * AUDIOLIB_ceilingDiv(delaySize, eleCount);

         se0Params.ICNT0  = delaySize;
         se0Params.ICNT1  = numChannels;
         se0Params.DIM1   = strideDelayElements;
         se0Params.DIMFMT = __SE_DIMFMT_2D;

         sa0Params.ICNT0  = delaySize;
         sa0Params.ICNT1  = numChannels;
         sa0Params.DIM1   = numSamples;
         sa0Params.DIMFMT = __SA_DIMFMT_2D;

         // Parameters for data copying from Input Buffer to Output Buffer
         pKerPrivArgs->nVecsDiff = numChannels * AUDIOLIB_ceilingDiv((numSamples - delaySize), eleCount);

         se1Params.ICNT0 = 1;
         se1Params.ICNT1 = (numSamples - delaySize) < eleCount ? (numSamples - delaySize) : eleCount;
         se1Params.DIM1  = strideInElements;
         se1Params.ICNT2 = AUDIOLIB_ceilingDiv((numSamples - delaySize), eleCount);
         se1Params.DIM2 =
             ((numSamples - delaySize) < eleCount ? (numSamples - delaySize) : eleCount) * strideInElements;
         se1Params.ICNT3  = numChannels;
         se1Params.DIM3   = 1;
         se1Params.DIMFMT = __SE_DIMFMT_4D;

         sa1Params.ICNT0  = numSamples - delaySize;
         sa1Params.ICNT1  = numChannels;
         sa1Params.DIM1   = numSamples;
         sa1Params.DIMFMT = __SA_DIMFMT_2D;

         // Parameters for data copying from Input Buffer to Delay Buffer
         se2Params.ICNT0  = 1;
         se2Params.ICNT1  = delaySize < eleCount ? delaySize : eleCount;
         se2Params.DIM1   = strideInElements;
         se2Params.ICNT2  = AUDIOLIB_ceilingDiv(delaySize, eleCount);
         se2Params.DIM2   = (delaySize < eleCount ? delaySize : eleCount) * strideInElements;
         se2Params.ICNT3  = numChannels;
         se2Params.DIM3   = 1;
         se2Params.DIMFMT = __SE_DIMFMT_4D;

         sa2Params.ICNT0  = delaySize;
         sa2Params.ICNT1  = numChannels;
         sa2Params.DIM1   = strideDelayElements;
         sa2Params.DIMFMT = __SA_DIMFMT_2D;
      }
      else {
         // Parameters for data copying from Delay buffer to Output Buffer
         pKerPrivArgs->nVecsSamples = numChannels * AUDIOLIB_ceilingDiv(numSamples, eleCount);

         se0Params.ICNT0  = numSamples;
         se0Params.ICNT1  = numChannels;
         se0Params.DIM1   = strideDelayElements;
         se0Params.DIMFMT = __SE_DIMFMT_2D;

         sa0Params.ICNT0  = numSamples;
         sa0Params.ICNT1  = numChannels;
         sa0Params.DIM1   = numSamples;
         sa0Params.DIMFMT = __SA_DIMFMT_2D;

         // Parameters for Left shift data in Delay Buffer
         pKerPrivArgs->nVecsDiff = numChannels * AUDIOLIB_ceilingDiv((delaySize - numSamples), eleCount);

         se3Params.ICNT0  = delaySize - numSamples;
         se3Params.ICNT1  = numChannels;
         se3Params.DIM1   = strideDelayElements;
         se3Params.DIMFMT = __SE_DIMFMT_2D;

         sa1Params.ICNT0  = delaySize - numSamples;
         sa1Params.ICNT1  = numChannels;
         sa1Params.DIM1   = strideDelayElements;
         sa1Params.DIMFMT = __SA_DIMFMT_2D;

         // Parameters for data copying from Input Buffer to Delay Buffer
         se2Params.ICNT0  = 1;
         se2Params.ICNT1  = numSamples < eleCount ? numSamples : eleCount;
         se2Params.DIM1   = strideInElements;
         se2Params.ICNT2  = AUDIOLIB_ceilingDiv((numSamples), eleCount);
         se2Params.DIM2   = (numSamples < eleCount ? numSamples : eleCount) * strideInElements;
         se2Params.ICNT3  = numChannels;
         se2Params.DIM3   = 1;
         se2Params.DIMFMT = __SE_DIMFMT_4D;

         sa2Params.ICNT0  = numSamples;
         sa2Params.ICNT1  = numChannels;
         sa2Params.DIM1   = strideDelayElements;
         sa2Params.DIMFMT = __SA_DIMFMT_2D;
      }
   }
   else if (mode == 1) {
      // Parameters for data copying from Input buffer to Circular delay Buffer
      pKerPrivArgs->nVecsSamples = numChannels * AUDIOLIB_ceilingDiv(numSamples, eleCount);

      se2Params.ICNT0  = 1;
      se2Params.ICNT1  = numSamples < eleCount ? numSamples : eleCount;
      se2Params.DIM1   = strideInElements;
      se2Params.ICNT2  = AUDIOLIB_ceilingDiv((numSamples), eleCount);
      se2Params.DIM2   = (numSamples < eleCount ? numSamples : eleCount) * strideInElements;
      se2Params.ICNT3  = numChannels;
      se2Params.DIM3   = 1;
      se2Params.DIMFMT = __SE_DIMFMT_4D;

      sa0Params.ICNT0  = numSamples;
      sa0Params.ICNT1  = numChannels;
      sa0Params.DIM1   = strideDelayElements;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      // Parameters for data copying from Delay buffer to Output Buffer
      uint32_t cbSize       = 512;
      uint32_t encCbSize    = 0;
      uint32_t totBlockSize = strideDelayElements * AUDIOLIB_sizeof(bufParamsDelay->data_type);

      while (totBlockSize > cbSize) {
         encCbSize++;
         cbSize *= 2;
      }

      // for delay buffer to outbuffer
      pKerPrivArgs->nVecsSamples = numChannels * AUDIOLIB_ceilingDiv(numSamples, eleCount);

      se0Params.ICNT0  = numSamples;
      se0Params.ICNT1  = numChannels;
      se0Params.DIM1   = strideDelayElements;
      se0Params.DIMFMT = __SE_DIMFMT_2D;
      se0Params.AM0    = __SE_AM_CIRC_CBK0;
      se0Params.CBK0   = encCbSize;

      sa1Params.ICNT0  = numSamples;
      sa1Params.ICNT1  = numChannels;
      sa1Params.DIM1   = numSamples;
      sa1Params.DIMFMT = __SA_DIMFMT_2D;
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = se3Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET) = seParamDeinterleave;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET) = saParamDeinterleave;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_delay_interleave_init_ci<float>(AUDIOLIB_kernelHandle          handle,
                                                                  const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                                  const AUDIOLIB_bufParams2D_t  *bufParamsDelay,
                                                                  const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                                  const AUDIOLIB_delay_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_delay_interleave_init_ci<double>(AUDIOLIB_kernelHandle          handle,
                                                                   const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                                   const AUDIOLIB_bufParams2D_t  *bufParamsDelay,
                                                                   const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                                   const AUDIOLIB_delay_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delay_init_ci(AUDIOLIB_kernelHandle          handle,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsDelay,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                       const AUDIOLIB_delay_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delay_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "numSamples: %d delaySize: %d numChannels: %d interleave: %d eleCount: %d\n",
                         pKerPrivArgs->numSamples, pKerPrivArgs->delaySize, pKerPrivArgs->numChannels,
                         pKerPrivArgs->interleave, eleCount);

   uint32_t mode                = pKerPrivArgs->mode;
   uint32_t delaySize           = pKerPrivArgs->delaySize;
   uint32_t numSamples          = pKerPrivArgs->numSamples;
   uint32_t numChannels         = pKerPrivArgs->numChannels;
   int32_t  strideInElements    = pKerPrivArgs->strideInElements;
   int32_t  strideOutElements   = pKerPrivArgs->strideOutElements;
   int32_t  strideDelayElements = pKerPrivArgs->strideDelayElements;
   uint8_t *pBlock              = pKerPrivArgs->bufPblock;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se2Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa2Params = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   se2Params.ELETYPE = SE_ELETYPE;
   se2Params.VECLEN  = SE_VECLEN;

   sa0Params.VECLEN = SA_VECLEN;
   sa1Params.VECLEN = SA_VECLEN;
   sa2Params.VECLEN = SA_VECLEN;

   // Linear Delay Parameters
   if (mode == 0) {
      if (delaySize <= numSamples) {
         // Parameters for data copying from Delay buffer to Output Buffer
         pKerPrivArgs->nVecsDelaySize = numChannels * AUDIOLIB_ceilingDiv(delaySize, eleCount);

         se0Params.ICNT0  = delaySize;
         se0Params.ICNT1  = numChannels;
         se0Params.DIM1   = strideDelayElements;
         se0Params.DIMFMT = __SE_DIMFMT_2D;

         sa0Params.ICNT0  = delaySize;
         sa0Params.ICNT1  = numChannels;
         sa0Params.DIM1   = strideOutElements;
         sa0Params.DIMFMT = __SA_DIMFMT_2D;

         // Parameters for data copying from Input buffer to Output Buffer
         pKerPrivArgs->nVecsDiff = numChannels * AUDIOLIB_ceilingDiv((numSamples - delaySize), eleCount);

         se1Params.ICNT0  = numSamples - delaySize;
         se1Params.ICNT1  = numChannels;
         se1Params.DIM1   = strideInElements;
         se1Params.DIMFMT = __SE_DIMFMT_2D;

         sa1Params.ICNT0  = numSamples - delaySize;
         sa1Params.ICNT1  = numChannels;
         sa1Params.DIM1   = strideOutElements;
         sa1Params.DIMFMT = __SA_DIMFMT_2D;

         // Parameters for data copying from Input buffer to Delay Buffer
         se2Params.ICNT0  = delaySize;
         se2Params.ICNT1  = numChannels;
         se2Params.DIM1   = strideInElements;
         se2Params.DIMFMT = __SE_DIMFMT_2D;

         sa2Params.ICNT0  = delaySize;
         sa2Params.ICNT1  = numChannels;
         sa2Params.DIM1   = strideDelayElements;
         sa2Params.DIMFMT = __SA_DIMFMT_2D;
      }
      else {
         // Parameters for data copying from Delay buffer to Output Buffer
         pKerPrivArgs->nVecsSamples = numChannels * AUDIOLIB_ceilingDiv(numSamples, eleCount);

         se0Params.ICNT0  = numSamples;
         se0Params.ICNT1  = numChannels;
         se0Params.DIM1   = strideDelayElements;
         se0Params.DIMFMT = __SE_DIMFMT_2D;

         sa0Params.ICNT0  = numSamples;
         sa0Params.ICNT1  = numChannels;
         sa0Params.DIM1   = strideOutElements;
         sa0Params.DIMFMT = __SA_DIMFMT_2D;

         // Parameters for left shifting the data in Delay Buffer
         pKerPrivArgs->nVecsDiff = numChannels * AUDIOLIB_ceilingDiv((delaySize - numSamples), eleCount);

         se1Params.ICNT0  = delaySize - numSamples;
         se1Params.ICNT1  = numChannels;
         se1Params.DIM1   = strideDelayElements;
         se1Params.DIMFMT = __SE_DIMFMT_2D;

         sa1Params.ICNT0  = delaySize - numSamples;
         sa1Params.ICNT1  = numChannels;
         sa1Params.DIM1   = strideDelayElements;
         sa1Params.DIMFMT = __SA_DIMFMT_2D;

         // Parameters for data copying from Input buffer to Delay Buffer
         se2Params.ICNT0  = numSamples;
         se2Params.ICNT1  = numChannels;
         se2Params.DIM1   = strideInElements;
         se2Params.DIMFMT = __SE_DIMFMT_2D;

         sa2Params.ICNT0  = numSamples;
         sa2Params.ICNT1  = numChannels;
         sa2Params.DIM1   = strideDelayElements;
         sa2Params.DIMFMT = __SA_DIMFMT_2D;
      }
   }
   // Circular Delay Parameters
   else if (mode == 1) {
      // Parameters for data copying from Input buffer to Delay Buffer
      se0Params.ICNT0  = numSamples;
      se0Params.ICNT1  = numChannels;
      se0Params.DIM1   = strideInElements;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = numSamples;
      sa0Params.ICNT1  = numChannels;
      sa0Params.DIM1   = strideDelayElements;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      uint32_t cbSize       = 512;
      uint32_t encCbSize    = 0;
      uint32_t totBlockSize = strideDelayElements * AUDIOLIB_sizeof(bufParamsDelay->data_type);

      while (totBlockSize > cbSize) {
         encCbSize++;
         cbSize *= 2;
      }

      // for delay buffer to outbuffer
      pKerPrivArgs->nVecsSamples = numChannels * AUDIOLIB_ceilingDiv(numSamples, eleCount);

      se1Params.ICNT0  = numSamples;
      se1Params.ICNT1  = numChannels;
      se1Params.DIM1   = strideDelayElements;
      se1Params.DIMFMT = __SE_DIMFMT_2D;
      se1Params.AM0    = __SE_AM_CIRC_CBK0;
      se1Params.CBK0   = encCbSize;

      sa1Params.ICNT0  = numSamples;
      sa1Params.ICNT1  = numChannels;
      sa1Params.DIM1   = strideOutElements;
      sa1Params.DIMFMT = __SA_DIMFMT_2D;
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = se2Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = sa2Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_delay_init_ci<float>(AUDIOLIB_kernelHandle          handle,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsDelay,
                                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                       const AUDIOLIB_delay_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_delay_init_ci<double>(AUDIOLIB_kernelHandle          handle,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsDelay,
                                                        const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                                        const AUDIOLIB_delay_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delay_interleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pDelay,
                                                  void *restrict pOut,
                                                  void *restrict pScratch)
{

   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delay_interleave_exec_ci \n");

   __SE_TEMPLATE_v1 se0Params           = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params           = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params           = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se3Params           = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SE_TEMPLATE_v1 seParamDeinterleave = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE4_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params           = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params           = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params           = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 saParamDeinterleave = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA3_PARAM_OFFSET);

   dataType *restrict pInLocal      = (dataType *) pIn;
   dataType *restrict pOutLocal     = (dataType *) pOut;
   dataType *restrict pDelayLocal   = (dataType *) pDelay;
   dataType *restrict pScratchLocal = (dataType *) pScratch;

   uint32_t mode             = pKerPrivArgs->mode;
   uint32_t delaySize        = pKerPrivArgs->delaySize;
   uint32_t numSamples       = pKerPrivArgs->numSamples;
   uint32_t numChannels      = pKerPrivArgs->numChannels;
   uint32_t strideInElements = pKerPrivArgs->strideInElements;

   int32_t nVecsSample    = pKerPrivArgs->nVecsSamples;
   int32_t nVecsDelaySize = pKerPrivArgs->nVecsDelaySize;
   int32_t nVecsDiff      = pKerPrivArgs->nVecsDiff;

   vec     inVec, delayVec;
   vec    *addrDelay, *addrOut;
   __vpred vpredDelay, vpredOut;

   // Linear Delay Implementation
   if (mode == 0) {
      if (delaySize <= numSamples) {

         // Copy data from pDelay buffer to pOut buffer and pIn buffer to pDelay buffer
         __SE0_OPEN(pDelayLocal, se0Params);
         __SE1_OPEN(pInLocal + strideInElements * (numSamples - delaySize), se2Params);
         __SA0_OPEN(sa0Params);
         __SA1_OPEN(sa2Params);
         for (int32_t counter = 0; counter < nVecsDelaySize; counter++) {
            inVec    = c7x::strm_eng<1, vec>::get_adv();
            delayVec = c7x::strm_eng<0, vec>::get_adv();

            vpredOut = c7x::strm_agen<0, vec>::get_vpred();
            addrOut  = c7x::strm_agen<0, vec>::get_adv(pScratchLocal);
            __vstore_pred(vpredOut, addrOut, delayVec);

            vpredDelay = c7x::strm_agen<1, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<1, vec>::get_adv(pDelayLocal);
            __vstore_pred(vpredDelay, addrDelay, inVec);
         }
         __SE0_CLOSE();
         __SE1_CLOSE();
         __SA0_CLOSE();
         __SA1_CLOSE();

         // Copy data from pIn buffer to pOut buffer
         if (nVecsDiff > 0) {
            __SE0_OPEN(pInLocal, se1Params);
            __SA0_OPEN(sa1Params);
            for (int32_t counter = 0; counter < nVecsDiff; counter++) {
               inVec = c7x::strm_eng<0, vec>::get_adv();

               vpredOut = c7x::strm_agen<0, vec>::get_vpred();
               addrOut  = c7x::strm_agen<0, vec>::get_adv(&pScratchLocal[delaySize]);
               __vstore_pred(vpredOut, addrOut, inVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();
         }
      }
      else {
         // Copy data from pDelay buffer to pOut buffer
         __SE0_OPEN(pDelayLocal, se0Params);
         __SA0_OPEN(sa0Params);
         for (int32_t counter = 0; counter < nVecsSample; counter++) {
            delayVec = c7x::strm_eng<0, vec>::get_adv();

            vpredOut = c7x::strm_agen<0, vec>::get_vpred();
            addrOut  = c7x::strm_agen<0, vec>::get_adv(pScratchLocal);
            __vstore_pred(vpredOut, addrOut, delayVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();

         // Left Shift the remaining elements within pDelay buffer
         __SE0_OPEN(pDelayLocal + numSamples, se3Params);
         __SA0_OPEN(sa1Params);
         for (int32_t counter = 0; counter < nVecsDiff; counter++) {
            delayVec = c7x::strm_eng<0, vec>::get_adv();

            vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<0, vec>::get_adv(pDelayLocal);
            __vstore_pred(vpredDelay, addrDelay, delayVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();

         // Copy data from pIn buffer to pDelay buffer
         __SE0_OPEN(pInLocal, se2Params);
         __SA0_OPEN(sa2Params);
         for (int32_t counter = 0; counter < nVecsSample; counter++) {
            inVec = c7x::strm_eng<0, vec>::get_adv();

            vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[delaySize - numSamples]);
            __vstore_pred(vpredDelay, addrDelay, inVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();
      }
   }
   else if (mode == 1) {
      // Circular Delay Implementation
      uint32_t readIdx       = pKerPrivArgs->readIdx;
      uint32_t writeIdx      = pKerPrivArgs->writeIdx;
      uint32_t delayBuffSize = pKerPrivArgs->delayBuffSize;

      uint32_t loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
      uint32_t loopWriteCnt2 = numSamples - loopWriteCnt1;

      uint32_t loopReadCnt1 = (delayBuffSize - readIdx) <= numSamples ? (delayBuffSize - readIdx) : numSamples;
      uint32_t loopReadCnt2 = numSamples - loopReadCnt1;
      int32_t  nVecs;

      nVecs           = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt1, eleCount);
      se2Params.ICNT1 = (loopWriteCnt1 < eleCount) ? loopWriteCnt1 : eleCount;
      se2Params.ICNT2 = AUDIOLIB_ceilingDiv(loopWriteCnt1, eleCount);
      se2Params.DIM2  = ((loopWriteCnt1 < eleCount) ? loopWriteCnt1 : eleCount) * strideInElements;
      sa0Params.ICNT0 = loopWriteCnt1;

      //  pIn buffer to pDelay buffer
      __SE0_OPEN(pInLocal, se2Params);
      __SA0_OPEN(sa0Params);
      for (int32_t counter = 0; counter < nVecs; counter++) {
         inVec      = c7x::strm_eng<0, vec>::get_adv();
         vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
         addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[writeIdx]);
         __vstore_pred(vpredDelay, addrDelay, inVec);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
      writeIdx = (writeIdx + loopWriteCnt1) % delayBuffSize;

      if (loopWriteCnt2 > 0) {

         se2Params.ICNT1 = (loopWriteCnt2 < eleCount) ? loopWriteCnt2 : eleCount;
         se2Params.ICNT2 = AUDIOLIB_ceilingDiv(loopWriteCnt2, eleCount);
         se2Params.DIM2  = ((loopWriteCnt2 < eleCount) ? loopWriteCnt2 : eleCount) * strideInElements;
         sa0Params.ICNT0 = loopWriteCnt2;

         nVecs = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt2, eleCount);
         __SE0_OPEN(pInLocal + strideInElements * loopWriteCnt1, se2Params);
         __SA0_OPEN(sa0Params);
         for (int32_t counter = 0; counter < nVecs; counter++) {
            inVec      = c7x::strm_eng<0, vec>::get_adv();
            vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<0, vec>::get_adv(pDelayLocal);
            __vstore_pred(vpredDelay, addrDelay, inVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();
         writeIdx = loopWriteCnt2;
      }

      //  pDelay buffer to pOut buffer
      nVecs = numChannels * AUDIOLIB_ceilingDiv(numSamples, eleCount);

      __SE0_OPEN(pDelayLocal + readIdx, se0Params);
      __SA0_OPEN(sa1Params);
      for (int32_t counter = 0; counter < nVecs; counter++) {
         delayVec = c7x::strm_eng<0, vec>::get_adv();
         vpredOut = c7x::strm_agen<0, vec>::get_vpred();
         addrOut  = c7x::strm_agen<0, vec>::get_adv(pScratchLocal);
         __vstore_pred(vpredOut, addrOut, delayVec);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
      readIdx = ((readIdx + numSamples) >= delayBuffSize) ? loopReadCnt2 : (readIdx + numSamples);

      pKerPrivArgs->readIdx  = readIdx;
      pKerPrivArgs->writeIdx = writeIdx;
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   // Interleave Output Buffer
   int32_t nVecs = numSamples * AUDIOLIB_ceilingDiv(numChannels, eleCount);
   __SE0_OPEN(pScratchLocal, seParamDeinterleave);
   __SA0_OPEN(saParamDeinterleave);
   for (int32_t counter = 0; counter < nVecs; counter++) {
      inVec = c7x::strm_eng<0, vec>::get_adv();

      vpredOut = c7x::strm_agen<0, vec>::get_vpred();
      addrOut  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpredOut, addrOut, inVec);
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_delay_interleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pDelay,
                                                                  void *restrict pOut,
                                                                  void *restrict pScratch);

template AUDIOLIB_STATUS AUDIOLIB_delay_interleave_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pDelay,
                                                                   void *restrict pOut,
                                                                   void *restrict pScratch);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delay_exec_ci(AUDIOLIB_kernelHandle handle,
                                       void *restrict pIn,
                                       void *restrict pDelay,
                                       void *restrict pOut,
                                       void *restrict pScratch)
{

   AUDIOLIB_STATUS          status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delay_PrivArgs *pKerPrivArgs = (AUDIOLIB_delay_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delay_exec_ci \n");

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se2Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa2Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);

   dataType *restrict pInLocal    = (dataType *) pIn;
   dataType *restrict pOutLocal   = (dataType *) pOut;
   dataType *restrict pDelayLocal = (dataType *) pDelay;

   uint32_t mode        = pKerPrivArgs->mode;
   uint32_t delaySize   = pKerPrivArgs->delaySize;
   uint32_t numSamples  = pKerPrivArgs->numSamples;
   uint32_t numChannels = pKerPrivArgs->numChannels;

   int32_t nVecsSample    = pKerPrivArgs->nVecsSamples;
   int32_t nVecsDelaySize = pKerPrivArgs->nVecsDelaySize;
   int32_t nVecsDiff      = pKerPrivArgs->nVecsDiff;

   vec     inVec, delayVec;
   vec    *addrDelay, *addrOut;
   __vpred vpredDelay, vpredOut;

   // Linear Delay Implementation
   if (mode == 0) {
      if (delaySize <= numSamples) {
         // Copy data from pDelay buffer to pOut buffer and pIn buffer to pDelay buffer
         __SE0_OPEN(pDelayLocal, se0Params);
         __SE1_OPEN(pInLocal + (numSamples - delaySize), se2Params);
         __SA0_OPEN(sa0Params);
         __SA1_OPEN(sa2Params);
         for (int32_t counter = 0; counter < nVecsDelaySize; counter++) {
            inVec    = c7x::strm_eng<1, vec>::get_adv();
            delayVec = c7x::strm_eng<0, vec>::get_adv();

            vpredOut = c7x::strm_agen<0, vec>::get_vpred();
            addrOut  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
            __vstore_pred(vpredOut, addrOut, delayVec);

            vpredDelay = c7x::strm_agen<1, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<1, vec>::get_adv(pDelayLocal);
            __vstore_pred(vpredDelay, addrDelay, inVec);
         }
         __SE0_CLOSE();
         __SE1_CLOSE();
         __SA0_CLOSE();
         __SA1_CLOSE();

         // Copy data from pIn buffer to pOut buffer
         if (nVecsDiff > 0) {
            __SE0_OPEN(pInLocal, se1Params);
            __SA0_OPEN(sa1Params);
            for (int32_t counter = 0; counter < nVecsDiff; counter++) {
               inVec = c7x::strm_eng<0, vec>::get_adv();

               vpredOut = c7x::strm_agen<0, vec>::get_vpred();
               addrOut  = c7x::strm_agen<0, vec>::get_adv(&pOutLocal[delaySize]);
               __vstore_pred(vpredOut, addrOut, inVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();
         }
      }
      else {

         // Copy data from pDelay buffer to pOut buffer
         __SE0_OPEN(pDelayLocal, se0Params);
         __SA0_OPEN(sa0Params);
         for (int32_t counter = 0; counter < nVecsSample; counter++) {
            delayVec = c7x::strm_eng<0, vec>::get_adv();

            vpredOut = c7x::strm_agen<0, vec>::get_vpred();
            addrOut  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
            __vstore_pred(vpredOut, addrOut, delayVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();

         // Left Shift the remaining elements within pDelay buffer
         __SE0_OPEN(pDelayLocal + numSamples, se1Params);
         __SA0_OPEN(sa1Params);
         for (int32_t counter = 0; counter < nVecsDiff; counter++) {
            delayVec = c7x::strm_eng<0, vec>::get_adv();

            vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<0, vec>::get_adv(pDelayLocal);
            __vstore_pred(vpredDelay, addrDelay, delayVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();

         // Copy data from pIn buffer to pDelay buffer
         __SE0_OPEN(pInLocal, se2Params);
         __SA0_OPEN(sa2Params);
         for (int32_t counter = 0; counter < nVecsSample; counter++) {
            inVec = c7x::strm_eng<0, vec>::get_adv();

            vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[delaySize - numSamples]);
            __vstore_pred(vpredDelay, addrDelay, inVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();
      }
   }
   else if (mode == 1) {
      // Circular Delay Implementation
      uint32_t readIdx       = pKerPrivArgs->readIdx;
      uint32_t writeIdx      = pKerPrivArgs->writeIdx;
      uint32_t delayBuffSize = pKerPrivArgs->delayBuffSize;

      uint32_t loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
      uint32_t loopWriteCnt2 = numSamples - loopWriteCnt1;

      uint32_t loopReadCnt1 = (delayBuffSize - readIdx) <= numSamples ? (delayBuffSize - readIdx) : numSamples;
      uint32_t loopReadCnt2 = numSamples - loopReadCnt1;
      int      nVecs;

      nVecs           = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt1, eleCount);
      se0Params.ICNT0 = loopWriteCnt1;
      sa0Params.ICNT0 = loopWriteCnt1;

      //  pIn buffer to pDelay buffer
      __SE0_OPEN(pInLocal, se0Params);
      __SA0_OPEN(sa0Params);
      for (int32_t counter = 0; counter < nVecs; counter++) {
         inVec      = c7x::strm_eng<0, vec>::get_adv();
         vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
         addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[writeIdx]);
         __vstore_pred(vpredDelay, addrDelay, inVec);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
      writeIdx = (writeIdx + loopWriteCnt1) % delayBuffSize;

      se0Params.ICNT0 = loopWriteCnt2;
      sa0Params.ICNT0 = loopWriteCnt2;

      nVecs = numChannels * AUDIOLIB_ceilingDiv(loopWriteCnt2, eleCount);
      if (loopWriteCnt2 > 0) {

         __SE0_OPEN(pInLocal + loopWriteCnt1, se0Params);
         __SA0_OPEN(sa0Params);
         for (int32_t counter = 0; counter < nVecs; counter++) {
            inVec      = c7x::strm_eng<0, vec>::get_adv();
            vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<0, vec>::get_adv(pDelayLocal);
            __vstore_pred(vpredDelay, addrDelay, inVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();
         writeIdx = loopWriteCnt2;
      }

      //  pDelay buffer to pOut buffer
      __SE0_OPEN(pDelayLocal + readIdx, se1Params);
      __SA0_OPEN(sa1Params);
      for (int32_t counter = 0; counter < nVecsSample; counter++) {
         delayVec = c7x::strm_eng<0, vec>::get_adv();
         vpredOut = c7x::strm_agen<0, vec>::get_vpred();
         addrOut  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpredOut, addrOut, delayVec);
      }
      __SE0_CLOSE();
      __SA0_CLOSE();
      readIdx = ((readIdx + numSamples) >= delayBuffSize) ? loopReadCnt2 : (readIdx + numSamples);

      pKerPrivArgs->readIdx  = readIdx;
      pKerPrivArgs->writeIdx = writeIdx;
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_delay_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                       void *restrict pIn,
                                                       void *restrict pDelay,
                                                       void *restrict pOut,
                                                       void *restrict pScratch);

template AUDIOLIB_STATUS AUDIOLIB_delay_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pDelay,
                                                        void *restrict pOut,
                                                        void *restrict pScratch);
