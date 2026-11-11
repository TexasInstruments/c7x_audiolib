// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_MUTENCH_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_MUTENCH_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_muteNCh.h"
#include <cstring>

/**
 * @brief Size of the `bufPblock` array within @ref AUDIOLIB_muteNCh_PrivArgs: three Streaming Engine (SE)
 * templates (fade data, hard data, hard gain) and three Stream Address Generator (SA) templates (fade data,
 * hard data, interleaved gain state).
 */
#define AUDIOLIB_MUTE_IXX_IXX_OXX_PBLOCK_SIZE (3 * SE_PARAM_SIZE + 3 * SA_PARAM_SIZE)

#define AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET (0) ///< SE0 template of the fading paths
#define AUDIOLIB_MUTENCH_SE_HARD_DATA_OFFSET                                                                           \
   (AUDIOLIB_MUTENCH_SE_FADE_DATA_OFFSET + SE_PARAM_SIZE) ///< SE0 template of the hard path
#define AUDIOLIB_MUTENCH_SE_HARD_GAIN_OFFSET                                                                           \
   (AUDIOLIB_MUTENCH_SE_HARD_DATA_OFFSET + SE_PARAM_SIZE) ///< SE1 template of the hard path
#define AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET                                                                           \
   (AUDIOLIB_MUTENCH_SE_HARD_GAIN_OFFSET + SE_PARAM_SIZE) ///< SA0 template of the fading paths
#define AUDIOLIB_MUTENCH_SA_HARD_DATA_OFFSET                                                                           \
   (AUDIOLIB_MUTENCH_SA_FADE_DATA_OFFSET + SA_PARAM_SIZE) ///< SA0 template of the hard path
#define AUDIOLIB_MUTENCH_SA_GAIN_OFFSET                                                                                \
   (AUDIOLIB_MUTENCH_SA_HARD_DATA_OFFSET + SA_PARAM_SIZE) ///< SA2 template: interleaved gain state

/// @name perfEst constants
/// Loop terms are from the compiled .asm (C7504 and C7524 compile to the same loops). The call and row overheads are a
/// least-squares fit to the Loki warm cycles of the test cases on AM62D (case count per variant noted); the model
/// matches each of them exactly. A row is one channel (planar) or one block of eleCount channels (interleaved).
/// @{
#define AUDIOLIB_MUTENCH_FADE_CYCLES_PER_ITERATION (4)            ///< ii of every fading loop (four vectors).
#define AUDIOLIB_MUTENCH_LINEAR_PIPE_UP_CYCLES (8)                ///< Prolog and epilog of a linear fading loop.
#define AUDIOLIB_MUTENCH_SMOOTH_PIPE_UP_CYCLES (7)                ///< Prolog and epilog of a smooth fading loop.
#define AUDIOLIB_MUTENCH_HARD_CYCLES_PER_VECTOR (1)               ///< ii of the hard loop.
#define AUDIOLIB_MUTENCH_HARD_PIPE_UP_CYCLES (4)                  ///< Prolog and epilog of the hard loop.
#define AUDIOLIB_MUTENCH_LINEAR_GAIN_UPDATE_CYCLES_PER_VECTOR (4) ///< ii of the planar linear end-of-frame update.
#define AUDIOLIB_MUTENCH_LINEAR_GAIN_UPDATE_PIPE_UP_CYCLES (14)   ///< Prolog and epilog of that loop.
#define AUDIOLIB_MUTENCH_SMOOTH_GAIN_UPDATE_CYCLES_PER_VECTOR (3) ///< ii of the planar smooth end-of-frame update.
#define AUDIOLIB_MUTENCH_SMOOTH_GAIN_UPDATE_PIPE_UP_CYCLES (11)   ///< Prolog and epilog of that loop.
#define AUDIOLIB_MUTENCH_HARD_CALL_OVERHEAD_CYCLES (77)           ///< Fitted (26 cases): wrapper, stream opens, call.
#define AUDIOLIB_MUTENCH_LINEAR_INTERLEAVE_CALL_OVERHEAD_CYCLES (64) ///< Fitted (5 cases).
#define AUDIOLIB_MUTENCH_LINEAR_INTERLEAVE_ROW_OVERHEAD_CYCLES (25)  ///< Fitted (5 cases): block setup and gain store.
#define AUDIOLIB_MUTENCH_LINEAR_PLANAR_CALL_OVERHEAD_CYCLES (62)     ///< Fitted (5 cases).
#define AUDIOLIB_MUTENCH_LINEAR_PLANAR_ROW_OVERHEAD_CYCLES (21)      ///< Fitted (5 cases): channel setup.
#define AUDIOLIB_MUTENCH_SMOOTH_INTERLEAVE_CALL_OVERHEAD_CYCLES (68) ///< Fitted (2 cases).
#define AUDIOLIB_MUTENCH_SMOOTH_INTERLEAVE_ROW_OVERHEAD_CYCLES (23)  ///< Fitted (2 cases): block setup and state store.
#define AUDIOLIB_MUTENCH_SMOOTH_PLANAR_CALL_OVERHEAD_CYCLES (68)     ///< Fitted (3 cases).
#define AUDIOLIB_MUTENCH_SMOOTH_PLANAR_ROW_OVERHEAD_CYCLES (19)      ///< Fitted (3 cases): channel setup.
#define AUDIOLIB_MUTENCH_SYMMETRIC_PIPE_UP_CYCLES (8)                ///< Prolog and epilog of a symmetric fading loop.
#define AUDIOLIB_MUTENCH_SYMMETRIC_GAIN_UPDATE_CYCLES_PER_VECTOR                                                       \
   (4)                                                             ///< ii of the planar symmetric end-of-frame update.
