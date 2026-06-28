// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_MUTENCH_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_MUTENCH_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_muteNCh.h"
#include <cstring>

/**
 * @brief Macro defining the size of the `bufPblock` array within the
 * @ref AUDIOLIB_muteNCh_PrivArgs structure. It allocates space for
 * two Streaming Engine (SE) templates and three Stream Address
 * Generator (SA) templates.
 */
#define AUDIOLIB_MUTE_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 3 * SA_PARAM_SIZE)

#define SE_PARAM_BASE (0x0000)              ///< Base offset for hardware accelerator parameter blocks.
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE) ///< Memory offset for the first Streaming Engine (SE0) template.
#define SE_SE1_PARAM_OFFSET                                                                                            \
   (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the second Streaming Engine (SE1) template.
#define SE_SA0_PARAM_OFFSET                                                                                            \
   (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the first Stream Address Generator (SA0) template.
#define SE_SA1_PARAM_OFFSET                                                                                            \
   (SE_SA0_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the second Stream Address Generator (SA1) template.
#define SE_SA2_PARAM_OFFSET                                                                                            \
   (SE_SA1_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the third Stream Address Generator (SA2) template.

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
#else
#define ALPHA_COEFF_INDEX 16
#endif

/**
 * @brief Internal enumeration of available fade types.
 */
typedef enum {
   AUDIOLIB_MUTE_FADE_TYPE_LINEAR = 0, ///< Linear ramp fade.
   AUDIOLIB_MUTE_FADE_TYPE_SMOOTH = 1, ///< Exponential (first-order filter) curve fade.
   AUDIOLIB_MUTE_FADE_TYPE_HARD   = 2  ///< Instantaneous gain change.
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
AUDIOLIB_STATUS AUDIOLIB_muteNChLinearFadeUnrolledDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                       void *restrict pIn,
                                                                       void *restrict pOut);

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
AUDIOLIB_STATUS
AUDIOLIB_hardMuteInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_hardMuteDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

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

/*!
 * @brief The internal state structure (handle) for the MuteNCh kernel.
 * @details This structure holds all the configuration, state, and pre-calculated
 * parameters required for the kernel to operate.
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_muteNCh_exec_cn and
    *         @ref AUDIOLIB_muteNCh_exec_ci.                        */
   pFxnAUDIOLIB_muteNCh_exec execute;
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
   /** @brief Pointer to an array holding the gain value at the start of a fade for each channel. */
   float *startGain;
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
   /** @brief Pre-calculated vector of coefficients for C7x-optimized smooth fades. */
   float alphaCoeff[ALPHA_COEFF_INDEX];
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
   /** @brief Pre-calculated multiplier to efficiently update the `alphaCoeff` vector in optimized smooth fades. */
   float alphaMultiplier;
   /** @brief Pointer to a buffer of pre-calculated gain vectors for C7x-optimized deinterleaved linear fades. */
   float *precalculatedGainVectors;
} AUDIOLIB_muteNCh_PrivArgs;

#endif /* AUDIOLIB_MUTENCH_IXX_IXX_OXX_PRIV_H_ */
