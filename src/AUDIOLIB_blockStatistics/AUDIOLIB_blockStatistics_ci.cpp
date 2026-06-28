// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_blockStatistics_priv.h"
#include "AUDIOLIB_inlines.h"

#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET (SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET (SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET (SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET + SE_PARAM_SIZE)

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_blockStatistics_init_ci(AUDIOLIB_kernelHandle         handle,
                                                 const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                 const AUDIOLIB_bufParams2D_t *bufParamsOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           eleCount     = c7x::element_count_of<vec>::value;

   pKerPrivArgs->wBlocks   = AUDIOLIB_ceilingDiv(pKerPrivArgs->inputBlockSize, eleCount);
   pKerPrivArgs->numBlocks = AUDIOLIB_ceilingDiv(pKerPrivArgs->numChannels, 2);

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_init_ci \n");

   __SE_TEMPLATE_v1 se0ParamsPerChanStats = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa0ParamsPerChanStats = __gen_SA_TEMPLATE_v1();
   __SE_TEMPLATE_v1 se1ParamsPerChanStats = __gen_SE_TEMPLATE_v1();
   __SA_TEMPLATE_v1 sa1ParamsPerChanStats = __gen_SA_TEMPLATE_v1();

   __SE_ELETYPE SE_ELETYPE = c7x::se_eletype<vec>::value;
   __SE_VECLEN  SE_VECLEN  = c7x::se_veclen<vec>::value;
   __SA_VECLEN  SA_VECLEN  = c7x::sa_veclen<vec>::value;

   // /******************************************************************************************/
   // /* Prepare streaming engine 0 to fetch input samples for per channel inputBlock statistics  */
   // /******************************************************************************************/
   se0ParamsPerChanStats.ICNT0         = eleCount;
   se0ParamsPerChanStats.DIM1          = eleCount;
   se0ParamsPerChanStats.ICNT1         = pKerPrivArgs->wBlocks % 2 == 0
                                             ? pKerPrivArgs->wBlocks
                                             : pKerPrivArgs->wBlocks + 1; // to ensure even number of blocks;
   se0ParamsPerChanStats.DIM2          = pKerPrivArgs->strideInElements * 2;
   se0ParamsPerChanStats.ICNT2         = pKerPrivArgs->numBlocks;
   se0ParamsPerChanStats.DECDIM1       = __SE_DECDIM_DIM1;
   se0ParamsPerChanStats.DECDIM1_WIDTH = pKerPrivArgs->inputBlockSize;
   se0ParamsPerChanStats.FILLVAL       = __SE_FILLVAL_USER;
   se0ParamsPerChanStats.DIMFMT        = __SE_DIMFMT_3D;
   se0ParamsPerChanStats.ELETYPE       = SE_ELETYPE;
   se0ParamsPerChanStats.VECLEN        = SE_VECLEN;

   // /******************************************************************************************/
   // /* Prepare streaming engine 1 to fetch input samples for per channel inputBlock statistics  */
   // /******************************************************************************************/
   se1ParamsPerChanStats.ICNT0         = eleCount; // 8
   se1ParamsPerChanStats.DIM1          = eleCount;
   se1ParamsPerChanStats.ICNT1         = pKerPrivArgs->wBlocks % 2 == 0
                                             ? pKerPrivArgs->wBlocks
                                             : pKerPrivArgs->wBlocks + 1; // to ensure even number of blocks
   se1ParamsPerChanStats.DIM2          = pKerPrivArgs->strideInElements * 2;
   se1ParamsPerChanStats.ICNT2         = (pKerPrivArgs->numChannels) / 2;
   se1ParamsPerChanStats.DECDIM1       = __SE_DECDIM_DIM1;
   se1ParamsPerChanStats.DECDIM1_WIDTH = pKerPrivArgs->inputBlockSize;
   se1ParamsPerChanStats.FILLVAL       = __SE_FILLVAL_USER;
   se1ParamsPerChanStats.DIMFMT        = __SE_DIMFMT_3D;
   se1ParamsPerChanStats.ELETYPE       = SE_ELETYPE;
   se1ParamsPerChanStats.VECLEN        = SE_VECLEN;

   // /******************************************************************************************/
   // /* Prepare SA template to store output for per channel inputBlock statistics                */
   // /******************************************************************************************/
   sa0ParamsPerChanStats.ICNT0  = 1;
   sa0ParamsPerChanStats.DIM1   = pKerPrivArgs->strideOutElements * 2;
   sa0ParamsPerChanStats.ICNT1  = pKerPrivArgs->numBlocks;
   sa0ParamsPerChanStats.DIMFMT = __SA_DIMFMT_2D;
   sa0ParamsPerChanStats.VECLEN = SA_VECLEN;

   // /******************************************************************************************/
   // /* Prepare SA template to store output for per channel inputBlock statistics                */
   // /******************************************************************************************/
   sa1ParamsPerChanStats.ICNT0  = 1;
   sa1ParamsPerChanStats.DIM1   = pKerPrivArgs->strideOutElements * 2;
   sa1ParamsPerChanStats.ICNT1  = pKerPrivArgs->numChannels > 2 ? pKerPrivArgs->numChannels / 2 : 1;
   sa1ParamsPerChanStats.DIMFMT = __SA_DIMFMT_2D;
   sa1ParamsPerChanStats.VECLEN = SA_VECLEN;

   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET) = se0ParamsPerChanStats;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET) = sa0ParamsPerChanStats;
   *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET) = se1ParamsPerChanStats;
   *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET) = sa1ParamsPerChanStats;

   return status;
}

