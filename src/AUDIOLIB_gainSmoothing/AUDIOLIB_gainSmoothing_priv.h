// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/********************************************************************************
 *  @file     AUDIOLIB_gainSmoothing_priv.h
 *
 *  @brief    File to hold private parameter of function AUDIOLIB_gainSmoothing
 *
 *  @version  0.1 - Oct 2026 : Initial Version
 *
 ********************************************************************************/

#ifndef AUDIOLIB_gainSmoothing_IXX_OXX_PRIV_H_
#define AUDIOLIB_gainSmoothing_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_gainSmoothing.h"

/*!
 * \file
 * \brief Header file for kernel's internal use. For the kernel's interface,
 * please see @ref AUDIOLIB_gainSmoothing
 */

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_gainSmoothing_PrivArgs structure.
 *
 */
#define AUDIOLIB_GAINSMOOTHING_PBLOCK_SIZE (3 * SE_PARAM_SIZE + 3 * SA_PARAM_SIZE)

#define SE_PARAM_BASE (0x0000)              ///< Base offset for hardware accelerator parameter blocks.
#define SE_SE_BYPASS_PARAM_OFFSET (SE_PARAM_BASE) ///< Memory offset for the first Streaming Engine (SE0) template.
#define SE_SA_BYPASS_PARAM_OFFSET                                                                                            \
   (SE_SE_BYPASS_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the first Stream Address Generator (SA0) template.

/// Fade-kernel data template (SE0). Planar: first half of the channels. Interleaved: lane layout.
#define AUDIOLIB_GAINSMOOTHING_SE_FADE_OFFSET (SE_SA_BYPASS_PARAM_OFFSET + SA_PARAM_SIZE)
/// Fade-kernel store template (SA0). Planar: first half of the channels. Interleaved: lane layout.
#define AUDIOLIB_GAINSMOOTHING_SA_FADE_OFFSET (AUDIOLIB_GAINSMOOTHING_SE_FADE_OFFSET + SE_PARAM_SIZE)
/// Planar only: fade-kernel data template of the second half of the channels (SE1).
#define AUDIOLIB_GAINSMOOTHING_SE_FADE_B_OFFSET (AUDIOLIB_GAINSMOOTHING_SA_FADE_OFFSET + SA_PARAM_SIZE)
/// Planar only: fade-kernel store template of the second half of the channels (SA1).
#define AUDIOLIB_GAINSMOOTHING_SA_FADE_B_OFFSET (AUDIOLIB_GAINSMOOTHING_SE_FADE_B_OFFSET + SE_PARAM_SIZE)

/// Number of independent ramp chains (sample rows) per interleaved fade iteration.
#define AUDIOLIB_GAINSMOOTHING_ROW_CHAINS (4)

/// @brief A small floating-point threshold to determine if the current gain is effectively at the target, allowing
/// smoothing to be bypassed.
#define AUDIOLIB_GAINSMOOTHING_GAIN_EPSILON 1e-6f

#if __C7X_VEC_SIZE_BITS__ == 256
#define AUDIOLIB_GAINSMOOTHING_LANES_F32 8
#else
#define AUDIOLIB_GAINSMOOTHING_LANES_F32 16
#endif

/*!
 * @brief Index into AUDIOLIB_gainSmoothing_PrivArgs::execute.
 * @details Deliberately valued 0 and 1 so that the boolean convergence test in
 *          @ref AUDIOLIB_gainSmoothing_exec can be used directly as the index.
 */
typedef enum {
   AUDIOLIB_GAINSMOOTHING_EXEC_RAMP      = 0, ///< Gain is still moving toward the target.
   AUDIOLIB_GAINSMOOTHING_EXEC_CONVERGED = 1, ///< Gain has reached the target; it is now constant.
   AUDIOLIB_GAINSMOOTHING_NUM_EXEC_VARIANTS   ///< Table size.
} AUDIOLIB_GAINSMOOTHING_execVariant;

/* AUDIOLIB_gainSmoothing_fadeType is declared in AUDIOLIB_gainSmoothing.h: callers must
   be able to name the fade types when populating AUDIOLIB_gainSmoothing_SetArgs. */

/*!
 *  @brief Function pointer type of every exec variant (optimized fade and constant-gain
 *         kernels, and the natural C kernels). It matches @ref AUDIOLIB_gainSmoothing_exec.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_gainSmoothing_exec)(AUDIOLIB_kernelHandle handle,
                                                           void *restrict pIn,
                                                           void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainSmoothing_init.
 *
 * @details Builds the streaming engine (SE) and streaming address generator (SA)
 *          templates for the constant-gain kernel and for the fade kernels of the buffer
 *          layout, and stores them in bufPblock. It also stores the loop counts
 *          (numOuter, numInner, bypassIters). The exec kernels only load these templates,
 *          so they spend no cycles working out the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel. The buffer geometry was
 *                                already resolved into it by @ref AUDIOLIB_gainSmoothing_init,
 *                                so bufParams are not passed down here.
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_init_ci(AUDIOLIB_kernelHandle                  handle,
                                               const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs);

/*!
 * @brief SYMMETRIC fade for interleaved buffers.
 * @details One channel per lane, channel blocks of one vector (outer loop). Four sample rows
 *          (vectors) per inner iteration, each with its own ramp chain gain0 * step^(r + 1)
 *          advanced by step^4. Rows are padded to whole groups of four; DECDIM zero-fills and
 *          store-masks the padding rows and the lanes past numChannels. A row whose sample
 *          offset reaches fadeSamplesLeft takes targetGain instead of the ramp gain.
 *
 * @tparam dataType Sample data type.
 *
 * @param [in]  handle  Active handle to the kernel, configured by
 *                      @ref AUDIOLIB_gainSmoothing_init and @ref AUDIOLIB_gainSmoothing_set.
 * @param [in]  pIn     Input buffer.
 * @param [out] pOut    Output buffer.
 *
 * @return Always @ref AUDIOLIB_SUCCESS (parameters are validated by the init/set API).
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSymmetricFade_exec_lane_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief SMOOTH fade for interleaved buffers. Same layout as
 *        @ref AUDIOLIB_gainSmoothingSymmetricFade_exec_lane_ci, with the one-pole decay chains
 *        (gain0 - target) * coeff^(r + 1).
 *
 * @tparam dataType Sample data type.
 *
 * @param [in]  handle  Active handle to the kernel, configured by
 *                      @ref AUDIOLIB_gainSmoothing_init and @ref AUDIOLIB_gainSmoothing_set.
 * @param [in]  pIn     Input buffer.
 * @param [out] pOut    Output buffer.
 *
 * @return Always @ref AUDIOLIB_SUCCESS (parameters are validated by the init/set API).
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_lane_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief SYMMETRIC fade for planar (non-interleaved) buffers.
 * @details Two channels per outer iteration: SE0/SA0 stream channels [0, H) and SE1/SA1
 *          channels [H, 2H), H = ceil(C / 2). For odd C >= 3 the last SE1/SA1 row does not
 *          exist: DECDIM zero-fills it and masks its stores (mono runs both halves on
 *          channel 0 in lockstep). Two vectors per channel per iteration. A lane whose sample
 *          offset reaches fadeSamplesLeft takes targetGain instead of the ramp gain.
 *
 * @tparam dataType Sample data type.
 *
 * @param [in]  handle  Active handle to the kernel, configured by
 *                      @ref AUDIOLIB_gainSmoothing_init and @ref AUDIOLIB_gainSmoothing_set.
 * @param [in]  pIn     Input buffer.
 * @param [out] pOut    Output buffer.
 *
 * @return Always @ref AUDIOLIB_SUCCESS (parameters are validated by the init/set API).
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSymmetricFade_exec_planar_ci(AUDIOLIB_kernelHandle handle,
                                                                   void *restrict pIn,
                                                                   void *restrict pOut);

/*!
 * @brief SMOOTH fade for planar (non-interleaved) buffers. Same channel-pair streaming as
 *        @ref AUDIOLIB_gainSmoothingSymmetricFade_exec_planar_ci, with one-pole decay chains.
 *
 * @tparam dataType Sample data type.
 *
 * @param [in]  handle  Active handle to the kernel, configured by
 *                      @ref AUDIOLIB_gainSmoothing_init and @ref AUDIOLIB_gainSmoothing_set.
 * @param [in]  pIn     Input buffer.
 * @param [out] pOut    Output buffer.
 *
 * @return Always @ref AUDIOLIB_SUCCESS (parameters are validated by the init/set API).
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_planar_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Constant-gain path. Used when fadeTime is 0, and once a ramp has converged
 *        on its target, at which point every remaining sample gets the same gain.
 *
 * @tparam dataType Sample data type.
 *
 * @param [in]  handle  Active handle to the kernel, configured by
 *                      @ref AUDIOLIB_gainSmoothing_init and @ref AUDIOLIB_gainSmoothing_set.
 * @param [in]  pIn     Input buffer.
 * @param [out] pOut    Output buffer.
 *
 * @return Always @ref AUDIOLIB_SUCCESS (parameters are validated by the init/set API).
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingBypass_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Execute function for the NATC implementation of the symmetric fade kernel.
 *
 * @tparam dataType     Sample data type.
 * @param [in]  handle  Active handle to the kernel
 * @param [in]  pIn     Pointer to buffer holding the input buffer
 * @param [out] pOut    Pointer to buffer holding the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 * @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSymmetricFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Execute function for the NATC implementation of the smooth fade kernel.
 *
 * @tparam dataType     Sample data type.
 * @param [in]  handle  Active handle to the kernel
 * @param [in]  pIn     Pointer to buffer holding the input buffer
 * @param [out] pOut    Pointer to buffer holding the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 * @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Dispatch table, indexed by @ref AUDIOLIB_GAINSMOOTHING_execVariant.
    *         @ref AUDIOLIB_gainSmoothing_exec indexes this with the result of its
    *         convergence test, so dispatch is a table lookup and neither the wrapper
    *         nor the exec variants themselves contain a bypass branch. */
   pFxnAUDIOLIB_gainSmoothing_exec execute[AUDIOLIB_GAINSMOOTHING_NUM_EXEC_VARIANTS];
   /** @brief Elements along the fastest-varying axis of the buffer (bufParams dim_x).
    *         Planar: samples per channel. Interleaved: channels per sample frame. */
   uint32_t numCols;
   /** @brief Number of rows to process. */
   uint32_t numRows;
   /** @brief Stride between consecutive rows in the input buffer, in elements. */
   uint32_t strideInElements;
   /** @brief Stride between consecutive rows in the output buffer, in elements. */
   uint32_t strideOutElements;
   /** @brief The current gain value, updated after each execution frame. */
   float currentGain;
   /** @brief The gain the ramp is moving toward. */
   float targetGain;
   /** @brief The smoothing coefficient (`alpha`) for the smooth fade algorithm. */
   float smoothingCoefficient;
     /** @brief The implementation style to use. See @ref AUDIOLIB_FUNCTION_STYLE for options. */
   int8_t funcStyle;
   /** @brief The audio sampling rate in Hz (e.g., 48000). */
   float   samplingRate;
   /** @brief Specifies the data format: 1 for interleaved, 0 for non-interleaved (planar). */
   uint8_t isInterleaved;
   /** @brief A copy of the last-set runtime arguments. */
   AUDIOLIB_gainSmoothing_SetArgs setArgs;
   /** @brief The per-sample gain ratio for SYMMETRIC fading. Multiplicative, not additive. */
   float gainStep;
   /** @brief SYMMETRIC: samples of the ramp still to run. A sample whose offset into the
    *         ramp reaches this count takes targetGain instead of the ramp gain. */
   float fadeSamplesLeft;
   /** @brief A raw buffer to store pre-calculated SE/SA hardware templates. */
   uint8_t bufPblock[AUDIOLIB_GAINSMOOTHING_PBLOCK_SIZE];
   /** @brief Planar only: per-lane powers base^(l + 1) of the gain step (SYMMETRIC) or the
    *         smoothing coefficient (SMOOTH), vector-aligned for CI processing. */
   float alphaCoeff[AUDIOLIB_GAINSMOOTHING_LANES_F32] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief Planar SYMMETRIC, optimized only: 1-based sample offset (l + 1) of each lane within
    *         one vector, compared against fadeSamplesLeft to pick the ramp or the target gain. */
   float laneOffset[AUDIOLIB_GAINSMOOTHING_LANES_F32] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief Planar SYMMETRIC: step^L, advances a ramp chain by one vector of samples. */
   float gainStepMultiplier;
   /** @brief Planar SYMMETRIC: step^(2L), advances a ramp chain by one pair of vectors. */
   float gainStepMultiplierBi;
   /** @brief Planar SMOOTH: coeff^L, advances a decay chain by one vector of samples. */
   float alphaMultiplier;
   /** @brief Planar SMOOTH: coeff^(2L), advances a decay chain by one pair of vectors. */
   float alphaMultiplierBi;
   /** @brief Gain-step (SYMMETRIC) or coefficient (SMOOTH) raised to the frame length: advances
    *         the running gain by one exec() call. */
   float factorPerBlock;
   /** @brief A flag (0 or 1) to indicate if gain ramp should be bypassed. */
   uint8_t bypassSmoothing;
   /** @brief Total number of input samples per processing block. */
   uint32_t inSamples;
   /** @brief Number of channels in the buffer (interleaved: columns, planar: rows). */
   uint32_t numChannels;
   /** @brief Number of samples per channel in one frame (interleaved: rows, planar: columns). */
   uint32_t numSamples;
   /** @brief Optimized only, fade outer loop count. Interleaved: channel blocks of one vector.
    *         Planar: channel pairs, ceil(numChannels / 2). */
   uint32_t numOuter;
   /** @brief Optimized only, fade inner loop count. Interleaved: groups of four (padded) rows.
    *         Planar: groups of two (padded) vectors per channel. */
   uint32_t numInner;
   /** @brief Optimized only: vectors streamed by the constant-gain (bypass) kernel. */
   uint32_t bypassIters;
} AUDIOLIB_gainSmoothing_PrivArgs;

#endif /* AUDIOLIB_gainSmoothing_IXX_OXX_PRIV_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainSmoothing_priv.h          */
/* ======================================================================== */
