// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_BALANCE_IXX_IXX_OXX_H_
#define AUDIOLIB_BALANCE_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_balance AUDIOLIB_balance
 * @brief Kernel for multichannel balance apply
 *
 * @details
 * This kernel adjusts the stereo balance of an audio signal. The balance is controlled
 * by a parameter balance in [-1, 1], where -1 is full left, 1 is full right, and 0 is center.
 * This parameter is used to calculate target gains for the left
 * and right channels using a constant-power panning law.
 *
 * The angle $$ \theta $$ is first calculated from the balance parameter balance:
 * $$ \theta = (1 + balance) \cdot \frac{\pi}{4} $$
 *
 * The target gains are then determined by sine-cosine panning law:
 * $$ G_{L, \text{target}} = \cos(\theta) $$
 * $$ G_{R, \text{target}} = \sin(\theta) $$
 *
 * To prevent abrupt changes when the balance is modified, the gains applied to the
 * signal, $$ G_L[n] $$ and $$ G_R[n] $$, are smoothed over time using a first-order
 * low-pass filter (exponential smoothing). The update equations for the current
 * gains at sample n are:
 * $$ G_L[n] = \alpha \cdot G_L[n-1] + (1 - \alpha) \cdot G_{L, \text{target}} $$
 * $$ G_R[n] = \alpha \cdot G_R[n-1] + (1 - \alpha) \cdot G_{R, \text{target}} $$
 *
 * The smoothing coefficient $$ \alpha $$ is derived from the `smoothingTime` $$ T_{\text{smooth}} $$
 * (in ms) and the `samplingRate` $$ f_s $$ (in Hz):
 * $$ \alpha = e^{\frac{-1}{f_s \cdot (T_{\text{smooth}} / 1000)}} $$
 *
 * If `smoothingTime` is set to zero, this smoothing is bypassed, and the target gains
 * are applied directly.
 *
 * The final output signals, $$ y_L[n] $$ and $$ y_R[n] $$, are computed by applying the
 * current (and possibly smoothed) gains to the input signals, $$ x_L[n] $$ and $$ x_R[n] $$:
 * $$ y_L[n] = x_L[n] \cdot G_L[n] $$
 * $$ y_R[n] = x_R[n] \cdot G_R[n] $$
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;

} AUDIOLIB_balance_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {
   float balance; /**< Panning control parameter ranging from -1 (full left) to 1 (full right), with 0 representing
                     center balance*/

   float smoothingTime; /**< Time constant (in milliseconds) for exponential smoothing of gain transitions, ranging from
                           0 to 1000 msec*/

   int32_t samplingRate; /**< Audio sampling rate (in Hz) defining the number of samples per second */

} AUDIOLIB_balance_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_balance_getHandleSize(AUDIOLIB_balance_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_balance_exec function is called. This
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

AUDIOLIB_STATUS AUDIOLIB_balance_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_balance_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_balance_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_balance_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_balance_init is called.
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
AUDIOLIB_balance_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_balance_InitArgs *pKerInitArgs);

/**
 * @brief Sets the balance parameters for the audio processing kernel.
 *
 * @param handle Active handle to the kernel.
 * @param pKerSetArgs Pointer to the structure containing balance parameters
 *        such as balance level, sampling rate, and smoothing time.
 *
 * @details This function updates the internal state of the kernel with the new
 *          balance settings provided in `pKerSetArgs`. It calculates the target
 *          gains for the left and right channels using a balance value to angle
 *          conversion and computes the smoothing coefficient based on the given
 *          smoothing time and sampling rate.
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */

AUDIOLIB_STATUS AUDIOLIB_balance_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_balance_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the initial balance parameters from the audio processing kernel.
 *
 * @param handle Active handle to the kernel.
 * @param pKerSetArgs Pointer to the structure where the current balance parameters
 *        will be stored, including balance level, sampling rate, and smoothing time.
 *
 * @details This function copies the internal state of the kernel's balance settings
 *          into the provided `pKerSetArgs` structure. It allows the application to
 *          query the current balance configuration.
 *
 * @return Status value indicating success. Refer to @ref AUDIOLIB_STATUS.
 */

AUDIOLIB_STATUS AUDIOLIB_balance_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_balance_SetArgs *pKerSetArgs);

/** * @brief This function provides an estimation of the performance characteristics
 *           of the balance kernel
 *  @param [in]  handle        : Active handle to the kernel
 *  @param [out] archCycles      : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles       : Pointer to store the estimated cycles for execution
 *
 * */
void AUDIOLIB_balance_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_balance_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_balance_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_balance_init is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the structure input buffer
 *  @param [out] pout      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 * */
AUDIOLIB_STATUS AUDIOLIB_balance_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                  const void *restrict pInL,
                                                  const void *restrict pInR,
                                                  const void *restrict pOutL,
                                                  const void *restrict pOutR);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_balance_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pInL       : Pointer to buffer holding the left input buffer
 *  @param [in]  pInR       : Pointer to buffer holding the right input buffer
 *  @param [in]  pOutL      : Pointer to buffer holding the left output buffer
 *  @param [in]  pOutR      : Pointer to buffer holding the right output buffer
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
 *               @ref AUDIOLIB_balance_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_balance_exec(AUDIOLIB_kernelHandle handle,
                      void *restrict pInL,
                      void *restrict pInR,
                      void *restrict pOutL,
                      void *restrict pOutR);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_BALANCE_IXX_IXX_OXX_H_ */