#define AUDIOLIB_MUTENCH_SYMMETRIC_GAIN_UPDATE_PIPE_UP_CYCLES (13) ///< Prolog and epilog of that loop.
#define AUDIOLIB_MUTENCH_SYMMETRIC_INTERLEAVE_CALL_OVERHEAD_CYCLES (64) ///< Fitted (4 cases).
#define AUDIOLIB_MUTENCH_SYMMETRIC_INTERLEAVE_ROW_OVERHEAD_CYCLES                                                      \
   (29)                                                             ///< Fitted (4 cases): block setup and gain store.
#define AUDIOLIB_MUTENCH_SYMMETRIC_PLANAR_CALL_OVERHEAD_CYCLES (63) ///< Fitted (5 cases).
#define AUDIOLIB_MUTENCH_SYMMETRIC_PLANAR_ROW_OVERHEAD_CYCLES                                                          \
   (27) ///< Fitted (5 cases): channel setup and ratio-table loads.
/// @}

/**
 * @brief Vectors handled per iteration of the fading loops: four independent gain chains hide the 4-cycle
 * multiply and 3-cycle add latencies at one vector per cycle.
 */
#define AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION (4)

/**
 * @brief Conversion factor from milliseconds to seconds.
 */
#define AUDIOLIB_MS_PER_SECOND 1000.0f

/**
 * @brief Defines the size of vector coefficient arrays based on the C7x vector width.
 * @details This ensures portability of optimized code between C7x cores with different
 * SIMD vector sizes (e.g., 256-bit or 512-bit).
 */
#if __C7X_VEC_SIZE_BITS__ == 256
#define ALPHA_COEFF_INDEX 8
#define AUDIOLIB_MUTENCH_GAIN_ELEDUP __SE_ELEDUP_8X
#else
#define ALPHA_COEFF_INDEX 16
#define AUDIOLIB_MUTENCH_GAIN_ELEDUP __SE_ELEDUP_16X
#endif

/**
 * @brief Lowest gain of the symmetric fade (-200 dB). A geometric ramp cannot reach 0, so it runs between this floor
 * and 1.0, and the output is exactly 0 while the gain sits on the floor.
 */
#define AUDIOLIB_MUTENCH_SYMMETRIC_FLOOR (1e-10f)
/**
 * @brief Number of symmetric fade directions: index 0 mutes (ratio below 1), index 1 unmutes. The index of a channel is
 * its target gain, 0.0 or 1.0.
 */
