// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_router_priv.h"

template <typename T> constexpr __SE_TRANSPOSE SETransposeConst();

template <> constexpr __SE_TRANSPOSE SETransposeConst<float>() { return __SE_TRANSPOSE_32BIT; };

template <> constexpr __SE_TRANSPOSE SETransposeConst<double>() { return __SE_TRANSPOSE_64BIT; };

#define SE_PARAM_BASE (0x0000)
#define MAX_SEPARAMS (16)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + MAX_SEPARAMS * SE_PARAM_SIZE)
#define SE_SA0_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE)

// Default template (for double or other types)
template <typename T> constexpr AUDIOLIB_data_type_e getDataTypeEnum() { return AUDIOLIB_FLOAT64; }

// Specialization for float
template <> constexpr AUDIOLIB_data_type_e getDataTypeEnum<float>() { return AUDIOLIB_FLOAT32; }

void AUDIOLIB_router_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_router_PrivArgs *pKerPrivArgs = (AUDIOLIB_router_PrivArgs *) handle;

   uint64_t routerStartupCycles   = 0;
   uint64_t routerTeardownCycles  = 0;
   uint64_t routerOperationCycles = 0;
   uint64_t routerOverheadCycles  = 0;
   uint32_t sVecs                 = pKerPrivArgs->sVecs;
   uint32_t runCount              = pKerPrivArgs->runCount;

   if (pKerPrivArgs->isInterleave == 0) {
      routerStartupCycles = 12;
      for (uint32_t i = 0; i < runCount; i++) {
         routerOverheadCycles += 25;
         routerOperationCycles += (1 * pKerPrivArgs->nVecs[i] + 4);
         routerTeardownCycles += 2;
         routerOverheadCycles += 2;
      }
      routerTeardownCycles += 2;
   }
   else {
      routerStartupCycles = 13;
      for (uint32_t i = 0; i < runCount; i++) {
         routerOverheadCycles += 25;
         routerOperationCycles += (1 * pKerPrivArgs->nVecs[i] + 4);
         routerTeardownCycles += 2;
         routerOverheadCycles += 2;
      }
      routerOverheadCycles += 18;
      routerOperationCycles += (1 * sVecs + 1);
      routerOverheadCycles += 6;
      routerTeardownCycles += 6;
   }
   routerOverheadCycles += routerStartupCycles + routerTeardownCycles;
   *estCycles  = routerOperationCycles + routerOverheadCycles;
   *archCycles = routerOperationCycles;
}

template <typename dataType> AUDIOLIB_STATUS AUDIOLIB_router_init_ci(AUDIOLIB_kernelHandle handle)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   return status;
}

