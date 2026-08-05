// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/********************************************************************************
 *  @file     AUDIOLIB_gainSmoothing_priv.h
 *
 *  @brief    File to hold private parameter of function AUDIOLIB_gainSmoothing
 *
 *  @version  0.1 - Mar 2022 : Initial Version
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
#define AUDIOLIB_GAINSMOOTHING_PBLOCK_SIZE (1* SE_PARAM_SIZE + 1* SA_PARAM_SIZE)

#define SE_PARAM_BASE (0x0000) ///< Base offset for hardware accelerator parameter blocks.
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE) ///< Memory offset for the first Streaming Engine (SE0) template.
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE) ///< Memory offset for the first Stream Address Generator (SA0) template.

/// @brief A small floating-point threshold to determine if the current gain is effectively at the target, allowing smoothing to be bypassed.
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
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_gainSmoothing_exec_ci
 *         and @ref AUDIOLIB_gainSmoothing_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_gainSmoothing_exec)(AUDIOLIB_kernelHandle handle, 
                                                      void *restrict pIn,
                                                      void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainSmoothing_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_gainSmoothing_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_gainSmoothing_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_gainSmoothing_exec_ci does not lose cycles
 *          to determine the hardware configuration.
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
AUDIOLIB_STATUS AUDIOLIB_gainSmoothing_init_ci(AUDIOLIB_kernelHandle             handle,
                                          const AUDIOLIB_gainSmoothing_InitArgs *pKerInitArgs);

/*!
 * @brief Execution functions for the C7x implementation.
 * @details These functions perform the core gainSmoothing/ungainSmoothing processing using
 *          hardware-accelerated, vectorized instructions for different fade types. 
 *          The function declaration conforms to the declaration of @ref AUDIOLIB_gainSmoothing_exec.
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

/*!
 * @brief Saturating clamp for the geometric ramp, which overshoots the target by design.
 * @details Selected at compile time via the fadeUp template parameter, so the exec
 *          functions contain no runtime compare or branch for the ramp direction.
 */
template <bool fadeUp> struct AUDIOLIB_gainSmoothing_clamp;

template <> struct AUDIOLIB_gainSmoothing_clamp<true> {
   /*! @brief Fading up: the gain approaches the target from below. */
   template <typename vec> static inline vec apply(vec gain, vec target) { return __min(gain, target); }
};

template <> struct AUDIOLIB_gainSmoothing_clamp<false> {
   /*! @brief Fading down: the gain approaches the target from above. */
   template <typename vec> static inline vec apply(vec gain, vec target) { return __max(gain, target); }
};

/*!
 * @brief Geometric (constant-dB) gain ramp.
 * @tparam dataType Sample type; only float is instantiated.
 * @tparam fadeUp   true when targetGain > currentGain. Selects the clamp direction.
 */
template <typename dataType, bool fadeUp>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSymmetricFade_exec_ci(AUDIOLIB_kernelHandle handle,
                        void *restrict pIn,
                        void *restrict pOut);

/*! @brief One-pole gain ramp. Direction-agnostic: it interpolates toward the target
 *         with no clamp, so it needs no fadeUp parameter. */
template<typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingSmoothFade_exec_ci(AUDIOLIB_kernelHandle handle,
                        void *restrict pIn,
                        void *restrict pOut);

/*! @brief Constant-gain path. Used when fadeTime is 0, and once a ramp has converged
 *         on its target, at which point every remaining sample gets the same gain. */
template<typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_gainSmoothingBypass_exec_ci(AUDIOLIB_kernelHandle handle,
                        void *restrict pIn,
                        void *restrict pOut);


/*!
 *  @brief Execute functions for the NATC implementation of the kernel. The function declaration conforms
 * to the declaration of @ref AUDIOLIB_gainSmoothing_exec.
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
AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSymmetricFade_exec_cn(AUDIOLIB_kernelHandle handle,
                         void *restrict pIn,
                         void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainSmoothingSmoothFade_exec_cn(AUDIOLIB_kernelHandle handle,
                         void *restrict pIn,
                         void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
  /*! @brief Dispatch table, indexed by @ref AUDIOLIB_gainSmoothing_execVariant.
   *         @ref AUDIOLIB_gainSmoothing_exec indexes this with the result of its
   *         convergence test, so dispatch is a table lookup and neither the wrapper
   *         nor the exec variants themselves contain a bypass branch. */
  pFxnAUDIOLIB_gainSmoothing_exec execute[AUDIOLIB_GAINSMOOTHING_NUM_EXEC_VARIANTS];
  /** @brief Elements along the fastest-varying axis of the buffer (bufParams dim_x).
   *         Planar: samples per channel. Interleaved: channels per sample frame. */
  uint32_t numCols;
  /** @brief Number of rows in the buffer (bufParams dim_y).
   *         Planar: channels. Interleaved: sample frames. */
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
  /** @brief A copy of the initialization arguments. */
  AUDIOLIB_gainSmoothing_InitArgs initArgs;
  /** @brief A copy of the last-set runtime arguments. */
  AUDIOLIB_gainSmoothing_SetArgs setArgs;
  /** @brief The per-sample gain ratio for SYMMETRIC fading. Multiplicative, not additive. */
  float gainStep;
  /** @brief A raw buffer to store pre-calculated SE/SA hardware templates. */
  uint8_t bufPblock[AUDIOLIB_GAINSMOOTHING_PBLOCK_SIZE];
  /*! @brief Pre-calculated per-lane gain coefficients. The _ci path reads this as one
   *         full-width vector, so it must be vector-aligned. */
  float alphaCoeff[AUDIOLIB_GAINSMOOTHING_LANES_F32] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));
  /** @brief Factor that advances alphaCoeff by one full vector of samples in SYMMETRIC fades. */
  float gainStepMultiplier;
  /** @brief Number of gain updates per exec: the outer loop counter. */
  uint32_t numBlocks;
  /** @brief Vectors covered by each gain update: the inner loop counter. */
  uint32_t wBlocks;
  /** @brief A flag (0 or 1) to indicate if gain ramp should be bypassed. */
  uint8_t bypassSmoothing;
  /** @brief Pre-calculated multiplier to efficiently update the alphaCoeff vector for smooth fades. */
  float alphaMultiplier;
} AUDIOLIB_gainSmoothing_PrivArgs;

#endif /* AUDIOLIB_gainSmoothing_IXX_OXX_PRIV_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainSmoothing_priv.h          */
/* ======================================================================== */
