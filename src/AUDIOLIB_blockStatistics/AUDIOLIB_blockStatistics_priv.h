// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_blockStatistics_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_blockStatistics_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_blockStatistics.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_blockStatistics_PrivArgs structure.
 *
 */

#define AUDIOLIB_BLOCKSTATISTICS_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_blockStatistics_exec_ci
 *         and @ref AUDIOLIB_blockStatistics_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_blockStatistics_exec)(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn,
                                                             void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_blockStatistics_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_blockStatistics_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_blockStatistics_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_blockStatistics_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of ouput buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_blockStatistics_init_ci(AUDIOLIB_kernelHandle         handle,
                                                 const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                 const AUDIOLIB_bufParams2D_t *bufParamsOut);

/**
 * @defgroup AUDIOLIB_blockStatistics_ci C7x Optimized Execution Functions
 * @{
 */

/**
 * @brief      C7x-optimized execution functions for various statistical operations.
 *
 * @details    These functions perform high-performance statistical calculations
 * using C7x vector instructions. Each
 * function corresponds to a specific `statisticsType`.
 *
 * @param[in]  handle       Active handle to the kernel.
 * @param[in]  pIn          Pointer to the input buffer.
 * @param[out] pOut         Pointer to the output buffer where the calculated
 * statistic (a single scalar value per channel) is stored.
 *
 * @return     Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_max_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_min_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_maxAbs_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_mean_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_rms_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_stdDev_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_variance_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_avgEnergy_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_add_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_sqrAdd_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @} */

/**
 * @defgroup AUDIOLIB_blockStatistics_cn Natural C Execution Functions
 * @{
 */

/**
 * @brief      Natural C reference implementations for various statistical operations.
 *
 * @details    These functions provide a clear, readable reference for the
 * kernel's statistical calculations. They are written for correctness
 * and are not performance-optimized.
 *
 * @param[in]  handle      Active handle to the kernel.
 * @param[in]  pIn         Pointer to buffer holding the input buffer.
 * @param[out] pOut        Pointer to buffer holding the output buffer.
 *
 * @return     Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_max_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_min_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_maxAbs_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_mean_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_rms_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_stdDev_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_variance_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_avgEnergy_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_add_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_sqrAdd_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Private data structure for the block statistics kernel handle.
 * @details This structure holds the internal state, configuration parameters,
 * and function pointers required for the kernel's operation.
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_blockStatistics_exec_cn and
    *         @ref AUDIOLIB_blockStatistics_exec_ci.                        */
   pFxnAUDIOLIB_blockStatistics_exec execute;
   uint32_t                          inputBlockSize;    /**< Samples per channel*/
   uint32_t                          outputBlockSize;   /**< Samples per channel*/
   uint32_t                          numChannels;       /**< Number of Input Channels*/
   uint8_t                           statisticsType;    /**< Selects the statistic to compute for each
                                                            subblock:0=max, 1=min, 2=max_abs, 3=mean, 4=rms, 5=stdDeviation,6=variance, 7=avg_energy,
                                                           8=sum,                              9=sum_squares.*/
   uint32_t                          strideInElements;  /**< Stride in Y dimension for input*/
   uint32_t                          strideOutElements; /**< Stride in Y dimension for output*/
   uint32_t                          dataType;          /**< Size of the data type*/
   uint32_t                          numBlocks;         /**< Number of Channels to process*/
   uint32_t                          wBlocks;           /**< Number of blocks to process*/
   AUDIOLIB_blockStatistics_InitArgs initArgs;          /**< Structure holding initialization parameters  */
   AUDIOLIB_blockStatistics_SetArgs  setArgs;           /**< Structure to hold the set parameters*/
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_BLOCKSTATISTICS_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_blockStatistics_PrivArgs;

#endif /* AUDIOLIB_blockStatistics_IXX_IXX_OXX_PRIV_H_ */
