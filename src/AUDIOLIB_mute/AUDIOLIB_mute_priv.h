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

#if __C7X_VEC_SIZE_BITS__ == 256
#define ALPHA_COEFF_INDEX 8
#else
#define ALPHA_COEFF_INDEX 16
#endif

/**
 * @brief Enumeration of available fade types.
 */
typedef enum {
   AUDIOLIB_MUTE_FADE_TYPE_LINEAR = 0, ///< Linear ramp fade.
   AUDIOLIB_MUTE_FADE_TYPE_SMOOTH = 1, ///< Exponential curve fade.
   AUDIOLIB_MUTE_FADE_TYPE_HARD   = 2  ///< Instantaneous gain change.
} FadeType;

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
} AUDIOLIB_mute_PrivArgs;

#endif /* AUDIOLIB_MUTE_IXX_IXX_OXX_PRIV_H_ */
