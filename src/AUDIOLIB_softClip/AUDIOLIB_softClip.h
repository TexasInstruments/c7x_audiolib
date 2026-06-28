// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_H_
#define AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_softClip AUDIOLIB_softClip
 * @brief Kernel for applying a soft clipping function to an audio signal.
 *
 * @details
 * Soft clipping is a form of signal limiting that smoothly compresses signal peaks
 * that exceed a certain threshold, rather than sharply cutting them off (hard clipping).
 * This results in a less harsh form of distortion, often perceived as "warmer".
 *
 * The kernel's behavior is defined by a piecewise function. Let `x` be the input sample,
 * `y` be the output sample, `T` be the `threshold`, and `E` be the `endKnee`.
 *
 * The output `y` is calculated as:
 * $$
 * y(x) =
 * \begin{cases}
 * x & \text{if } |x| < T ;\\
 * \\
 * \text{sgn} \left( (E - T) \frac{|x| - T}{|x| - 2T + E} + T \right) & \text{if } |x| \ge T
 * \end{cases}
 * $$
 *
 * ### Parameter Behavior
 *
 * - **Threshold (`T`):** Below this level, the signal passes through unchanged (the linear region). Above this level,
 * the signal is smoothly compressed.
 * - **End Knee (`E`):** This is the maximum output level that the curve asymptotically approaches as the input level
 * increases.
 *
 * ---
 * ### Implementation Details
 *
 * For input signals where $$|x| \ge T$$, the calculation is performed in two steps to match the source code
 * implementation:
 *
 * 1.  First, an intermediate positive output value, let's call it $$y_{mag}$$, is calculated based on the absolute
 * value of the input, $$|x|$$. This corresponds to the `out` variable in the C code.
 * $$ y_{mag} = (E - T) \left( \frac{|x| - T}{|x| - 2T + E} \right) + T $$
 *
 * 2.  Then, the original sign of the input is reapplied to this intermediate value to get the final output, which
 * directly mirrors the `if/else` logic in the code:
 * $$
 * y =
 * \begin{cases}
 * -y_{mag} & \text{if } x < 0 \\
 * \\
 * y_{mag} & \text{if } x \ge 0
 * \end{cases}
 * $$
 *
 * @ingroup  AUDIOLIB
 */
/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;
} AUDIOLIB_softClip_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {
   float threshold; /**< The amplitude (linear scale) at which soft clipping begin*/
   float endKnee;   /**< *The amplitude where the soft clipping curve fully transitions to its maximum limit. Between
                       threshold and endKnee, the signal is smoothly compressed*/

} AUDIOLIB_softClip_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_softClip_getHandleSize(AUDIOLIB_softClip_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_softClip_exec function is called. This
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

AUDIOLIB_STATUS AUDIOLIB_softClip_init(AUDIOLIB_kernelHandle             handle,
                                       AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                       AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                       const AUDIOLIB_softClip_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_softClip_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_softClip_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_softClip_init is called.
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
 *  AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS AUDIOLIB_softClip_init_checkParams(AUDIOLIB_kernelHandle             handle,
                                                   const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                                   const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                                   const AUDIOLIB_softClip_InitArgs *pKerInitArgs);
/**
 *  @brief       This function updates the internal state of the kernel with the new
 *               softClip settings provided in pKerSetArgs. This function must be
 *               called before the @ref AUDIOLIB_softClip_exec function is called.
 *
 *  @param [in]  handle      : Active handle to the kernel.
 *  @param [in]  pKerSetArgs : Pointer to the structure containing softClip parameters
 *                             such as threshold, endKnee.
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *  AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS
AUDIOLIB_softClip_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_softClip_SetArgs *pKerSetArgs);

/**
 *  @brief       This function copies the internal state of the kernel's softClip settings
 *               into the provided pKerSetArgs structure. It allows the application to
 *               query the current softClip configuration. This function is called when
 *               the user wants to know about softClip settings.
 *
 *  @param [in]  handle      : Active handle to the kernel.
 *  @param [in]  pKerSetArgs : Pointer to the structure where the current softClip parameters
 *                             will be stored, including threshold, endKnee.
 *
 *  @return      Status value indicating success. Refer to @ref
 *  AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS
AUDIOLIB_softClip_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_softClip_SetArgs *pKerSetArgs);
/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_softClip_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_softClip_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_softClip_init is called.
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

AUDIOLIB_STATUS
AUDIOLIB_softClip_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_softClip_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 * * @par Memory Requirements
 * | Buffer | dimY | dimX |  Comments     |
 * | :----: | :--: | :--: | :--------:    |
 * | pIn    | M    | N    | Input Buffer  |
 * | pOut   | M    | N    | Output Buffer |
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
 *               @ref AUDIOLIB_softClip_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_softClip_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This function is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  archCycles     :  Arch cycles used in that purticluar kernel
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *
 *  @return      Void.
 *
 *  @remarks     None
 */
void AUDIOLIB_softClip_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_H_ */
