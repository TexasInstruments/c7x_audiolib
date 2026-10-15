// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_AGC_IXX_IXX_OXX_H_
#define AUDIOLIB_AGC_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_agc AUDIOLIB_agc
 * @brief Slowly varying RMS based gain computer for automatic gain control (AGC)
 *
 * @details
 * This kernel computes the per-sample linear gain for automatic gain control and
 * applies it to the input signal. The input is an M x N buffer and the output is also an M x N buffer with each
 * sample multiplied by the computed per-sample gain.
 *
 * For every call of @ref AUDIOLIB_agc_exec the following steps are done:
 *
 * 1. Mean energy of the block across all channels and samples:
 * \f[ e = \frac{1}{M N} \sum_{m=0}^{M-1} \sum_{n=0}^{N-1} x_m[n]^2 \f]
 *
 * 2. Block-rate smoothing of the energy:
 * \f[ E = (1 - \alpha_B) E + \alpha_B\, e, \qquad
 *     \alpha_B = 1 - e^{-N / (f_s T_s)} \f]
 *
 * 3. Level in dB: \f$ L = 10 \log_{10}(E) \f$
 *
 * 4. If \f$ L \geq \f$ activationThreshold, the target gain is
 * \f[ G_{dB} = (\text{targetLevel} - L) \left(1 - \frac{1}{\text{ratio}}\right) \f]
 * limited to [-maxAttenuation, maxGain], and \f$ G_T = 10^{G_{dB}/20} \f$.
 * Otherwise the target gain is held. If enableRecovery is set, the held gain
 * is instead moved towards 0 dB by recoveryRate dB/sec.
 *
 * 5. Sample-rate smoothing of the output gain:
 * \f[ g[n] = G_T + (1 - \alpha_S)(g[n-1] - G_T), \qquad
 *     \alpha_S = 1 - e^{-1 / (f_s T_s)} \f]
 *
 * Where \f$ f_s \f$ is the sampling rate and \f$ T_s \f$ is smoothingTime in seconds.
 *
 * 6. Calculated gain is multiplied with input: out = gain * in
 *
 * If the input is ratio dB louder than targetLevel, the output after applying the
 * gain is 1 dB above targetLevel.
 *
 * Supported Data Types:
 * - Single Precision: `AUDIOLIB_FLOAT32`
 * - Double Precision: `AUDIOLIB_FLOAT64`
 *
 * @ingroup AUDIOLIB
 */

/**@{*/

/** @brief Values of @ref AUDIOLIB_agc_SetArgs::enableRecovery */
#define AUDIOLIB_AGC_RECOVERY_DISABLE ((uint8_t) 0U) // Hold the target gain below activationThreshold
#define AUDIOLIB_AGC_RECOVERY_ENABLE ((uint8_t) 1U)  // Slew the target gain back to 0 dB below activationThreshold

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE */
   int8_t funcStyle;
   /** @brief Sampling rate of the input in Hz. Must be greater than 0. */
   int32_t samplingRate;
   /** @brief Initial value of the linear output gain. */
   float currentGain;
} AUDIOLIB_agc_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {
   /** @brief Target output level in dB. Typical range -50 to 50. */
   float targetLevel;
   /** @brief Maximum attenuation in dB. Must be greater than or equal to 0. */
   float maxAttenuation;
   /** @brief Maximum gain in dB. Must be greater than or equal to 0. */
   float maxGain;
   /** @brief Compression ratio in dB/dB. Must be greater than or equal to 1. */
   float ratio;
   /** @brief Level in dB below which the gain is frozen (or recovers to 0 dB). */
   float activationThreshold;
   /** @brief Time constant of the energy and gain smoothing in msec. Must be greater than 0. */
   float smoothingTime;
   /** @brief @ref AUDIOLIB_AGC_RECOVERY_ENABLE to slew the held gain back to 0 dB when below
    *         activationThreshold, @ref AUDIOLIB_AGC_RECOVERY_DISABLE to hold it. */
   uint8_t enableRecovery;
   /** @brief Rate at which the gain recovers to 0 dB, in dB/sec. Must be greater than or equal to 0. */
   float recoveryRate;
} AUDIOLIB_agc_SetArgs;

/**
 * @brief  Returns the size in bytes of the handle the application must allocate.
 * @param [in] pKerInitArgs : Pointer to the init parameters
 * @return Handle size in bytes
 */
int32_t AUDIOLIB_agc_getHandleSize(AUDIOLIB_agc_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_agc_set and @ref AUDIOLIB_agc_exec
 *               functions are called. This function resets the internal
 *               state and sets up the configuration of the streaming engine.
 *               The results of these operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output gain buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_agc_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_agc_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_agc_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_agc_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_agc_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS
AUDIOLIB_agc_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_agc_InitArgs *pKerInitArgs);

/**
 * @brief Sets the AGC parameters for the audio processing kernel.
 *
 * @param [in] handle      : Active handle to the kernel.
 * @param [in] pKerSetArgs : Pointer to the structure containing the AGC parameters
 *
 * @details This function validates the parameters, stores a copy of them in the
 *          handle and computes the derived coefficients used by
 *          @ref AUDIOLIB_agc_exec. The internal state (smoothed energy, target
 *          gain and current gain) is not reset, so the parameters can be changed
 *          between calls of @ref AUDIOLIB_agc_exec.
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 * @remarks Must be called after @ref AUDIOLIB_agc_init and before
 *          @ref AUDIOLIB_agc_exec.
 */
AUDIOLIB_STATUS AUDIOLIB_agc_set(AUDIOLIB_kernelHandle handle, const AUDIOLIB_agc_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the AGC parameters last set on the audio processing kernel.
 *
 * @param [in]  handle      : Active handle to the kernel.
 * @param [out] pKerSetArgs : Pointer to the structure where the current AGC parameters
 *                            will be stored.
 *
 * @details This function copies the parameters last passed to @ref AUDIOLIB_agc_set
 *          into the provided `pKerSetArgs` structure.
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_agc_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_agc_SetArgs *pKerSetArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_agc_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_agc_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_agc_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the input buffer
 *  @param [in]  pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS
AUDIOLIB_agc_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function. The internal
 *               state (smoothed energy, target gain and current gain) is
 *               carried across calls.
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Memory Requirements
 * | Buffer   | Comments         |
 * | :--      | :----------------|
 * | pIn      | 2D Input Buffer  |
 * | pOut     | 2D Output Buffer |
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
 *               @ref AUDIOLIB_agc_init and @ref AUDIOLIB_agc_set.
 */
AUDIOLIB_STATUS
AUDIOLIB_agc_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [out] archCycles     :  Arch cycles used in that particular kernel
 *  @param [out] estCycles      :  Cycles estimated for that particular kernel
 */
void AUDIOLIB_agc_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_AGC_IXX_IXX_OXX_H_ */
