// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_GAINSMOOTHING_IXX_OXX_H_
#define AUDIOLIB_GAINSMOOTHING_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_gainSmoothing AUDIOLIB_gainSmoothing
 * @brief A kernel that ramps a gain from a start value to a target value across a
 *        multichannel audio buffer.
 *
 * @details
 * The kernel computes
 * $$ y[n] = x[n] \cdot G[n] $$
 * where \f$G[n]\f$ moves from `currentGain` toward `targetGain` over `fadeTime`. Ramping
 * the gain rather than stepping it avoids the click that a discontinuous gain change
 * produces. The gains are arbitrary positive floats; this is not restricted to fades to
 * or from silence.
 *
 * State is carried across calls: \f$G[n]\f$ resumes where the previous frame ended, so a
 * ramp longer than one frame spans frames seamlessly. Once the target is reached the gain
 * stays there until the next @ref AUDIOLIB_gainSmoothing_set.
 *
 * ### fadeType = 0, AUDIOLIB_gainSmoothing_FADE_TYPE_SYMMETRIC - geometric ramp
 * The gain is multiplied by a constant ratio each sample, i.e. it moves at a constant
 * number of dB per sample:
 * $$ r = \left( \frac{G_{\text{target}}}{G_{\text{current}}} \right)^{1/N},
 *    \qquad N = \frac{T_{\text{fade}}}{1000} \cdot f_s $$
 * $$ G[n] = \text{clamp}\left( G[n-1] \cdot r \right) $$
 * \f$G\f$ reaches \f$G_{\text{target}}\f$ exactly at sample \f$N\f$ and is clamped there.
 * Both gains must be strictly positive. Here `fadeTime` is the total ramp **duration**.
 *
 * ### fadeType = 1, AUDIOLIB_gainSmoothing_FADE_TYPE_SMOOTH - one-pole approach
 * The gain is filtered by a first-order low-pass toward the target:
 * $$ \alpha = e^{\frac{-1}{f_s \cdot (T_{\text{fade}} / 1000)}} $$
 * $$ G[n] = \alpha \cdot G[n-1] + (1 - \alpha) \cdot G_{\text{target}} $$
 * This is asymptotic: \f$G\f$ approaches but never exactly equals
 * \f$G_{\text{target}}\f$. Here `fadeTime` is a **time constant** \f$\tau\f$, not a
 * duration - after `fadeTime` the gain has covered only about 63% of the distance to the
 * target. Any finite gain values are accepted.
 *
 * ### fadeTime = 0
 * Ramping is bypassed for either fade type and `targetGain` is applied to every sample.
 *
 * @note Only @ref AUDIOLIB_FLOAT32 buffers are supported.
 * @note `fadeTime` means different things for the two fade types - see above.
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief The shape of the gain ramp. See the group description for the formulas.
 */
typedef enum {
   /** @brief Geometric ramp: constant dB per sample, reaches the target exactly at
     *        fadeTime. Requires currentGain > 0 and targetGain > 0. */
   AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SYMMETRIC = 0,
   /** @brief One-pole approach: asymptotic, and fadeTime is a time constant rather than
     *        a duration. Accepts any finite gain values. */
   AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH = 1,
   /** @brief Highest valid fadeType. Used to range-check @ref AUDIOLIB_gainSmoothing_set. */
   AUDIOLIB_GAINSMOOTHING_FADE_TYPE_MAX = AUDIOLIB_GAINSMOOTHING_FADE_TYPE_SMOOTH
} AUDIOLIB_gainSmoothing_fadeType;

/**
 * @brief Initialization parameters for the gainSmoothing kernel.
 * @details These parameters are set once when the kernel is initialized.
 */
typedef struct {
   /** @brief The implementation style to use. See @ref AUDIOLIB_FUNCTION_STYLE for options. */
   int8_t funcStyle;
   /** @brief The audio sampling rate in Hz (e.g., 48000). */
   float   samplingRate;
   /** @brief Specifies the data format: 1 for interleaved, 0 for non-interleaved (planar). */
   uint8_t isInterleaved;
} AUDIOLIB_gainSmoothing_InitArgs;

