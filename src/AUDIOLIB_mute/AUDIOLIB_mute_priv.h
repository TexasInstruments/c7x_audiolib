// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_MUTE_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_MUTE_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_mute.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_mute_PrivArgs structure.
 *
 */
#define AUDIOLIB_MUTE_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE)

#define SE_PARAM_BASE (0x0000)              ///< Base offset for hardware accelerator parameter blocks.
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE) ///< Memory offset for the first Streaming Engine (SE0) template.
#define SE_SE1_PARAM_OFFSET                                                                                            \
   (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the second Streaming Engine (SE1) template.
#define SE_SA0_PARAM_OFFSET                                                                                            \
   (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the first Stream Address Generator (SA0) template.
#define SE_SA1_PARAM_OFFSET                                                                                            \
   (SE_SA0_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the second Stream Address Generator (SA1) template.

/// @brief A small floating-point threshold to determine if the current gain is effectively at the target, allowing
/// smoothing to be bypassed.
#define AUDIOLIB_MUTE_GAIN_THRESHOLD 1e-6f

/// @brief Conversion factor from milliseconds to seconds.
#define AUDIOLIB_MS_PER_SECOND 1000.0f

/// @brief Level the symmetric fade treats as fully muted: -200 dB, the AWE MuteSymmetric muteThreshold of 1e-10.
/// A geometric ramp can never reach 0, so it runs between this floor and 1.0 and the exec path outputs exact zeros
/// whenever the gain sits on the floor.
#define AUDIOLIB_MUTE_SYMMETRIC_FLOOR 1e-10f

/// @name perfEst constants for the symmetric fade
/// Loop terms are from the compiled .asm (C7504 and C7524 compile to the same loops). The call overhead is a fit to the
/// Loki warm cycles of the ramp-state test cases on AM62D, which the model matches to within 2 cycles. On AM275 the
/// single-vector variant for an odd block count runs about 7 cycles longer than modelled.
/// @{
#define AUDIOLIB_MUTE_SYMMETRIC_STARTUP_CYCLES 21        ///< Stream open and register setup before the loop.
#define AUDIOLIB_MUTE_SYMMETRIC_TEARDOWN_CYCLES 8        ///< Stream close, gain extract and return.
#define AUDIOLIB_MUTE_SYMMETRIC_CALL_OVERHEAD_CYCLES 33  ///< Fitted: exec wrapper, call and return, outside the loop.
#define AUDIOLIB_MUTE_SYMMETRIC_SINGLE_VECTOR_II 4 ///< Cycles per pair of blocks (four vectors), wBlocks == 1.
#define AUDIOLIB_MUTE_SYMMETRIC_SINGLE_VECTOR_ODD_BLOCK_CYCLES 5 ///< Fitted: the block done on its own when the count is odd.
#define AUDIOLIB_MUTE_SYMMETRIC_SINGLE_VECTOR_PIPE_UP_CYCLES 15 ///< Fitted: pipeline fill of the wBlocks == 1 loop.
#define AUDIOLIB_MUTE_SYMMETRIC_BLOCK_OVERHEAD_CYCLES                                                                  \
   14 ///< Fitted: gain update and inner-loop set-up per block of the nested loop.
/// @}

#if __C7X_VEC_SIZE_BITS__ == 256
#define ALPHA_COEFF_INDEX 8
#else
#define ALPHA_COEFF_INDEX 16
#endif

/**
 * @brief Enumeration of available fade types.
 */
typedef enum {
   AUDIOLIB_MUTE_FADE_TYPE_LINEAR    = 0, ///< Linear ramp fade.
   AUDIOLIB_MUTE_FADE_TYPE_SMOOTH    = 1, ///< Exponential curve fade.
   AUDIOLIB_MUTE_FADE_TYPE_HARD      = 2, ///< Instantaneous gain change.
   AUDIOLIB_MUTE_FADE_TYPE_SYMMETRIC = 3  ///< Geometric (constant dB/s) ramp: equal time to mute and unmute.
} FadeType;

/*!
 * @brief Index into AUDIOLIB_mute_PrivArgs::symmetricNextExecute.
 * @details Deliberately valued 0 and 1 so that the convergence test at the end of the symmetric ramp can be used
 *          directly as the index.
 */
typedef enum {
   AUDIOLIB_MUTE_EXEC_RAMP      = 0, ///< Gain is still moving toward the target.
   AUDIOLIB_MUTE_EXEC_CONVERGED = 1, ///< Gain has reached the target; it is now constant.
   AUDIOLIB_MUTE_NUM_EXEC_VARIANTS   ///< Table size.
} AUDIOLIB_mute_execVariant;

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_mute_exec_ci
 *         and @ref AUDIOLIB_mute_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_mute_exec)(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_mute_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_mute_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_mute_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_mute_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of ouput buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_mute_init_ci(AUDIOLIB_kernelHandle         handle,
                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      const AUDIOLIB_mute_InitArgs *pKerInitArgs);

/*!
 * @brief Execution functions for the C7x implementation.
 * @details These functions perform the core mute/unmute processing using
 *          hardware-accelerated, vectorized instructions for different fade types.
 *          The function declaration conforms to the declaration of @ref AUDIOLIB_mute_exec.
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
AUDIOLIB_STATUS AUDIOLIB_muteLinearFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_unMuteLinearFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteSmoothFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_unMuteSmoothFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief Geometric (constant dB/s) gain ramp of the symmetric fade, for both mute and unmute. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteSymmetricFade_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief The same ramp for wBlocks == 1 (mono or stereo planar, up to 8 channels interleaved): no inner loop, two
 *         blocks per iteration. One variant per parity of numBlocks, so neither contains a conditional. Both need
 *         numBlocks >= 2: @ref AUDIOLIB_mute_set sends a single block to the generic function. */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteSymmetricFadeSingleVectorEvenBlocks_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                          void *restrict        pIn,
                                                                          void *restrict        pOut);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteSymmetricFadeSingleVectorOddBlocks_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                         void *restrict        pIn,
                                                                         void *restrict        pOut);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_hardMute_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_hardUnMute_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief Execute functions for the NATC implementation of the kernel. The function declaration conforms
 * to the declaration of @ref AUDIOLIB_mute_exec.
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
AUDIOLIB_STATUS AUDIOLIB_muteHard_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteLinearFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_muteSmoothFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_muteSymmetricFade_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_mute_exec_cn and
    *         @ref AUDIOLIB_mute_exec_ci.                        */
   pFxnAUDIOLIB_mute_exec execute;
   /** @brief Number of samples per channel in the input buffer. */
   uint32_t samples;
   /** @brief Number of audio channels. */
   uint32_t channels;
   /** @brief Stride between consecutive rows in the input buffer, in elements. */
   uint32_t strideInElements;
   /** @brief Stride between consecutive rows in the output buffer, in elements. */
   uint32_t strideOutElements;
   /** @brief The current gain value, updated after each execution frame. */
   float currentGain;
   /** @brief The target gain value (0.0 for mute, 1.0 for unmute). */
   float targetGain;
   /** @brief The smoothing coefficient (`alpha`) for the smooth fade algorithm. */
   float smoothingCoefficient;
   /** @brief A copy of the initialization arguments. */
   AUDIOLIB_mute_InitArgs initArgs;
   /** @brief A copy of the last-set runtime arguments. */
   AUDIOLIB_mute_SetArgs setArgs;
   /** @brief The per-sample gain increment for linear fading. */
   float gainStep;
   /** @brief A raw buffer to store pre-calculated SE/SA hardware templates. */
   uint8_t bufPblock[AUDIOLIB_MUTE_IXX_IXX_OXX_PBLOCK_SIZE];
   /** @brief Pre-calculated vector of coefficients for C7x-optimized fades. */
   float alphaCoeff[ALPHA_COEFF_INDEX];
   /** @brief Pre-calculated vector used to update the gain vector in linear fades. */
   float gainStepAdder[ALPHA_COEFF_INDEX];
   /** @brief Pre-calculated offset for the second input stream pointer (SE1). */
   uint32_t pInOffset;
   /** @brief Pre-calculated offset for the second output stream pointer (SA1). */
   uint32_t pOutOffset;
   /** @brief Pre-calculated outer loop counter for the `exec` function. */
   uint32_t numBlocks;
   /** @brief Pre-calculated inner loop counter for the `exec` function. */
   uint32_t wBlocks;
   /** @brief A flag (0 or 1) to indicate if gain smoothing should be bypassed. */
   int32_t bypassSmoothing;
   /** @brief Pre-calculated multiplier to efficiently update the alphaCoeff vector for smooth fades. */
   float alphaMultiplier;
   /** @brief Variants the symmetric ramp can hand over to, indexed by @ref AUDIOLIB_mute_execVariant. At the end of
    * each frame the ramp loop stores the entry selected by "gain has reached the target" into execute, so the table
    *         lookup is a data index and neither the ramp nor @ref AUDIOLIB_mute_exec contains a convergence branch. */
   pFxnAUDIOLIB_mute_exec symmetricNextExecute[AUDIOLIB_MUTE_NUM_EXEC_VARIANTS];
} AUDIOLIB_mute_PrivArgs;

#endif /* AUDIOLIB_MUTE_IXX_IXX_OXX_PRIV_H_ */
