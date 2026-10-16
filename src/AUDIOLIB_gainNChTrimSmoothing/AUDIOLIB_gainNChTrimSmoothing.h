// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_H_
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_gainNChTrimSmoothing AUDIOLIB_gainNChTrimSmoothing
 * @brief A kernel that applies an independent, smoothed gain to every channel of a
 *        multichannel audio buffer, with a choice of two fade shapes.
 *
 * @details
 * For every channel \f$c\f$ the kernel computes
 * $$ y_c[n] = x_c[n] \cdot G_c[n] $$
 * The gain each channel moves toward is the product of a common master gain and a
 * per-channel trim gain:
 * $$ G_{\text{target},c} = G_{\text{master}} \cdot G_{\text{trim},c} $$
 *
 * ### fadeType = 0, AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SYMMETRIC - geometric ramp
 * `fadeTime` is the ramp **duration**. When @ref AUDIOLIB_gainNChTrimSmoothing_set changes
 * a channel's target, that channel fades from its running gain \f$G_{0,c}\f$ to the new
 * target by a constant ratio per sample (a straight line in dB):
 * $$ N = \left\lfloor \frac{T_{\text{fade}}}{1000} \cdot f_s \right\rfloor, \qquad
 *    r_c = \left( \frac{G_{\text{target},c}}{G_{0,c}} \right)^{1/N} $$
 * $$ G_c[n] = G_{0,c} \cdot r_c^{\,n}, \quad 0 \le n < N, \qquad
 *    G_c[n] = G_{\text{target},c}, \quad n \ge N $$
 * The first sample after set() is scaled by \f$G_{0,c}\f$, and the gain snaps exactly to
 * the target at sample \f$N\f$. A channel whose target did not change keeps a fade that is
 * already running rather than restarting it.
 *
 * ### fadeType = 1, AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH - one-pole approach
 * `fadeTime` is a **time constant** \f$\tau\f$. Each sample first updates the gain and then
 * scales the sample:
 * $$ k = 1 - e^{\frac{-1}{f_s \cdot (T_{\text{fade}} / 1000)}} $$
 * $$ G_c[n] = G_c[n-1] \cdot (1 - k) + G_{\text{target},c} \cdot k $$
 * The gain approaches the target asymptotically and never snaps.
 *
 * ### fadeTime = 0
 * Either fade type applies the target from the first sample.
 *
 * State is carried across calls: a fade longer than one frame resumes where the previous
 * frame ended.
 *
 * @note Only @ref AUDIOLIB_FLOAT32 buffers are supported.
 * @note Gains are linear, not dB. For SYMMETRIC they must be >= 0; a gain of 0 is treated
 *       as 1e-10 (-200 dB) when the ratio is computed, since the geometric fade cannot pass
 *       through 0, and the fade still ends exactly on the target.
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/** @brief Maximum number of channels the kernel's per-channel fade state can hold. */
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS (64)

/**
 * @brief The shape of the gain fade. See the group description for the formulas.
 */
typedef enum {
   /** @brief Geometric ramp: constant dB per sample, reaches the target exactly at fadeTime. */
   AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SYMMETRIC = 0,
   /** @brief One-pole approach: asymptotic, and fadeTime is a time constant. */
   AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH = 1,
   /** @brief Highest valid fadeType. Used to range-check @ref AUDIOLIB_gainNChTrimSmoothing_set. */
   AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_MAX = AUDIOLIB_GAINNCHTRIMSMOOTHING_FADE_TYPE_SMOOTH
} AUDIOLIB_gainNChTrimSmoothing_fadeType;

/**
 * @brief Initialization parameters for the gainNChTrimSmoothing kernel.
 * @details These parameters are set once when the kernel is initialized.
 */
typedef struct {
   /** @brief The implementation style to use. See @ref AUDIOLIB_FUNCTION_STYLE for options. */
   int8_t funcStyle;
   /** @brief The audio sampling rate in Hz (e.g., 48000). Must be > 0. */
   float samplingRate;
   /** @brief Specifies the data format: 1 for interleaved, 0 for non-interleaved (planar). */
   uint8_t isInterleaved;
} AUDIOLIB_gainNChTrimSmoothing_InitArgs;

/**
 * @brief Runtime-settable parameters for configuring the gainNChTrimSmoothing kernel.
 * @details Used with @ref AUDIOLIB_gainNChTrimSmoothing_set. The gain arrays are read
 *          during the call and are not referenced afterwards, so the caller may reuse them.
 */