template AUDIOLIB_STATUS AUDIOLIB_blockStatistics_init_ci<float>(AUDIOLIB_kernelHandle         handle,
                                                                 const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 const AUDIOLIB_bufParams2D_t *bufParamsOut);
template AUDIOLIB_STATUS AUDIOLIB_blockStatistics_init_ci<double>(AUDIOLIB_kernelHandle         handle,
                                                                  const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                  const AUDIOLIB_bufParams2D_t *bufParamsOut);

// Fast square root for single float value using C7x intrinsic
template <typename dataType> static inline dataType fast_sqrt(dataType x)
{
   dataType y = __recip_sqrt(x); // C7x intrinsic for 1/sqrt(x)
   // One Newton-Raphson iteration for improved accuracy
   y = y * (1.5f - (x * 0.5f * y * y));
   return x * y;
}

// Template struct to provide hex value for std::numeric_limits<dataType>::lowest()
template <typename dataType> struct LowestHexValue {
   static constexpr uint64_t value = 0;
};

// Specialization for float
template <> struct LowestHexValue<float> {
   static constexpr uint64_t value = 0xFF7FFFFF; // Hex for std::numeric_limits<float>::lowest() (-FLT_MAX)
};

// Specialization for double
template <> struct LowestHexValue<double> {
   static constexpr uint64_t value = 0xFFEFFFFFFFFFFFFF; // Hex for std::numeric_limits<double>::lowest() (-DBL_MAX)
};

// Template struct to provide hex value for std::numeric_limits<dataType>::lowest()
template <typename dataType> struct MaxHexValue {
   static constexpr uint64_t value = 0;
};

// Specialization for float
template <> struct MaxHexValue<float> {
   static constexpr uint64_t value = 0x7F7FFFFF; // Hex for std::numeric_limits<float>::max() (FLT_MAX)
};

// Specialization for double
template <> struct MaxHexValue<double> {
   static constexpr uint64_t value = 0x7FEFFFFFFFFFFFFF; // Hex for std::numeric_limits<double>::max() (DBL_MAX)
};

#ifdef _HOST_BUILD
constexpr uint64_t LowestHexValue<float>::value;
constexpr uint64_t LowestHexValue<double>::value;
constexpr uint64_t MaxHexValue<float>::value;
constexpr uint64_t MaxHexValue<double>::value;
#endif

// Template declaration without definition
template <typename dataType>
inline void extract_max_values(typename c7x::make_full_vector<dataType>::type maxVecA,
                               typename c7x::make_full_vector<dataType>::type maxVecB,
                               typename c7x::make_full_vector<dataType>::type maxVecC,
                               typename c7x::make_full_vector<dataType>::type maxVecD,
                               dataType                                      &maxSubBlockValA,
                               dataType                                      &maxSubBlockValB);

// Specialization for float
template <>
inline void extract_max_values<float>(typename c7x::make_full_vector<float>::type maxVecA,
                                      typename c7x::make_full_vector<float>::type maxVecB,
                                      typename c7x::make_full_vector<float>::type maxVecC,
                                      typename c7x::make_full_vector<float>::type maxVecD,
                                      float                                      &maxSubBlockValA,
                                      float                                      &maxSubBlockValB)
{
   using vec   = typename c7x::make_full_vector<float>::type;
   vec maxVec1 = __max(maxVecA, maxVecC);
   vec maxVec2 = __max(maxVecB, maxVecD);

   maxVec1 = __sort_desc(maxVec1);
   maxVec2 = __sort_desc(maxVec2);

   maxSubBlockValA = __get_vector_element(maxVec1, (uint) 0);
   maxSubBlockValB = __get_vector_element(maxVec2, (uint) 0);
}

