// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_BLOCKSTATISTICS_IXX_IXX_OXX_H_
#define AUDIOLIB_BLOCKSTATISTICS_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_blockStatistics AUDIOLIB_blockStatistics
 * @brief Kernel for Single Channel/multichannel BlockStatistics apply
 *
 * @details
 *          - Kernel for applying same BlockStatistics on multiple channels
 *
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Table of supported statistics
 *
 * @details
 * Let \f$x_i\f$ represent the i-th sample in a block of size \f$N\f$. The mean, \f$\mu\f$, is used in the variance and
 * standard deviation calculations.
 *
 * - **Maximum** (`AUDIOLIB_STAT_BLOCK_MAX`)
 * \f[ \max(x_1, x_2, ..., x_N) \f]
 *
 * - **Minimum** (`AUDIOLIB_STAT_BLOCK_MIN`)
 * \f[ \min(x_1, x_2, ..., x_N) \f]
 *
 * - **Maximum Absolute** (`AUDIOLIB_STAT_BLOCK_MAX_ABS`)
 * \f[ \max(|x_1|, |x_2|, ..., |x_N|) \f]
 *
 * - **Mean** (`AUDIOLIB_STAT_BLOCK_MEAN`)
 * \f[ \mu = \frac{1}{N} \sum_{i=1}^{N} x_i \f]
 *
 * - **Root Mean Square (RMS)** (`AUDIOLIB_STAT_BLOCK_RMS`)
 * \f[ \sqrt{\frac{1}{N} \sum_{i=1}^{N} x_i^2} \f]
 *
 * - **Standard Deviation** (`AUDIOLIB_STAT_BLOCK_STDDEV`)
 * \f[ \sigma = \sqrt{\frac{1}{N} \sum_{i=1}^{N} (x_i - \mu)^2} \f]
 *
 * - **Variance** (`AUDIOLIB_STAT_BLOCK_VARIANCE`)
 * \f[ \sigma^2 = \frac{1}{N} \sum_{i=1}^{N} (x_i - \mu)^2 \f]
 *
 * - **Average Energy** (`AUDIOLIB_STAT_BLOCK_AVG_ENERGY`)
 * \f[ \frac{1}{N} \sum_{i=1}^{N} x_i^2 \f]
 *
 * - **Sum** (`AUDIOLIB_STAT_BLOCK_SUM`)
 * \f[ \sum_{i=1}^{N} x_i \f]
 *
 * - **Sum of Squares** (`AUDIOLIB_STAT_BLOCK_SUM_SQUARES`)
 * \f[ \sum_{i=1}^{N} x_i^2 \f]
 *
 */
/**
 * @brief Enum defining the types of statistics to compute for each subblock
 */
typedef enum {
   AUDIOLIB_STAT_BLOCK_MAX = 0,    /**< Maximum value */
   AUDIOLIB_STAT_BLOCK_MIN,        /**< Minimum value */
   AUDIOLIB_STAT_BLOCK_MAX_ABS,    /**< Maximum absolute value */
   AUDIOLIB_STAT_BLOCK_MEAN,       /**< Mean value */
   AUDIOLIB_STAT_BLOCK_RMS,        /**< Root mean square */
   AUDIOLIB_STAT_BLOCK_STDDEV,     /**< Standard deviation */
   AUDIOLIB_STAT_BLOCK_VARIANCE,   /**< Variance */
   AUDIOLIB_STAT_BLOCK_AVG_ENERGY, /**< Average energy */
   AUDIOLIB_STAT_BLOCK_SUM,        /**< Sum of values */
   AUDIOLIB_STAT_BLOCK_SUM_SQUARES /**< Sum of squared values */
} AUDIOLIB_blockStatisticsType;

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;
} AUDIOLIB_blockStatistics_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {
   /**< Selects the statistic to compute.
     Refer to @ref AUDIOLIB_blockStatisticsType for options. */
   uint8_t statisticsType; /**< Selects the statistic to compute for each subblock:
                                 0=max, 1=min, 2=max_abs, 3=mean, 4=rms, 5=std,6=variance, 7=avg_energy, 8=sum,
                              9=sum_squares. */
   int8_t funcStyle; /**< Variant of the function. Refer to @ref AUDIOLIB_FUNCTION_STYLE. */
} AUDIOLIB_blockStatistics_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_blockStatistics_getHandleSize(AUDIOLIB_blockStatistics_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_blockStatistics_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */

AUDIOLIB_STATUS AUDIOLIB_blockStatistics_init(AUDIOLIB_kernelHandle                    handle,
                                              AUDIOLIB_bufParams2D_t                  *bufParamsIn,
                                              AUDIOLIB_bufParams2D_t                  *bufParamsOut,
                                              const AUDIOLIB_blockStatistics_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_blockStatistics_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_blockStatistics_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_blockStatistics_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_init_checkParams(AUDIOLIB_kernelHandle                    handle,
                                          const AUDIOLIB_bufParams2D_t            *bufParamsIn,
                                          const AUDIOLIB_bufParams2D_t            *bufParamsOut,
                                          const AUDIOLIB_blockStatistics_InitArgs *pKerInitArgs);

/**
 *  @brief       This function sets the kernel parameters for sub-block statistics.
 *
 *  @param [in]  handle       : Active handle to the kernel.
 *  @param [in]  pKerSetArgs  : Pointer to the structure containing the parameters to set the kernel.
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 *  @remarks     Ensure that the handle and pKerSetArgs pointer are valid before calling this function.
 */

#ifdef __cplusplus
extern "C" {
#endif
AUDIOLIB_STATUS AUDIOLIB_blockStatistics_set(AUDIOLIB_kernelHandle             handle,
                                             AUDIOLIB_blockStatistics_SetArgs *pKerSetArgs);
#ifdef __cplusplus
}
#endif

/**
 *  @brief       This function retrieves the kernel parameters for sub-block statistics.
 *
 *  @param [in]  handle       : Active handle to the kernel.
 *  @param [out] pKerSetArgs  : Pointer to the structure containing the parameters to set the kernel.
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 *  @remarks     Ensure that the handle and pKerSetArgs pointer are valid before calling this function.
 */
AUDIOLIB_STATUS AUDIOLIB_blockStatistics_get(AUDIOLIB_kernelHandle             handle,
                                             AUDIOLIB_blockStatistics_SetArgs *pKerSetArgs);
/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_blockStatistics_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_blockStatistics_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_blockStatistics_init is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the structure input buffer
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS AUDIOLIB_blockStatistics_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                          const void *restrict pIn,
                                                          const void *restrict pOut);

/**
 *  @brief        This function is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  archCycles     :  Arch cycles used in that particular kernel
 *  @param [in]  estCycles      :  Cycles estimated for that particular kernel
 *
 *  @remarks     None
 */
void AUDIOLIB_blockStatistics_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_blockStatistics_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - None
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_blockStatistics_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_BLOCKSTATISTICS_IXX_IXX_OXX_H_ */
