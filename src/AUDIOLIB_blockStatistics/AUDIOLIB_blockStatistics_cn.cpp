// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_blockStatistics_priv.h"
#include <cmath>

// Templatized inline functions for per-channel statistics
template <typename dataType> inline dataType AUDIOLIB_perChan_max_exec_cn(const dataType *data, uint32_t size)
{
   if (size == 0)
      return dataType(0);
   dataType max_val = data[0];
   for (uint32_t i = 1; i < size; ++i) {
      if (data[i] > max_val)
         max_val = data[i];
   }
   return max_val;
}

template <typename dataType> inline dataType AUDIOLIB_perChan_min_exec_cn(const dataType *data, uint32_t size)
{
   if (size == 0)
      return dataType(0);
   dataType min_val = data[0];
   for (uint32_t i = 1; i < size; ++i) {
      if (data[i] < min_val)
         min_val = data[i];
   }
   return min_val;
}

template <typename dataType> inline dataType AUDIOLIB_perChan_maxAbs_exec_cn(const dataType *data, uint32_t size)
{
   if (size == 0)
      return dataType(0);
   dataType max_abs = std::abs(data[0]);
   for (uint32_t i = 1; i < size; ++i) {
      dataType abs_val = std::abs(data[i]);
      if (abs_val > max_abs)
         max_abs = abs_val;
   }
   return max_abs;
}

template <typename dataType> inline dataType AUDIOLIB_perChan_mean_exec_cn(const dataType *data, uint32_t size)
{
   if (size == 0)
      return dataType(0);
   dataType sum = dataType(0);
   for (uint32_t i = 0; i < size; ++i) {
      sum += data[i];
   }
   return sum / static_cast<dataType>(size);
}

template <typename dataType> inline dataType AUDIOLIB_perChan_rms_exec_cn(const dataType *data, uint32_t size)
{
   if (size == 0)
      return dataType(0);
   dataType sum_squares = dataType(0);
   for (uint32_t i = 0; i < size; ++i) {
      sum_squares += data[i] * data[i];
   }
   return std::sqrt(sum_squares / static_cast<dataType>(size));
}

template <typename dataType> inline dataType AUDIOLIB_perChan_stdDev_exec_cn(const dataType *data, uint32_t size)
{
   if (size <= 1)
      return dataType(0);
   dataType mean          = AUDIOLIB_perChan_mean_exec_cn(data, size);
   dataType sumSquaredDev = dataType(0);
   for (uint32_t i = 0; i < size; ++i) {
      dataType dev = data[i] - mean;
      sumSquaredDev += dev * dev;
   }
   return std::sqrt(sumSquaredDev / static_cast<dataType>(size));
}

template <typename dataType> inline dataType AUDIOLIB_perChan_var_exec_cn(const dataType *data, uint32_t size)
{
   if (size <= 1)
      return dataType(0);
   dataType mean          = AUDIOLIB_perChan_mean_exec_cn(data, size);
   dataType sumSquaredDev = dataType(0);
   for (uint32_t i = 0; i < size; ++i) {
      dataType dev = data[i] - mean;
      sumSquaredDev += dev * dev;
   }
   return sumSquaredDev / static_cast<dataType>(size);
}

template <typename dataType> inline dataType AUDIOLIB_perChan_avgEnergy_exec_cn(const dataType *data, uint32_t size)
{
   if (size == 0)
      return dataType(0);
   dataType sum_squares = dataType(0);
   for (uint32_t i = 0; i < size; ++i) {
      sum_squares += data[i] * data[i];
   }
   return sum_squares / static_cast<dataType>(size);
}

template <typename dataType> inline dataType AUDIOLIB_perChan_add_exec_cn(const dataType *data, uint32_t size)
{
   if (size == 0)
      return dataType(0);
   dataType sum = dataType(0);
   for (uint32_t i = 0; i < size; ++i) {
      sum += data[i];
   }
   return sum;
}

template <typename dataType> inline dataType AUDIOLIB_perChan_sqrAdd_exec_cn(const dataType *data, uint32_t size)
{
   if (size == 0)
      return dataType(0);
   dataType sum_squares = dataType(0);
   for (uint32_t i = 0; i < size; ++i) {
      sum_squares += data[i] * data[i];
   }
   return sum_squares;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_max_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_max_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_max_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_min_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_min_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_min_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_maxAbs_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_maxAbs_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_maxAbs_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_mean_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_mean_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_mean_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_rms_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_rms_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_rms_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_stdDev_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_stdDev_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_stdDev_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_variance_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_variance_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_var_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_avgEnergy_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_avgEnergy_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_avgEnergy_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_add_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_add_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_add_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_sqrAdd_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_sqrAdd_exec_cn\n");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t inputBlockSize = pKerPrivArgs->inputBlockSize;
   uint32_t channels       = pKerPrivArgs->numChannels;
   uint32_t strideIn       = pKerPrivArgs->strideInElements;
   uint32_t strideOut      = pKerPrivArgs->strideOutElements;

   for (uint32_t ch = 0; ch < channels; ++ch) {
      dataType *channelData     = pInLocal + (ch * strideIn);
      pOutLocal[ch * strideOut] = AUDIOLIB_perChan_sqrAdd_exec_cn(channelData, inputBlockSize);
   }

   return status;
}

// Explicit instantiations for float and double
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_max_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_min_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_maxAbs_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_mean_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_rms_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_stdDev_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_variance_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS AUDIOLIB_blockStatistics_avgEnergy_exec_cn<float>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_add_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_sqrAdd_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_max_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_min_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_maxAbs_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_mean_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_rms_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_stdDev_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS AUDIOLIB_blockStatistics_variance_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                                           void *restrict pIn,
                                                                           void *restrict pOut);
template AUDIOLIB_STATUS AUDIOLIB_blockStatistics_avgEnergy_exec_cn<double>(AUDIOLIB_kernelHandle handle,
                                                                            void *restrict pIn,
                                                                            void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_add_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_sqrAdd_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