/**
 * @brief Runtime-settable parameters for configuring the gainSmoothing kernel.
 * @details This structure is used with AUDIOLIB_gainSmoothing_set() to control the muting behavior dynamically.
 */
typedef struct {
   /** @brief The gain the ramp starts from. Applied on the first exec() after set();
     *        later frames continue from the kernel's internal running gain. */
   float currentGain;
   /** @brief The gain the ramp moves toward. */
   float targetGain;
   /** @brief SYMMETRIC: total ramp duration in ms. SMOOTH: one-pole time constant in ms.
     *        A value of 0 disables ramping and applies targetGain directly. Must be >= 0. */
   float fadeTime;
   /** @brief The type of fade curve to apply. See @ref AUDIOLIB_gainSmoothing_fadeType. */
   uint8_t  fadeType;
} AUDIOLIB_gainSmoothing_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 *  @remarks      The buffer must be allocated with at least AUDIOLIB_L2DATA_ALIGNMENT
 *                (64-byte) alignment: the optimized kernels perform full-width vector
 *                loads from coefficients held inside it.
 */
int32_t AUDIOLIB_gainSmoothing_getHandleSize(AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs);


/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_gainSmoothing_exec function is called. This
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

AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gainSmoothing_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gainSmoothing_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gainSmoothing_init is called.
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
AUDIOLIB_gainSmoothing_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs);

/**
 * @brief Sets the runtime parameters for the gainSmoothing kernel.
 *
 * @param [in] handle       A valid handle to the kernel instance.
 * @param [in] pKerSetArgs  Pointer to the structure containing the desired gainSmoothing parameters.
 *
 * @details This function updates the internal state of the kernel based on the provided settings.
 * It calculates the necessary gain coefficients for the selected fade type. For a detailed
 * explanation of the formulas used, please see the @ref AUDIOLIB_gainSmoothing group description.
 *
 * @pre @ref AUDIOLIB_gainSmoothing_init must have been called on this handle first; this
 *      function reads the sampling rate and buffer geometry established there.
 *
 * @retval AUDIOLIB_SUCCESS           Parameters accepted and applied.
 * @retval AUDIOLIB_ERR_NULL_POINTER  handle or pKerSetArgs is NULL.
 * @retval AUDIOLIB_ERR_INVALID_VALUE fadeType out of range, fadeTime < 0, or - for
 *                                    SYMMETRIC only - currentGain <= 0 or targetGain <= 0,
 *                                    which would make the geometric ramp produce inf/NaN.
 */
AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_set(AUDIOLIB_kernelHandle handle, 
                                     AUDIOLIB_gainSmoothing_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the current runtime parameters from the gainSmoothing kernel.
 *
 * @param [in]  handle       A valid handle to the kernel instance.
 * @param [out] pKerSetArgs  Pointer to a structure where the current gainSmoothing parameters will be stored.
 * @return Status code indicating success or failure. See @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_get(AUDIOLIB_kernelHandle handle, 
                                     AUDIOLIB_gainSmoothing_SetArgs *pKerSetArgs);

/** * @brief Estimates the performance of the gainSmoothing kernel.
 *  @param [in]  handle        : Active handle to the kernel
 *  @param [out] archCycles      : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles       : Pointer to store the estimated cycles for execution
 * *  @details This function provides an estimation of the performance characteristics
 *           of the gainSmoothing kernel
 * */
void AUDIOLIB_gainSmoothing_perfEst(AUDIOLIB_kernelHandle         handle,
                              uint64_t                     *archCycles,
                              uint64_t                     *estCycles);
                                       
/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gainSmoothing_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gainSmoothing_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gainSmoothing_init is called.
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
AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                   const void *restrict pIn,
                                                   const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_gainSmoothing_exec
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
 *               @ref AUDIOLIB_gainSmoothing_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_gainSmoothing_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_gainSmoothing_IXX_OXX_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainSmoothing.h                   */
/* ======================================================================== */
