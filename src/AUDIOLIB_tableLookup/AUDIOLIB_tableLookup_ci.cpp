// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "../common/c71/AUDIOLIB_inlines.h"
#include "AUDIOLIB_tableLookup_priv.h"
// /*******************************************************************************
//  *
//  * INITIALIZATION FUNCTIONS
//  *
//  ******************************************************************************/

void AUDIOLIB_tableLookup_perfEst(AUDIOLIB_kernelHandle                handle,
                                  uint64_t                            *archCycles,
                                  uint64_t                            *estCycles,
                                  uint32_t                             data_type,
                                  uint32_t                             lookuptableSize,
                                  const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs

)
{
   uint32_t eleCount = 0;
#if __C7X_VEC_SIZE_BITS__ == 512

   if (data_type == AUDIOLIB_FLOAT32) {
      eleCount = 16;
   }
#else
   if (data_type == AUDIOLIB_FLOAT32) {
      eleCount = 8;
   }

#endif

   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;
   uint32_t                       nVecs        = pKerPrivArgs->nVecs;

   uint32_t tableLookupStartupCycles   = 0;
   uint32_t tableLookupOperationCycles = 0;
   uint32_t tableLookupTearDownCycles  = 0;
   uint32_t tableLookupOverheadCycles  = 0;
   uint32_t mainVecs                   = nVecs / eleCount;
   uint32_t remVecs                    = nVecs % eleCount;

#if defined(__C7524__)
   if (pKerPrivArgs->tableSamples > pKerInitArgs->tableLookupSize) {

      tableLookupStartupCycles += 38;
      tableLookupOverheadCycles += 17;
      tableLookupOperationCycles = 39 + mainVecs * 8;
      tableLookupTearDownCycles  = 13;

      tableLookupOverheadCycles += 17 + 2;
      tableLookupOperationCycles += (11 + remVecs * 2);
   }
   else {
      tableLookupStartupCycles += 42;
      nVecs                      = AUDIOLIB_ceilingDiv(nVecs, 2);
      tableLookupOperationCycles = 20 + nVecs * 5;
      tableLookupTearDownCycles  = 11;
   }
#else

   tableLookupStartupCycles += 38;
   tableLookupOverheadCycles += 17;
   tableLookupOperationCycles = 39 + mainVecs * 8;
   tableLookupTearDownCycles  = 13;

   tableLookupOverheadCycles += 17 + 2;
   tableLookupOperationCycles += (11 + remVecs * 2);
#endif
   *archCycles = tableLookupOperationCycles;
   *estCycles =
       tableLookupStartupCycles + tableLookupOperationCycles + tableLookupOverheadCycles + tableLookupTearDownCycles;
}

#if defined(__C7524__)

