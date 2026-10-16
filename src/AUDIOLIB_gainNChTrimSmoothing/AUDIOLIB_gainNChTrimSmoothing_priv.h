// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/********************************************************************************
 *  @file     AUDIOLIB_gainNChTrimSmoothing_priv.h
 *
 *  @brief    File to hold private parameter of function AUDIOLIB_gainNChTrimSmoothing
 *
 *  @version  0.1 - Oct 2026 : Initial Version
 *
 ********************************************************************************/

#ifndef AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_PRIV_H_
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_gainNChTrimSmoothing.h"

/*!
 * \file
 * \brief Header file for kernel's internal use. For the kernel's interface,
 * please see @ref AUDIOLIB_gainNChTrimSmoothing
 */

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_gainNChTrimSmoothing_PrivArgs structure.
 *
 */
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_PBLOCK_SIZE (4 * SE_PARAM_SIZE + 3 * SA_PARAM_SIZE)

/// Offset of the fade-kernel data template (SE0; planar: first half of the channels).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_OFFSET (0x0000)
/// Offset of the fade-kernel store template (SA0; planar: first half of the channels).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_OFFSET (AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_OFFSET + SE_PARAM_SIZE)
/// Offset of the constant-gain kernel data template (SE0).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_BYP_DATA_OFFSET (AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_OFFSET + SA_PARAM_SIZE)
/// Offset of the constant-gain kernel store template (SA0).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_BYP_DATA_OFFSET (AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_BYP_DATA_OFFSET + SE_PARAM_SIZE)
/// Offset of the constant-gain kernel gain template (SE1, reads laneTarget).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_BYP_GAIN_OFFSET (AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_BYP_DATA_OFFSET + SA_PARAM_SIZE)
/// Planar only: fade-kernel data template of the second half of the channels (SE1).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_B_OFFSET (AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_BYP_GAIN_OFFSET + SE_PARAM_SIZE)
/// Planar only: fade-kernel store template of the second half of the channels (SA1).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_SA_FADE_B_OFFSET (AUDIOLIB_GAINNCHTRIMSMOOTHING_SE_FADE_B_OFFSET + SE_PARAM_SIZE)

/// Number of independent ramp chains (rows) per interleaved fade iteration.
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_ROW_CHAINS (4)

/// @brief Relative tolerance under which a channel's new target is treated as equal to its
/// previous target, so a running SYMMETRIC fade is kept. Matches numpy.isclose(rtol=1e-6).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_RTOL (1e-6f)

/// @brief Absolute tolerance paired with the relative tolerance above (numpy.isclose atol).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_ATOL (1e-8f)

/// @brief Floor applied to both gains when computing the SYMMETRIC fade ratio, which is
/// undefined for a gain of 0. 1e-10 is -200 dB.
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_GAIN_FLOOR (1e-10f)

/// @brief Number of float32 lanes in one full C7x vector, derived from the device-native
/// vector type (vector-length agnostic; no device fork).
#define AUDIOLIB_GAINNCHTRIMSMOOTHING_LANES_F32                                                                        \
   ((uint32_t) c7x::element_count_of<c7x::make_full_vector<float>::type>::value)

/*!
 * @brief Stream layout chosen by @ref AUDIOLIB_gainNChTrimSmoothing_init_ci.
 */
typedef enum {
   /** Planar: one channel per row; channel pairs streamed on SE0/SE1. */
   AUDIOLIB_GAINNCHTRIMSMOOTHING_LAYOUT_PLANAR = 0,
   /** Interleaved: one channel per lane, channel blocks of one vector. */
   AUDIOLIB_GAINNCHTRIMSMOOTHING_LAYOUT_BLOCK = 1
} AUDIOLIB_GAINNCHTRIMSMOOTHING_layout;

/*!
 * @brief Index into AUDIOLIB_gainNChTrimSmoothing_PrivArgs::execute.
 * @details Deliberately valued 0 and 1 so that the convergence test in
 *          @ref AUDIOLIB_gainNChTrimSmoothing_exec can be used directly as the index.
 */