// Specialization for double
template <>
inline void extract_max_values<double>(typename c7x::make_full_vector<double>::type maxVecA,
                                       typename c7x::make_full_vector<double>::type maxVecB,
                                       typename c7x::make_full_vector<double>::type maxVecC,
                                       typename c7x::make_full_vector<double>::type maxVecD,
                                       double                                      &maxSubBlockValA,
                                       double                                      &maxSubBlockValB)
{
   // Compute max for maxVecA and maxVecC
   double2 inVecA2 = __max(maxVecA.lo(), maxVecA.hi()); // double2 of maxVecA
   double  inVecA1 = __max(inVecA2.lo(), inVecA2.hi()); // double of maxVecA
   double2 inVecC2 = __max(maxVecC.lo(), maxVecC.hi()); // double2 of maxVecC
   double  inVecC1 = __max(inVecC2.lo(), inVecC2.hi()); // double of maxVecC
   maxSubBlockValA = __max(inVecA1, inVecC1);

   // Compute max for maxVecB and maxVecD
   double2 inVecB2 = __max(maxVecB.lo(), maxVecB.hi()); // double2 of maxVecB
   double  inVecB1 = __max(inVecB2.lo(), inVecB2.hi()); // double of maxVecB
   double2 inVecD2 = __max(maxVecD.lo(), maxVecD.hi()); // double2 of maxVecD
   double  inVecD1 = __max(inVecD2.lo(), inVecD2.hi()); // double of maxVecD
   maxSubBlockValB = __max(inVecB1, inVecD1);
}

// Template declaration without definition
template <typename dataType>
inline void extract_min_values(typename c7x::make_full_vector<dataType>::type minVecA,
                               typename c7x::make_full_vector<dataType>::type minVecB,
                               typename c7x::make_full_vector<dataType>::type minVecC,
                               typename c7x::make_full_vector<dataType>::type minVecD,
                               dataType                                      &minSubBlockValA,
                               dataType                                      &minSubBlockValB);

// Specialization for float
template <>
inline void extract_min_values<float>(typename c7x::make_full_vector<float>::type minVecA,
                                      typename c7x::make_full_vector<float>::type minVecB,
                                      typename c7x::make_full_vector<float>::type minVecC,
                                      typename c7x::make_full_vector<float>::type minVecD,
                                      float                                      &minSubBlockValA,
                                      float                                      &minSubBlockValB)
{
   using vec   = typename c7x::make_full_vector<float>::type;
   vec minVec1 = __min(minVecA, minVecC);
   vec minVec2 = __min(minVecB, minVecD);

   minVec1 = __sort_asc(minVec1);
   minVec2 = __sort_asc(minVec2);

   minSubBlockValA = __get_vector_element(minVec1, (uint) 0);
   minSubBlockValB = __get_vector_element(minVec2, (uint) 0);
}