template <typename dataType> AUDIOLIB_STATUS AUDIOLIB_router_init_interLeave_set_ci(AUDIOLIB_kernelHandle handle)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_router_PrivArgs *pKerPrivArgs = (AUDIOLIB_router_PrivArgs *) handle;

   uint32_t runCount = pKerPrivArgs->runCount;

   if (runCount > 0) {
      typedef typename c7x::make_full_vector<dataType>::type vec;

      uint32_t  eleCount       = c7x::element_count_of<vec>::value;
      uint8_t  *pBlock         = pKerPrivArgs->bufPblock;
      uint32_t *runLengthArray = pKerPrivArgs->runLengthArray;

      uint32_t *restrict nVecs = pKerPrivArgs->nVecs;

      AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_router_init_ci \n");
      AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d dim_x %d dim_y %d\n",
                            pKerPrivArgs->dim_x, pKerPrivArgs->dim_y, pKerPrivArgs->strideIn, eleCount,
                            pKerPrivArgs->dim_x, pKerPrivArgs->dim_y);

      __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
      __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
      __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

      /**********************************************************************/
      /* Prepare SE/SA template                                             */
      /**********************************************************************/

      __SE_TEMPLATE_v1 se0Params[runCount], se1Params;

      for (uint32_t i = 0; i < runCount; i++) {

         se0Params[i]           = __gen_SE_TEMPLATE_v1();
         se0Params[i].ELETYPE   = SE_ELETYPE;
         se0Params[i].VECLEN    = SE_VECLEN;
         se0Params[i].TRANSPOSE = SETransposeConst<dataType>();

         se0Params[i].ICNT0  = 1;
         se0Params[i].ICNT1  = (pKerPrivArgs->samples > eleCount) ? eleCount : pKerPrivArgs->samples;
         se0Params[i].DIM1   = pKerPrivArgs->strideIn;
         se0Params[i].ICNT2  = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, 2 * eleCount);
         se0Params[i].DIM2   = 2 * pKerPrivArgs->strideIn * eleCount;
         se0Params[i].ICNT3  = runLengthArray[i];
         se0Params[i].DIM3   = 1;
         se0Params[i].DIMFMT = __SE_DIMFMT_4D;

         nVecs[i] = runLengthArray[i] * AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, 2 * eleCount);
      }

      se1Params           = __gen_SE_TEMPLATE_v1();
      se1Params.TRANSPOSE = SETransposeConst<dataType>();
      se1Params.ICNT0     = 1;
      se1Params.ICNT1 =
          (pKerPrivArgs->outputChannels > (uint32_t) (eleCount)) ? (eleCount) : pKerPrivArgs->outputChannels;
      se1Params.DIM1    = pKerPrivArgs->samples;
      se1Params.ICNT2   = AUDIOLIB_ceilingDiv(pKerPrivArgs->outputChannels, 2 * eleCount);
      se1Params.DIM2    = 2 * pKerPrivArgs->samples * eleCount;
      se1Params.ICNT3   = pKerPrivArgs->samples;
      se1Params.DIM3    = 1;
      se1Params.ELETYPE = SE_ELETYPE;
      se1Params.VECLEN  = SE_VECLEN;
      se1Params.DIMFMT  = __SE_DIMFMT_4D;

      __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
      __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();

      sa0Params.VECLEN        = SA_VECLEN;
      sa0Params.ICNT0         = 2 * eleCount;
      sa0Params.ICNT1         = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, 2 * eleCount);
      sa0Params.DIM1          = 2 * eleCount;
      sa0Params.ICNT2         = pKerPrivArgs->outputChannels;
      sa0Params.DIM2          = pKerPrivArgs->samples;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa0Params.DECDIM1_WIDTH = pKerPrivArgs->samples;

      sa1Params.ICNT0         = 2 * eleCount;
      sa1Params.ICNT1         = AUDIOLIB_ceilingDiv(pKerPrivArgs->outputChannels, 2 * eleCount);
      sa1Params.DIM1          = 2 * eleCount;
      sa1Params.ICNT2         = pKerPrivArgs->samples;
      sa1Params.DIM2          = pKerPrivArgs->strideOut;
      sa1Params.VECLEN        = SA_VECLEN;
      sa1Params.DIMFMT        = __SA_DIMFMT_3D;
      sa1Params.DECDIM1       = __SA_DECDIM_DIM1;
      sa1Params.DECDIM1_WIDTH = pKerPrivArgs->outputChannels;
      pKerPrivArgs->sVecs     = pKerPrivArgs->samples * AUDIOLIB_ceilingDiv(pKerPrivArgs->outputChannels, 2 * eleCount);

      if (pKerPrivArgs->samples <= eleCount) {
         pKerPrivArgs->seOpenOffset1 = 0;
         pKerPrivArgs->seOpenOffset2 = 0;
      }
      else {
         pKerPrivArgs->seOpenOffset1 = (eleCount * pKerPrivArgs->strideIn);
         pKerPrivArgs->seOpenOffset2 = (eleCount * pKerPrivArgs->samples);
      }
      memcpy((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET, se0Params, sizeof(se0Params));
      *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   }
   else {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }

   return status;
}

