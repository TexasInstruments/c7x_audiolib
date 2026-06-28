// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_MUTE_IXX_IXX_OXX_H_
#define AUDIOLIB_MUTE_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_mute AUDIOLIB_mute
 * @brief A kernel for applying multichannel audio muting with various fade options.
 *
 * @details
 * This kernel mutes or unmutes an audio signal by applying a gain factor, \f$G[n]\f$,
 * which transitions between 1.0 (unmuted) and 0.0 (muted). To avoid audible clicks,
 * the transition can be smoothed over a specified `fadeTime`. The kernel supports
 * three distinct fade behaviors.
 *
 * ### 1. Hard Mute (`fadeType = 2` or `fadeTime = 0`)
 * The gain changes instantly to its target value.
 * $$ G[n] = G_{\text{target}} $$
 * where \f$G_{\text{target}}\f$ is either 0.0 or 1.0.
 *
 * ### 2. Linear Fade (`fadeType = 0`)
 * The gain changes linearly at a constant rate over the specified `fadeTime`.
 * The per-sample gain step, \f$G_{\text{step}}\f$, is calculated based on the
 * total number of samples in the fade duration:
 * $$ G_{\text{step}} = \frac{G_{\text{target}} - G_{\text{current}}}{ (T_{\text{fade}} / 1000) \cdot f_s } $$
 * The gain at each sample, \f$G[n]\f$, is then updated as:
 * $$ G[n] = G[n-1] + G_{\text{step}} $$
 * where \f$T_{\text{fade}}\f$ is the fade time in milliseconds and \f$f_s\f$ is the sampling rate.
 *
 * ### 3. Smooth Fade (`fadeType = 1`)
 * The gain follows an exponential curve, which provides a more natural-sounding fade.
 * This is implemented using a first-order low-pass filter where the gain
 * smoothly approaches the target. The update equation is:
 * $$ G[n] = \alpha \cdot G[n-1] + (1 - \alpha) \cdot G_{\text{target}} $$
 * The smoothing coefficient, \f$\alpha\f$, is derived from the `fadeTime` (acting as the
 * time constant \f$\tau\f$) and the sampling rate \f$f_s\f$:
 * $$ \alpha = e^{\frac{-1}{f_s \cdot (\text{T}_{\text{fade}} / 1000)}} $$
 *
 * The final output signal, \f$y[n]\f$, is computed by applying the current gain
 * \f$G[n]\f$ to the input signal, \f$x[n]\f$:
 * $$ y[n] = x[n] \cdot G[n] $$
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Initialization parameters for the Mute kernel.
 * @details These parameters are set once when the kernel is initialized.
 */
typedef struct {
   /** @brief The implementation style to use. See @ref AUDIOLIB_FUNCTION_STYLE for options. */
   int8_t funcStyle;
   /** @brief The audio sampling rate in Hz (e.g., 48000). */
   int32_t samplingRate;
   /** @brief Specifies the data format: 1 for interleaved, 0 for non-interleaved (planar). */
   uint8_t isInterleaved;
} AUDIOLIB_mute_InitArgs;

/**
 * @brief Runtime-settable parameters for configuring the Mute kernel.
 * @details This structure is used with AUDIOLIB_mute_set() to control the muting behavior dynamically.
 */
typedef struct {
   /** @brief Control switch for muting: 1 to engage mute (fade-out), 0 to disengage (fade-in). */
   uint32_t isMute;
   /** @brief The duration of the fade-in or fade-out ramp in milliseconds. A value of 0 results in a hard (instant)
    * mute. */
   float fadeTime;
   /** @brief The type of fade curve to apply. See @ref FadeType for options. */
   uint32_t fadeType;
} AUDIOLIB_mute_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_mute_getHandleSize(AUDIOLIB_mute_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_mute_exec function is called. This
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

AUDIOLIB_STATUS AUDIOLIB_mute_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_mute_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_mute_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_mute_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_mute_init is called.
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
AUDIOLIB_mute_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_mute_InitArgs *pKerInitArgs);

/**
 * @brief Sets the runtime parameters for the Mute kernel.
 *
 * @param [in] handle       A valid handle to the kernel instance.
 * @param [in] pKerSetArgs  Pointer to the structure containing the desired mute parameters.
 *
 * @details This function updates the internal state of the kernel based on the provided settings.
 * It calculates the necessary gain coefficients for the selected fade type. For a detailed
 * explanation of the formulas used, please see the @ref AUDIOLIB_mute group description.
 *
 * @return Status code indicating success or failure. See @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_mute_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_mute_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the current runtime parameters from the Mute kernel.
 *
 * @param [in]  handle       A valid handle to the kernel instance.
 * @param [out] pKerSetArgs  Pointer to a structure where the current mute parameters will be stored.
 * @return Status code indicating success or failure. See @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_mute_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_mute_SetArgs *pKerSetArgs);

/** * @brief Estimates the performance of the mute kernel.
 *  @param [in]  handle        : Active handle to the kernel
 *  @param [out] archCycles      : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles       : Pointer to store the estimated cycles for execution
 * *  @details This function provides an estimation of the performance characteristics
 *           of the mute kernel
 * */
void AUDIOLIB_mute_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_mute_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_mute_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_mute_init is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the input buffer
 *  @param [out] pout      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 * */
AUDIOLIB_STATUS
AUDIOLIB_mute_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_mute_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [in]  pOut       : Pointer to buffer holding the output buffer
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
 *               @ref AUDIOLIB_mute_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_mute_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_MUTE_IXX_IXX_OXX_H_ */
