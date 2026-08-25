// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_delayNChannel_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE2_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE3_PARAM_OFFSET (SE_SE2_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA0_PARAM_OFFSET (SE_SE3_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA2_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + SA_PARAM_SIZE)

void AUDIOLIB_delayNChannel_perfEst(AUDIOLIB_kernelHandle handle,
                                    uint64_t             *archCycles,
                                    uint64_t             *estCycles,
                                    uint64_t              dataType)
{
   AUDIOLIB_delayNChannel_PrivArgs *pKerPrivArgs = (AUDIOLIB_delayNChannel_PrivArgs *) handle;

   uint64_t delayNChannelStartupCycles   = 0;
   uint64_t delayNChannelTeardownCycles  = 0;
   uint64_t delayNChannelOperationCycles = 0;
   uint64_t delayNChannelOverheadCycles  = 0;

   uint32_t mode        = pKerPrivArgs->mode;
   uint32_t interleave  = pKerPrivArgs->interleave;
   uint32_t numSamples  = pKerPrivArgs->numSamples;
   uint32_t numChannels = pKerPrivArgs->numChannels;

   uint32_t i;
   uint32_t chDelaySize;
   uint32_t vecLen  = 0;
   uint32_t loopCnt = 0;

   if (dataType == AUDIOLIB_FLOAT32) {
      vecLen = 8;
   }
   else {
      vecLen = 4;
   }

   if (interleave == AUDIOLIB_DATA_FORMAT_INTERLEAVED) {
      if (mode == 0) {
         for (i = 0; i < numChannels; i++) {
            chDelaySize = pKerPrivArgs->delaySize[i];

            if (chDelaySize <= numSamples) {
               loopCnt = AUDIOLIB_ceilingDiv(chDelaySize, vecLen);
               delayNChannelStartupCycles += 25 + 7;
               delayNChannelTeardownCycles += 3;
               delayNChannelOperationCycles += 1 + loopCnt * 2;

               loopCnt = AUDIOLIB_ceilingDiv(numSamples - chDelaySize, vecLen);
               if (loopCnt > 0) {
                  delayNChannelStartupCycles += 26;
                  delayNChannelTeardownCycles += 3;
                  delayNChannelOperationCycles += 1 + loopCnt;
               }
            }
            else {
               loopCnt = AUDIOLIB_ceilingDiv(numSamples, vecLen);
               delayNChannelStartupCycles += 23 + 26;
               delayNChannelTeardownCycles += 3 + 3;
               delayNChannelOperationCycles += 2 + 2 * loopCnt;

               loopCnt = AUDIOLIB_ceilingDiv(chDelaySize - numSamples, vecLen);
               delayNChannelStartupCycles += 18;
               delayNChannelTeardownCycles += 3;
               delayNChannelOperationCycles += 1 + loopCnt;
            }
         }

         delayNChannelOverheadCycles += 50;
      }
      else if (mode == 1) {
         uint32_t writeIdx;
         uint32_t loopWriteCnt1, loopWriteCnt2;

         uint32_t delayBuffSize = pKerPrivArgs->delayBuffSize;

         for (i = 0; i < numChannels; i++) {
            writeIdx = pKerPrivArgs->writeIdx[i];

            loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
            loopWriteCnt2 = numSamples - loopWriteCnt1;

            loopCnt = AUDIOLIB_ceilingDiv(loopWriteCnt1, vecLen);
            delayNChannelStartupCycles += 35;
            delayNChannelTeardownCycles += 3;
            delayNChannelOperationCycles += 1 + loopCnt;

            loopCnt = AUDIOLIB_ceilingDiv(loopWriteCnt2, vecLen);
            if (loopCnt > 0) {
               delayNChannelStartupCycles += 25;
               delayNChannelTeardownCycles += 3;
               delayNChannelOperationCycles += 1 + loopCnt;
            }

            loopCnt = AUDIOLIB_ceilingDiv(numSamples, vecLen);
            delayNChannelStartupCycles += 23;
            delayNChannelTeardownCycles += 3;
            delayNChannelOperationCycles += 1 + loopCnt;
         }

         delayNChannelOverheadCycles += 50;
      }

      loopCnt = numSamples * AUDIOLIB_ceilingDiv(numChannels, vecLen);
      delayNChannelStartupCycles += 24;
      delayNChannelTeardownCycles += 3;
      delayNChannelOperationCycles += 1 + loopCnt;
   }
   else {
      if (mode == 0) {
         for (i = 0; i < numChannels; i++) {
            chDelaySize = pKerPrivArgs->delaySize[i];

            if (chDelaySize <= numSamples) {
               loopCnt = AUDIOLIB_ceilingDiv(chDelaySize, vecLen);
               delayNChannelStartupCycles += 24;
               delayNChannelTeardownCycles += 3;
               delayNChannelOperationCycles += 1 + loopCnt * 2;

               loopCnt = AUDIOLIB_ceilingDiv(numSamples - chDelaySize, vecLen);
               if (loopCnt > 0) {
                  delayNChannelStartupCycles += 13;
                  delayNChannelTeardownCycles += 3;
                  delayNChannelOperationCycles += 1 + loopCnt;
               }
            }
            else {
               loopCnt = AUDIOLIB_ceilingDiv(numSamples, vecLen);
               delayNChannelStartupCycles += 13 + 13;
               delayNChannelTeardownCycles += 3 + 3;
               delayNChannelOperationCycles += 2 + 2 * loopCnt;

               loopCnt = AUDIOLIB_ceilingDiv(chDelaySize - numSamples, vecLen);
               delayNChannelStartupCycles += 13;
               delayNChannelTeardownCycles += 3;
               delayNChannelOperationCycles += 1 + loopCnt;
            }
         }

         delayNChannelOverheadCycles += 50;
      }
      else if (mode == 1) {
         uint32_t writeIdx;
         uint32_t loopWriteCnt1, loopWriteCnt2;

         uint32_t delayBuffSize = pKerPrivArgs->delayBuffSize;

         for (i = 0; i < numChannels; i++) {
            writeIdx = pKerPrivArgs->writeIdx[i];

            loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
            loopWriteCnt2 = numSamples - loopWriteCnt1;

            loopCnt = AUDIOLIB_ceilingDiv(loopWriteCnt1, vecLen);
            delayNChannelStartupCycles += 13;
            delayNChannelTeardownCycles += 3;
            delayNChannelOperationCycles += 1 + loopCnt;

            loopCnt = AUDIOLIB_ceilingDiv(loopWriteCnt2, vecLen);
            if (loopCnt > 0) {
               delayNChannelStartupCycles += 13;
               delayNChannelTeardownCycles += 3;
               delayNChannelOperationCycles += 1 + loopCnt;
            }

            loopCnt = AUDIOLIB_ceilingDiv(numSamples, vecLen);
            delayNChannelStartupCycles += 13;
            delayNChannelTeardownCycles += 3;
            delayNChannelOperationCycles += 1 + loopCnt;
         }

         delayNChannelOverheadCycles += 50;
      }
   }

   delayNChannelOverheadCycles += delayNChannelStartupCycles + delayNChannelTeardownCycles;
   *estCycles  = delayNChannelOperationCycles + delayNChannelOverheadCycles;
   *archCycles = delayNChannelOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delayNChannel_interleave_init_ci(AUDIOLIB_kernelHandle                  handle,
                                                          const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                                          const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                                          const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                                          const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                  status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delayNChannel_PrivArgs *pKerPrivArgs = (AUDIOLIB_delayNChannel_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delayNChannel_interleave_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "numSamples: %d numChannels: %d interleave: %d eleCount: %d\n", pKerPrivArgs->numSamples,
                         pKerPrivArgs->numChannels, pKerPrivArgs->interleave, eleCount);

   uint32_t mode                = pKerPrivArgs->mode;
   uint32_t numSamples          = pKerPrivArgs->numSamples;
   uint32_t numChannels         = pKerPrivArgs->numChannels;
   uint32_t strideInElements    = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements   = pKerPrivArgs->strideOutElements;
   uint32_t strideDelayElements = pKerPrivArgs->strideDelayElements;
   uint8_t *pBlock              = pKerPrivArgs->bufPblock;

   __SE_TEMPLATE_v1 seParamInterleave   = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seParamDelay        = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saParamDelay        = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seParamCircDelay    = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seParamDeinterleave = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saParamDeinterleave = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   seParamInterleave.ELETYPE   = SE_ELETYPE;
   seParamInterleave.VECLEN    = SE_VECLEN;
   seParamDelay.ELETYPE        = SE_ELETYPE;
   seParamDelay.VECLEN         = SE_VECLEN;
   seParamCircDelay.ELETYPE    = SE_ELETYPE;
   seParamCircDelay.VECLEN     = SE_VECLEN;
   seParamDeinterleave.ELETYPE = SE_ELETYPE;
   seParamDeinterleave.VECLEN  = SE_VECLEN;

   saParamDelay.VECLEN        = SA_VECLEN;
   saParamDeinterleave.VECLEN = SA_VECLEN;

   if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
      seParamInterleave.TRANSPOSE   = __SE_TRANSPOSE_32BIT;
      seParamDeinterleave.TRANSPOSE = __SE_TRANSPOSE_32BIT;
   }
   else {
      seParamInterleave.TRANSPOSE   = __SE_TRANSPOSE_64BIT;
      seParamDeinterleave.TRANSPOSE = __SE_TRANSPOSE_64BIT;
   }

   seParamInterleave.ICNT0  = 1;
   seParamInterleave.ICNT1  = eleCount;
   seParamInterleave.DIM1   = strideInElements;
   seParamInterleave.ICNT2  = AUDIOLIB_ceilingDiv(numSamples, eleCount);
   seParamInterleave.DIM2   = eleCount * strideInElements;
   seParamInterleave.DIMFMT = __SE_DIMFMT_3D;

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
      seParamDelay.DIMFMT = __SE_DIMFMT_1D;

      saParamDelay.DIMFMT = __SA_DIMFMT_1D;
   }
   // Circular Delay Parameters
   else if (mode == 1) {
      /* Determine Circular Delay Buffer size.
       * CB size = 2^(encoding + 9) bytes. */
      uint32_t cbSize       = 512;
      uint32_t encCbSize    = 0;
      uint32_t totBlockSize = strideDelayElements * AUDIOLIB_sizeof(bufParamsDelay->data_type);
      while (totBlockSize > cbSize) {
         encCbSize++;
         cbSize *= 2;
      }

      seParamCircDelay.DIMFMT = __SE_DIMFMT_1D;
      seParamCircDelay.CBK0   = encCbSize;
      seParamCircDelay.AM0    = __SE_AM_CIRC_CBK0;
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = seParamInterleave;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = seParamDelay;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = seParamCircDelay;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET) = seParamDeinterleave;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = saParamDelay;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET) = saParamDeinterleave;

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_delayNChannel_interleave_init_ci<float>(AUDIOLIB_kernelHandle                  handle,
                                                 const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                                 const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                                 const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                                 const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS
AUDIOLIB_delayNChannel_interleave_init_ci<double>(AUDIOLIB_kernelHandle                  handle,
                                                  const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                                  const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                                  const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                                  const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delayNChannel_init_ci(AUDIOLIB_kernelHandle                  handle,
                                               const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                               const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                               const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                               const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                  status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delayNChannel_PrivArgs *pKerPrivArgs = (AUDIOLIB_delayNChannel_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delayNChannel_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "numSamples: %d numChannels: %d interleave: %d eleCount: %d\n", pKerPrivArgs->numSamples,
                         pKerPrivArgs->numChannels, pKerPrivArgs->interleave, eleCount);

   uint32_t mode                = pKerPrivArgs->mode;
   uint32_t strideDelayElements = pKerPrivArgs->strideDelayElements;
   uint8_t *pBlock              = pKerPrivArgs->bufPblock;

   __SE_TEMPLATE_v1 seParamDelay     = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 saParamDelay     = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 seParamCircDelay = __gen_SE_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   seParamDelay.ELETYPE     = SE_ELETYPE;
   seParamDelay.VECLEN      = SE_VECLEN;
   seParamCircDelay.ELETYPE = SE_ELETYPE;
   seParamCircDelay.VECLEN  = SE_VECLEN;

   saParamDelay.VECLEN = SA_VECLEN;

   // Linear Delay Parameters
   if (mode == 0) {
      seParamDelay.DIMFMT = __SE_DIMFMT_1D;

      saParamDelay.DIMFMT = __SA_DIMFMT_1D;
   }
   // Circular Delay Parameters
   else if (mode == 1) {
      uint32_t cbSize       = 512;
      uint32_t encCbSize    = 0;
      uint32_t totBlockSize = strideDelayElements * AUDIOLIB_sizeof(bufParamsDelay->data_type);
      /* The limit of totBlockSizeB is checked in AUDIOLIB_asrc_init_checkParams() through maxSampleCountPerBlock. */
      while (totBlockSize > cbSize) {
         encCbSize++;
         cbSize *= 2;
      }
      seParamCircDelay.DIMFMT = __SE_DIMFMT_1D;
      seParamCircDelay.CBK0   = encCbSize;
      seParamCircDelay.AM0    = __SE_AM_CIRC_CBK0;
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = seParamDelay;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET) = seParamCircDelay;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = saParamDelay;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_delayNChannel_init_ci<float>(AUDIOLIB_kernelHandle                  handle,
                                                               const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                                               const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                                               const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                                               const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_delayNChannel_init_ci<double>(AUDIOLIB_kernelHandle                  handle,
                                                                const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                                                const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                                                const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                                                const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delayNChannel_interleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pDelay,
                                                          void *restrict pOut,
                                                          void *restrict pScratch)
{

   AUDIOLIB_STATUS                  status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delayNChannel_PrivArgs *pKerPrivArgs = (AUDIOLIB_delayNChannel_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delayNChannel_interleave_exec_ci \n");

   __SE_TEMPLATE_v1 seParamInterleave   = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 seParamDelay        = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 seParamCircDelay    = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SE_TEMPLATE_v1 seParamDeinterleave = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE3_PARAM_OFFSET);
   __SA_TEMPLATE_v1 saParamDelay        = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 saParamDeinterleave = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA2_PARAM_OFFSET);

   dataType *restrict pInLocal      = (dataType *) pIn;
   dataType *restrict pOutLocal     = (dataType *) pOut;
   dataType *restrict pDelayLocal   = (dataType *) pDelay;
   dataType *restrict pScratchLocal = (dataType *) pScratch;

   uint32_t mode                = pKerPrivArgs->mode;
   uint32_t numSamples          = pKerPrivArgs->numSamples;
   uint32_t numChannels         = pKerPrivArgs->numChannels;
   uint32_t strideInElements    = pKerPrivArgs->strideInElements;
   uint32_t strideDelayElements = pKerPrivArgs->strideDelayElements;

   uint32_t i;
   int32_t  nVecs = 0;
   uint32_t chDelaySize, chDelayDiff;

   vec     inVec, delayVec;
   vec    *addrDelay, *addrOut;
   __vpred vpredDelay, vpredOut;

   // Linear Delay Implementation
   if (mode == 0) {
      for (i = 0; i < numChannels; i++) {
         chDelaySize = pKerPrivArgs->delaySize[i];

         if (chDelaySize <= numSamples) {
            // Copy data from pDelay buffer to pOut buffer and pIn buffer to pDelay buffer
            nVecs                   = AUDIOLIB_ceilingDiv(chDelaySize, eleCount);
            seParamDelay.ICNT0      = chDelaySize;
            saParamDelay.ICNT0      = chDelaySize;
            seParamInterleave.ICNT1 = (chDelaySize < eleCount) ? chDelaySize : eleCount;
            seParamInterleave.ICNT2 = AUDIOLIB_ceilingDiv(chDelaySize, eleCount);
            seParamInterleave.DIM2  = ((chDelaySize < eleCount) ? chDelaySize : eleCount) * strideInElements;

            __SE0_OPEN(&pDelayLocal[i * strideDelayElements], seParamDelay);
            __SE1_OPEN(&pInLocal[strideInElements * (numSamples - chDelaySize) + i], seParamInterleave);
            __SA0_OPEN(saParamDelay);
            __SA1_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               inVec    = c7x::strm_eng<1, vec>::get_adv();
               delayVec = c7x::strm_eng<0, vec>::get_adv();

               vpredOut = c7x::strm_agen<0, vec>::get_vpred();
               addrOut  = c7x::strm_agen<0, vec>::get_adv(&pScratchLocal[i * numSamples]);
               __vstore_pred(vpredOut, addrOut, delayVec);

               vpredDelay = c7x::strm_agen<1, vec>::get_vpred();
               addrDelay  = c7x::strm_agen<1, vec>::get_adv(&pDelayLocal[i * strideDelayElements]);
               __vstore_pred(vpredDelay, addrDelay, inVec);
            }
            __SE0_CLOSE();
            __SE1_CLOSE();
            __SA0_CLOSE();
            __SA1_CLOSE();

            // Copy data from pIn buffer to pOut buffer
            chDelayDiff = numSamples - chDelaySize;
            nVecs       = AUDIOLIB_ceilingDiv(chDelayDiff, eleCount);
            if (nVecs > 0) {
               saParamDelay.ICNT0      = chDelayDiff;
               seParamInterleave.ICNT1 = (chDelayDiff < eleCount) ? chDelayDiff : eleCount;
               seParamInterleave.ICNT2 = AUDIOLIB_ceilingDiv(chDelayDiff, eleCount);
               seParamInterleave.DIM2  = ((chDelayDiff < eleCount) ? chDelayDiff : eleCount) * strideInElements;

               __SE0_OPEN(&pInLocal[i], seParamInterleave);
               __SA0_OPEN(saParamDelay);
               for (int32_t counter = 0; counter < nVecs; counter++) {
                  inVec = c7x::strm_eng<0, vec>::get_adv();

                  vpredOut = c7x::strm_agen<0, vec>::get_vpred();
                  addrOut  = c7x::strm_agen<0, vec>::get_adv(&pScratchLocal[i * numSamples + chDelaySize]);
                  __vstore_pred(vpredOut, addrOut, inVec);
               }
               __SE0_CLOSE();
               __SA0_CLOSE();
            }
         }
         else {
            // Copy data from pDelay buffer to pOut buffer
            nVecs              = AUDIOLIB_ceilingDiv(numSamples, eleCount);
            seParamDelay.ICNT0 = numSamples;
            saParamDelay.ICNT0 = numSamples;

            __SE0_OPEN(&pDelayLocal[i * strideDelayElements], seParamDelay);
            __SA0_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               delayVec = c7x::strm_eng<0, vec>::get_adv();

               vpredOut = c7x::strm_agen<0, vec>::get_vpred();
               addrOut  = c7x::strm_agen<0, vec>::get_adv(&pScratchLocal[i * numSamples]);
               __vstore_pred(vpredOut, addrOut, delayVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();

            // Left Shift the remaining elements within pDelay buffer
            chDelayDiff        = chDelaySize - numSamples;
            nVecs              = AUDIOLIB_ceilingDiv(chDelayDiff, eleCount);
            seParamDelay.ICNT0 = chDelayDiff;
            saParamDelay.ICNT0 = chDelayDiff;

            __SE0_OPEN(&pDelayLocal[i * strideDelayElements + numSamples], seParamDelay);
            __SA0_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               delayVec = c7x::strm_eng<0, vec>::get_adv();

               vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
               addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[i * strideDelayElements]);
               __vstore_pred(vpredDelay, addrDelay, delayVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();

            // Copy data from pIn buffer to pDelay buffer
            nVecs                   = AUDIOLIB_ceilingDiv(numSamples, eleCount);
            saParamDelay.ICNT0      = numSamples;
            seParamInterleave.ICNT1 = eleCount;
            seParamInterleave.ICNT2 = AUDIOLIB_ceilingDiv(numSamples, eleCount);
            seParamInterleave.DIM2  = eleCount * strideInElements;

            __SE0_OPEN(&pInLocal[i], seParamInterleave);
            __SA0_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               inVec = c7x::strm_eng<0, vec>::get_adv();

               vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
               addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[i * strideDelayElements + chDelayDiff]);
               __vstore_pred(vpredDelay, addrDelay, inVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();
         }
      }
   }
   // Circular Delay Implementation
   else if (mode == 1) {
      uint32_t readIdx, writeIdx;
      uint32_t loopWriteCnt1, loopWriteCnt2;
      uint32_t loopReadCnt1, loopReadCnt2;

      uint32_t delayBuffSize = pKerPrivArgs->delayBuffSize;

      for (i = 0; i < numChannels; i++) {
         readIdx  = pKerPrivArgs->readIdx[i];
         writeIdx = pKerPrivArgs->writeIdx[i];

         loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
         loopWriteCnt2 = numSamples - loopWriteCnt1;

         loopReadCnt1 = (delayBuffSize - readIdx) <= numSamples ? (delayBuffSize - readIdx) : numSamples;
         loopReadCnt2 = numSamples - loopReadCnt1;

         nVecs                   = AUDIOLIB_ceilingDiv(loopWriteCnt1, eleCount);
         saParamDelay.ICNT0      = loopWriteCnt1;
         seParamInterleave.ICNT1 = (loopWriteCnt1 < eleCount) ? loopWriteCnt1 : eleCount;
         seParamInterleave.ICNT2 = AUDIOLIB_ceilingDiv(loopWriteCnt1, eleCount);
         seParamInterleave.DIM2  = ((loopWriteCnt1 < eleCount) ? loopWriteCnt1 : eleCount) * strideInElements;

         // Copy data from pIn buffer to pDelay buffer
         __SE0_OPEN(&pInLocal[i], seParamInterleave);
         __SA0_OPEN(saParamDelay);
         for (int32_t counter = 0; counter < nVecs; counter++) {
            inVec      = c7x::strm_eng<0, vec>::get_adv();
            vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[i * strideDelayElements + writeIdx]);
            __vstore_pred(vpredDelay, addrDelay, inVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();
         writeIdx = (writeIdx + loopWriteCnt1) % delayBuffSize;

         if (loopWriteCnt2 > 0) {
            nVecs                   = AUDIOLIB_ceilingDiv(loopWriteCnt2, eleCount);
            saParamDelay.ICNT0      = loopWriteCnt2;
            seParamInterleave.ICNT1 = (loopWriteCnt2 < eleCount) ? loopWriteCnt2 : eleCount;
            seParamInterleave.ICNT2 = AUDIOLIB_ceilingDiv(loopWriteCnt2, eleCount);
            seParamInterleave.DIM2  = ((loopWriteCnt2 < eleCount) ? loopWriteCnt2 : eleCount) * strideInElements;

            __SE0_OPEN(&pInLocal[strideInElements * loopWriteCnt1 + i], seParamInterleave);
            __SA0_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               inVec      = c7x::strm_eng<0, vec>::get_adv();
               vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
               addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[i * strideDelayElements]);
               __vstore_pred(vpredDelay, addrDelay, inVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();
            writeIdx = loopWriteCnt2;
         }

         // Copy data from pDelay buffer to pOut buffer
         nVecs                  = AUDIOLIB_ceilingDiv(numSamples, eleCount);
         seParamCircDelay.ICNT0 = numSamples;
         saParamDelay.ICNT0     = numSamples;

         __SE0_OPEN(&pDelayLocal[i * strideDelayElements + readIdx], seParamCircDelay);
         __SA0_OPEN(saParamDelay);
         for (int32_t counter = 0; counter < nVecs; counter++) {
            delayVec = c7x::strm_eng<0, vec>::get_adv();
            vpredOut = c7x::strm_agen<0, vec>::get_vpred();
            addrOut  = c7x::strm_agen<0, vec>::get_adv(&pScratchLocal[i * numSamples]);
            __vstore_pred(vpredOut, addrOut, delayVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();
         readIdx = ((readIdx + numSamples) >= delayBuffSize) ? loopReadCnt2 : (readIdx + numSamples);

         pKerPrivArgs->readIdx[i]  = readIdx;
         pKerPrivArgs->writeIdx[i] = writeIdx;
      }
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   // Interleave Output Buffer
   nVecs = numSamples * AUDIOLIB_ceilingDiv(numChannels, eleCount);
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

template AUDIOLIB_STATUS AUDIOLIB_delayNChannel_interleave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                          void *restrict pIn,
                                                                          void *restrict pDelay,
                                                                          void *restrict pOut,
                                                                          void *restrict pScratch);

template AUDIOLIB_STATUS AUDIOLIB_delayNChannel_interleave_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pDelay,
                                                                           void *restrict pOut,
                                                                           void *restrict pScratch);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delayNChannel_exec_ci(AUDIOLIB_kernelHandle handle,
                                               void *restrict pIn,
                                               void *restrict pDelay,
                                               void *restrict pOut,
                                               void *restrict pScratch)
{

   AUDIOLIB_STATUS                  status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_delayNChannel_PrivArgs *pKerPrivArgs = (AUDIOLIB_delayNChannel_PrivArgs *) handle;

   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_delayNChannel_exec_ci \n");

   __SE_TEMPLATE_v1 seParamDelay     = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SE_TEMPLATE_v1 seParamCircDelay = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE2_PARAM_OFFSET);
   __SA_TEMPLATE_v1 saParamDelay     = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   dataType *restrict pInLocal    = (dataType *) pIn;
   dataType *restrict pOutLocal   = (dataType *) pOut;
   dataType *restrict pDelayLocal = (dataType *) pDelay;

   uint32_t mode                = pKerPrivArgs->mode;
   uint32_t numSamples          = pKerPrivArgs->numSamples;
   uint32_t numChannels         = pKerPrivArgs->numChannels;
   uint32_t strideInElements    = pKerPrivArgs->strideInElements;
   uint32_t strideOutElements   = pKerPrivArgs->strideOutElements;
   uint32_t strideDelayElements = pKerPrivArgs->strideDelayElements;

   uint32_t i;
   int32_t  nVecs = 0;
   uint32_t chDelaySize, chDelayDiff;

   vec     inVec, delayVec;
   vec    *addrDelay, *addrOut;
   __vpred vpredDelay, vpredOut;

   // Linear Delay Implementation
   if (mode == 0) {
      for (i = 0; i < numChannels; i++) {
         chDelaySize = pKerPrivArgs->delaySize[i];

         if (chDelaySize <= numSamples) {
            // Copy data from pDelay buffer to pOut buffer and pIn buffer to pDelay buffer
            nVecs              = AUDIOLIB_ceilingDiv(chDelaySize, eleCount);
            seParamDelay.ICNT0 = chDelaySize;
            saParamDelay.ICNT0 = chDelaySize;

            __SE0_OPEN(&pDelayLocal[i * strideDelayElements], seParamDelay);
            __SE1_OPEN(&pInLocal[i * strideInElements + (numSamples - chDelaySize)], seParamDelay);
            __SA0_OPEN(saParamDelay);
            __SA1_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               inVec    = c7x::strm_eng<1, vec>::get_adv();
               delayVec = c7x::strm_eng<0, vec>::get_adv();

               vpredOut = c7x::strm_agen<0, vec>::get_vpred();
               addrOut  = c7x::strm_agen<0, vec>::get_adv(&pOutLocal[i * strideOutElements]);
               __vstore_pred(vpredOut, addrOut, delayVec);

               vpredDelay = c7x::strm_agen<1, vec>::get_vpred();
               addrDelay  = c7x::strm_agen<1, vec>::get_adv(&pDelayLocal[i * strideDelayElements]);
               __vstore_pred(vpredDelay, addrDelay, inVec);
            }
            __SE0_CLOSE();
            __SE1_CLOSE();
            __SA0_CLOSE();
            __SA1_CLOSE();

            // Copy data from pIn buffer to pOut buffer
            chDelayDiff = numSamples - chDelaySize;
            nVecs       = AUDIOLIB_ceilingDiv(chDelayDiff, eleCount);
            if (nVecs > 0) {
               seParamDelay.ICNT0 = chDelayDiff;
               saParamDelay.ICNT0 = chDelayDiff;

               __SE0_OPEN(&pInLocal[i * strideInElements], seParamDelay);
               __SA0_OPEN(saParamDelay);
               for (int32_t counter = 0; counter < nVecs; counter++) {
                  inVec = c7x::strm_eng<0, vec>::get_adv();

                  vpredOut = c7x::strm_agen<0, vec>::get_vpred();
                  addrOut  = c7x::strm_agen<0, vec>::get_adv(&pOutLocal[i * strideOutElements + chDelaySize]);
                  __vstore_pred(vpredOut, addrOut, inVec);
               }
               __SE0_CLOSE();
               __SA0_CLOSE();
            }
         }
         else {
            // Copy data from pDelay buffer to pOut buffer
            nVecs              = AUDIOLIB_ceilingDiv(numSamples, eleCount);
            seParamDelay.ICNT0 = numSamples;
            saParamDelay.ICNT0 = numSamples;

            __SE0_OPEN(&pDelayLocal[i * strideDelayElements], seParamDelay);
            __SA0_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               delayVec = c7x::strm_eng<0, vec>::get_adv();

               vpredOut = c7x::strm_agen<0, vec>::get_vpred();
               addrOut  = c7x::strm_agen<0, vec>::get_adv(&pOutLocal[i * strideOutElements]);
               __vstore_pred(vpredOut, addrOut, delayVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();

            // Left Shift the remaining elements within pDelay buffer
            chDelayDiff        = chDelaySize - numSamples;
            nVecs              = AUDIOLIB_ceilingDiv(chDelayDiff, eleCount);
            seParamDelay.ICNT0 = chDelayDiff;
            saParamDelay.ICNT0 = chDelayDiff;

            __SE0_OPEN(&pDelayLocal[i * strideDelayElements + numSamples], seParamDelay);
            __SA0_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               delayVec = c7x::strm_eng<0, vec>::get_adv();

               vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
               addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[i * strideDelayElements]);
               __vstore_pred(vpredDelay, addrDelay, delayVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();

            // Copy data from pIn buffer to pDelay buffer
            nVecs              = AUDIOLIB_ceilingDiv(numSamples, eleCount);
            seParamDelay.ICNT0 = numSamples;
            saParamDelay.ICNT0 = numSamples;
            __SE0_OPEN(&pInLocal[i * strideInElements], seParamDelay);
            __SA0_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               inVec = c7x::strm_eng<0, vec>::get_adv();

               vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
               addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[i * strideDelayElements + chDelayDiff]);
               __vstore_pred(vpredDelay, addrDelay, inVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();
         }
      }
   }
   // Circular Delay Implementation
   else if (mode == 1) {
      uint32_t readIdx, writeIdx;
      uint32_t loopWriteCnt1, loopWriteCnt2;
      uint32_t loopReadCnt1, loopReadCnt2;

      uint32_t delayBuffSize = pKerPrivArgs->delayBuffSize;

      for (i = 0; i < numChannels; i++) {
         readIdx  = pKerPrivArgs->readIdx[i];
         writeIdx = pKerPrivArgs->writeIdx[i];

         loopWriteCnt1 = (delayBuffSize - writeIdx) <= numSamples ? (delayBuffSize - writeIdx) : numSamples;
         loopWriteCnt2 = numSamples - loopWriteCnt1;

         loopReadCnt1 = (delayBuffSize - readIdx) <= numSamples ? (delayBuffSize - readIdx) : numSamples;
         loopReadCnt2 = numSamples - loopReadCnt1;

         nVecs              = AUDIOLIB_ceilingDiv(loopWriteCnt1, eleCount);
         seParamDelay.ICNT0 = loopWriteCnt1;
         saParamDelay.ICNT0 = loopWriteCnt1;

         // Copy data from pIn buffer to pDelay buffer
         __SE0_OPEN(&pInLocal[i * strideInElements], seParamDelay);
         __SA0_OPEN(saParamDelay);
         for (int32_t counter = 0; counter < nVecs; counter++) {
            inVec      = c7x::strm_eng<0, vec>::get_adv();
            vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
            addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[i * strideDelayElements + writeIdx]);
            __vstore_pred(vpredDelay, addrDelay, inVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();
         writeIdx = (writeIdx + loopWriteCnt1) % delayBuffSize;

         nVecs              = AUDIOLIB_ceilingDiv(loopWriteCnt2, eleCount);
         seParamDelay.ICNT0 = loopWriteCnt2;
         saParamDelay.ICNT0 = loopWriteCnt2;

         if (loopWriteCnt2 > 0) {

            __SE0_OPEN(&pInLocal[i * strideInElements + loopWriteCnt1], seParamDelay);
            __SA0_OPEN(saParamDelay);
            for (int32_t counter = 0; counter < nVecs; counter++) {
               inVec      = c7x::strm_eng<0, vec>::get_adv();
               vpredDelay = c7x::strm_agen<0, vec>::get_vpred();
               addrDelay  = c7x::strm_agen<0, vec>::get_adv(&pDelayLocal[i * strideDelayElements]);
               __vstore_pred(vpredDelay, addrDelay, inVec);
            }
            __SE0_CLOSE();
            __SA0_CLOSE();
            writeIdx = loopWriteCnt2;
         }

         // Copy data from pDelay buffer to pOut buffer
         nVecs                  = AUDIOLIB_ceilingDiv(numSamples, eleCount);
         seParamCircDelay.ICNT0 = numSamples;
         saParamDelay.ICNT0     = numSamples;

         __SE0_OPEN(&pDelayLocal[i * strideDelayElements + readIdx], seParamCircDelay);
         __SA0_OPEN(saParamDelay);
         for (int32_t counter = 0; counter < nVecs; counter++) {
            delayVec = c7x::strm_eng<0, vec>::get_adv();
            vpredOut = c7x::strm_agen<0, vec>::get_vpred();
            addrOut  = c7x::strm_agen<0, vec>::get_adv(&pOutLocal[i * strideOutElements]);
            __vstore_pred(vpredOut, addrOut, delayVec);
         }
         __SE0_CLOSE();
         __SA0_CLOSE();
         readIdx = ((readIdx + numSamples) >= delayBuffSize) ? loopReadCnt2 : (readIdx + numSamples);

         pKerPrivArgs->readIdx[i]  = readIdx;
         pKerPrivArgs->writeIdx[i] = writeIdx;
      }
   }
   else {
      status = AUDIOLIB_ERR_NOT_IMPLEMENTED;
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_delayNChannel_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                               void *restrict pIn,
                                                               void *restrict pDelay,
                                                               void *restrict pOut,
                                                               void *restrict pScratch);

template AUDIOLIB_STATUS AUDIOLIB_delayNChannel_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                void *restrict pIn,
                                                                void *restrict pDelay,
                                                                void *restrict pOut,
                                                                void *restrict pScratch);
