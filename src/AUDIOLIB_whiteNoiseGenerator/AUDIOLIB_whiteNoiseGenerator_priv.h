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

#define SE_PARAM_BASE (0x0000)              ///< Base offset for hardware accelerator parameter blocks.
#define SE_SA0_PARAM_OFFSET (SE_PARAM_BASE) ///< SA0 template: the output store stream.
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE) ///< SA1 template: the row-end predicates.

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
/* State vectors per _ci loop iteration: init picks 5 or 8 for the row length (see
 * AUDIOLIB_whiteNoiseGenerator_execCycles). At least 5 are needed to hide the 5-cycle loop-carried multiply-add of
 * the LCG state (VMPYWW 4 + VADDW 1); with 5 or 8 the loop runs at 1 cycle per vector. Rows run in whole
 * iterations, so 8 suits rows of a multiple of 64 samples and 5 suits rows of up to 40 samples. */
#define AUDIOLIB_WHITENOISEGENERATOR_SHORT_UNROLL (5)
#define AUDIOLIB_WHITENOISEGENERATOR_LONG_UNROLL (8)
// Normalization factor for 32-bit LCG: 1.0f / (2^32)
#define AUDIOLIB_LCG_NORM_FACTOR_F32 (2.3283064e-10f)
// 2^-31: maps a signed 32-bit value to [-1, +1]
#define AUDIOLIB_LCG_SIGNED_NORM_FACTOR_F32 (4.6566129e-10f)
// XOR with this flips the top bit: (int32_t) (x ^ 2^31) = x - 2^31, the LCG output centred on zero
#define AUDIOLIB_LCG_SIGN_FLIP (0x80000000U)

/* LCG steps between the start states of neighbouring channels. 64 * spacing <= 2^32, so no two of the
 * 64 channels repeat each other's samples for 51274189 samples (about 17.8 minutes at 48 kHz). It is not a
 * power of two on purpose: in a mod 2^32 LCG, states k steps apart are correlated when k is divisible by
 * 2^28 or more (a 2^31 offset gives a correlation of -0.5), and a 2^26 spacing put channels 4, 8, 16 and
 * 32 apart on such offsets. With this odd spacing (about 0.382 * 2^27), d * spacing stays at least
 * 1508038 steps away from any multiple of 2^27 for every channel distance d = 1..63, so any two channels
 * are uncorrelated at every lag up to 1508038 samples (31 s at 48 kHz). */
#define AUDIOLIB_WHITENOISEGENERATOR_CHANNEL_SPACING (51274189U)

/* --- Cycle model of the C7x exec function (C7504 and C7524, cycles per exec call) ---
 * Loop terms are from the compiled .asm; the call overhead is a least-squares fit to the Loki cycles of all
 * functional test cases. */
/** @brief ii of the _ci loop per state vector: 1 cycle per vector for both unrolls (ii 5 per 5 vectors, ii 8 per 8).
 *         Bound by the .M/.N units (two multiplies per vector, VMPYWW and VMPYSP, on two vector multiply units)
 *         and by the .D2 store port (one VST8W per vector). */
#define AUDIOLIB_WHITENOISEGENERATOR_CYCLES_PER_VECTOR (1)
/** @brief Pipeline fill and drain of the _ci loop, paid once per call; 12 for both unrolls (compiled .asm). */
#define AUDIOLIB_WHITENOISEGENERATOR_PIPE_UP_CYCLES (12)
/** @brief Cycles per call outside the loop (handle reads, state set-up, SA0/SA1 open and close, state update,
 *         return), least-squares fit to Loki over all functional cases: exact on every case. The 8-vector
 *         function sets up 3 more state vectors and saves and restores two more vector registers. */
#define AUDIOLIB_WHITENOISEGENERATOR_UNROLL5_CALL_OVERHEAD_CYCLES (48)
#define AUDIOLIB_WHITENOISEGENERATOR_UNROLL8_CALL_OVERHEAD_CYCLES (56)

/*!
 * @brief Modelled cycles of one C7x exec call with @p numVectors state vectors per loop iteration.
 *        @ref AUDIOLIB_whiteNoiseGenerator_init calls it for both unrolls and keeps the faster one, and
 *        @ref AUDIOLIB_whiteNoiseGenerator_perfEst calls it for the chosen one, so the two cannot disagree.
 *
 * @param [in]  numChannels : Number of channel rows
 * @param [in]  samples     : Samples per row
 * @param [in]  numVectors  : State vectors per loop iteration (5 or 8)
 *
 * @return      Modelled cycles of one exec call
 */