#define AUDIOLIB_MUTENCH_SYMMETRIC_NUM_DIRECTIONS (2)

/**
 * @brief Index of the execution function that runs while a fade is in progress (and for smooth fades always).
 */
#define AUDIOLIB_MUTENCH_EXEC_FADING (0)
/**
 * @brief Index of the execution function that runs once every channel sits on its target: the hard path.
 */
#define AUDIOLIB_MUTENCH_EXEC_CONVERGED (1)
/**
 * @brief Number of entries of the execution function table in @ref AUDIOLIB_muteNCh_PrivArgs.
 */
#define AUDIOLIB_MUTENCH_NUM_EXEC (2)

/**
 * @brief Internal enumeration of available fade types.
 */
typedef enum {
   AUDIOLIB_MUTE_FADE_TYPE_LINEAR    = 0, ///< Linear ramp fade.
   AUDIOLIB_MUTE_FADE_TYPE_SMOOTH    = 1, ///< Exponential (first-order filter) curve fade.
   AUDIOLIB_MUTE_FADE_TYPE_HARD      = 2, ///< Instantaneous gain change.
   AUDIOLIB_MUTE_FADE_TYPE_SYMMETRIC = 3  ///< Constant ratio per sample (straight line in dB), between the floor and 1.
} FadeType;

/*!
 * @brief A function pointer type for the kernel's `exec` function.
 * @details This allows the handle to point to either the natural C (`_cn`) or
 * the C7x-optimized (`_ci`) implementation at runtime.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_muteNCh_exec)(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pIn,
                                                     void *restrict pOut);

/*!
 * @brief       The initialization function for the C7x-optimized implementation.
 * @details     This function prepares the hardware accelerator (Streaming Engine, Stream
 * Address Generator) templates based on the buffer and kernel parameters.
 * These pre-calculated templates are stored in the handle's `bufPblock`
 * to be used by the `exec_ci` function, minimizing setup overhead during execution.
 * @param [in]  handle        Active handle to the kernel.
 * @param [in]  bufParamsIn   Pointer to the input buffer's dimensional information.
 * @param [out] bufParamsOut  Pointer to the output buffer's dimensional information.
 * @param [in]  pKerInitArgs  Pointer to the kernel's initialization parameters.
 * @return      Status value indicating success or failure. See @ref AUDIOLIB_STATUS.
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNCh_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_muteNCh_InitArgs *pKerInitArgs);

/*!
 * @brief Execution functions for the C7x implementation.
 * @details These functions perform the core mute/unmute processing using
 *          hardware-accelerated, vectorized instructions for different fade types.
 *          The function declaration conforms to the declaration of @ref AUDIOLIB_muteNCh_exec.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 */

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChLinearFadeDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChLinearFadeInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChExponentialFadeDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                    void *restrict pIn,
                                                                    void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChExponentialFadeInterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChSymmetricFadeDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                  void *restrict pIn,
                                                                  void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChSymmetricFadeInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChHard_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief Execute functions for the NATC implementation of the kernel. The function declaration conforms
 * to the declaration of @ref AUDIOLIB_muteNCh_exec.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteNChHard_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChLinearFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChSmoothFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteNChSymmetricFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief The internal state structure (handle) for the MuteNCh kernel.
 * @details This structure holds all the configuration, state, and pre-calculated
 * parameters required for the kernel to operate.
 */
