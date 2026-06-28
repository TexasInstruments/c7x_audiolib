// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_H_
#define AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_sinusoidGenerator AUDIOLIB_sinusoidGenerator
 * @brief Kernel for generating a sine wave with smooth frequency transitions.
 *
 * @details
 * This kernel generates an output signal \f$y[n]\f$ by calculating the sine
 * of a phase \f$\phi[n]\f$:
 * $$ y[n] = \sin(\phi[n]) $$
 *
 * The phase \f$\phi[n]\f$ is accumulated at each sample by a phase increment value,
 * \f$\Delta\phi[n]\f$, which determines the instantaneous frequency \f$f[n]\f$.
 * The phase wraps around \f$2\pi\f$ to remain in range.
 * $$ \phi[n] = (\phi[n-1] + \Delta\phi[n]) \pmod{2\pi} $$
 *
 * The phase increment \f$\Delta\phi[n]\f$ is calculated from the frequency \f$f[n]\f$
 * and the sampling rate \f$f_s\f$:
 * $$ \Delta\phi[n] = \frac{2 \pi f[n]}{f_s} $$
 *
 * The kernel supports smooth transitions between a `startFrequency` and a
 * `targetFrequency` over a specified `smoothingTime`.
 *
 * ### 1. Instant Frequency Change (`smoothingTime = 0`)
 * The phase increment \f$\Delta\phi[n]\f$ is immediately set to the target value
 * derived from `targetFrequency` (\f$f_{\text{target}}\f$).
 * $$ \Delta\phi[n] = \Delta\phi_{\text{target}} = \frac{2 \pi f_{\text{target}}}{f_s} $$
 *
 * ### 2. Smooth Frequency Change (`smoothingTime > 0`)
 * The phase increment \f$\Delta\phi[n]\f$ is updated using a first-order low-pass
 * filter, allowing it to smoothly approach the target phase increment
 * \f$\Delta\phi_{\text{target}}\f$.
 * $$ \Delta\phi[n] = \alpha \cdot \Delta\phi[n-1] + (1 - \alpha) \cdot \Delta\phi_{\text{target}} $$
 *
 * The smoothing coefficient, \f$\alpha\f$, is derived from the `smoothingTime`
 * (\f$T_{\text{smooth}}\f$ in milliseconds, acting as the time constant \f$\tau\f$)
 * and the sampling rate \f$f_s\f$:
 * $$ \alpha = e^{\frac{-1}{f_s \cdot (T_{\text{smooth}} / 1000)}} $$
 *
 * The initial phase \f$\phi[0]\f$ is set via `startPhase` (in degrees), which is
 * converted to radians:
 * $$ \phi[0] = \frac{\text{startPhase} \cdot \pi}{180.0} $$
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Initialization parameters for the Sinusoid Generator kernel.
 * @details These parameters are set once when the kernel is initialized.
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;
   /** @brief Execution mode (e.g., scalar, vector). 0 for scalar, 1 for vector. */
   uint32_t executionMode;
} AUDIOLIB_sinusoidGenerator_InitArgs;

/**
 * @brief Runtime-settable parameters for configuring the Sinusoid Generator.
 * @details This structure is used with @ref AUDIOLIB_sinusoidGenerator_set
 * to control the generator's behavior dynamically.
 */
typedef struct {
   float startFrequency;  /**< @brief The frequency of the signal at the start (in Hz). */
   float targetFrequency; /**< @brief The target frequency for the signal to transition to (in Hz). */
   float startPhase;      /**< @brief The phase at which the signal starts (in degrees). */
   float smoothingTime;   /**< @brief The time to transition from start to target frequency (in milliseconds). 0 for
                             instant change. */
   int32_t  samplingRate; /**< @brief The audio sampling rate in Hz (e.g., 48000). */
   uint32_t data_type;    /**< @brief The precision of the signal (e.g., @ref AUDIOLIB_FLOAT32). */
} AUDIOLIB_sinusoidGenerator_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_sinusoidGenerator_getHandleSize(AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_sinusoidGenerator_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
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

AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_init(AUDIOLIB_kernelHandle                      handle,
                                                AUDIOLIB_bufParams1D_t                    *bufParamsOut,
                                                const AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_sinusoidGenerator_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_sinusoidGenerator_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_sinusoidGenerator_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
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

AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_init_checkParams(AUDIOLIB_kernelHandle                      handle,
                                                            const AUDIOLIB_bufParams1D_t              *bufParamsOut,
                                                            const AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs);
/**
 * @brief       Sets the runtime parameters for the Sinusoid Generator.
 *
 * @param [in]  handle      : Active handle to the kernel.
 * @param [in]  pKerSetArgs : Pointer to the structure containing sinusoid
 * parameters.
 *
 * @details     This function updates the internal state of the kernel.
 * It calculates the necessary `phaseInc` and smoothing
 * coefficients based on the provided settings. For a detailed
 * explanation of the formulas, see the
 * @ref AUDIOLIB_sinusoidGenerator group description.
 *
 * @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 * @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_sinusoidGenerator_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_sinusoidGenerator_SetArgs *pKerSetArgs);

/**
 * @brief       Retrieves the current runtime parameters from the Sinusoid
 * Generator kernel.
 *
 * @param [in]  handle      : Active handle to the kernel.
 * @param [out] pKerSetArgs : Pointer to a structure where the current
 * sinusoid parameters will be stored.
 *
 * @details     This function copies the internal state of the kernel's
 * settings into the provided `pKerSetArgs` structure.
 *
 * @return      Status value indicating success. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 * @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_sinusoidGenerator_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_sinusoidGenerator_SetArgs *pKerSetArgs);
/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_sinusoidGenerator_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_sinusoidGenerator_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_sinusoidGenerator_init is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [out] pout      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS
AUDIOLIB_sinusoidGenerator_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_sinusoidGenerator_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 * * @par Memory Requirements
 * | Buffer | dimY | dimX |  Comments     |
 * | :----: | :--: | :--: | :--------:    |
 * | pOut   | 1    | M    | Output Buffer |
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
 *               @ref AUDIOLIB_sinusoidGenerator_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_sinusoidGenerator_exec(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  dataType       :  Datatype of purticular test case
 *  @param [in]  archCycles     :  Arch cycles used in that purticluar kernel
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *
 *  @return      Void.
 *
 *  @remarks     None
 */
void AUDIOLIB_sinusoidGenerator_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_H_ */
