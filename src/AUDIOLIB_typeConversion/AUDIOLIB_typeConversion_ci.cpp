// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_typeConversion_priv.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA0_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SE_PARAM_SIZE)
#define REM_ELEMENTS_OFFSET (SE_SA1_PARAM_OFFSET + SE_PARAM_SIZE)

template <typename T> struct c7x_vec_type;
template <> struct c7x_vec_type<double> {
   using type = c7x::double_vec;
};

template <> struct c7x_vec_type<float> {
   using type = c7x::float_vec;
};

template <typename VecT> struct c7x_vec_cast;

template <> struct c7x_vec_cast<c7x::float_vec> {
   template <typename SrcT> static c7x::float_vec convert(const SrcT &val) { return c7x::as_float_vec(val); }
};

template <> struct c7x_vec_cast<c7x::double_vec> {
   template <typename SrcT> static c7x::double_vec convert(const SrcT &val) { return c7x::as_double_vec(val); }
};

// Default template (for double or other types)
template <typename T> constexpr AUDIOLIB_data_type_e getDataTypeEnum() { return AUDIOLIB_FLOAT64; }

// Specialization for float
template <> constexpr AUDIOLIB_data_type_e getDataTypeEnum<float>() { return AUDIOLIB_FLOAT32; }

void AUDIOLIB_typeConversion_perfEst(AUDIOLIB_kernelHandle handle,
                                     uint64_t             *archCycles,
                                     uint64_t             *estCycles,
                                     uint32_t              inDataType,
                                     uint32_t              outDataType)
{

   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   uint8_t                          *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                          remElements  = *(uint32_t *) ((uint8_t *) pBlock + REM_ELEMENTS_OFFSET);
   uint32_t                          nVecs        = pKerPrivArgs->nVecs;

   uint64_t typeConversionStartupCycles   = 0;
   uint64_t typeConversionTeardownCycles  = 0;
   uint64_t typeConversionOperationCycles = 0;
   uint64_t typeConversionOverheadCycles  = 0;

   if (inDataType == AUDIOLIB_INT16) {
      typeConversionStartupCycles   = 24;
      typeConversionOperationCycles = 7 + nVecs * 1;
      typeConversionTeardownCycles  = 3;
   }

   else if (inDataType == AUDIOLIB_FLOAT32 && outDataType == AUDIOLIB_INT16) {
      typeConversionStartupCycles = 14 + 15;

      typeConversionStartupCycles += 26;
      typeConversionOperationCycles += (7 + remElements * 1);
      typeConversionTeardownCycles = 3;

      typeConversionStartupCycles += 26;
      typeConversionOperationCycles += 7 + nVecs * 1;
      typeConversionTeardownCycles = 3;
   }

   else if (inDataType == AUDIOLIB_FLOAT64 && outDataType == AUDIOLIB_INT16) {
      typeConversionStartupCycles = 14 + 15;

      typeConversionStartupCycles += 26;
      typeConversionOperationCycles += (13 + nVecs * 2);
      typeConversionTeardownCycles += 3;

      typeConversionStartupCycles += 26;
      typeConversionOperationCycles += (13 + remElements * 1);
      typeConversionTeardownCycles += 3;
   }

   else if (inDataType == AUDIOLIB_INT32 || inDataType == AUDIOLIB_FLOAT64) {
      typeConversionStartupCycles = 24 + 4;

      typeConversionOperationCycles = 7 + nVecs * 1;

      typeConversionTeardownCycles = 2;
   }

   else if (inDataType == AUDIOLIB_FLOAT32 || inDataType == AUDIOLIB_FLOAT64) {

      typeConversionStartupCycles = 23 + 4;
      nVecs                       = pKerPrivArgs->nVecs;

      typeConversionOperationCycles = 9 + nVecs * 1;

      typeConversionTeardownCycles = 3;
   }

   typeConversionOverheadCycles += typeConversionStartupCycles + typeConversionTeardownCycles;
   *estCycles  = typeConversionOperationCycles + typeConversionOverheadCycles;
   *archCycles = typeConversionOperationCycles;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_q23ToFloat_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs                      *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_data_type_e dataVal = getDataTypeEnum<dataType>();

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_q23ToFloat_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d samples %d channels %d\n",
                         pKerPrivArgs->samples, pKerPrivArgs->channels, pKerPrivArgs->strideIn, eleCount,
                         pKerPrivArgs->samples, pKerPrivArgs->channels);

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/

   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<c7x::int_vec>::value;
   __SE_VECLEN  SE_VECLEN1 = c7x::se_veclen<c7x::long_vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<c7x::int_vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   se0Params.VECLEN  = SE_VECLEN;
   sa0Params.VECLEN  = SA_VECLEN;
   se0Params.ELETYPE = SE_ELETYPE;

   if (dataVal == AUDIOLIB_FLOAT64) {
      se0Params.PROMOTE = __SE_PROMOTE_2X_SIGNEXT;
      se0Params.VECLEN  = SE_VECLEN1;
   }

   if (pKerPrivArgs->strideIn == pKerPrivArgs->samples && pKerPrivArgs->strideOut == pKerPrivArgs->samples) {

      se0Params.ICNT0  = pKerPrivArgs->channels * pKerPrivArgs->samples;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0     = pKerPrivArgs->channels * pKerPrivArgs->samples;
      sa0Params.DIMFMT    = __SA_DIMFMT_1D;
      pKerPrivArgs->nVecs = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples * pKerPrivArgs->channels, eleCount);
   }
   else {

      se0Params.ICNT0  = (pKerPrivArgs->samples);
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.ICNT1  = pKerPrivArgs->channels;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = (pKerPrivArgs->samples);
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->channels;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      pKerPrivArgs->nVecs = pKerPrivArgs->channels * AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q23 to FLOAT32 init function ");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floatToQ23_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs                      *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_q23ToFloat_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d samples %d channels %d\n",
                         pKerPrivArgs->samples, pKerPrivArgs->channels, pKerPrivArgs->strideIn, eleCount,
                         pKerPrivArgs->samples, pKerPrivArgs->channels);

   AUDIOLIB_data_type_e dataVal = getDataTypeEnum<dataType>();

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/

   __SA_VECLEN  SA_VECLEN;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   se0Params.VECLEN  = SE_VECLEN;
   se0Params.ELETYPE = SE_ELETYPE;

   if (dataVal == AUDIOLIB_FLOAT64) {
      SA_VECLEN = c7x::sa_veclen<c7x::int_hvec>::value;
   }
   else {
      SA_VECLEN = c7x::sa_veclen<c7x::int_vec>::value;
   }
   if (pKerPrivArgs->samples == pKerPrivArgs->strideOut && pKerPrivArgs->samples == pKerPrivArgs->strideIn) {

      se0Params.ICNT0  = pKerPrivArgs->samples * pKerPrivArgs->channels;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0  = pKerPrivArgs->samples * pKerPrivArgs->channels;
      sa0Params.DIMFMT = __SA_DIMFMT_1D;
      sa0Params.VECLEN = SA_VECLEN;

      pKerPrivArgs->nVecs = (AUDIOLIB_ceilingDiv(pKerPrivArgs->samples * pKerPrivArgs->channels, eleCount));
   }
   else {

      se0Params.ICNT0         = (pKerPrivArgs->samples > eleCount) ? eleCount : pKerPrivArgs->samples;
      se0Params.DIM1          = pKerPrivArgs->strideIn;
      se0Params.ICNT1         = pKerPrivArgs->channels;
      se0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
      se0Params.DIM2          = eleCount;
      se0Params.DECDIM1_WIDTH = pKerPrivArgs->samples;
      se0Params.DECDIM1       = __SE_DECDIM_DIM2;
      se0Params.DIMFMT        = __SE_DIMFMT_3D;

      sa0Params.ICNT0         = (pKerPrivArgs->samples > eleCount) ? eleCount : pKerPrivArgs->samples;
      sa0Params.DIM1          = pKerPrivArgs->strideOut;
      sa0Params.ICNT1         = (pKerPrivArgs->channels);
      sa0Params.ICNT2         = AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
      sa0Params.DIM2          = eleCount;
      sa0Params.DIMFMT        = __SA_DIMFMT_3D;
      sa0Params.DECDIM1_WIDTH = pKerPrivArgs->samples;
      sa0Params.DECDIM1       = __SA_DECDIM_DIM2;
      sa0Params.VECLEN        = SA_VECLEN;

      pKerPrivArgs->nVecs = pKerPrivArgs->channels * AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT32 to Q23 init function ");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_q15ToFloat_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs                      *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   uint8_t                                               *pBlock   = pKerPrivArgs->bufPblock;
   uint32_t                                               eleCount = c7x::element_count_of<vec>::value;

   AUDIOLIB_data_type_e dataVal = getDataTypeEnum<dataType>();

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_q15ToFloat_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d samples %d channels %d\n",
                         pKerPrivArgs->samples, pKerPrivArgs->channels, pKerPrivArgs->strideIn, eleCount,
                         pKerPrivArgs->samples, pKerPrivArgs->channels);

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/

   __SE_VECLEN SE_VECLEN = c7x::se_veclen<vec>::value;
   __SA_VECLEN SA_VECLEN = c7x::sa_veclen<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();

   se0Params.VECLEN = SE_VECLEN;
   sa0Params.VECLEN = SA_VECLEN;

   if (dataVal == AUDIOLIB_FLOAT32) {
      typedef typename c7x::short_hvec vecIn;
      __SE_ELETYPE                     SE_ELETYPE = c7x::se_eletype<vecIn>::value;
      se0Params.PROMOTE                           = __SE_PROMOTE_2X_SIGNEXT;
      se0Params.ELETYPE                           = SE_ELETYPE;
   }
   else {
      typedef typename c7x::short_qvec vecIn;
      __SE_ELETYPE                     SE_ELETYPE = c7x::se_eletype<vecIn>::value;
      se0Params.PROMOTE                           = __SE_PROMOTE_4X_SIGNEXT;
      se0Params.ELETYPE                           = SE_ELETYPE;
   }

   if (pKerPrivArgs->strideIn == pKerPrivArgs->samples && pKerPrivArgs->strideOut == pKerPrivArgs->samples) {

      se0Params.ICNT0  = pKerPrivArgs->channels * pKerPrivArgs->samples;
      se0Params.DIMFMT = __SE_DIMFMT_1D;

      sa0Params.ICNT0     = pKerPrivArgs->channels * pKerPrivArgs->samples;
      sa0Params.DIMFMT    = __SA_DIMFMT_1D;
      pKerPrivArgs->nVecs = pKerPrivArgs->channels * AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   }
   else {
      se0Params.ICNT0  = pKerPrivArgs->samples;
      se0Params.DIM1   = pKerPrivArgs->strideIn;
      se0Params.ICNT1  = pKerPrivArgs->channels;
      se0Params.DIMFMT = __SE_DIMFMT_2D;

      sa0Params.ICNT0  = pKerPrivArgs->samples;
      sa0Params.DIM1   = pKerPrivArgs->strideOut;
      sa0Params.ICNT1  = pKerPrivArgs->channels;
      sa0Params.DIMFMT = __SA_DIMFMT_2D;

      pKerPrivArgs->nVecs = pKerPrivArgs->channels * AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   }

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q15 to FLOAT32 init function ");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floatToQ15_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs                      *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   typedef typename c7x::make_full_vector<dataType>::type vec;
   typedef typename c7x::make_full_vector<c7x::short_vec>::type outVec;
   uint8_t                                                     *pBlock      = pKerPrivArgs->bufPblock;
   uint32_t                                                     eleCount    = c7x::element_count_of<vec>::value;
   uint32_t                                                     remElements = 0;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floatToQ15_init_ci \n");
   AUDIOLIB_DEBUGPRINTFN(0, "dim_x %d dim_y %d strideElements %d eleCount %d samples %d channels %d\n",
                         pKerPrivArgs->samples, pKerPrivArgs->channels, pKerPrivArgs->strideIn, eleCount,
                         pKerPrivArgs->samples, pKerPrivArgs->channels);

   /**********************************************************************/
   /* Prepare SE/SA template                                             */
   /**********************************************************************/
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<outVec>::value;
   __SA_VECLEN  SA_VECLEN1 = c7x::sa_veclen<c7x::short_hvec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;

   __SE_TEMPLATE_v1 se0Params = __gen_SE_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1Params = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0Params = __gen_SA_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1Params = __gen_SA_TEMPLATE_v1();

   se0Params.VECLEN  = SE_VECLEN;
   se1Params.VECLEN  = SE_VECLEN;
   se0Params.ELETYPE = SE_ELETYPE;
   se1Params.ELETYPE = SE_ELETYPE;

   sa0Params.VECLEN = SA_VECLEN1;
   sa1Params.VECLEN = SA_VECLEN;

   se0Params.ICNT0  = pKerPrivArgs->samples % (2 * eleCount);
   se0Params.DIM1   = pKerPrivArgs->strideIn;
   se0Params.ICNT1  = pKerPrivArgs->channels;
   se0Params.DIMFMT = __SE_DIMFMT_2D;

   sa0Params.ICNT0  = pKerPrivArgs->samples % (2 * eleCount);
   sa0Params.DIM1   = pKerPrivArgs->strideOut;
   sa0Params.ICNT1  = pKerPrivArgs->channels;
   sa0Params.DIMFMT = __SA_DIMFMT_2D;

   se1Params.ICNT0  = eleCount;
   se1Params.DIM1   = 2 * eleCount;
   se1Params.ICNT1  = (pKerPrivArgs->samples > 2 * eleCount) ? pKerPrivArgs->samples / (2 * eleCount)
                                                             : AUDIOLIB_ceilingDiv(pKerPrivArgs->samples, eleCount);
   se1Params.DIM2   = pKerPrivArgs->strideIn;
   se1Params.ICNT2  = pKerPrivArgs->channels;
   se1Params.DIMFMT = __SE_DIMFMT_3D;

   sa1Params.ICNT0  = 2 * eleCount;
   sa1Params.DIM1   = 2 * eleCount;
   sa1Params.ICNT1  = pKerPrivArgs->samples / (2 * eleCount);
   sa1Params.DIM2   = pKerPrivArgs->strideOut;
   sa1Params.ICNT2  = pKerPrivArgs->channels;
   sa1Params.DIMFMT = __SA_DIMFMT_3D;

   pKerPrivArgs->nVecs = pKerPrivArgs->channels * (pKerPrivArgs->samples / (2 * eleCount));
   remElements = pKerPrivArgs->channels * AUDIOLIB_ceilingDiv(pKerPrivArgs->samples % (2 * eleCount), (eleCount));

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET) = se0Params;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET) = se1Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET) = sa0Params;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET) = sa1Params;
   *(uint32_t *) ((uint8_t *) pBlock + REM_ELEMENTS_OFFSET)         = remElements;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT32 to Q15 init function ");

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_q15ToFloat_init_ci<float>(AUDIOLIB_kernelHandle                   handle,
                                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_q15ToFloat_init_ci<double>(AUDIOLIB_kernelHandle                   handle,
                                                             const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                                             const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                                             const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_floatToQ15_init_ci<float>(AUDIOLIB_kernelHandle                   handle,
                                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_floatToQ15_init_ci<double>(AUDIOLIB_kernelHandle                   handle,
                                                             const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                                             const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                                             const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_q23ToFloat_init_ci<float>(AUDIOLIB_kernelHandle                   handle,
                                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_q23ToFloat_init_ci<double>(AUDIOLIB_kernelHandle                   handle,
                                                             const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                                             const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                                             const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_floatToQ23_init_ci<float>(AUDIOLIB_kernelHandle                   handle,
                                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

template AUDIOLIB_STATUS AUDIOLIB_floatToQ23_init_ci<double>(AUDIOLIB_kernelHandle                   handle,
                                                             const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                                             const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                                             const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

template <typename dataTypeOut>
inline void AUDIOLIB_q15tofl_convert(dataTypeOut *restrict pOutLocal, int32_t blockSize);

template <typename dataTypeOut>
inline void AUDIOLIB_q23tofl_convert(AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs, dataTypeOut *restrict pOutLocal);

template <typename dataTypeOut>
inline void AUDIOLIB_q31tofl_convert(AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs, dataTypeOut *restrict pOutLocal);

template <typename dataTypeIn>
inline void AUDIOLIB_fltoq15_convert(AUDIOLIB_kernelHandle handle,
                                     void *restrict pIn,
                                     void *restrict pOut,
                                     __SE_TEMPLATE_v1 se0Params,
                                     __SA_TEMPLATE_v1 sa0Params,
                                     uint32_t         blockSize);

template <typename dataTypeIn>
inline void AUDIOLIB_fltoq15SmallSamples_convert(AUDIOLIB_kernelHandle handle,
                                                 void *restrict pIn,
                                                 void *restrict pOut,
                                                 __SE_TEMPLATE_v1 se0Params,
                                                 __SA_TEMPLATE_v1 sa0Params,
                                                 uint32_t         blockSize);

template <typename dataTypeIn>
inline void AUDIOLIB_fltoq23_convert(AUDIOLIB_kernelHandle handle,
                                     void *restrict pIn,
                                     void *restrict pOut,
                                     __SE_TEMPLATE_v1 se0Params,
                                     __SA_TEMPLATE_v1 sa0Params,
                                     uint32_t         blockSize);

template <typename dataTypeIn>
inline void AUDIOLIB_fltoq31_convert(AUDIOLIB_kernelHandle handle,
                                     void *restrict pIn,
                                     void *restrict pOut,
                                     __SE_TEMPLATE_v1 se0Params,
                                     __SA_TEMPLATE_v1 sa0Params,
                                     uint32_t         blockSize);

template <>
inline void AUDIOLIB_fltoq15_convert<float>(AUDIOLIB_kernelHandle handle,
                                            void *restrict pIn,
                                            void *restrict pOut,
                                            __SE_TEMPLATE_v1 se0Params,
                                            __SA_TEMPLATE_v1 sa0Params,
                                            uint32_t         loopCount)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for FLOAT32 to Q15 conversion");

   float *restrict pInLocal    = (float *) pIn;
   int16_t *restrict pOutLocal = (int16_t *) pOut;

   typedef typename c7x::make_full_vector<float>::type vecIn;
   typedef typename c7x::short_vec                     vecOut;

   int32_t eleCount = c7x::element_count_of<vecIn>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "pInLocal: %p pOutLocal: %p blockSize: %d loopCount: %d remainingElements: %d\n", pInLocal,
                         pOutLocal, blockSize, loopCount, remainingElements);

   vecIn mulFactor = (vecIn) Q15_SCALE_FACTOR;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + eleCount, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < loopCount; i++) {
      vecIn loadVec1 = c7x::strm_eng<0, vecIn>::get_adv();
      vecIn loadVec2 = c7x::strm_eng<1, vecIn>::get_adv();

      vecIn  mulLoadVec1   = loadVec1 * mulFactor;
      vecIn  mulLoadVec2   = loadVec2 * mulFactor;
      vecOut intermediate1 = __float_to_short(mulLoadVec1);
      vecOut intermediate2 = __float_to_short(mulLoadVec2);

      c7x::short_vec *addr = c7x::strm_agen<0, c7x::short_vec>::get_adv(pOutLocal);
      __vstore_packl_2src(addr, c7x::as_int_vec(intermediate1), c7x::as_int_vec(intermediate2));
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT32 to Q15 ");
}

template <>
inline void AUDIOLIB_fltoq15SmallSamples_convert<float>(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pOut,
                                                        __SE_TEMPLATE_v1 se0Params,
                                                        __SA_TEMPLATE_v1 sa0Params,
                                                        uint32_t         blockSize)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for FLOAT32 to Q15 small channel conversion");

   float *restrict pInLocal    = (float *) pIn;
   int16_t *restrict pOutLocal = (int16_t *) pOut;

   typedef typename c7x::make_full_vector<float>::type vecIn;
   typedef typename c7x::short_vec                     vecOut;

   int32_t                           eleCount     = c7x::element_count_of<vecIn>::value;
   int32_t                           loopCount    = blockSize;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   int32_t i;

   AUDIOLIB_DEBUGPRINTFN(0, "pInLocal: %p pOutLocal: %p blockSize: %d loopCount: %d remainingElements: %d\n", pInLocal,
                         pOutLocal, blockSize, loopCount, remainingElements);

   vecIn mulFactor = (vecIn) Q15_SCALE_FACTOR;

   __SE0_OPEN(pInLocal + (pKerPrivArgs->samples - (pKerPrivArgs->samples % (2 * eleCount))), se0Params);
   __SA0_OPEN(sa0Params);
   pOutLocal += (pKerPrivArgs->samples - (pKerPrivArgs->samples % (2 * eleCount)));

   for (i = 0; i < loopCount; i++) {
      vecIn  loadVec1      = c7x::strm_eng<0, vecIn>::get_adv();
      vecIn  mulLoadVec1   = loadVec1 * mulFactor;
      vecOut intermediate1 = __float_to_short(mulLoadVec1);

      __vpred vpred = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *addr  = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred_packl_2src(vpred, addr, c7x::as_int_vec(intermediate1), c7x::int_vec(0));
   }

   __SE0_CLOSE();
   __SA0_CLOSE();
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT32 to Q15 small channelconversion");
}
template <>
inline void AUDIOLIB_fltoq15SmallSamples_convert<double>(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pOut,
                                                         __SE_TEMPLATE_v1 se0Params,
                                                         __SA_TEMPLATE_v1 sa0Params,
                                                         uint32_t         blockSize)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for FLOAT64 to Q15 small samples conversion");

   double *restrict pInLocal   = (double *) pIn;
   int16_t *restrict pOutLocal = (int16_t *) pOut;

   typedef typename c7x::make_full_vector<double>::type vecIn;

   int32_t                           eleCount     = c7x::element_count_of<vecIn>::value;
   int32_t                           loopCount    = blockSize;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   int32_t i;

   AUDIOLIB_DEBUGPRINTFN(0, "pInLocal: %p pOutLocal: %p blockSize: %d loopCount: %d remainingElements: %d\n", pInLocal,
                         pOutLocal, blockSize, loopCount, remainingElements);

   vecIn mulFactor = (vecIn) Q15_SCALE_FACTOR;

   __SE0_OPEN(pInLocal + (pKerPrivArgs->samples - (pKerPrivArgs->samples % (2 * eleCount))), se0Params);
   __SA0_OPEN(sa0Params);
   pOutLocal += (pKerPrivArgs->samples - (pKerPrivArgs->samples % (2 * eleCount)));

   for (i = 0; i < loopCount; i++) {

      vecIn loadVec1 = c7x::strm_eng<0, vecIn>::get_adv();

      loadVec1              = loadVec1 * mulFactor;
      c7x::int_vec dpToInt1 = __double_to_int(loadVec1);

      c7x::int_vec intermediate1 = (__int_to_short_sat((dpToInt1)));
      intermediate1              = __pack_consec_low(intermediate1, intermediate1);

      __vpred          vpred = c7x::strm_agen<0, c7x::int_vec>::get_vpred();
      c7x::short_hvec *addr  = c7x::strm_agen<0, c7x::short_hvec>::get_adv(pOutLocal);
      __vstore_pred_packl(vpred, addr, intermediate1);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT64 to Q15 small channelconversion");
}

template <>
inline void AUDIOLIB_fltoq15_convert<double>(AUDIOLIB_kernelHandle handle,
                                             void *restrict pIn,
                                             void *restrict pOut,
                                             __SE_TEMPLATE_v1 se0Params,
                                             __SA_TEMPLATE_v1 sa0Params,
                                             uint32_t         blockSize)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for FLOAT64 to Q15 conversion");

   double *restrict pInLocal   = (double *) pIn;
   int16_t *restrict pOutLocal = (int16_t *) pOut;

   typedef typename c7x::make_full_vector<double>::type vecIn;

   int32_t eleCount  = c7x::element_count_of<vecIn>::value;
   int32_t loopCount = blockSize;

   int32_t i;

   AUDIOLIB_DEBUGPRINTFN(0, "pInLocal: %p pOutLocal: %p blockSize: %d loopCount: %d remainingElements: %d\n", pInLocal,
                         pOutLocal, blockSize, loopCount, remainingElements);

   vecIn mulFactor = (vecIn) Q15_SCALE_FACTOR;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + eleCount, se0Params);
   __SA0_OPEN(sa0Params);

   for (i = 0; i < loopCount; i++) {

      vecIn loadVec1 = c7x::strm_eng<0, vecIn>::get_adv();
      vecIn loadVec2 = c7x::strm_eng<1, vecIn>::get_adv();

      loadVec1 = loadVec1 * mulFactor;
      loadVec2 = loadVec2 * mulFactor;

      c7x::int_vec dpToInt1 = __double_to_int(loadVec1);
      c7x::int_vec dpToInt2 = __double_to_int(loadVec2);

      c7x::int_vec pack1 = __pack_consec_low(dpToInt2, dpToInt1);

      pack1 = __int_to_short_sat(pack1);

      c7x::short_hvec *addr = c7x::strm_agen<0, c7x::short_hvec>::get_adv(pOutLocal);
      __vstore_packl(addr, c7x::as_int_vec(pack1));
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT64 to Q15 conversion");
}

template <> inline void AUDIOLIB_q15tofl_convert<float>(float *restrict pOutLocal, int32_t blockSize)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for Q15 to FLOAT conversion");

   typedef typename c7x::int_vec                       vecLoad;
   typedef typename c7x::make_full_vector<float>::type vecOut;
   vecOut                                              outDat, scale;

   scale = (vecOut) Q15_SCALE_FACTOR_INV;

   for (int32_t counter = 0; counter < blockSize; counter += 1) {
      vecLoad inDat = c7x::strm_eng<0, vecLoad>::get_adv();

      vecOut inDatConv = __int_to_float(c7x::as_int_vec(inDat));
      outDat           = inDatConv * scale;

      /* Store the result */
      __vpred vpred = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *addr  = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, outDat);
   }

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT to Q15 conversion");
}

template <> inline void AUDIOLIB_q15tofl_convert<double>(double *restrict pOutLocal, int32_t nVecs)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for Q15 to FLOAT64 conversion");

   typedef typename c7x::long_vec                       vecLoad;
   typedef typename c7x::make_full_vector<double>::type vecOut;
   vecOut                                               outDat, scale;

   scale = (vecOut) Q15_SCALE_FACTOR_INV;

   for (int32_t counter = 0; counter < nVecs; counter += 1) {
      vecLoad inDat     = c7x::strm_eng<0, vecLoad>::get_adv();
      vecOut  inDatConv = __low_int_to_double(c7x::as_int_vec(inDat));
      outDat            = inDatConv * scale;

      /* Store the result */
      __vpred vpred = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *addr  = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, outDat);
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q15 to FLOAT64 conversion");
}

template <>
inline void AUDIOLIB_q23tofl_convert<float>(AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs, float *restrict pOutLocal)
{

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for Q23 to FLOAT conversion");

   typedef typename c7x::int_vec                       vecLoad;
   typedef typename c7x::make_full_vector<float>::type vecOut;
   vecOut                                              outDat, scale;

   scale = (vecOut) Q23_SCALE_FACTOR_INV;

   for (uint32_t counter = 0; counter < pKerPrivArgs->nVecs; counter += 1) {
      vecLoad inDat = c7x::strm_eng<0, vecLoad>::get_adv();

      vecOut inDatConv = __int_to_float(c7x::as_int_vec(inDat));
      outDat           = inDatConv * scale;

      /* Store the result */
      __vpred vpred = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *addr  = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, outDat);
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q23 to FLOAT conversion");
}

template <>
inline void AUDIOLIB_q23tofl_convert<double>(AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs, double *restrict pOutLocal)
{

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for Q23 to FLOAT64 conversion");

   typedef typename c7x::int_vec                        vecLoad;
   typedef typename c7x::make_full_vector<double>::type vecOut;
   vecOut                                               scale = (vecOut) Q23_SCALE_FACTOR_INV;

   for (uint32_t counter = 0; counter < pKerPrivArgs->nVecs; counter += 1) {
      vecLoad inDat         = c7x::strm_eng<0, vecLoad>::get_adv();
      vecOut  inDatConvHigh = __low_int_to_double(c7x::as_int_vec(inDat));

      vecOut outDatHigh = inDatConvHigh * scale;

      __vpred predTemp  = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *pStoreVec = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred(predTemp, pStoreVec, (outDatHigh));
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q23 to FLOAT64 conversion");
}

template <>
inline void AUDIOLIB_q31tofl_convert<float>(AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs, float *restrict pOutLocal)
{

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for Q31 to FLOAT conversion");

   typedef typename c7x::int_vec                       vecLoad;
   typedef typename c7x::make_full_vector<float>::type vecOut;
   vecOut                                              outDat, scale;

   scale = (vecOut) Q31_SCALE_FACTOR_INV;

   for (uint32_t counter = 0; counter < pKerPrivArgs->nVecs; counter += 1) {
      vecLoad inDat = c7x::strm_eng<0, vecLoad>::get_adv();

      vecOut inDatConv = __int_to_float(c7x::as_int_vec(inDat));
      outDat           = inDatConv * scale;

      /* Store the result */
      __vpred vpred = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *addr  = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, outDat);
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q31 to FLOAT conversion");
}

template <>
inline void AUDIOLIB_q31tofl_convert<double>(AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs, double *restrict pOutLocal)
{

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for Q31 to FLOAT64 conversion");

   typedef typename c7x::int_vec                        vecLoad;
   typedef typename c7x::make_full_vector<double>::type vecOut;
   vecOut                                               scale = (vecOut) Q31_SCALE_FACTOR_INV;

   for (uint32_t counter = 0; counter < pKerPrivArgs->nVecs; counter += 1) {
      vecLoad inDat         = c7x::strm_eng<0, vecLoad>::get_adv();
      vecOut  inDatConvHigh = __low_int_to_double(c7x::as_int_vec(inDat));

      vecOut outDatHigh = inDatConvHigh * scale;

      __vpred predTemp  = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *pStoreVec = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred(predTemp, pStoreVec, (outDatHigh));
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q31 to FLOAT64 conversion");
}

template <>
inline void AUDIOLIB_fltoq23_convert<float>(AUDIOLIB_kernelHandle handle,
                                            void *restrict pIn,
                                            void *restrict pOut,
                                            __SE_TEMPLATE_v1 se0Params,
                                            __SA_TEMPLATE_v1 sa0Params,
                                            uint32_t         loopCount)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for FLOAT32 to Q23 conversion");

   float *restrict pInLocal    = (float *) pIn;
   int32_t *restrict pOutLocal = (int32_t *) pOut;

   typedef typename c7x::make_full_vector<float>::type vecIn;
   typedef typename c7x::int_vec                       vecOut;

   AUDIOLIB_DEBUGPRINTFN(0, "pInLocal: %p pOutLocal: %p blockSize: %d loopCount: %d remainingElements: %d\n", pInLocal,
                         pOutLocal, blockSize, loopCount, remainingElements);

   vecIn mulFactor = (vecIn) Q23_SCALE_FACTOR;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   for (uint32_t i = 0; i < loopCount; i++) {
      vecIn loadVec1 = c7x::strm_eng<0, vecIn>::get_adv();

      vecIn mulLoadVec1 = loadVec1 * mulFactor;

      vecOut vecLoad = __float_to_int(mulLoadVec1);
      vecLoad        = __vgsatw_vkv(vecLoad, (c7x::uchar_qvec) 24);

      __vpred vpred = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *addr  = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, vecLoad);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT to Q23 conversion");
}

template <>
inline void AUDIOLIB_fltoq23_convert<double>(AUDIOLIB_kernelHandle handle,
                                             void *restrict pIn,
                                             void *restrict pOut,
                                             __SE_TEMPLATE_v1 se0Params,
                                             __SA_TEMPLATE_v1 sa0Params,
                                             uint32_t         loopCount)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for FLOAT64 to Q23 conversion");

   double *restrict pInLocal   = (double *) pIn;
   int32_t *restrict pOutLocal = (int32_t *) pOut;

   typedef typename c7x::make_full_vector<double>::type vecIn;
   typedef typename c7x::int_vec                        vecOut;

   AUDIOLIB_DEBUGPRINTFN(0, "pInLocal: %p pOutLocal: %p blockSize: %d loopCount: %d remainingElements: %d\n", pInLocal,
                         pOutLocal, blockSize, loopCount, remainingElements);

   vecIn mulFactor = (vecIn) Q23_SCALE_FACTOR;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < loopCount; i++) {
      vecIn          loadVec1      = c7x::strm_eng<0, vecIn>::get_adv();
      vecIn          mulLoadVec1   = loadVec1 * mulFactor;
      vecOut         intermediate1 = __double_to_int(mulLoadVec1);
      vecOut         vecLoad       = __vgsatw_vkv(intermediate1, (c7x::uchar_qvec) 24);
      __vpred        predTemp      = c7x::strm_agen<0, c7x::long_vec>::get_vpred();
      c7x::int_hvec *addr          = c7x::strm_agen<0, c7x::int_hvec>::get_adv(pOutLocal);
      __vstore_pred_packl(predTemp, addr, c7x::as_long_vec(vecLoad));
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT64 to Q23 conversion");
}

template <>
inline void AUDIOLIB_fltoq31_convert<float>(AUDIOLIB_kernelHandle handle,
                                            void *restrict pIn,
                                            void *restrict pOut,
                                            __SE_TEMPLATE_v1 se0Params,
                                            __SA_TEMPLATE_v1 sa0Params,
                                            uint32_t         loopCount)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for FLOAT32 to Q31 conversion");

   float *restrict pInLocal    = (float *) pIn;
   int32_t *restrict pOutLocal = (int32_t *) pOut;

   typedef typename c7x::make_full_vector<float>::type vecIn;
   typedef typename c7x::int_vec                       vecOut;

   AUDIOLIB_DEBUGPRINTFN(0, "pInLocal: %p pOutLocal: %p blockSize: %d loopCount: %d remainingElements: %d\n", pInLocal,
                         pOutLocal, blockSize, loopCount, remainingElements);

   vecIn mulFactor = (vecIn) Q31_SCALE_FACTOR;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);
   for (uint32_t i = 0; i < loopCount; i++) {
      vecIn loadVec1 = c7x::strm_eng<0, vecIn>::get_adv();

      vecIn mulLoadVec1 = loadVec1 * mulFactor;

      vecOut vecLoad = __float_to_int(mulLoadVec1);

      __vpred vpred = c7x::strm_agen<0, vecOut>::get_vpred();
      vecOut *addr  = c7x::strm_agen<0, vecOut>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, vecLoad);
   }

   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT to Q31 conversion");
}