// Specialization for double
template <>
inline void extract_min_values<double>(typename c7x::make_full_vector<double>::type minVecA,
                                       typename c7x::make_full_vector<double>::type minVecB,
                                       typename c7x::make_full_vector<double>::type minVecC,
                                       typename c7x::make_full_vector<double>::type minVecD,
                                       double                                      &minSubBlockValA,
                                       double                                      &minSubBlockValB)
{
   // Compute min for minVecA and minVecC
   double2 inVecA2 = __min(minVecA.lo(), minVecA.hi()); // double2 of minVecA
   double  inVecA1 = __min(inVecA2.lo(), inVecA2.hi()); // double of minVecA
   double2 inVecC2 = __min(minVecC.lo(), minVecC.hi()); // double2 of minVecC
   double  inVecC1 = __min(inVecC2.lo(), inVecC2.hi()); // double of minVecC
   minSubBlockValA = __min(inVecA1, inVecC1);

   // Compute min for minVecB and minVecD
   double2 inVecB2 = __min(minVecB.lo(), minVecB.hi()); // double2 of minVecB
   double  inVecB1 = __min(inVecB2.lo(), inVecB2.hi()); // double of minVecB
   double2 inVecD2 = __min(minVecD.lo(), minVecD.hi()); // double2 of minVecD
   double  inVecD1 = __min(inVecD2.lo(), inVecD2.hi()); // double of minVecD
   minSubBlockValB = __min(inVecB1, inVecD1);
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_max_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   // Set ECR registers
   __ECR_SE0_SBND = LowestHexValue<dataType>::value;
   __ECR_SE1_SBND = LowestHexValue<dataType>::value;

   vec      vecInA, vecInB, vecInC, vecInD;
   dataType maxSubBlockValA = 0.0f;
   dataType maxSubBlockValB = 0.0f;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec maxVecA = c7x::strm_eng<0, vec>::get_adv();
      vec maxVecB = c7x::strm_eng<1, vec>::get_adv();
      vec maxVecC = c7x::strm_eng<0, vec>::get_adv();
      vec maxVecD = c7x::strm_eng<1, vec>::get_adv();

      // 2 + trip_cnt * 2
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vecInA = c7x::strm_eng<0, vec>::get_adv();
         vecInB = c7x::strm_eng<1, vec>::get_adv();

         maxVecA = __max(vecInA, maxVecA);
         maxVecB = __max(vecInB, maxVecB);

         vecInC = c7x::strm_eng<0, vec>::get_adv();
         vecInD = c7x::strm_eng<1, vec>::get_adv();

         maxVecC = __max(vecInC, maxVecC);
         maxVecD = __max(vecInD, maxVecD);
      }

      extract_max_values(maxVecA, maxVecB, maxVecC, maxVecD, maxSubBlockValA, maxSubBlockValB);

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, maxSubBlockValA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, maxSubBlockValB);
   }

   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_max_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_max_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_min_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   __ECR_SE1_SBND = MaxHexValue<dataType>::value;
   __ECR_SE0_SBND = MaxHexValue<dataType>::value;

   vec      vecInA, vecInB, vecInC, vecInD;
   dataType minSubBlockValA = 0.0f;
   dataType minSubBlockValB = 0.0f;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec minVecA = c7x::strm_eng<0, vec>::get_adv();
      ;
      vec minVecB = c7x::strm_eng<1, vec>::get_adv();
      ;
      vec minVecC = c7x::strm_eng<0, vec>::get_adv();
      ;
      vec minVecD = c7x::strm_eng<1, vec>::get_adv();
      ;

      // 2 + trip_cnt * 2
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vecInA = c7x::strm_eng<0, vec>::get_adv();
         vecInB = c7x::strm_eng<1, vec>::get_adv();

         minVecA = __min(vecInA, minVecA);
         minVecB = __min(vecInB, minVecB);

         vecInC = c7x::strm_eng<0, vec>::get_adv();
         vecInD = c7x::strm_eng<1, vec>::get_adv();

         minVecC = __min(vecInC, minVecC);
         minVecD = __min(vecInD, minVecD);
      }

      extract_min_values(minVecA, minVecB, minVecC, minVecD, minSubBlockValA, minSubBlockValB);

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, minSubBlockValA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, minSubBlockValB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_min_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_min_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_maxAbs_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   __ECR_SE0_SBND = 0;
   __ECR_SE1_SBND = 0;

   vec      vecInA, vecInB, vecInC, vecInD;
   dataType maxSubBlockValA = 0.0f;
   dataType maxSubBlockValB = 0.0f;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec maxVecA = c7x::strm_eng<0, vec>::get_adv();
      vec maxVecB = c7x::strm_eng<1, vec>::get_adv();
      vec maxVecC = c7x::strm_eng<0, vec>::get_adv();
      vec maxVecD = c7x::strm_eng<1, vec>::get_adv();

      maxVecA = __abs(maxVecA);
      maxVecB = __abs(maxVecB);
      maxVecC = __abs(maxVecC);
      maxVecD = __abs(maxVecD);

      // 3 + trip_cnt * 3
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vecInA = c7x::strm_eng<0, vec>::get_adv();
         vecInB = c7x::strm_eng<1, vec>::get_adv();

         maxVecA = __max(__abs(vecInA), maxVecA);
         maxVecB = __max(__abs(vecInB), maxVecB);

         vecInC = c7x::strm_eng<0, vec>::get_adv();
         vecInD = c7x::strm_eng<1, vec>::get_adv();

         maxVecC = __max(__abs(vecInC), maxVecC);
         maxVecD = __max(__abs(vecInD), maxVecD);
      }

      extract_max_values(maxVecA, maxVecB, maxVecC, maxVecD, maxSubBlockValA, maxSubBlockValB);

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, maxSubBlockValA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, maxSubBlockValB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_maxAbs_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_maxAbs_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_add_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   __ECR_SE0_SBND = 0;
   __ECR_SE1_SBND = 0;

   dataType sumA = 0, sumB = 0;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec accA = c7x::strm_eng<0, vec>::get_adv();
      vec accB = c7x::strm_eng<1, vec>::get_adv();
      vec accC = c7x::strm_eng<0, vec>::get_adv();
      vec accD = c7x::strm_eng<1, vec>::get_adv();

      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vec vecInA = c7x::strm_eng<0, vec>::get_adv();
         vec vecInB = c7x::strm_eng<1, vec>::get_adv();
         vec vecInC = c7x::strm_eng<0, vec>::get_adv();
         vec vecInD = c7x::strm_eng<1, vec>::get_adv();

         accA += vecInA;
         accB += vecInB;
         accC += vecInC;
         accD += vecInD;
      }

      accA += accC;
      accB += accD;

      // Horizontal add to get the sum of all elements in the vector
      c7x_horizontal_add((accA), &sumA);
      c7x_horizontal_add((accB), &sumB);

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, sumA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, sumB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_add_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_add_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_sqrAdd_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   __ECR_SE0_SBND = 0;
   __ECR_SE1_SBND = 0;

   dataType sumA = 0, sumB = 0;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec accA = c7x::strm_eng<0, vec>::get_adv();
      vec accB = c7x::strm_eng<1, vec>::get_adv();
      vec accC = c7x::strm_eng<0, vec>::get_adv();
      vec accD = c7x::strm_eng<1, vec>::get_adv();

      accA = accA * accA;
      accB = accB * accB;
      accC = accC * accC;
      accD = accD * accD;

      // 6 + trip_cnt * 3
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vec vecInA = c7x::strm_eng<0, vec>::get_adv();
         vec vecInB = c7x::strm_eng<1, vec>::get_adv();
         vec vecInC = c7x::strm_eng<0, vec>::get_adv();
         vec vecInD = c7x::strm_eng<1, vec>::get_adv();

         accA += vecInA * vecInA;
         accB += vecInB * vecInB;
         accC += vecInC * vecInC;
         accD += vecInD * vecInD;
      }

      accA += accC;
      accB += accD;

      // Horizontal add to get the sum of all elements in the vector
      c7x_horizontal_add((accA), &sumA);
      c7x_horizontal_add((accB), &sumB);

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, sumA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, sumB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_sqrAdd_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_sqrAdd_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_mean_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;
   uint32_t                           inpBlockSize = pKerPrivArgs->inputBlockSize;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   dataType recip = __recip((dataType) inpBlockSize);

   __ECR_SE0_SBND = 0;
   __ECR_SE1_SBND = 0;

   dataType sumA = 0, sumB = 0;

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec accA = c7x::strm_eng<0, vec>::get_adv();
      vec accB = c7x::strm_eng<1, vec>::get_adv();
      vec accC = c7x::strm_eng<0, vec>::get_adv();
      vec accD = c7x::strm_eng<1, vec>::get_adv();

      // 2 + trip_cnt * 3
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vec vecInA = c7x::strm_eng<0, vec>::get_adv();
         vec vecInB = c7x::strm_eng<1, vec>::get_adv();
         vec vecInC = c7x::strm_eng<0, vec>::get_adv();
         vec vecInD = c7x::strm_eng<1, vec>::get_adv();

         accA += vecInA;
         accB += vecInB;
         accC += vecInC;
         accD += vecInD;
      }

      accA += accC;
      accB += accD;

      // Horizontal add to get the sum of all elements in the vector
      c7x_horizontal_add((accA), &sumA);
      c7x_horizontal_add((accB), &sumB);

      dataType meanA = sumA * recip;
      dataType meanB = sumB * recip;

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, meanA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, meanB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_mean_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_mean_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_avgEnergy_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;
   uint32_t                           inpBlockSize = pKerPrivArgs->inputBlockSize;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   __ECR_SE0_SBND = 0;
   __ECR_SE1_SBND = 0;

   dataType sumA = 0, sumB = 0;
   dataType recip = __recip((dataType) inpBlockSize);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec accA = c7x::strm_eng<0, vec>::get_adv();
      vec accB = c7x::strm_eng<1, vec>::get_adv();
      vec accC = c7x::strm_eng<0, vec>::get_adv();
      vec accD = c7x::strm_eng<1, vec>::get_adv();

      // Square the input vectors to calculate energy
      accA = accA * accA;
      accB = accB * accB;
      accC = accC * accC;
      accD = accD * accD;

      // 6 + trip_cnt * 3
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vec vecInA = c7x::strm_eng<0, vec>::get_adv();
         vec vecInB = c7x::strm_eng<1, vec>::get_adv();
         vec vecInC = c7x::strm_eng<0, vec>::get_adv();
         vec vecInD = c7x::strm_eng<1, vec>::get_adv();

         accA += vecInA * vecInA;
         accB += vecInB * vecInB;
         accC += vecInC * vecInC;
         accD += vecInD * vecInD;
      }

      accA += accC;
      accB += accD;

      c7x_horizontal_add((accA), &sumA);
      c7x_horizontal_add((accB), &sumB);

      dataType avgEnergyA = sumA * recip;
      dataType avgEnergyB = sumB * recip;

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, avgEnergyA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, avgEnergyB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS AUDIOLIB_blockStatistics_avgEnergy_exec_ci<float>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pOut);
template AUDIOLIB_STATUS AUDIOLIB_blockStatistics_avgEnergy_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                            void *restrict pIn,
                                                                            void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_rms_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;
   uint32_t                           inpBlockSize = pKerPrivArgs->inputBlockSize;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   __ECR_SE0_SBND = 0;
   __ECR_SE1_SBND = 0;

   dataType sumA = 0, sumB = 0;
   dataType recip = __recip((dataType) inpBlockSize);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec accA = c7x::strm_eng<0, vec>::get_adv();
      vec accB = c7x::strm_eng<1, vec>::get_adv();
      vec accC = c7x::strm_eng<0, vec>::get_adv();
      vec accD = c7x::strm_eng<1, vec>::get_adv();

      accA = accA * accA;
      accB = accB * accB;
      accC = accC * accC;
      accD = accD * accD;

      // 6 + trip_cnt * 3
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vec vecInA = c7x::strm_eng<0, vec>::get_adv();
         vec vecInB = c7x::strm_eng<1, vec>::get_adv();
         vec vecInC = c7x::strm_eng<0, vec>::get_adv();
         vec vecInD = c7x::strm_eng<1, vec>::get_adv();

         accA += vecInA * vecInA;
         accB += vecInB * vecInB;
         accC += vecInC * vecInC;
         accD += vecInD * vecInD;
      }

      accA += accC;
      accB += accD;

      c7x_horizontal_add((accA), &sumA);
      c7x_horizontal_add((accB), &sumB);

      dataType meanA = sumA * recip;
      dataType meanB = sumB * recip;

      dataType rmsA = fast_sqrt(meanA);
      dataType rmsB = fast_sqrt(meanB);

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, rmsA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, rmsB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_rms_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_rms_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_stdDev_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;
   uint32_t                           inpBlockSize = pKerPrivArgs->inputBlockSize;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   __ECR_SE0_SBND = 0;
   __ECR_SE1_SBND = 0;

   dataType sumA = 0, sumB = 0, sumSqrA = 0, sumSqrB = 0;
   dataType recip = __recip((dataType) inpBlockSize);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec accA = c7x::strm_eng<0, vec>::get_adv();
      vec accB = c7x::strm_eng<1, vec>::get_adv();
      vec accC = c7x::strm_eng<0, vec>::get_adv();
      vec accD = c7x::strm_eng<1, vec>::get_adv();

      vec accSqrA = accA * accA;
      vec accSqrB = accB * accB;
      vec accSqrC = accC * accC;
      vec accSqrD = accD * accD;

      // 10 + trip_cnt * 3
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (input BlockSize / eleCount)
      {
         vec vecInA = c7x::strm_eng<0, vec>::get_adv();
         vec vecInB = c7x::strm_eng<1, vec>::get_adv();
         vec vecInC = c7x::strm_eng<0, vec>::get_adv();
         vec vecInD = c7x::strm_eng<1, vec>::get_adv();

         accA += vecInA;
         accB += vecInB;
         accC += vecInC;
         accD += vecInD;

         accSqrA += vecInA * vecInA;
         accSqrB += vecInB * vecInB;
         accSqrC += vecInC * vecInC;
         accSqrD += vecInD * vecInD;
      }

      accA += accC;
      accB += accD;
      accSqrA += accSqrC;
      accSqrB += accSqrD;

      c7x_horizontal_add((accA), &sumA);
      c7x_horizontal_add((accB), &sumB);
      c7x_horizontal_add((accSqrA), &sumSqrA);
      c7x_horizontal_add((accSqrB), &sumSqrB);

      // Calculate average energy and mean
      dataType avgEnergyA = sumSqrA * recip;
      dataType avgEnergyB = sumSqrB * recip;

      dataType meanA = sumA * recip;
      dataType meanB = sumB * recip;

      // calculate variance
      dataType varianceA = avgEnergyA - (meanA * meanA);
      dataType varianceB = avgEnergyB - (meanB * meanB);

      // calculate standard deviation
      dataType stdDevA = fast_sqrt(varianceA);
      dataType stdDevB = fast_sqrt(varianceB);

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, stdDevA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, stdDevB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_stdDev_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_stdDev_exec_ci<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_variance_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   uint8_t                           *pBlock       = pKerPrivArgs->bufPblock;
   uint32_t                           strideIn     = pKerPrivArgs->strideInElements;
   uint32_t                           strideOut    = pKerPrivArgs->strideOutElements;
   uint32_t                           wBlocks      = pKerPrivArgs->wBlocks;
   uint32_t                           numBlocks    = pKerPrivArgs->numBlocks;
   uint32_t                           inpBlockSize = pKerPrivArgs->inputBlockSize;

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   __SE_TEMPLATE_v1 se0Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SE_TEMPLATE_v1 se1Params = *(__SE_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SE1_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa0Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA0_PER_CHANNEL_STATS_PARAM_OFFSET);
   __SA_TEMPLATE_v1 sa1Params = *(__SA_TEMPLATE_v1 *) ((uint8_t *) pBlock + SE_SA1_PER_CHANNEL_STATS_PARAM_OFFSET);

   __ECR_SE0_SBND = 0;
   __ECR_SE1_SBND = 0;

   dataType sumA = 0, sumB = 0, sumSqrA = 0, sumSqrB = 0;
   dataType recip = __recip((dataType) inpBlockSize);

   __SE0_OPEN(pInLocal, se0Params);
   __SE1_OPEN(pInLocal + strideIn, se1Params);
   __SA0_OPEN(sa0Params);
   __SA1_OPEN(sa1Params);
   for (uint i = 0; i < numBlocks; i++) {
      vec accA = c7x::strm_eng<0, vec>::get_adv();
      vec accB = c7x::strm_eng<1, vec>::get_adv();
      vec accC = c7x::strm_eng<0, vec>::get_adv();
      vec accD = c7x::strm_eng<1, vec>::get_adv();

      vec accSqrA = accA * accA;
      vec accSqrB = accB * accB;
      vec accSqrC = accC * accC;
      vec accSqrD = accD * accD;

      // 10 + trip_cnt * 3
      for (uint j = 2; j < wBlocks; j += 2) // here wBlocks = (inputBlock Size / eleCount)
      {
         vec vecInA = c7x::strm_eng<0, vec>::get_adv();
         vec vecInB = c7x::strm_eng<1, vec>::get_adv();
         vec vecInC = c7x::strm_eng<0, vec>::get_adv();
         vec vecInD = c7x::strm_eng<1, vec>::get_adv();

         accA += vecInA;
         accB += vecInB;
         accC += vecInC;
         accD += vecInD;

         accSqrA += vecInA * vecInA;
         accSqrB += vecInB * vecInB;
         accSqrC += vecInC * vecInC;
         accSqrD += vecInD * vecInD;
      }

      accA += accC;
      accB += accD;
      accSqrA += accSqrC;
      accSqrB += accSqrD;

      // Horizontal add to get the sum of all elements in the vector
      c7x_horizontal_add((accA), &sumA);
      c7x_horizontal_add((accB), &sumB);
      c7x_horizontal_add((accSqrA), &sumSqrA);
      c7x_horizontal_add((accSqrB), &sumSqrB);

      // calculate average energy
      dataType avgEnergyA = sumSqrA * recip;
      dataType avgEnergyB = sumSqrB * recip;

      // Calculate mean
      dataType meanA = sumA * recip;
      dataType meanB = sumB * recip;

      // Calculate variance
      dataType varianceA = avgEnergyA - (meanA * meanA);
      dataType varianceB = avgEnergyB - (meanB * meanB);

      __vpred   vpred = c7x::strm_agen<0, dataType>::get_vpred();
      dataType *addr  = c7x::strm_agen<0, dataType>::get_adv(pOutLocal);
      __vstore_pred(vpred, addr, varianceA);

      vpred = c7x::strm_agen<1, dataType>::get_vpred();
      addr  = c7x::strm_agen<1, dataType>::get_adv(pOutLocal + strideOut);
      __vstore_pred(vpred, addr, varianceB);
   }
   __SE0_CLOSE();
   __SE1_CLOSE();
   __SA0_CLOSE();
   __SA1_CLOSE();

   return status;
}
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_variance_exec_ci<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS AUDIOLIB_blockStatistics_variance_exec_ci<double>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pOut);

