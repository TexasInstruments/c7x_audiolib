// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_gainNCh_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA0_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)

// Default template (for double or other types)
template <typename T> constexpr AUDIOLIB_data_type_e getDataTypeEnum() { return AUDIOLIB_FLOAT64; }

// Specialization for float
template <> constexpr AUDIOLIB_data_type_e getDataTypeEnum<float>() { return AUDIOLIB_FLOAT32; }

void AUDIOLIB_gainNCh_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_gainNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNCh_PrivArgs *) handle;

   uint64_t gainNChStartupCycles   = 0;
   uint64_t gainNChTeardownCycles  = 0;
   uint64_t gainNChOperationCycles = 0;
   uint64_t gainNChOverheadCycles  = 0;
   uint32_t nVecs                  = 0;

   if (pKerPrivArgs->isInterleave == 0) {
      nVecs                  = pKerPrivArgs->nVecs;
      gainNChStartupCycles   = 27 + 17;
      gainNChOperationCycles = 4 + nVecs * 1;
      gainNChTeardownCycles  = 3;
   }
   else {
      if (pKerPrivArgs->customImplementation == 1) {
         gainNChOverheadCycles += 9 + 27;
         nVecs                  = pKerPrivArgs->nVecs;
         gainNChOperationCycles = 8 + nVecs * 1;
         gainNChTeardownCycles  = 3;
      }
      else {
         nVecs                  = pKerPrivArgs->nVecs;
         gainNChStartupCycles   = 27 + 17;
         gainNChOperationCycles = 4 + nVecs * 1;
         gainNChTeardownCycles  = 3;
      }
   }
   gainNChOverheadCycles += gainNChStartupCycles + gainNChTeardownCycles;
   *estCycles  = gainNChOperationCycles + gainNChOverheadCycles;
   *archCycles = gainNChOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNCh_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_gainNCh_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNCh_PrivArgs                             *pKerPrivArgs = (AUDIOLIB_gainNCh_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_data_type_e dataVal = getDataTypeEnum<dataType>();

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNCh_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d dim_x %d dim_y %d\n", pKerPrivArgs->dim_x,
                         pKerPrivArgs->dim_y, pKerPrivArgs->strideIn, eleCount, pKerPrivArgs->dim_x,
                         pKerPrivArgs->dim_y);

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELEDUP  SE_ELEDUP  = c7x::se_eledup<dataType, vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/

   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;

   se1Params.ELETYPE = SE_ELETYPE;
   se1Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;

   /**********************************************************************/
   /* Prepare SE/SA template                     */
   /**********************************************************************/

   // for interleave data
   if ((int32_t) pKerPrivArgs->isInterleave == 1) {
      uint32_t blockSize = pKerPrivArgs->dim_y * pKerPrivArgs->dim_x;

      se0Params.ICNT0  = blockSize;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      se1Params.DIMFMT = __SE_DIMFMT_2D;
      se1Params.ICNT0  = pKerPrivArgs->dim_x;
      se1Params.DIM1   = 0;
      se1Params.ICNT1  = AUDIOLIB_ceilingDiv(blockSize, eleCount);

      sa0Params.ICNT0     = blockSize;
      sa0Params.DIMFMT    = __SA_DIMFMT_1D;
      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(blockSize, eleCount);

// Setting optimized parameters for ELEDUP for interleaved data when the number of dim_y is 2, 4 for 256 bit device
#if __C7X_VEC_SIZE_BITS__ == 256
      if (pKerPrivArgs->customImplementation) {
         se1Params.GRPDUP = __SE_GRPDUP_ON;

         if ((pKerPrivArgs->dim_x == 4) && (dataVal == AUDIOLIB_FLOAT64)) {

            se1Params.GRPDUP = __SE_GRPDUP_OFF;
         }
         else if (((pKerPrivArgs->dim_x == 2) && (dataVal == AUDIOLIB_FLOAT32)) ||
                  ((pKerPrivArgs->dim_x == 2) && (dataVal == AUDIOLIB_FLOAT64))) {
            se1Params.VECLEN = __SE_VECLEN_2ELEMS;
         }
         else if ((pKerPrivArgs->dim_x == 4) && (dataVal == AUDIOLIB_FLOAT32)) {

            se1Params.VECLEN = __SE_VECLEN_4ELEMS;
         }
         else {
         }
      }
// Setting optimized parameters for ELEDUP for interleaved data when the number of dim_y is 2, 4  ,8 for 512 bit
// device
#else
      if (pKerPrivArgs->customImplementation) {

         se1Params.GRPDUP = __SE_GRPDUP_ON;

         if ((pKerPrivArgs->dim_x == 8) && (dataVal == AUDIOLIB_FLOAT64)) {

            se1Params.GRPDUP = __SE_GRPDUP_OFF;
         }
         else if ((pKerPrivArgs->dim_x == 8 && dataVal == AUDIOLIB_FLOAT32)) {
            se1Params.VECLEN = __SE_VECLEN_8ELEMS;
         }

         else if ((pKerPrivArgs->dim_x == 4 && dataVal == AUDIOLIB_FLOAT32) ||
                  (pKerPrivArgs->dim_x == 4 && dataVal == AUDIOLIB_FLOAT64)) {
            se1Params.VECLEN = __SE_VECLEN_4ELEMS;
         }

         else if ((pKerPrivArgs->dim_x == 2 && dataVal == AUDIOLIB_FLOAT32) ||
                  (pKerPrivArgs->dim_x == 2 && dataVal == AUDIOLIB_FLOAT64)) {

            se1Params.VECLEN = __SE_VECLEN_2ELEMS;
         }

         else {
         }
      }
#endif

      // for the other dim_y in interleave data
      else {
         se0Params.ICNT0         = (pKerPrivArgs->dim_x < eleCount) ? pKerPrivArgs->dim_x : eleCount;
         se0Params.DIM1          = pKerPrivArgs->strideIn;
         se0Params.ICNT1         = pKerPrivArgs->dim_y;
         se0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
         se0Params.DIM2          = eleCount;
         se0Params.DIMFMT        = __SE_DIMFMT_3D;
         se0Params.DECDIM1_WIDTH = pKerPrivArgs->dim_x;
         se0Params.DECDIM1       = __SE_DECDIM_DIM2;

         se1Params.DIMFMT = __SE_DIMFMT_3D;
         se1Params.ICNT0  = (pKerPrivArgs->dim_x < eleCount) ? pKerPrivArgs->dim_x : eleCount;
         se1Params.ICNT1  = pKerPrivArgs->dim_y;
         se1Params.DIM1   = 0;
         se1Params.ICNT2  = AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
         se1Params.DIM2   = eleCount;

         sa0Params.ICNT0         = (pKerPrivArgs->dim_x < eleCount) ? pKerPrivArgs->dim_x : eleCount;
         sa0Params.DIM1          = pKerPrivArgs->strideOut;
         sa0Params.ICNT1         = pKerPrivArgs->dim_y;
         sa0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
         sa0Params.DIM2          = eleCount;
         sa0Params.DIMFMT        = __SA_DIMFMT_3D;
         sa0Params.DECDIM1_WIDTH = pKerPrivArgs->dim_x;
         sa0Params.DECDIM1       = __SA_DECDIM_DIM2;

         pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
      }
   }
   else {

      se0Params.ICNT0  = pKerPrivArgs->dim_x;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.ICNT1  = pKerPrivArgs->dim_y;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      se1Params.ELEDUP = SE_ELEDUP;

      se1Params.ICNT0  = 1;
      se1Params.ICNT1  = AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
      se1Params.DIM1   = 0;
      se1Params.DIMFMT = __SE_DIMFMT_3D;
      se1Params.ICNT2  = pKerPrivArgs->dim_y;
      se1Params.DIM2   = 1;

      sa0Params.ICNT0  = pKerPrivArgs->dim_x;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->dim_y;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      pKerPrivArgs->nVecs = pKerPrivArgs->dim_y * AUDIOLIB_ceilingDiv(pKerPrivArgs->dim_x, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_gainNCh_init_ci<float>(AUDIOLIB_kernelHandle            handle,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                                         const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                                         const AUDIOLIB_gainNCh_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_gainNCh_init_ci<double>(AUDIOLIB_kernelHandle            handle,
                                                          const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                                          const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                                          const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                                          const AUDIOLIB_gainNCh_InitArgs *pKerInitArgs);

template <typename dataType>
inline void AUDIOLIB_evalGain(void *restrict pIn,
                              void *restrict pGain,
                              void *restrict pOut,
                              __SE_TEMPLATE_v1 se0Params,
                              __SE_TEMPLATE_v1 se1Params,
                              __SA_TEMPLATE_v1 sa0Params,
                              uint32_t         nVecs);

template <typename dataType>
inline void AUDIOLIB_evalGain(void *restrict pIn,
                              void *restrict pGain,
                              void *restrict pOut,
                              __SE_TEMPLATE_v1 se0Params,
                              __SE_TEMPLATE_v1 se1Params,
                              __SA_TEMPLATE_v1 sa0Params,
                              uint32_t         nVecs)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   dataType *restrict pInLocal   = (dataType *) pIn;
   dataType *restrict pGainLocal = (dataType *) pGain;
   dataType *restrict pOutLocal  = (dataType *) pOut;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pGainLocal, se1Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t counter = 0; counter < nVecs; counter++) {

      vec gainNCh = c7x::strm_eng<1, vec>::get_adv();
      vec inVec   = c7x::strm_eng<0, vec>::get_adv();

      vec outVec = gainNCh * inVec;

      __vpred vpred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *addr  = c7x::strm_agen<0, vec>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, outVec);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
}
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNCh_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut)
{

   AUDIOLIB_STATUS            status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainNCh_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainNCh_PrivArgs *) handle;
   uint8_t                   *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                   nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainNCh_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params;
   __SE_TEMPLATE_v1 se1Params;
   __SA_TEMPLATE_v1 sa0Params;

   se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   AUDIOLIB_evalGain<dataType>(pIn, pGain, pOut, se0Params, se1Params, sa0Params, nVecs);

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_gainNCh_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pGain,
                                                         void *restrict pOut);

template AUDIOLIB_STATUS AUDIOLIB_gainNCh_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pGain,
                                                          void *restrict pOut);
