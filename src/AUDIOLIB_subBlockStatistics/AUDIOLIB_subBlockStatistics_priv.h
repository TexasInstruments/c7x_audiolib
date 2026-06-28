// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_subBlockStatistics.h"

/**
 * @brief Defines the size of the parameter block buffer.
 * @details This buffer holds pre-calculated hardware configurations for the
 * C7x Streaming Engine (SE) and Stream Address Generator (SA)
 */

#define AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_PBLOCK_SIZE (4 * SE_PARAM_SIZE + 4 * SA_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_subBlockStatistics_exec_ci
 *         and @ref AUDIOLIB_subBlockStatistics_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_subBlockStatistics_exec)(AUDIOLIB_kernelHandle handle,
                                                                void *restrict pIn,
                                                                void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_subBlockStatistics_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_subBlockStatistics_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_subBlockStatistics_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_subBlockStatistics_exec_ci does not lose cycles
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
AUDIOLIB_STATUS AUDIOLIB_subBlockStatistics_init_ci(AUDIOLIB_kernelHandle         handle,
                                                    const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                    const AUDIOLIB_bufParams2D_t *bufParamsOut);

/**
 * @defgroup AUDIOLIB_subBlockStatistics_ci C7x Optimized Execution Functions
 * @{
 */

/**
 * @brief      C7x-optimized execution functions for various statistical operations.
 *
 * @details    These functions perform high-performance statistical calculations
 * using C7x vector instructions. Each function corresponds to a
 * specific `statisticsType` and is selected at runtime via the
 * `execute` function pointer.
 *
 * @param[in]  handle      Active handle to the kernel.
 * @param[in]  pIn         Pointer to the input buffer.
 * @param[out] pOut        Pointer to the output buffer where the calculated
 * statistics are stored.
 *
 * @return     Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_max_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_min_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_maxAbs_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_mean_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_rms_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_stdDev_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_variance_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_avgEnergy_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_add_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_sqrAdd_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @defgroup AUDIOLIB_subBlockStatistics_cn Natural C Execution Functions
 * @{
 */
/**
 * @brief      Natural C reference implementations for various statistical operations.
 *
 * @details    These functions each implement a specific statistical operation
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
AUDIOLIB_subBlockStatistics_max_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_min_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_maxAbs_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_mean_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_rms_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_stdDev_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_variance_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_avgEnergy_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_add_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_sqrAdd_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_subBlockStatistics_exec_cn and
    *         @ref AUDIOLIB_subBlockStatistics_exec_ci.                        */
   pFxnAUDIOLIB_subBlockStatistics_exec execute;
   uint32_t                             inputBlockSize;  /**< Samples per channel*/
   uint32_t                             outputBlockSize; /**< Samples per channel*/
   uint32_t                             subBlockSize;    /**< Samples per subblock per channel*/
   uint32_t                             numChannels;     /**< Number of Input Channels*/
   uint8_t                              statisticsType;  /**
                                                          * @brief Selects the statistic to compute for each sub-block.
                                                          * @details The valid options are defined in the
                                                          * ::AUDIOLIB_StatisticsType enum.
                                                          */
   uint32_t strideInElements;                            /**< Stride in Y dimension for input*/
   uint32_t strideOutElements;                           /**< Stride in Y dimension for output*/
   uint8_t  flagPerChanStats;                            /**< Selects channel processing mode: 0 for all-channel
                                                             statistics (single-channel output), 1 for per-channel statistics
                                                             (multi-channel output).*/
   uint32_t pInOffsets[2];                               /**
                                                          * @brief Pre-calculated offsets for the input data pointer.
                                                          * @details Index `[0]` is for all-channel mode, index `[1]` is for per-channel mode.
                                                          */
   uint32_t pOutOffsets[2];                              /**
                                                          * @brief Pre-calculated offsets for the output data pointer.
                                                          * @details Index `[0]` is for all-channel mode, index `[1]` is for per-channel mode.
                                                          */
   uint32_t numBlocks[2];                                /**
                                                          * @brief Pre-calculated loop counter for the number of blocks.
                                                          * @details Index `[0]` is for all-channel mode, index `[1]` is for per-channel mode.
                                                          */
   uint32_t wBlocksArray[2];                             /**
                                                          * @brief Pre-calculated processing width in vector-sized chunks.
                                                          * @details Index `[0]` is for all-channel mode, index `[1]` is for per-channel mode.
                                                          */
   uint32_t subBlockSizes[2];                            /**
                                                          * @brief Pre-calculated number of samples per sub-block calculation.
                                                          * @details Index `[0]` is for all-channel mode, index `[1]` is for per-channel mode.
                                                          */
   uint32_t                             dataType;        /**< Data type of the input and output buffers */
   AUDIOLIB_subBlockStatistics_InitArgs initArgs;        /**< Structure holding initialization parameters  */
   AUDIOLIB_subBlockStatistics_SetArgs  setArgs;         /**< Structure to hold the set parameters*/
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_subBlockStatistics_PrivArgs;

#endif /* AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_PRIV_H_ */