static inline uint64_t AUDIOLIB_whiteNoiseGenerator_execCycles(int32_t numChannels, int32_t samples, int32_t numVectors)
{
   // every row runs whole iterations of numVectors vectors, one cycle per vector
   uint64_t iterationsPerRow = (uint64_t) AUDIOLIB_ceilingDiv(samples, STATE_INDEX * numVectors);
   uint64_t loopCycles       = (uint64_t) numChannels * iterationsPerRow * (uint64_t) numVectors *
                         AUDIOLIB_WHITENOISEGENERATOR_CYCLES_PER_VECTOR;

   uint64_t callCycles = (numVectors == AUDIOLIB_WHITENOISEGENERATOR_LONG_UNROLL)
                             ? AUDIOLIB_WHITENOISEGENERATOR_UNROLL8_CALL_OVERHEAD_CYCLES
                             : AUDIOLIB_WHITENOISEGENERATOR_UNROLL5_CALL_OVERHEAD_CYCLES;

   return callCycles + AUDIOLIB_WHITENOISEGENERATOR_PIPE_UP_CYCLES + loopCycles;
}

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
                                                     const AUDIOLIB_bufParams2D_t                *bufParamsOut,
                                                     const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs);

/*!
 * @brief C7x execution function. The function declaration conforms to the declaration of
 *        @ref AUDIOLIB_whiteNoiseGenerator_exec.
 *
 * @details One software-pipelined loop covers every channel row. It runs 5 full-width state vectors, each holding
 * the next LCG outputs with the top bit flipped, and stores 5 vectors (40 samples) per iteration through SA0. Every row
 * runs whole iterations: SA0's DECDIM1 masks the lanes past the end of the row, so no store reaches the next channel
 * and no remainder code is needed. On a row's last iteration SA1's predicate (DECDIM1 again) is false, and VSEL picks
 * the jump to the next channel's first outputs instead of the step within the row, without a branch.
 *
 * @param [in]  handle       :  Active handle to the kernel
 * @param [out] pOut         :  Pointer to the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_unroll5_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/*!
 * @brief As @ref AUDIOLIB_whiteNoiseGenerator_unroll5_exec_ci with 8 state vectors per loop iteration (64 samples);
 *        init selects it when it pads the rows less (see @ref AUDIOLIB_whiteNoiseGenerator_execCycles).
 *
 * @param [in]  handle       :  Active handle to the kernel
 * @param [out] pOut         :  Pointer to the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_unroll8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut);

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
   int32_t                               samples;     /**< \brief Samples per channel (from bufParamsOut->dim_x). */
   int32_t                               numChannels; /**< \brief Number of channels (from bufParamsOut->dim_y). */
   int32_t strideOut;     /**< \brief Distance between channel rows, in elements (from bufParamsOut->stride_y). */
   float   range;         /**< \brief The output range [-range, +range]. */
   float   scale;         /**< \brief range / 2^31: maps (int32_t) (x ^ 2^31) to [-range, +range] in one multiply. */
   int32_t numVectors;    /**< \brief State vectors per _ci loop iteration: 5 or 8, chosen by init. */
   int32_t numIterations; /**< \brief Loop iterations per row in the _ci loop (numVectors vectors each, at least 1). */
   int32_t totalIterations; /**< \brief numIterations * numChannels: iterations of the _ci loop. */
   /* LCG jump constants, X_n+k = (multiplier * X_n + increment) mod 2^32, for the step counts k below */
   uint32_t jumpHeadMultiplierNextFrame; /**< \brief k = samples: from one frame to the next. */
   uint32_t jumpHeadIncNextFrame;        /**< \brief k = samples: from one frame to the next. */
   uint32_t channelMultiplier;           /**< \brief k = CHANNEL_SPACING: from one channel to the next. */
   uint32_t channelIncrement;            /**< \brief k = CHANNEL_SPACING. */
   uint32_t rowJumpMultiplier; /**< \brief k = CHANNEL_SPACING - (numIterations - 1) * numVectors * STATE_INDEX:
                                * replaces the step of a row's last iteration, to the next channel's first outputs. */
   uint32_t rowJumpIncrement;  /**< \brief k as for rowJumpMultiplier. */
   /** \brief k = v * STATE_INDEX: from state vector 1 to state vector v + 1 (entry 0 is the identity). */
   uint32_t vectorMultiplier[AUDIOLIB_WHITENOISEGENERATOR_LONG_UNROLL];
   uint32_t vectorIncrement[AUDIOLIB_WHITENOISEGENERATOR_LONG_UNROLL]; /**< \brief k as for vectorMultiplier. */
   uint32_t iterationMultiplier; /**< \brief k = numVectors * STATE_INDEX: one loop iteration. With one iteration per
                                  *          row it equals rowJumpMultiplier, so SA1's predicate is not needed. */
   uint32_t iterationIncrement;  /**< \brief k as for iterationMultiplier. */
   /** \brief STATE_INDEX consecutive LCG states X_n+1 .. X_n+STATE_INDEX, where X_n+1 is channel 0's next output.
    *         Channel c's are CHANNEL_SPACING * c steps further. _ci uses all of them; _cn uses only X_n+1. */
   uint32_t                              states[STATE_INDEX];
   AUDIOLIB_whiteNoiseGenerator_InitArgs initArgs; /**< \brief Structure holding initialization parameters. */
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_PBLOCK_SIZE]; /*< Array to hold SE/SA template */
} AUDIOLIB_whiteNoiseGenerator_PrivArgs;

#endif /* AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_PRIV_H_ */