void AUDIOLIB_tableLookup_set_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn1)
{
   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;
   uint32_t                       tableSize    = pKerPrivArgs->tableSamples;
   uint32_t *restrict pTableLocal              = (uint32_t *) pIn1;

   for (uint32_t i = 0; i < tableSize; i++) {
      __ilut_init((uint32_t) i, (uint32_t) pTableLocal[i]);
   }
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableLookup_init_ci(AUDIOLIB_kernelHandle                handle,
                                             const AUDIOLIB_bufParams1D_t        *bufParamsIn0,
                                             const AUDIOLIB_bufParams1D_t        *bufParamsIn1,
                                             const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                             const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS status                      = AUDIOLIB_SUCCESS;
   __ILTER                                     = __ILUT_RW;
   __ILTCR                                     = __ILUT_SIGNED;
   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;
   uint8_t                       *pBlock       = pKerPrivArgs->bufPblock;

   int32_t eleCount = c7x::element_count_of<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

   se0Params.ICNT0   = pKerPrivArgs->srcSamples;
   se0Params.ELETYPE = SE_ELETYPE;
   se0Params.VECLEN  = SE_VECLEN;
   se0Params.DIMFMT  = __SE_DIMFMT_1D;

   sa0Params.ICNT0  = pKerPrivArgs->srcSamples;
   sa0Params.VECLEN = SA_VECLEN;
   sa0Params.DIMFMT = __SA_DIMFMT_1D;

   pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(pKerPrivArgs->srcSamples, eleCount);

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_tableLookup_init_ci<float>(AUDIOLIB_kernelHandle                handle,
                                                             const AUDIOLIB_bufParams1D_t        *bufParamsIn0,
                                                             const AUDIOLIB_bufParams1D_t        *bufParamsIn1,
                                                             const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                                             const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_tableLookup_init_ci<double>(AUDIOLIB_kernelHandle                handle,
                                                              const AUDIOLIB_bufParams1D_t        *bufParamsIn0,
                                                              const AUDIOLIB_bufParams1D_t        *bufParamsIn1,
                                                              const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                                              const AUDIOLIB_tableLookup_InitArgs *pKerInitArgs);

// /*******************************************************************************
//  *
//  * EXECUTION FUNCTIONS
//  *
//  ******************************************************************************/

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableLookup_exec_ci(AUDIOLIB_kernelHandle handle,
                                             void *restrict pIn0,
                                             void *restrict pIn1,
                                             void *restrict pOut)
{

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_tableLookup_PrivArgs *pKerPrivArgs = (AUDIOLIB_tableLookup_PrivArgs *) handle;
   uint8_t                       *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                       nVecs        = pKerPrivArgs->nVecs;
   float                          minVal       = pKerPrivArgs->minVal;
   float                          divisor      = pKerPrivArgs->divisor;
   uint32_t                       tableSamples = pKerPrivArgs->tableSamples;

   dataType *restrict pInLocalSrc  = (dataType *) pIn0;
   dataType *restrict pOutLocalOut = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   __SE0_OPEN(pInLocalSrc, se0Params);
   __SA0_OPEN(sa0Params);

   vec minVec     = (vec) minVal;
   vec divisorVec = (vec) divisor;
   vec roundVec   = (vec) (tableSamples - 1);

   // 20 + trip_cnt * 5
   for (uint32_t i = 0; i < nVecs; i += 2) {

      vec srcVec = c7x::strm_eng<0, vec>::get_adv();

      srcVec = srcVec - minVec;
      srcVec = srcVec * divisorVec;
      srcVec = srcVec + (vec) 0.5;

      //   check for the minimum condition,replacing with table index 0
      __vpred result = __cmp_le_pred(srcVec, vec(0));
      srcVec         = __select(result, vec(0), srcVec);

      result = __cmp_le_pred(srcVec, roundVec);
      srcVec = __select(result, srcVec, roundVec);

      c7x::int_vec index = __float_to_int_rtz(srcVec);

      // reading the values from the LUT table
      c7x::int_vec   read_values       = __ilut_read_int(c7x::as_uint_vec(index));
      c7x::float_vec float_read_values = c7x::reinterpret<c7x::float_vec>(read_values);

      // storing the vector to the output
      __vpred outPred = c7x::strm_agen<0, vec>::get_vpred();
      vec    *outPtr  = c7x::strm_agen<0, vec>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, float_read_values);

      srcVec = c7x::strm_eng<0, vec>::get_adv();

      srcVec = srcVec - minVec;
      srcVec = srcVec * divisorVec;
      srcVec = srcVec + (vec) 0.5;

      //   check for the minimum condition,replacing with table index 0
      result = __cmp_le_pred(srcVec, vec(0));
      srcVec = __select(result, vec(0), srcVec);

      result = __cmp_le_pred(srcVec, roundVec);
      srcVec = __select(result, srcVec, roundVec);

      index = __float_to_int_rtz(srcVec);

      // reading the values from the LUT table
      read_values       = __ilut_read_int(c7x::as_uint_vec(index));
      float_read_values = c7x::reinterpret<c7x::float_vec>(read_values);

      // storing the vector to the output
      outPred = c7x::strm_agen<0, vec>::get_vpred();
      outPtr  = c7x::strm_agen<0, vec>::get_adv(pOutLocalOut);
      __vstore_pred(outPred, outPtr, float_read_values);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_tableLookup_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn0,
                                                             void *restrict pIn1,
                                                             void *restrict pOut);

#endif