typedef struct {
   /** @brief Per-channel linear gain applied from now on, numChannels entries. It seeds the
    *         running gain with the channel at rest and already includes masterGain. Pass
    *         NULL to keep the running gain, as every later set() from a user interface does. */
   float *initialGain;
   /** @brief Per-channel linear trim gain, numChannels entries. */
   float *trimGain;
   /** @brief Linear master gain, common to all channels. */
   float masterGain;
   /** @brief SYMMETRIC: ramp duration in ms. SMOOTH: one-pole time constant in ms.
    *         A value of 0 applies the target gains directly. Must be >= 0. */
   float fadeTime;
   /** @brief The type of fade curve to apply. See @ref AUDIOLIB_gainNChTrimSmoothing_fadeType. */
   uint8_t fadeType;
} AUDIOLIB_gainNChTrimSmoothing_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 *  @remarks      The buffer must be allocated with at least AUDIOLIB_L2DATA_ALIGNMENT
 *                (64-byte) alignment: the optimized kernels perform full-width vector
 *                loads from per-channel state held inside it.
 */
int32_t AUDIOLIB_gainNChTrimSmoothing_getHandleSize(AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_gainNChTrimSmoothing_exec function is called. This
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
 *  @remarks     Application is expected to provide a valid handle. All channels start
 *               at unity gain with no fade in progress.
 */
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_init(AUDIOLIB_kernelHandle                         handle,
                                                   AUDIOLIB_bufParams2D_t                       *bufParamsIn,
                                                   AUDIOLIB_bufParams2D_t                       *bufParamsOut,
                                                   const AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gainNChTrimSmoothing_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gainNChTrimSmoothing_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gainNChTrimSmoothing_init is called.
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
AUDIOLIB_gainNChTrimSmoothing_init_checkParams(AUDIOLIB_kernelHandle                         handle,
                                               const AUDIOLIB_bufParams2D_t                 *bufParamsIn,
                                               const AUDIOLIB_bufParams2D_t                 *bufParamsOut,
                                               const AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs);

/**
 * @brief Sets the runtime parameters for the gainNChTrimSmoothing kernel.
 *
 * @param [in] handle       A valid handle to the kernel instance.
 * @param [in] pKerSetArgs  Pointer to the structure containing the desired parameters.
 *
 * @details Computes each channel's target gain and the fade coefficients for the selected
 *          fade type. For the formulas, see the @ref AUDIOLIB_gainNChTrimSmoothing group
 *          description.
 *
 * @pre @ref AUDIOLIB_gainNChTrimSmoothing_init must have been called on this handle first;
 *      this function reads the sampling rate and channel count established there.
 *
 * @retval AUDIOLIB_SUCCESS           Parameters accepted and applied.
 * @retval AUDIOLIB_ERR_NULL_POINTER  handle, pKerSetArgs or trimGain is NULL.
 * @retval AUDIOLIB_ERR_INVALID_VALUE fadeType out of range, fadeTime < 0, the fade is longer
 *                                    than INT32_MAX samples, or (SYMMETRIC) a gain < 0.
 */
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_set(AUDIOLIB_kernelHandle                  handle,
                                                  AUDIOLIB_gainNChTrimSmoothing_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the last-set runtime parameters from the gainNChTrimSmoothing kernel.
 *
 * @param [in]  handle       A valid handle to the kernel instance.
 * @param [out] pKerSetArgs  Pointer to a structure where the parameters will be stored. The
 *                           gain pointers are the ones last passed to set().
 * @return Status code indicating success or failure. See @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_get(AUDIOLIB_kernelHandle                  handle,
                                                  AUDIOLIB_gainNChTrimSmoothing_SetArgs *pKerSetArgs);

/** * @brief Estimates the performance of the gainNChTrimSmoothing kernel.
 *  @param [in]  handle        : Active handle to the kernel
 *  @param [out] archCycles      : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles       : Pointer to store the estimated cycles for execution
 * *  @details This function provides an estimation of the performance characteristics
 *           of the gainNChTrimSmoothing kernel
 * */
void AUDIOLIB_gainNChTrimSmoothing_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gainNChTrimSmoothing_exec function.
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
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                               const void *restrict pIn,
                                                               const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_gainNChTrimSmoothing
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Memory Requirements
 *
 *      Non-interleave data
 *
 *     | Buffer         | dimY | dimX | Comments |
 *     | :--            | :--: | :--: | :-----|
 *     | pIn            | M    | N    | M channels of N samples |
 *     | pOut           | M    | N    | M channels of N samples |
 *
 *      Interleave data
 *
 *     | Buffer         | dimY | dimX | Comments |
 *     | :--            | :--: | :--: | :-----|
 *     | pIn            | M    | N    | M samples of N channels |
 *     | pOut           | M    | N    | M samples of N channels |
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_gainNChTrimSmoothing_init and
 *               @ref AUDIOLIB_gainNChTrimSmoothing_set.
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothing_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainNChTrimSmoothing.h                   */
/* ======================================================================== */