template <typename dataType> AUDIOLIB_STATUS AUDIOLIB_router_init_set_ci(AUDIOLIB_kernelHandle handle)
{
   AUDIOLIB_STATUS           status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_router_PrivArgs *pKerPrivArgs = (AUDIOLIB_router_PrivArgs *) handle;

   uint32_t runCount = pKerPrivArgs->runCount;

   if (runCount > 0) {
      typedef typename c7x::make_full_vector<dataType>::type vec;

      uint8_t  *pBlock         = pKerPrivArgs->bufPblock;
      uint32_t *runLengthArray = pKerPrivArgs->runLengthArray;
      uint32_t  eleCount       = c7x::element_count_of<vec>::value;
      uint32_t *nVecs          = pKerPrivArgs->nVecs;

      AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_router_init_ci \n");
      AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d dim_x %d dim_y %d\n",
                            pKerPrivArgs->dim_x, pKerPrivArgs->dim_y, pKerPrivArgs->strideIn, eleCount,
                            pKerPrivArgs->dim_x, pKerPrivArgs->dim_y);

      __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
      __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
      __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

      /**********************************************************************/
      /* Prepare SE/SA template                                             */
      /**********************************************************************/

      __SE_TEMPLATE_v1 se0Params[runCount];
      for (uint32_t i = 0; i < runCount; i++) {

         se0Params[i]         = __gen_SE_TEMPLATE_v1();
         se0Params[i].ELETYPE = SE_ELETYPE;
         se0Params[i].VECLEN  = SE_VECLEN;

         se0Params[i].ICNT0  = pKerPrivArgs->samples;
         se0Params[i].ICNT1  = runLengthArray[i];
         se0Params[i].DIM1   = pKerPrivArgs->strideIn;
         se0Params[i].DIMFMT = __SE_DIMFMT_2D;

         nVecs[i] = runLengthArray[i] * AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
      }

      __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

      sa0Params.VECLEN = SA_VECLEN;
      sa0Params.ICNT0  = pKerPrivArgs->samples;
      sa0Params.ICNT1  = pKerPrivArgs->outputChannels;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      memcpy((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET, se0Params, sizeof(se0Params));
      *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   }
   else {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_router_init_ci<float>(AUDIOLIB_kernelHandle handle);

template AUDIOLIB_STATUS AUDIOLIB_router_init_ci<double>(AUDIOLIB_kernelHandle handle);

template AUDIOLIB_STATUS AUDIOLIB_router_init_set_ci<float>(AUDIOLIB_kernelHandle handle);

template AUDIOLIB_STATUS AUDIOLIB_router_init_set_ci<double>(AUDIOLIB_kernelHandle handle);

template AUDIOLIB_STATUS AUDIOLIB_router_init_interLeave_set_ci<float>(AUDIOLIB_kernelHandle handle);

template AUDIOLIB_STATUS AUDIOLIB_router_init_interLeave_set_ci<double>(AUDIOLIB_kernelHandle handle);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_router_interLeave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                   void *restrict pIn,
                                                   void *restrict pOut,
                                                   void *restrict pOutScratch)
{

   AUDIOLIB_STATUS                                        status = AUDIOLIB_SUCCESS;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_router_PrivArgs *pKerPrivArgs   = (AUDIOLIB_router_PrivArgs *) handle;
   uint8_t                  *pBlock         = pKerPrivArgs->bufPblock;
   uint32_t                 *nVecs          = pKerPrivArgs->nVecs;
   uint32_t                  runCount       = pKerPrivArgs->runCount;
   uint32_t                 *runOffsetArray = pKerPrivArgs->runOffsetArray;
   uint32_t                 *runMuteArray   = pKerPrivArgs->runMuteArray;
   uint32_t                  svecs          = pKerPrivArgs->sVecs;
   uint32_t                  seOpenOffset1  = pKerPrivArgs->seOpenOffset1;
   uint32_t                  seOpenOffset2  = pKerPrivArgs->seOpenOffset2;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   dataType *restrict pInLocal         = (dataType *) pIn;
   dataType *restrict pOutLocal        = (dataType *) pOut;
   dataType *restrict pOutScratchLocal = (dataType *) pOutScratch;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_router_interLeave_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 *restrict se0Params = (__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params           = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params           = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params           = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);

   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < runCount; i++) {

      __SE0_OPEN(pInLocal + runOffsetArray[i], se0Params[i]);
      __SE1_OPEN(pInLocal + (seOpenOffset1) + runOffsetArray[i], se0Params[i]);

      vec gain = (vec) runMuteArray[i];

      uint32_t loopCount = nVecs[i];

      for (uint32_t j = 0; j < loopCount; j++) {
         vec inDat1 = c7x::strm_eng<0, vec>::get_adv();
         vec inDat2 = c7x::strm_eng<1, vec>::get_adv();

         inDat1 = gain * inDat1;
         inDat2 = gain * inDat2;

         __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutScratchLocal);
         __vstore_pred(vpred, addr, inDat1);

         vpred = c7x::strm_agen<0, vec>::get_vpred();
         addr  = c7x::strm_agen<0, vec>::get_adv(pOutScratchLocal);
         __vstore_pred(vpred, addr, inDat2);
      }
      __SE0_CLOSE();
   }
   __SA0_CLOSE();

   __SE0_OPEN(pOutScratchLocal, se1Params);
   __SE1_OPEN(pOutScratchLocal + seOpenOffset2, se1Params);
   __SA0_OPEN(sa1Params);

   for (uint32_t i = 0; i < svecs; i++) {

      vec loadVec1 = c7x::strm_eng<0, vec>::get_adv();
      vec loadVec2 = c7x::strm_eng<1, vec>::get_adv();

      __vpred predTemp  = c7x::strm_agen<0, vec>::get_vpred();
      vec    *pStoreVec = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(predTemp, pStoreVec, loadVec1);

      predTemp  = c7x::strm_agen<0, vec>::get_vpred();
      pStoreVec = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(predTemp, pStoreVec, loadVec2);
   }

   __SA0_CLOSE();
   __SE0_CLOSE();
   __SE1_CLOSE();

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_router_exec_ci(AUDIOLIB_kernelHandle handle,
                                        void *restrict pIn,
                                        void *restrict pOut,
                                        void *restrict pOutScratch)

