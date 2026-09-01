// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_GAIN_IXX_IXX_OXX_H_
#define AUDIOLIB_GAIN_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_gain AUDIOLIB_gain
 * @brief Kernel for multichannel gain apply
 *
 * @details
 * @brief Kernel for applying the same gain to multiple channels.
 *
 * ---
 * **Example (gain = 2.0):**
 *
 * \f[
 *   X =
 *   \begin{bmatrix}
 *     1 & 2 & 3 \\
 *     4 & 5 & 6 \\
 *     7 & 8 & 9
 *   \end{bmatrix}
 *   \quad \xrightarrow{\;\text{apply gain}=2.0\;}
 *   Y =
 *   \begin{bmatrix}
 *     2 & 4 & 6 \\
 *     8 & 10 & 12 \\
 *     14 & 16 & 18
 *   \end{bmatrix}
 * \f]
 *
 * ---
 * **Mathematical Definition:**
 *
 * \f[
 *   Y[i][j] = g \cdot X[i][j]
 * \f]
 *
 * Where:
 * - \f$X\f$ = input 2D matrix
 * - \f$Y\f$ = output 2D matrix
 * - \f$g\f$ = scalar gain applied uniformly across all channels
 *@ingroup AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;
} AUDIOLIB_gain_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_gain_getHandleSize(AUDIOLIB_gain_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_gain_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of gain
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

AUDIOLIB_STATUS AUDIOLIB_gain_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams1D_t       *bufParamsGain,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_gain_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gain_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gain_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gain_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of gain
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
AUDIOLIB_gain_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams1D_t *bufParamsGain,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_gain_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gain_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gain_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gain_init is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the structure input buffer
 *  @param [in]  pGain     :  Pointer to the structure input gain
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS AUDIOLIB_gain_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                               const void *restrict pIn,
                                               const void *restrict pGain,
                                               const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_gain_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [in]  pGain      : Pointer to buffer holding the input gain
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  * @par Memory Requirements
 * | Buffer   | dimY | dimX | Comments |
 * | :--      | :--: | :--: | :-----|
 * | pIn      | M    | N    | 2D Input Buffer |
 * | pGain    | 1    | 1    | 2D Delay Buffer |
 * | pOut     | M    | N    | 2D Output Buffer|
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
 *               @ref AUDIOLIB_gain_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_gain_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  archCycles     :  Arch cycles used in that purticluar kernel
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *
 *
 *  @remarks     None
 */
void AUDIOLIB_gain_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_GAIN_IXX_IXX_OXX_H_ */
