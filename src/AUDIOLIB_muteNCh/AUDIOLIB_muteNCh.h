// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_MUTENCH_IXX_IXX_OXX_H_
#define AUDIOLIB_MUTENCH_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_muteNCh AUDIOLIB_muteNCh
 * @brief A kernel for applying multichannel audio muting with various fade options.
 *
 * @details
 * This kernel mutes or unmutes a multi-channel audio signal by applying a per-channel gain
 * factor, \f$G_c[n]\f$, which transitions between 1.0 (unmuted) and 0.0 (muted). To avoid
 * audible clicks, the transition can be smoothed over a specified `fadeTime`. The kernel
 * supports three distinct fade behaviors for each channel.
 *
 * ### 1. Hard Mute (`fadeType = 2` or `fadeTime = 0`)
 * The gain for a channel changes instantly to its target value.
 * $$ G_c[n] = G_{c, \text{target}} $$
 * where \f$G_{c, \text{target}}\f$ is either 0.0 or 1.0.
 *
 * ### 2. Linear Fade (`fadeType = 0`)
 * The gain changes linearly at a constant rate over the specified `fadeTime`.
 * The per-sample gain step, \f$G_{c, \text{step}}\f$, is calculated based on the
 * total number of samples in the fade duration:
 * $$ G_{c, \text{step}} = \frac{G_{c, \text{target}} - G_{c, \text{current}}}{ (T_{\text{fade}} / 1000) \cdot f_s } $$
 * The gain at each sample, \f$G_c[n]\f$, is then updated as:
 * $$ G_c[n] = G_c[n-1] + G_{c, \text{step}} $$
 * where \f$T_{\text{fade}}\f$ is the fade time in milliseconds and \f$f_s\f$ is the sampling rate.
 *
 * ### 3. Smooth Fade (`fadeType = 1`)
 * The gain follows an exponential curve, which provides a more natural-sounding fade.
 * This is implemented using a first-order low-pass filter where the gain
 * smoothly approaches the target. The update equation is:
 * $$ G_c[n] = \alpha \cdot G_c[n-1] + (1 - \alpha) \cdot G_{c, \text{target}} $$
 * The smoothing coefficient, \f$\alpha\f$, is derived from the `fadeTime` (acting as the
 * time constant \f$\tau\f$) and the sampling rate \f$f_s\f$:
 * $$ \alpha = e^{\frac{-1}{f_s \cdot (T_{\text{fade}} / 1000)}} $$
 *
 * The final output signal for each channel, \f$y_c[n]\f$, is computed by applying the current gain
 * \f$G_c[n]\f$ to the input signal, \f$x_c[n]\f$:
 * $$ y_c[n] = x_c[n] \cdot G_c[n] $$
 *
 * The supported datatype is float.
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Initialization parameters for the MuteNCh kernel.
 * @details These parameters are set once when the kernel is initialized and do not change during runtime.
 */
typedef struct {
   /** @brief The implementation style to use. See @ref AUDIOLIB_FUNCTION_STYLE for options. */
   int8_t funcStyle;
   /** @brief The audio sampling rate in Hz (e.g., 48000). */
   int32_t samplingRate;
   /** @brief Specifies the data format: 1 for interleaved, 0 for non-interleaved (planar). */
   uint8_t isInterleaved;
   /** @brief The total number of audio channels to process. */
   uint32_t numChannels;
} AUDIOLIB_muteNCh_InitArgs;

/**
 * @brief Runtime-settable parameters for configuring the Mute kernel.
 * @details This structure is used with AUDIOLIB_muteNCh_set() to control the muting behavior dynamically.
 */
typedef struct {
   /** @brief Per-channel control switch for muting. This is an array of size `numChannels`, where each element is
    * 1 to engage mute (fade-out) or 0 to disengage (fade-in) for that channel. */
   uint32_t *isMute;
   /** @brief The duration of the fade-in or fade-out ramp in milliseconds. A value of 0 results in a hard
    * (instantaneous) mute. */
   float fadeTime;
   /** @brief The type of fade curve to apply. See @ref FadeType for options. */
   uint32_t fadeType;
} AUDIOLIB_muteNCh_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_muteNCh_getHandleSize(AUDIOLIB_muteNCh_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_muteNCh_exec function is called. This
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

AUDIOLIB_STATUS AUDIOLIB_muteNCh_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_muteNCh_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_muteNCh_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_muteNCh_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_muteNCh_init is called.
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
AUDIOLIB_muteNCh_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_muteNCh_InitArgs *pKerInitArgs);

/**
 * @brief Sets the runtime parameters for the MuteNCh kernel.
 *
 * @param [in] handle       A valid handle to the kernel instance.
 * @param [in] pKerSetArgs  Pointer to the structure containing the desired mute parameters.
 *
 * @details This function updates the internal state of the kernel based on the provided settings.
 * It calculates the necessary gain coefficients and assigns the correct internal processing
 * function based on the selected fade type. For a detailed explanation of the formulas,
 * see the @ref AUDIOLIB_muteNCh group description.
 *
 * @return Status code indicating success or failure. See @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_muteNCh_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_muteNCh_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the current runtime parameters from the MuteNCh kernel.
 *
 * @param [in]  handle       A valid handle to the kernel instance.
 * @param [out] pKerSetArgs  Pointer to a structure where the current mute parameters will be stored.
 * @return Status code indicating success or failure. See @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_muteNCh_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_muteNCh_SetArgs *pKerSetArgs);

/**
 *  @brief Estimates the performance of the muteNCh kernel.
 *  @param [in]  handle        : Active handle to the kernel
 *  @param [out] archCycles      : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles       : Pointer to store the estimated cycles for execution
 *  @details This function provides an estimation of the performance characteristics
 *           of the muteNCh kernel
 */
void AUDIOLIB_muteNCh_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_muteNCh_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_muteNCh_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_muteNCh_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the input buffer
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 * */
AUDIOLIB_STATUS
AUDIOLIB_muteNCh_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 * @brief       This is the main processing function for the MuteNCh kernel.
 *
 * @details     This function applies the configured mute, unmute, or fade logic to a frame of multi-channel audio data.
 *
 * @param [in]  handle     Active handle to the kernel.
 * @param [in]  pIn        Pointer to the buffer holding the input audio data.
 * @param [out] pOut       Pointer to the buffer where the output audio data will be written.
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 * @par Performance Considerations:
 * For best performance:
 * - Input and output data buffers should reside in L2 memory.
 * - Buffer pointers should be 64-byte aligned.
 *
 * @remarks     Before calling this function, the application must have successfully called
 * @ref AUDIOLIB_muteNCh_init and @ref AUDIOLIB_muteNCh_set.
 */

AUDIOLIB_STATUS
AUDIOLIB_muteNCh_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_MUTENCH_IXX_IXX_OXX_H_ */
