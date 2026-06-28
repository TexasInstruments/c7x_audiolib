// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_H_
#define AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_subBlockStatistics AUDIOLIB_subBlockStatistics
 * @brief Kernel for Single Channel/multichannel subBlockStatistics apply
 *
 * @details
 *          - Kernel for applying same subBlockStatistics on multiple channels
 *
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Table of supported statistics for sub-blocks.
 *
 * @details
 * This kernel computes statistics over sub-blocks of the input signal.
 * In the formulas below, let $$x_i$$ represent the i-th sample within the set of samples being processed, and let $$N$$
 * be the total number of samples in that set. The mean, $$\mu$$, is defined as $$\mu = \frac{1}{N} \sum_{i=1}^{N}
 * x_i$$.
 *
 * The definition of $$N$$ depends on the `flagPerChanStats` parameter:
 * - When `flagPerChanStats = 0` (FALSE), statistics are computed across all channels combined.
 * In this case, $$N = \text{subBlockSize} \times \text{numChannels}$$.
 * - When `flagPerChanStats = 1` (TRUE), statistics are computed for each channel independently.
 * In this case, $$N = \text{subBlockSize}$$.
 *
 * ---
 *
 * - **Maximum** (`AUDIOLIB_STAT_MAX`)
 * $$ \max(x_1, x_2, ..., x_N) $$
 *
 * - **Minimum** (`AUDIOLIB_STAT_MIN`)
 * $$ \min(x_1, x_2, ..., x_N) $$
 *
 * - **Maximum Absolute** (`AUDIOLIB_STAT_MAX_ABS`)
 * $$ \max(|x_1|, |x_2|, ..., |x_N|) $$
 *
 * - **Mean** (`AUDIOLIB_STAT_MEAN`)
 * $$ \mu = \frac{1}{N} \sum_{i=1}^{N} x_i $$
 *
 * - **Root Mean Square (RMS)** (`AUDIOLIB_STAT_RMS`)
 * $$ \sqrt{\frac{1}{N} \sum_{i=1}^{N} x_i^2} $$
 *
 * - **Standard Deviation** (`AUDIOLIB_STAT_STDDEV`)
 * $$ \sigma = \sqrt{\frac{1}{N} \sum_{i=1}^{N} (x_i - \mu)^2} $$
 *
 * - **Variance** (`AUDIOLIB_STAT_VARIANCE`)
 * $$ \sigma^2 = \frac{1}{N} \sum_{i=1}^{N} (x_i - \mu)^2 $$
 *
 * - **Average Energy** (`AUDIOLIB_STAT_AVG_ENERGY`)
 * $$ \frac{1}{N} \sum_{i=1}^{N} x_i^2 $$
 *
 * - **Sum** (`AUDIOLIB_STAT_SUM`)
 * $$ \sum_{i=1}^{N} x_i $$
 *
 * - **Sum of Squares** (`AUDIOLIB_STAT_SUM_SQUARES`)
 * $$ \sum_{i=1}^{N} x_i^2 $$
 *
 */

/**
 * @brief Enum defining the types of statistics to compute for each subblock
 */
typedef enum {
   AUDIOLIB_STAT_MAX = 0,    /**< Maximum value */
   AUDIOLIB_STAT_MIN,        /**< Minimum value */
   AUDIOLIB_STAT_MAX_ABS,    /**< Maximum absolute value */
   AUDIOLIB_STAT_MEAN,       /**< Mean value */
   AUDIOLIB_STAT_RMS,        /**< Root mean square */
   AUDIOLIB_STAT_STDDEV,     /**< Standard deviation */
   AUDIOLIB_STAT_VARIANCE,   /**< Variance */
   AUDIOLIB_STAT_AVG_ENERGY, /**< Average energy */
   AUDIOLIB_STAT_SUM,        /**< Sum of values */
   AUDIOLIB_STAT_SUM_SQUARES /**< Sum of squared values */
} AUDIOLIB_StatisticsType;

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t   funcStyle;
   uint32_t subBlockSize; /**< Samples per subblock per channel*/
} AUDIOLIB_subBlockStatistics_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {

   uint8_t flagPerChanStats; /**< Selects channel processing mode: 0 for all-channel statistics (single-channel
                                  output), 1 for per-channel statistics (multi-channel output). */
   uint8_t statisticsType;   /**< Selects the statistic to compute.
                                  Refer to @ref AUDIOLIB_StatisticsType for options. */
   int8_t funcStyle;         /**< Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE*/
} AUDIOLIB_subBlockStatistics_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_subBlockStatistics_getHandleSize(AUDIOLIB_subBlockStatistics_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_subBlockStatistics_exec function is called. This
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

AUDIOLIB_STATUS AUDIOLIB_subBlockStatistics_init(AUDIOLIB_kernelHandle                       handle,
                                                 AUDIOLIB_bufParams2D_t                     *bufParamsIn,
                                                 AUDIOLIB_bufParams2D_t                     *bufParamsOut,
                                                 const AUDIOLIB_subBlockStatistics_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_subBlockStatistics_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_subBlockStatistics_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_subBlockStatistics_init is called.
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
AUDIOLIB_subBlockStatistics_init_checkParams(AUDIOLIB_kernelHandle                       handle,
                                             const AUDIOLIB_bufParams2D_t               *bufParamsIn,
                                             const AUDIOLIB_bufParams2D_t               *bufParamsOut,
                                             const AUDIOLIB_subBlockStatistics_InitArgs *pKerInitArgs);

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
AUDIOLIB_STATUS AUDIOLIB_subBlockStatistics_set(AUDIOLIB_kernelHandle                handle,
                                                AUDIOLIB_subBlockStatistics_SetArgs *pKerSetArgs);
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
AUDIOLIB_STATUS AUDIOLIB_subBlockStatistics_get(AUDIOLIB_kernelHandle                handle,
                                                AUDIOLIB_subBlockStatistics_SetArgs *pKerSetArgs);
/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_subBlockStatistics_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_subBlockStatistics_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_subBlockStatistics_init is called.
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

AUDIOLIB_STATUS AUDIOLIB_subBlockStatistics_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                             const void *restrict pIn,
                                                             const void *restrict pOut);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  archCycles     :  Arch cycles used in that particular kernel
 *  @param [in]  estCycles      :  Cycles estimated for that particular kenel
 *
 *  @return      Void.
 *
 *  @remarks     None
 */
void AUDIOLIB_subBlockStatistics_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_subBlockStatistics_exec
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
 *               @ref AUDIOLIB_subBlockStatistics_init
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_subBlockStatistics_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_SUBBLOCKSTATISTICS_IXX_IXX_OXX_H_ */
