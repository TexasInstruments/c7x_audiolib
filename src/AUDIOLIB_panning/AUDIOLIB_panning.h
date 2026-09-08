// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_panning.h
*******************************************************************************
*/

#ifndef AUDIOLIB_panning_IXX_IXX_OXX_H_
#define AUDIOLIB_panning_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_panning AUDIOLIB_panning
 * @brief Kernel for positioning a mono audio signal across an arbitrary
 *        horizontal speaker layout using 2D Vector Base Amplitude Panning (VBAP).
 *
 * @details
 * The kernel takes a single mono input and distributes it across @c numChannels
 * output channels so that the signal is perceived to originate from
 * @c sourceAzimuth degrees. The speaker layout is described at init time by
 * @c channelConfig, an array of @ref AUDIOLIB_CHANNEL_TYPE values; each entry's
 * nominal angle is looked up in @ref gChannelPositionMap. Any
 * @c AUDIOLIB_CHANNEL_LOW_FREQUENCY_EFFECTS entry is excluded from panning, and
 * at least two distinct directions are required.
 *
 * @par Co-located channels
 * Several channel types share an angle by design, e.g. @c FRONT_LEFT and
 * @c FRONT_LEFT_WOOFER are both drivers of the speaker at -30 deg. Channels at the
 * same azimuth are treated as one direction for the purpose of choosing the
 * speaker pair, and the gain computed for that direction is then applied to every
 * channel occupying it, each receiving the full value on the assumption that a
 * downstream crossover splits the band between the drivers. A layout whose
 * channels are all co-located therefore has only one direction and is rejected,
 * even though it may have two or more channels.
 *
 * @par Gain calculation (VBAP)
 * The active speakers are sorted by azimuth and the adjacent pair bracketing the
 * source direction is selected. Writing that pair's unit vectors as the rows of
 * \f$ L \f$ and the source direction as
 * \f$ p = [\cos(\theta_{src}), \sin(\theta_{src})] \f$, the pair gains solve
 * \f$ g = L^{-1} p \f$ and are then power-normalised:
 * \f[ [g_1, g_2] \leftarrow \frac{[g_1, g_2]}{\sqrt{g_1^2 + g_2^2}} \f]
 * so that \f$ g_1^2 + g_2^2 = 1 \f$. The inverse of each adjacent pair's
 * \f$ 2\times2 \f$ matrix is precomputed once during
 * @ref AUDIOLIB_panning_init. All non-pair channels receive a gain of zero.
 *
 * If either component of \f$ g \f$ is non-positive the source lies outside the
 * selected pair's arc; the kernel then falls back to placing unity gain on the
 * single speaker angularly nearest the source.
 *
 * @par Gain smoothing
 * The gains produced above are targets. Each output channel's applied gain
 * approaches its target with first-order exponential smoothing:
 * \f[ G_c[n] = \alpha G_c[n-1] + (1 - \alpha) G_{c,target} \f]
 * where the coefficient is derived from @c smoothingTime and @c samplingRate:
 * \f[ \alpha = e^{\frac{-1}{f_s \cdot (T_{smooth}/1000)}} \f]
 * The running gain persists across @ref AUDIOLIB_panning_exec calls, so a change
 * of @c sourceAzimuth between frames is traversed smoothly rather than abruptly.
 * If @c smoothingTime is zero, smoothing is bypassed and the target gains are
 * applied directly.
 *
 * @par Output
 * For each output channel @c c:
 * \f[ y_c[n] = x[n] \cdot G_c[n] \f]
 * Both interleaved and non-interleaved (planar) output layouts are supported,
 * selected by @c isInterleave at init time.
 */
/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;
   /** @brief Number of output channels in the speaker layout, i.e. the number of
     *        entries in @c channelConfig. At least two must be directional
     *        (non-LFE) or init fails with @c AUDIOLIB_ERR_INVALID_DIMENSION. */
   int32_t numChannels;
   /** @brief Pointer to an array of @c numChannels @ref AUDIOLIB_CHANNEL_TYPE
     *        values naming the speaker at each output channel. Each speaker's
     *        angle is looked up in @ref gChannelPositionMap; the kernel is not
     *        given angles directly. */
   void *channelConfig;
   /** @brief Output data format: 1 for interleaved, 0 for non-interleaved (planar).
     *        Any other value is rejected. */
   int32_t isInterleave;
} AUDIOLIB_panning_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {

   /** @brief Time constant, in milliseconds, of the first-order exponential
     *        smoothing applied to gain transitions. Must be finite and >= 0; a
     *        value of 0 bypasses smoothing and applies the target gains directly. */
   float smoothingTime;
   /** @brief Audio sampling rate (in Hz) defining the number of samples per second.
     *        Must be greater than zero. */
   int32_t samplingRate;
   /** @brief Direction the source should be perceived from, in degrees. 0 is
     *        front-center, negative is to the left of the listener. Values are
     *        wrapped into [-180, 180], so any finite angle is accepted. */
   float sourceAzimuth;
} AUDIOLIB_panning_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes, or -1 if @c pKerInitArgs is NULL or
 *                specifies zero channels.
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_panning_getHandleSize(AUDIOLIB_panning_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_panning_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of the mono input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of the per-channel gain buffer.
 *                                @c dim_x is the number of channels.
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS. Returns @c AUDIOLIB_ERR_INVALID_DIMENSION if @c channelConfig
 * yields fewer than two distinct directions, either because it has fewer than two
 * non-LFE channels or because all of them are co-located.
 *
 *  @remarks     Application is expected to provide a valid handle. This function
 *               must be called before @ref AUDIOLIB_panning_set, which depends on
 *               the frame length recorded here.
 */

AUDIOLIB_STATUS AUDIOLIB_panning_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams1D_t          *bufParamsGain,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_panning_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_panning_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_panning_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_panning_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of the mono input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of the per-channel gain buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS. Specifically @c AUDIOLIB_ERR_NULL_POINTER for a NULL handle,
 * buffer-parameter structure or @c channelConfig; @c AUDIOLIB_ERR_INVALID_TYPE if
 * the buffers are not all @c AUDIOLIB_FLOAT32; @c AUDIOLIB_ERR_INVALID_VALUE for a
 * non-positive @c numChannels, an @c isInterleave other than 0 or 1, or a
 * @c channelConfig entry that is not a valid @ref AUDIOLIB_CHANNEL_TYPE; and
 * @c AUDIOLIB_ERR_INVALID_DIMENSION if @c numChannels disagrees with the output
 * dimensions, the gain buffer is shorter than @c numChannels, the frame is empty,
 * the input frame length differs from the output's, the output row stride is
 * shorter than a row, or (interleaved) the input row stride is shorter than one
 * element.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_panning_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_panning_InitArgs *pKerInitArgs);