typedef enum {
   AUDIOLIB_GAINNCHTRIMSMOOTHING_EXEC_RAMP      = 0, ///< At least one channel is still fading.
   AUDIOLIB_GAINNCHTRIMSMOOTHING_EXEC_CONVERGED = 1, ///< Every channel is on its target; gains are constant.
   AUDIOLIB_GAINNCHTRIMSMOOTHING_NUM_EXEC_VARIANTS   ///< Table size.
} AUDIOLIB_GAINNCHTRIMSMOOTHING_execVariant;

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_gainNChTrimSmoothing_exec.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_gainNChTrimSmoothing_exec)(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainNChTrimSmoothing_init.
 *
 * @details Chooses the stream layout (planar / interleaved channel block), builds all
 *          streaming engine and address generator templates and stores them in bufPblock,
 *          so the exec functions only load them. Also fills the planar lane table
 *          (laneOffset) and resets laneTarget to unity. Every row dimension is padded to whole loop iterations and the padding
 *          is zero-filled / store-masked by DECDIM, so no kernel has a remainder path.
 *
 *  @param [in]  handle        :  Active handle to the kernel. The buffer geometry was
 *                                already resolved into it by @ref AUDIOLIB_gainNChTrimSmoothing_init.
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_init_ci(AUDIOLIB_kernelHandle                         handle,
                                                      const AUDIOLIB_gainNChTrimSmoothing_InitArgs *pKerInitArgs);

/*!
 * @brief C7x part of @ref AUDIOLIB_gainNChTrimSmoothing_set: fills the per-lane power and
 *        target tables for the layout and fade type (double precision), and picks the
 *        ramp / converged kernels.
 *
 * @param [in]  handle         Active handle; the per-channel state has already been updated.
 * @param [in]  fadeType       @ref AUDIOLIB_gainNChTrimSmoothing_fadeType.
 * @param [in]  isInstant      SMOOTH only: 1 when the target weight is exactly 1 (fadeTime 0).
 * @param [out] pRampFxn       Kernel for execute[RAMP].
 * @param [out] pConvergedFxn  Kernel for execute[CONVERGED].
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNChTrimSmoothing_set_ci(AUDIOLIB_kernelHandle                   handle,
                                                     uint8_t                                 fadeType,
                                                     uint8_t                                 isInstant,
                                                     pFxnAUDIOLIB_gainNChTrimSmoothing_exec *pRampFxn,
                                                     pFxnAUDIOLIB_gainNChTrimSmoothing_exec *pConvergedFxn);

/*!
 * @brief SYMMETRIC fade for interleaved buffers, C7x implementation.
 * @details Lane-per-channel: one vector of channels per sample row, channel blocks
 *          outer. Four rows (vectors) per iteration with four independent ramp chains
 *          (gain0 * step^s) advanced by the table rowAdv, so the 4-cycle multiply
 *          recurrence overlaps. Lanes whose fade budget is used up snap
 *          exactly to the target. The result matches natural C to rounding.
 *
 * @tparam dataType Sample data type.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingLaneSymmetric_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief SMOOTH fade for interleaved buffers, C7x implementation.
 * @details Same layout and four-chain structure as
 *          @ref AUDIOLIB_gainNChTrimSmoothingLaneSymmetric_exec_ci; the chains hold the
 *          one-pole decay (gain0 - target) * coeff^(s + 1) and the gain is decay + target.
 *
 * @tparam dataType Sample data type.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingLaneSmooth_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief SYMMETRIC fade for planar buffers, C7x implementation.
 * @details Two channels per outer iteration: SE0/SA0 stream channels [0, H) and SE1/SA1
 *          channels [H, 2H), H = ceil(C / 2). For odd C >= 3 the last SE1/SA1 row (channel C)
 *          does not exist: DECDIM zero-fills it without reading memory and masks its stores
 *          (mono runs both halves on channel 0 in lockstep). Two
 *          vectors per channel per iteration, i.e. four independent chains.
 *
 * @tparam dataType Sample data type.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingPlanarSymmetric_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief SMOOTH fade for planar buffers, C7x implementation.
 * @details Same channel-pair streaming as
 *          @ref AUDIOLIB_gainNChTrimSmoothingPlanarSymmetric_exec_ci, with the one-pole decay
 *          chains of the SMOOTH fade.
 *
 * @tparam dataType Sample data type.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingPlanarSmooth_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Constant-gain path for both layouts: every sample of a channel is scaled by its
 *        target. Data on SE0, the matching per-lane target vector on SE1 (laneTarget), store
 *        on SA0. Used once every channel of a SYMMETRIC fade has reached its target, and for
 *        a SMOOTH fade with fadeTime 0. Kept in AUDIOLIB_gainNChTrimSmoothing_PrivArgs::bypassFxn.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingBypass_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Execute function for the NATC implementation of the SYMMETRIC fade.
 *
 * @tparam dataType     Sample data type.
 * @param [in]  handle  Active handle to the kernel
 * @param [in]  pIn     Pointer to buffer holding the input buffer
 * @param [out] pOut    Pointer to buffer holding the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingSymmetric_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Execute function for the NATC implementation of the SMOOTH fade.
 *
 * @tparam dataType     Sample data type.
 * @param [in]  handle  Active handle to the kernel
 * @param [in]  pIn     Pointer to buffer holding the input buffer
 * @param [out] pOut    Pointer to buffer holding the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainNChTrimSmoothingSmooth_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 * @details The per-channel arrays are vector-aligned and sized to a whole number of
 *          vectors, so the optimized kernels can load and store a full channel block
 *          without reading past them.
 */
typedef struct {
   /*! @brief Dispatch table filled by @ref AUDIOLIB_gainNChTrimSmoothing_set, indexed by
    *         @ref AUDIOLIB_GAINNCHTRIMSMOOTHING_execVariant. @ref AUDIOLIB_gainNChTrimSmoothing_exec
    *         indexes it with its convergence test, so a SYMMETRIC fade switches to the
    *         constant-gain kernel once it ends and the fade kernels contain no bypass branch. */
   pFxnAUDIOLIB_gainNChTrimSmoothing_exec execute[AUDIOLIB_GAINNCHTRIMSMOOTHING_NUM_EXEC_VARIANTS];
   /*! @brief Constant-gain kernel for the buffer layout, chosen once in
    *         @ref AUDIOLIB_gainNChTrimSmoothing_init so set() never has to test the layout for it. */
   pFxnAUDIOLIB_gainNChTrimSmoothing_exec bypassFxn;
   /** @brief SYMMETRIC: fade samples left on the channel that finishes last. Every channel
    *         advances by one frame per exec(), so this falls by numSamples per call and the
    *         whole kernel has converged when it reaches 0. */
   uint32_t maxRemainingFadeSamples;
   /** @brief Number of channels in the buffer. */
   uint32_t numChannels;

   /** @brief Number of samples per channel in one frame. */
   uint32_t numSamples;
   /** @brief Stride between consecutive rows in the input buffer, in elements. */
   uint32_t strideInElements;
   /** @brief Stride between consecutive rows in the output buffer, in elements. */
   uint32_t strideOutElements;
   /** @brief The implementation style to use. See @ref AUDIOLIB_FUNCTION_STYLE for options. */
   int8_t funcStyle;
   /** @brief The audio sampling rate in Hz (e.g., 48000). */
   float samplingRate;
   /** @brief Specifies the data format: 1 for interleaved, 0 for non-interleaved (planar). */
   uint8_t isInterleaved;
   /** @brief A copy of the last-set runtime arguments. */
   AUDIOLIB_gainNChTrimSmoothing_SetArgs setArgs;
   /** @brief SMOOTH: weight of the previous gain, exp(-1 / (fs * tau)) = 1 - k. The distance
    *         to the target decays by this factor per sample. */
   float smoothingCoeff;
   /** @brief Optimized only: stream layout, @ref AUDIOLIB_GAINNCHTRIMSMOOTHING_layout. */
   uint8_t layout;
   /** @brief Optimized only. Interleaved: channel blocks. Planar: channel
    *         pairs, ceil(numChannels / 2). */
   uint32_t numOuter;
   /** @brief Optimized only, fade loop trip count per outer iteration. Interleaved: groups of
    *         four (padded) vector rows. Planar: groups of two (padded) vectors per channel. */
   uint32_t numInner;
   /** @brief Optimized only: vectors streamed by the constant-gain kernel. */
   uint32_t bypassIters;
   /** @brief Per-channel running gain, carried from one exec() to the next. */
   float currentGain[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS]
       __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief Per-channel gain the fade moves toward: masterGain * trimGain[ch]. */
   float targetGain[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS]
       __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief SYMMETRIC: per-channel, per-sample fade ratio. Multiplicative, not additive. */
   float gainStep[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS]
       __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief SYMMETRIC: per-channel number of samples left in the fade; 0 once the target is
    *         reached. Signed so the optimized kernels can compare it as a vector. */
   int32_t remainingFadeSamples[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS]
       __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /*
    * Optimized-only tables, filled by AUDIOLIB_gainNChTrimSmoothing_set_ci in double precision.
    * "base" is gainStep[ch] (SYMMETRIC) or smoothingCoeff (SMOOTH); SMOOTH exponents carry
    * an extra +1 because the one-pole updates before it applies.
    */
   /** @brief Per channel, base^numSamples: one frame forward (all layouts). Vector-aligned. */
   float framePow[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief Interleaved: seed powers of the four row chains, per channel, base^j. */
   float rowPow[AUDIOLIB_GAINNCHTRIMSMOOTHING_ROW_CHAINS][AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS]
       __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief Interleaved: per channel, base^4 (one group of four rows forward). */
   float rowAdv[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief Planar: per channel, base^l for lane l. */
   float lanePow[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS][AUDIOLIB_GAINNCHTRIMSMOOTHING_LANES_F32]
       __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief Planar: per channel, base^L, one vector forward. */
   float blockPow[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS];
   /** @brief Per-lane target gain as the kernels consume it. Block: targetGain copy.
    *         Planar: row ch holds targetGain[ch] broadcast over L lanes. Streamed by SE1 in the constant-gain kernel. */
   float laneTarget[AUDIOLIB_GAINNCHTRIMSMOOTHING_MAX_NUM_CHANNELS * AUDIOLIB_GAINNCHTRIMSMOOTHING_LANES_F32]
       __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief Planar: per-lane sample offset l subtracted from the SYMMETRIC fade budget. */
   int32_t laneOffset[AUDIOLIB_GAINNCHTRIMSMOOTHING_LANES_F32] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
   /** @brief A raw buffer to store pre-calculated SE/SA hardware templates. */
   uint8_t bufPblock[AUDIOLIB_GAINNCHTRIMSMOOTHING_PBLOCK_SIZE];
} AUDIOLIB_gainNChTrimSmoothing_PrivArgs;

#endif /* AUDIOLIB_GAINNCHTRIMSMOOTHING_IXX_OXX_PRIV_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainNChTrimSmoothing_priv.h          */
/* ======================================================================== */
