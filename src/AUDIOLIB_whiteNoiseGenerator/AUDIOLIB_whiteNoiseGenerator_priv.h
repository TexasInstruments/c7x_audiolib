// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_whiteNoiseGenerator.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_whiteNoiseGenerator_PrivArgs structure.
 *
 */
#define AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_PBLOCK_SIZE (2 * SA_PARAM_SIZE)

#define SE_PARAM_BASE (0x0000) ///< Base offset for hardware accelerator parameter blocks.
#define SE_SA0_PARAM_OFFSET                                                                                            \
   (SE_PARAM_BASE + SE_PARAM_SIZE) ///< Offset for the first Stream Address Generator (SA) parameter set.

#if __C7X_VEC_SIZE_BITS__ == 256
#define STATE_INDEX 8
#else
#define STATE_INDEX 16
#endif

// --- LCG (Linear Congruential Generator) Parameters ---

// Base LCG parameters: X_n+1 = (a * X_n + c)
#define AUDIOLIB_LCG_MULTIPLIER (1664525U)
#define AUDIOLIB_LCG_INCREMENT (1013904223U)
#define AUDIOLIB_LCG_MOD_MASK (0xFFFFFFFFULL) // Mask for 32-bit (uint64_t)
#define AUDIOLIB_UNROLL_FACTOR (4)
// Normalization factor for 32-bit LCG: 1.0f / (2^32)
#define AUDIOLIB_LCG_NORM_FACTOR_F32 (2.3283064e-10f)

// Pre-calculated parameters to jump 8 steps: X_n+8
#define AUDIOLIB_LCG_A_JUMP8 (3934847009U)
#define AUDIOLIB_LCG_C_JUMP8 (2748932008U)

// Pre-calculated parameters to jump 32 steps: X_n+32
#define AUDIOLIB_LCG_A_JUMP32 (666245249U)
#define AUDIOLIB_LCG_C_JUMP32 (95141024U)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_whiteNoiseGenerator_exec_ci
 *         and @ref AUDIOLIB_whiteNoiseGenerator_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_whiteNoiseGenerator_exec)(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_whiteNoiseGenerator_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_whiteNoiseGenerator_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_whiteNoiseGenerator_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_whiteNoiseGenerator_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 * @param [in]  handle        :  Active handle to the kernel
 * @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 * information of output buffer
 * @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 * @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_init_ci(AUDIOLIB_kernelHandle                        handle,
                                                     const AUDIOLIB_bufParams1D_t                *bufParamsOut,
                                                     const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs);

/*!
 * @brief This function is the execution function for the C7x
 * implementation of the kernel. The function declaration conforms
 * to the declaration of @ref AUDIOLIB_whiteNoiseGenerator_exec.
 *
 * @details This function executes the C7x vectorized implementation.
 * It uses **4 parallel 8-wide state vectors** (processing 32 samples
 * per loop iteration) for high throughput. The state is advanced
 * using pre-calculated 8-step (`a8/c8`) and 32-step (`a32/c32`)
 * LCG parameters.
 *
 * At the end of the frame, the final N-step state update is
 * performed in a single vector operation using the pre-calculated
 * `jumpHead...` parameters from the `init` function.
 *
 * @param [in]  handle       :  Active handle to the kernel
 * @param [out] pOut         :  Pointer to the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/*!
 *  @brief This function is the execution function for the Natural C
 *         implementation of the whiteNoiseGenerator kernel. The function declaration
 *         conforms to the declaration of @ref AUDIOLIB_whiteNoiseGenerator_exec.
 *
 * @details This function executes the Natural C implementation of the whiteNoiseGenerator kernel
 *
 * @param [in]  handle       :  Active handle to the kernel
 * @param [out] pOut         :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    * @ref AUDIOLIB_whiteNoiseGenerator_exec_cn and
    * @ref AUDIOLIB_whiteNoiseGenerator_exec_ci.                        */
   pFxnAUDIOLIB_whiteNoiseGenerator_exec execute;
   int32_t  samples;   /**< \brief Number of samples in the output buffer (from bufParamsOut->dim_x). */
   uint32_t seed;      /**< \brief The current 32-bit seed (X_n). Used by _cn and synced with states[0]. */
   float    range;     /**< \brief The output range [-range, +range]. */
   float    twoXRange; /**< \brief Pre-calculated (2 * range) for the _ci implementation. */
   uint32_t nVecs;     /**< \brief Number of 32-sample vectors to process in the _ci loop. */
   uint32_t jumpHeadMultiplierNextFrame; /**< \brief Pre-calculated LCG multiplier (A_N) to jump N=samples steps. */
   uint32_t jumpHeadIncNextFrame;        /**< \brief Pre-calculated LCG increment (C_N) to jump N=samples steps. */
   uint32_t states[STATE_INDEX];         /**< \brief The 8 or 16 parallel LCG states (X_n to X_n+N-1). Used by _ci. */
   AUDIOLIB_whiteNoiseGenerator_InitArgs initArgs; /**< \brief Structure holding initialization parameters. */
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_PBLOCK_SIZE]; /*< Array to hold SE/SA template */
} AUDIOLIB_whiteNoiseGenerator_PrivArgs;

#endif /* AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_PRIV_H_ */