/**
 * @brief Computes the per-channel target gains for a source direction.
 *
 * @param [in]  handle      Active handle to the kernel.
 * @param [out] pGain       Buffer receiving one target gain per output channel.
 *                          Written in full on every call: channels not
 *                          contributing to the source direction are set to zero.
 *                          Refer to @ref AUDIOLIB_panning_exec for the buffer's
 *                          size requirement.
 * @param [in]  pKerSetArgs Pointer to the structure containing the source
 *                          direction and smoothing parameters.
 *
 * @details Selects the speaker pair bracketing @c sourceAzimuth, solves the VBAP
 *          gains for that pair, power-normalises them and writes the result into
 *          @c pGain. Also derives the exponential smoothing coefficient from
 *          @c smoothingTime and @c samplingRate, and selects the execution
 *          variant matching the smoothing and interleave configuration. See
 *          @ref AUDIOLIB_panning for the full gain derivation.
 *
 *          May be called repeatedly between @ref AUDIOLIB_panning_exec calls to
 *          move the source; the kernel's running gain continues from where the
 *          previous frame left off so the transition is smooth. When smoothing
 *          is bypassed the target gains also become the running gains, so
 *          enabling smoothing later ramps from the gain last applied.
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *         Returns @c AUDIOLIB_ERR_NULL_POINTER for a NULL argument,
 *         @c AUDIOLIB_ERR_INVALID_DIMENSION if @ref AUDIOLIB_panning_init did not
 *         succeed, and @c AUDIOLIB_ERR_INVALID_VALUE if @c samplingRate is not
 *         positive, @c smoothingTime is negative or not finite, or
 *         @c sourceAzimuth is not finite.
 *
 * @remarks Must be called after @ref AUDIOLIB_panning_init, which establishes the
 *          speaker geometry and frame length this function depends on.
 */

AUDIOLIB_STATUS
AUDIOLIB_panning_set(AUDIOLIB_kernelHandle handle, void *restrict pGain, AUDIOLIB_panning_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the panning parameters currently in effect.
 *
 * @param [in]  handle      Active handle to the kernel.
 * @param [out] pKerSetArgs Pointer to the structure receiving the current
 *                          @c sourceAzimuth, @c samplingRate and
 *                          @c smoothingTime.
 *
 * @details Copies the settings established by the most recent
 *          @ref AUDIOLIB_panning_set out of the kernel's internal state, allowing
 *          the application to query the active panning configuration.
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */

AUDIOLIB_STATUS AUDIOLIB_panning_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_panning_SetArgs *pKerSetArgs);

/**
 *  @brief       This function provides an estimation of the performance
 *               characteristics of the panning kernel.
 *
 *  @details     The estimate is derived from the loop trip counts recorded during
 *               @ref AUDIOLIB_panning_init and the execution variant selected by
 *               @ref AUDIOLIB_panning_set, so both must have been called first.
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [out] archCycles : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles  : Pointer to store the estimated cycles for execution,
 *                            i.e. the architecture cycles plus fixed overhead
 */
void AUDIOLIB_panning_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_panning_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_panning_exec, and must be called before
 *               @ref AUDIOLIB_panning_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the mono input buffer
 *  @param [in]  pGain     :  Pointer to the per-channel gain buffer
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 * */
AUDIOLIB_STATUS AUDIOLIB_panning_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                  const void *restrict pIn,
                                                  const void *restrict pGain,
                                                  const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Scales the mono input by each channel's smoothed gain and writes
 *               the result to the corresponding output channel. Please refer to
 *               details under @ref AUDIOLIB_panning.
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to the buffer holding the mono input frame
 *  @param [in]  pGain      : Pointer to the per-channel target gains produced by
 *                            @ref AUDIOLIB_panning_set
 *  @param [out] pOut       : Pointer to the buffer receiving the panned output
 *                            channels, interleaved or planar per @c isInterleave
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS. Returns @c AUDIOLIB_ERR_FAILURE if @ref AUDIOLIB_panning_set
 * has not been called since the most recent @ref AUDIOLIB_panning_init.
 *
 *  @par Assumptions:
 *    - The gain buffer is consumed a whole vector at a time, so it must be
 *      allocated to a multiple of 64 bytes even when the channel count occupies
 *      fewer. Any padding lanes are read but do not affect the output.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_panning_init followed by
 *               @ref AUDIOLIB_panning_set. This ensures resource configuration
 *               and error checks are done only once for several invocations of
 *               this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_panning_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_panning_IXX_IXX_OXX_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_panning.h                   */
/* ======================================================================== */