{

   AUDIOLIB_STATUS                                        status = AUDIOLIB_SUCCESS;
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_router_PrivArgs *pKerPrivArgs   = (AUDIOLIB_router_PrivArgs *) handle;
   uint8_t                  *pBlock         = pKerPrivArgs->bufPblock;
   uint32_t                 *nVecs          = pKerPrivArgs->nVecs;
   uint32_t                  runCount       = pKerPrivArgs->runCount;
   uint32_t                 *runOffsetArray = pKerPrivArgs->runOffsetArray;
   uint32_t                 *runMuteArray   = pKerPrivArgs->runMuteArray;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_router_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 *se0Params = (__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1  sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < runCount; i++) {

      __SE0_OPEN(pInLocal + runOffsetArray[i], se0Params[i]);

      vec gain = (vec) runMuteArray[i];

      uint32_t loopCount = nVecs[i];

      for (uint32_t j = 0; j < loopCount; j++) {
         vec inDat = c7x::strm_eng<0, vec>::get_adv();

         inDat = gain * inDat;

         __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
         vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
         __vstore_pred(vpred, addr, inDat);
      }
      __SE0_CLOSE();
   }

   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_router_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pOut,
                                                        void *restrict pOutScratch);

template AUDIOLIB_STATUS AUDIOLIB_router_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pOut,
                                                         void *restrict pOutScratch);

template AUDIOLIB_STATUS AUDIOLIB_router_interLeave_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pOut,
                                                                   void *restrict pOutScratch);

template AUDIOLIB_STATUS AUDIOLIB_router_interLeave_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                    void *restrict pIn,
                                                                    void *restrict pOut,
                                                                    void *restrict pOutScratch);