typedef struct {
   /*! @brief Execution variants, set by @ref AUDIOLIB_muteNCh_set: index
    *         @ref AUDIOLIB_MUTENCH_EXEC_FADING runs while a fade is in progress,
    *         @ref AUDIOLIB_MUTENCH_EXEC_CONVERGED once every channel sits on its target. */
   pFxnAUDIOLIB_muteNCh_exec execute[AUDIOLIB_MUTENCH_NUM_EXEC];
   /** @brief Samples until every linear fade has reached its target (0: converged). Smooth fades never converge,
    * so both table entries are the smooth variant. */
   uint32_t remainingFadeSamples;
   /** @brief Number of samples per channel in one processing frame. */
   uint32_t samples;
   /** @brief Number of audio channels. */
   uint32_t channels;
   /** @brief Stride between consecutive rows in the input buffer, in elements. */
   uint32_t strideInElements;
   /** @brief Stride between consecutive rows in the output buffer, in elements. */
   uint32_t strideOutElements;
   /** @brief Pointer to an array holding the current gain value for each channel. This state is persistent across calls
    * to `exec`. */
   float *currentGain;
   /** @brief Pointer to an array holding the target gain value (0.0 for mute, 1.0 for unmute) for each channel. */
   float *targetGain;
   /** @brief Pointer to an array holding, for each channel, the far end of a full fade to its target (1.0 when
    * muting, 0.0 when unmuting). It sets the linear step, so every linear fade runs at the same rate. */
   float *startGain;
   /** @brief Pointer to an array holding, for each channel, the distance of its smooth-fade gain from the target
    * (currentGain - targetGain). A smooth fade shrinks it by alpha every sample, so it keeps its full relative
    * precision and an unmute settles on exactly 1.0. */
   float *smoothDistance;
   /** @brief Pre-calculated value of `smoothingCoefficient` raised to the power of `samples`, used for efficient state
    * update in smooth fade mode. */
   float alphaN;
   /** @brief The smoothing coefficient (`alpha`) for the smooth fade algorithm, derived from `fadeTime`. */
   float smoothingCoefficient;
   /** @brief A copy of the initialization arguments. */
   AUDIOLIB_muteNCh_InitArgs initArgs;
   /** @brief A copy of the last-set runtime arguments. */
   AUDIOLIB_muteNCh_SetArgs setArgs;
   /** @brief Pointer to an array holding the per-sample gain increment for linear fading for each channel. */
   float *gainStep;
   /** @brief A raw buffer to store pre-calculated SE/SA hardware templates. */
   uint8_t bufPblock[AUDIOLIB_MUTE_IXX_IXX_OXX_PBLOCK_SIZE];
   /** @brief alpha^1 .. alpha^ALPHA_COEFF_INDEX, the per-lane coefficients of the planar C7x-optimized smooth fade. */
   float alphaCoeff[ALPHA_COEFF_INDEX];
   /** @brief Outer loop count of the `exec_ci` functions: channels (planar) or blocks of eleCount channels
    * (interleaved). */
   uint32_t numBlocks;
   /** @brief Inner loop count of the fading `exec_ci` functions: groups of AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION
    * vectors per channel (planar) or samples (interleaved). */
   uint32_t wBlocks;
   /** @brief Loop count of the hard `exec_ci` function: vectors in one frame. */
   uint32_t numHardVectors;
   /** @brief A flag (0 or 1) to indicate if gain smoothing should be bypassed. */
   int32_t bypassSmoothing;
   /** @brief alpha^ALPHA_COEFF_INDEX: one vector further along the planar C7x-optimized smooth fade. */
   float alphaMultiplier;
   /** @brief Pointer to a buffer of pre-calculated gain vectors for C7x-optimized deinterleaved linear fades. */
   float *precalculatedGainVectors;
   /** @brief Symmetric fade, per direction: ratio^1 .. ratio^ALPHA_COEFF_INDEX, the per-lane ratios of one planar
    * vector. */
   float symmetricLaneRatio[AUDIOLIB_MUTENCH_SYMMETRIC_NUM_DIRECTIONS][ALPHA_COEFF_INDEX];
   /** @brief Symmetric fade, per direction: ratio^(k * ALPHA_COEFF_INDEX) for k = 1 .. 4, one to four planar vectors
    * on. */
   float symmetricVectorRatio[AUDIOLIB_MUTENCH_SYMMETRIC_NUM_DIRECTIONS][AUDIOLIB_MUTENCH_VECTORS_PER_ITERATION];
   /** @brief Symmetric fade, per direction: ratio^samples, one frame on. */
   float symmetricFrameRatio[AUDIOLIB_MUTENCH_SYMMETRIC_NUM_DIRECTIONS];
} AUDIOLIB_muteNCh_PrivArgs;

#endif /* AUDIOLIB_MUTENCH_IXX_IXX_OXX_PRIV_H_ */