template <>
inline void AUDIOLIB_fltoq31_convert<double>(AUDIOLIB_kernelHandle handle,
                                             void *restrict pIn,
                                             void *restrict pOut,
                                             __SE_TEMPLATE_v1 se0Params,
                                             __SA_TEMPLATE_v1 sa0Params,
                                             uint32_t         loopCount)
{
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for FLOAT64 to Q31 conversion");

   double *restrict pInLocal   = (double *) pIn;
   int32_t *restrict pOutLocal = (int32_t *) pOut;

   typedef typename c7x::make_full_vector<double>::type vecIn;
   typedef typename c7x::int_vec                        vecOut;

   AUDIOLIB_DEBUGPRINTFN(0, "pInLocal: %p pOutLocal: %p blockSize: %d loopCount: %d remainingElements: %d\n", pInLocal,
                         pOutLocal, blockSize, loopCount, remainingElements);

   vecIn mulFactor = (vecIn) Q31_SCALE_FACTOR;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   for (uint32_t i = 0; i < loopCount; i++) {
      vecIn  loadVec1      = c7x::strm_eng<0, vecIn>::get_adv();
      vecIn  mulLoadVec1   = loadVec1 * mulFactor;
      vecOut intermediate1 = __double_to_int(mulLoadVec1);

      __vpred        predTemp = c7x::strm_agen<0, c7x::long_vec>::get_vpred();
      c7x::int_hvec *addr     = c7x::strm_agen<0, c7x::int_hvec>::get_adv(pOutLocal);
      __vstore_pred_packl(predTemp, addr, c7x::as_long_vec(intermediate1));
   }
   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT64 to Q31 conversion");
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_q15ToFloat_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   uint8_t                          *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                          nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Entering template function for Q15 to FLOAT execution");
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_q15ToFloat_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params;
   __SA_TEMPLATE_v1 sa0Params;

   se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   int16_t *restrict pInLocal   = (int16_t *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   AUDIOLIB_q15tofl_convert<dataType>(pOutLocal, nVecs);

   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q15 to FLOAT EXECUTION");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floatToQ15_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                                        status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs                      *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   uint8_t                                               *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                                               nVecs        = pKerPrivArgs->nVecs;
   typedef typename c7x::make_full_vector<dataType>::type vecIn;
   uint32_t                                               eleCount = c7x::element_count_of<vecIn>::value;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floatToQ15_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params   = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params   = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params   = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params   = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PARAM_OFFSET);
   uint32_t         remElements = *(uint32_t *) ((uint8_t *) pBlock + REM_ELEMENTS_OFFSET);

   if (pKerPrivArgs->samples < 2 * eleCount) {
      AUDIOLIB_fltoq15SmallSamples_convert<dataType>(handle, pIn, pOut, se0Params, sa0Params, remElements);
   }
   else {
      AUDIOLIB_fltoq15_convert<dataType>(handle, pIn, pOut, se1Params, sa1Params, nVecs);
      if (remElements > 0) {
         AUDIOLIB_fltoq15SmallSamples_convert<dataType>(handle, pIn, pOut, se0Params, sa0Params, remElements);
      }
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT to Q15 execution");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_q23ToFloat_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   uint8_t                          *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_q23ToFloat_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   int32_t *restrict pInLocal   = (int32_t *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   AUDIOLIB_q23tofl_convert<dataType>(pKerPrivArgs, pOutLocal);

   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q23 to FLOAT execution");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_q31ToFloat_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   uint8_t                          *pBlock       = pKerPrivArgs->bufPblock;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_q31ToFloat_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   int32_t *restrict pInLocal   = (int32_t *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE0_OPEN(pInLocal, se0Params);
   __SA0_OPEN(sa0Params);

   AUDIOLIB_q31tofl_convert<dataType>(pKerPrivArgs, pOutLocal);

   __SE0_CLOSE();
   __SA0_CLOSE();

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q31 to FLOAT execution");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floatToQ23_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   uint8_t                          *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                          nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floatToQ23_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);
   __SE_TEMPLATE_v1 se0Params;
   __SA_TEMPLATE_v1 sa0Params;

   se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   AUDIOLIB_fltoq23_convert<dataType>(handle, pIn, pOut, se0Params, sa0Params, nVecs);

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT to Q23 execution");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floatToQ31_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{

   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;
   uint8_t                          *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                          nVecs        = pKerPrivArgs->nVecs;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_floatToQ31_exec_ci nVecs %d\n", pKerPrivArgs->nVecs);

   __SE_TEMPLATE_v1 se0Params;
   __SA_TEMPLATE_v1 sa0Params;

   se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PARAM_OFFSET);
   sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PARAM_OFFSET);

   AUDIOLIB_fltoq31_convert<dataType>(handle, pIn, pOut, se0Params, sa0Params, nVecs);

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT to Q31 execution");

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_q15ToFloat_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q15ToFloat_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ15_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ15_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q23ToFloat_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q23ToFloat_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q31ToFloat_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q31ToFloat_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ23_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ23_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ31_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ31_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