void AUDIOLIB_blockStatistics_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles)
{
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   uint32_t numBlocks = pKerPrivArgs->numBlocks;
   uint32_t wBlocks   = pKerPrivArgs->wBlocks;

   uint64_t blockStatisticsStartupCycles =
       37; // For loading SE/SA params and Fetching the necessary loop count variables from pkerPrivArgs and SE/SA open
   uint64_t blockStatisticsTeardownCycles  = 6;
   uint64_t blockStatisticsOperationCycles = 0;
   uint64_t blockStatisticsOverheadCycles  = 0;

   switch (pKerPrivArgs->statisticsType) {
   case AUDIOLIB_STAT_BLOCK_MAX:
   case AUDIOLIB_STAT_BLOCK_MIN:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (3 + 2 + (wBlocks - 2) + 5 + 14) * numBlocks;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
         blockStatisticsOperationCycles += (3 + 2 + (wBlocks - 2) + 5 + 16) * numBlocks;
      }
      break;
   case AUDIOLIB_STAT_BLOCK_MAX_ABS:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (3 + 3 + ((wBlocks - 2) / 2) * 3 + 3 + 15) * numBlocks;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
         blockStatisticsOperationCycles += (3 + 3 + ((wBlocks - 2) / 2) * 3 + 3 + 16) * numBlocks;
      }
      break;
   case AUDIOLIB_STAT_BLOCK_MEAN:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (3 + 2 + ((wBlocks - 2) / 2) * 3 + 4 + 26) * numBlocks;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
         blockStatisticsOperationCycles += (3 + 2 + ((wBlocks - 2) / 2) * 3 + 4 + 22) * numBlocks;
      }
      break;
   case AUDIOLIB_STAT_BLOCK_RMS:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (7 + 6 + ((wBlocks - 2) / 2) * 3 + 2 + 49) * numBlocks;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
         blockStatisticsOperationCycles += (7 + 7 + ((wBlocks - 2) / 2) * 4 + 2 + 47) * numBlocks;
      }
      break;
   case AUDIOLIB_STAT_BLOCK_STDDEV:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (10 + 10 + ((wBlocks - 2) / 2) * 3 + 58) * numBlocks;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
         blockStatisticsOperationCycles += (10 + 8 + ((wBlocks - 2) / 2) * 4 + 55) * numBlocks;
      }
      break;
   case AUDIOLIB_STAT_BLOCK_VARIANCE:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (9 + 10 + ((wBlocks - 2) / 2) * 3 + 34) * numBlocks;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
         blockStatisticsOperationCycles += (9 + 8 + ((wBlocks - 2) / 2) * 4 + 32) * numBlocks;
      }
      break;
   case AUDIOLIB_STAT_BLOCK_AVG_ENERGY:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (6 + 6 + ((wBlocks - 2) / 2) * 3 + 4 + 26) * numBlocks;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
         blockStatisticsOperationCycles += (6 + 7 + ((wBlocks - 2) / 2) * 4 + 4 + 22) * numBlocks;
      }
      break;
   case AUDIOLIB_STAT_BLOCK_SUM:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (3 + 2 + ((wBlocks - 2) / 2) * 3 + 4 + 22) * numBlocks;
      }
      else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
         blockStatisticsOperationCycles += (3 + 2 + ((wBlocks - 2) / 2) * 3 + 4 + 18) * numBlocks;
      }
      break;
   case AUDIOLIB_STAT_BLOCK_SUM_SQUARES:
      if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
         blockStatisticsOperationCycles += (6 + (6 + ((wBlocks - 2) / 2) * 3 + 4 + 22)) * numBlocks;
      }
      else {
         blockStatisticsOperationCycles += ((6 + (7 + ((wBlocks - 2) / 2) * 4 + 4 + 18))) * numBlocks;
      }
      break;
   }

   blockStatisticsOverheadCycles = blockStatisticsStartupCycles + blockStatisticsTeardownCycles;
   *estCycles                    = blockStatisticsOperationCycles + blockStatisticsOverheadCycles;
   *archCycles                   = blockStatisticsOperationCycles;
}
