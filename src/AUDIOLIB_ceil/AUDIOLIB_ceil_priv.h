// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_CEIL_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_CEIL_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_ceil.h"

/** @brief Size of bufPblock in @ref AUDIOLIB_ceil_PrivArgs: one SE and one SA template. */
#define AUDIOLIB_CEIL_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE)

/**
 * @brief Cycle model, used by @ref AUDIOLIB_ceil_init to pick the variant and by
 *        @ref AUDIOLIB_ceil_perfEst. x2 and x8 are the loops unrolled by two and by
 *        eight. Pipeline and ii terms are from the compiled .asm, identical on C7504
 *        and C7524, except the three fill-and-drain terms marked "measured", where
 *        Loki shows one cycle more than the compiler. The call overhead (call, both
 *        stream opens, address generator, return) is fitted to Loki over all 64 test
 *        cases: exact on C7504 for six or more vectors, up to three cycles high for
 *        one or two vectors, one cycle high on C7524. Re-measure it if work moves
 *        into or out of the loop.
 */
#define AUDIOLIB_CEIL_SP_PIPE_UP_CYCLES (11)         // Fill and drain, float, x2.
#define AUDIOLIB_CEIL_SP_CYCLES_PER_VEC_PAIR (3)     // ii, float, x2.
#define AUDIOLIB_CEIL_DP_PIPE_UP_CYCLES (13)         // Fill and drain, double, x2. Measured; compiler 12.
#define AUDIOLIB_CEIL_DP_CYCLES_PER_VEC_PAIR (3)     // ii, double, x2.
#define AUDIOLIB_CEIL_SP_UNROLL8_PIPE_UP_CYCLES (12) // Fill and drain, float, x8. Measured; compiler 11.
#define AUDIOLIB_CEIL_DP_UNROLL8_PIPE_UP_CYCLES (14) // Fill and drain, double, x8. Measured; compiler 13.
#define AUDIOLIB_CEIL_UNROLL8_CYCLES_PER_8_VECS (11) // ii, both types, x8.
#define AUDIOLIB_CEIL_CALL_OVERHEAD_CYCLES (44)      // Cycles outside the loop, fitted to Loki.

/** @brief Modelled loop cycles of @ref AUDIOLIB_ceil_exec_ci for @p nVecs vectors. */
static inline uint64_t AUDIOLIB_ceil_unroll2LoopCycles(uint32_t dataType, uint32_t nVecs)
{
   uint64_t nPairs = AUDIOLIB_ceilingDiv(nVecs, 2U);
   uint64_t cycles = 0U;

   if (dataType == AUDIOLIB_FLOAT64) {
      cycles = AUDIOLIB_CEIL_DP_PIPE_UP_CYCLES + nPairs * AUDIOLIB_CEIL_DP_CYCLES_PER_VEC_PAIR;
   }
   else {
      cycles = AUDIOLIB_CEIL_SP_PIPE_UP_CYCLES + nPairs * AUDIOLIB_CEIL_SP_CYCLES_PER_VEC_PAIR;
   }

   return cycles;
}

/**
 * @brief Modelled loop cycles of @ref AUDIOLIB_ceil_unroll8_exec_ci for @p nVecs
 *        vectors, including the masked surplus up to a multiple of eight.
 */
static inline uint64_t AUDIOLIB_ceil_unroll8LoopCycles(uint32_t dataType, uint32_t nVecs)
{
   uint64_t nEightVecGroups = AUDIOLIB_ceilingDiv(nVecs, 8U);
   uint64_t pipeUpCycles    = (dataType == AUDIOLIB_FLOAT64) ? AUDIOLIB_CEIL_DP_UNROLL8_PIPE_UP_CYCLES
                                                             : AUDIOLIB_CEIL_SP_UNROLL8_PIPE_UP_CYCLES;

   return pipeUpCycles + nEightVecGroups * AUDIOLIB_CEIL_UNROLL8_CYCLES_PER_8_VECS;
}

/** @brief Signature shared by the execution variants, see @ref AUDIOLIB_ceil_exec. */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_ceil_exec)(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pOut);

/**
 * @brief  C7x part of @ref AUDIOLIB_ceil_init: builds the SE and SA templates and
 *         stores them, with the vector count, in the handle.
 * @param [in] handle       : Active handle to the kernel
 * @param [in] bufParamsIn  : Dimensions and type of the input buffer
 * @param [in] bufParamsOut : Dimensions and type of the output buffer
 * @param [in] pKerInitArgs : Pointer to the init parameters
 * @return Status value, see @ref AUDIOLIB_STATUS
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ceil_init_ci(AUDIOLIB_kernelHandle         handle,
                                             const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                             const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                             const AUDIOLIB_ceil_InitArgs *pKerInitArgs);

/**
 * @brief  C7x execution, unrolled by two. Reads the input through both streaming
 *         engines and writes through SA0, whose predicate masks the partial last
 *         vector.
 * @param [in]  handle : Active handle to the kernel
 * @param [in]  pIn    : Input buffer
 * @param [out] pOut   : Output buffer
 * @return Status value, see @ref AUDIOLIB_STATUS
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ceil_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief  @ref AUDIOLIB_ceil_exec_ci unrolled by eight, selected by
 *         @ref AUDIOLIB_ceil_init where the cycle model makes it faster.
 * @param [in]  handle : Active handle to the kernel
 * @param [in]  pIn    : Input buffer
 * @param [out] pOut   : Output buffer
 * @return Status value, see @ref AUDIOLIB_STATUS
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_ceil_unroll8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief  Natural C execution, used as the reference.
 * @param [in]  handle : Active handle to the kernel
 * @param [in]  pIn    : Input buffer
 * @param [out] pOut   : Output buffer
 * @return Status value, see @ref AUDIOLIB_STATUS
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ceil_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/** @brief Kernel state kept in the handle, for internal use only. */
typedef struct {
   pFxnAUDIOLIB_ceil_exec execute; /**< \brief Execution variant chosen by @ref AUDIOLIB_ceil_init */

   uint32_t nVecs;     /**< \brief Number of vectors in the buffer */
   uint32_t dim_x;     /**< \brief Width of buffer in X dimension in elements. */
   uint32_t dim_y;     /**< \brief Height of buffer in Y dimension in elements. */
   int32_t  inStride;  /**< \brief Stride of input buffer in elements. */
   int32_t  outStride; /**< \brief Stride of output buffer in elements. */
   uint32_t dataType;  /**< \brief Data type of the buffer elements. */

   uint8_t bufPblock[AUDIOLIB_CEIL_IXX_IXX_OXX_PBLOCK_SIZE]; /**< \brief bufPblock array to store SE/SA template */
} AUDIOLIB_ceil_PrivArgs;

#endif /* AUDIOLIB_CEIL_IXX_IXX_OXX_PRIV_H_ */
