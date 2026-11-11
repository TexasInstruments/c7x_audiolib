// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ASYMMETRICLIMITER_IXX_IXX_OXX_H_
#define AUDIOLIB_ASYMMETRICLIMITER_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_asymmetricLimiter AUDIOLIB_asymmetricLimiter
 * @brief Kernel for limiting audio data to asymmetric lower and upper bounds
 *
 * @details
 * This kernel applies an asymmetric limiter to audio data across multiple channels.
 * Each input sample is constrained to lie within a specified range defined by independent
 * lower and upper bounds, which need not be symmetric about zero. Values below the lower
 * bound are limited to the lower bound, values above the upper bound are limited to the
 * upper bound, and values within the range remain unchanged.
 *
 * The limiting operation is applied uniformly across all channels in a 2D data matrix,
 * where each row represents a channel and each column represents a sample. The kernel
 * supports multi-channel interleaved or non-interleaved memory layouts, provided
 * the dimensional bounds and stride configurations are correctly set.
 *
 * Supported Data Types:
 * - Single Precision: `AUDIOLIB_FLOAT32`
 * - Double Precision: `AUDIOLIB_FLOAT64`
 *
 * ---
 * **Example (asymmetricLimiterLower = -1.0, asymmetricLimiterUpper = 0.5):**
 *
 * \f[
 *   X =
 *   \begin{bmatrix}
 *     -2.5 & 0.5 & 1.5 \\
 *     0.0 & -1.2 & 0.8 \\
 *     2.0 & -0.5 & 0.0
 *   \end{bmatrix}
 *   \quad \xrightarrow{\;\text{limit to }[-1.0, 0.5]\;}
 *   Y =
 *   \begin{bmatrix}
 *     -1.0 & 0.5 & 0.5 \\
 *     0.0 & -1.0 & 0.5 \\
 *     0.5 & -0.5 & 0.0
 *   \end{bmatrix}
 * \f]
 *
 * ---
 * **Mathematical Definition:**
 *
 * \f[
 *   Y[i][j] = \begin{cases}
 *     \text{asymmetricLimiterLower} & \text{if } X[i][j] < \text{asymmetricLimiterLower} \\
 *     X[i][j] & \text{if } \text{asymmetricLimiterLower} \leq X[i][j] \leq \text{asymmetricLimiterUpper} \\
 *     \text{asymmetricLimiterUpper} & \text{if } X[i][j] > \text{asymmetricLimiterUpper}
 *   \end{cases}
 * \f]
 *
 * Where:
 * - \f$X\f$ = input 2D matrix (M rows × N columns)
 * - \f$Y\f$ = output 2D matrix (M rows × N columns)
 * - \f$\text{asymmetricLimiterLower}\f$ = lower limiting bound
 * - \f$\text{asymmetricLimiterUpper}\f$ = upper limiting bound
 *
 *
 * @ingroup AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;
} AUDIOLIB_asymmetricLimiter_InitArgs;

/**
 * @brief Structure containing the run-time parameters of the kernel, to be
 *        configured using @ref AUDIOLIB_asymmetricLimiter_set
 */
typedef struct {
   /** @brief Lower bound for the limited data. Note: this is evaluated at single-precision even for FLOAT64 data. */
   float asymmetricLimiterLower;
   /** @brief Upper bound for the limited data. Note: this is evaluated at single-precision even for FLOAT64 data. */
   float asymmetricLimiterUpper;
} AUDIOLIB_asymmetricLimiter_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_asymmetricLimiter_getHandleSize(AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_asymmetricLimiter_exec function is called. This
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

AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs);

/**
 *  @brief       This function sets the run-time parameters (lower and upper
 *               limiting bounds) of the kernel.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  pKerSetArgs   :  Pointer to the structure holding the lower and
 *                                upper limiting bounds
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     This function must be called after @ref AUDIOLIB_asymmetricLimiter_init
 *               and before @ref AUDIOLIB_asymmetricLimiter_exec. It may be called
 *               again at any time to change the bounds without re-initializing the kernel.
 */
AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_set(AUDIOLIB_kernelHandle handle,
                                               const AUDIOLIB_asymmetricLimiter_SetArgs *pKerSetArgs);

/**
 *  @brief       This function retrieves the run-time parameters currently
 *               configured in the kernel.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [out] pKerSetArgs   :  Pointer to the structure where the current lower
 *                                and upper limiting bounds will be stored
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_get(AUDIOLIB_kernelHandle handle,
                                               AUDIOLIB_asymmetricLimiter_SetArgs *pKerSetArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_asymmetricLimiter_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_asymmetricLimiter_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_asymmetricLimiter_init is called.
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
AUDIOLIB_asymmetricLimiter_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_asymmetricLimiter_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_asymmetricLimiter_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_asymmetricLimiter_init is called.
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

AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                               const void *restrict pIn,
                                               const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_asymmetricLimiter_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Memory Requirements
 * | Buffer   | DataType                 | dimY | dimX | Comments          |
 * | :---     | :---                     | :---:| :---:| :---              |
 * | pIn      | bufParamsIn->data_type   | M    | N    | 2D Input Buffer   |
 * | pOut     | bufParamsOut->data_type  | M    | N    | 2D Output Buffer  |
 *
 *  @par Assumptions:
 *    - @ref AUDIOLIB_asymmetricLimiter_set has been called after init to configure the bounds
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_asymmetricLimiter_init and
 *               @ref AUDIOLIB_asymmetricLimiter_set.
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This function is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [out] archCycles     :  Arch cycles used in that particular kernel
 *  @param [out] estCycles      :  Cycles estimated for that particular kernel
 *
 *  @return      Void.
 *
 *  @remarks     None
 */
void AUDIOLIB_asymmetricLimiter_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_ASYMMETRICLIMITER_IXX_IXX_OXX_H_ */
