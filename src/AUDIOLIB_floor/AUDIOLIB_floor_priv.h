// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_FLOOR_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_FLOOR_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_floor.h"

/**
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_floor_PrivArgs structure.
 *
 */

#define AUDIOLIB_FLOOR_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE)

/**
 * @brief Rounding bias for double, equal to 2^52 where 52 is the number of
 *        mantissa bits. This is also the smallest double magnitude at or above
 *        which every representable value is already an integer, and therefore
 *        the upper bound, exclusive, on the magnitude of double precision
 *        input: at or above it the bias can shift the value into a coarser
 *        binade instead of leaving it alone. Inf and NaN are exempt, since the
 *        double precision path is entirely native floating point and propagates
 *        them. See the precondition documented on @ref AUDIOLIB_floor_exec.
 *
 */

#define AUDIOLIB_FLOOR_ROUND_BIAS_DOUBLE (4503599627370496.0)

/**
 * @brief Cycles spent filling and draining the software pipeline of the single
 *        precision execution loop, as reported by the compiler.
 *
 */

#define AUDIOLIB_FLOOR_SP_PIPE_UP_CYCLES (9)

/**
 * @brief Initiation interval of the single precision execution loop, i.e.
 *        cycles per pair of sample vectors processed.
 *
 */

#define AUDIOLIB_FLOOR_SP_CYCLES_PER_VEC_PAIR (4)

/**
 * @brief Cycles spent filling and draining the software pipeline of the single
 *        precision execution loop unrolled by four, as reported by the
 *        compiler.
 *
 */

#define AUDIOLIB_FLOOR_SP_UNROLL4_PIPE_UP_CYCLES (13)

/**
 * @brief Initiation interval of the single precision execution loop unrolled by
 *        four, i.e. cycles per group of four sample vectors processed.
 *
 */

#define AUDIOLIB_FLOOR_SP_UNROLL4_CYCLES_PER_VEC_QUAD (6)

/**
 * @brief Smallest vector count for which the single precision variant unrolled
 *        by four is chosen over the one unrolled by two.
 *
 *        The unrolled by four variant is faster per vector (1.5 cycles against
 *        2.0) but drains a longer pipeline (13 cycles against 9), so it only
 *        wins once the buffer is long enough to amortise that. The two are
 *        equal at eight vectors, which is where the threshold is set.
 *
 */

#define AUDIOLIB_FLOOR_SP_UNROLL4_MIN_VECS (8)

/**
 * @brief Cycles spent filling and draining the software pipeline of the double
 *        precision execution loop, as reported by the compiler.
 *
 */

#define AUDIOLIB_FLOOR_DP_PIPE_UP_CYCLES (12)

/**
 * @brief Initiation interval of the double precision execution loop, i.e.
 *        cycles per pair of sample vectors processed.
 *
 */

#define AUDIOLIB_FLOOR_DP_CYCLES_PER_VEC_PAIR (4)

/**
 * @brief Cycles the execution function spends outside its loop, which the loop
 *        model above does not account for. This covers the call itself,
 *        reloading the streaming engine and address generator templates from
 *        the parameter block, opening both, closing both, and the return. The
 *        opens dominate, because opening a stream drains pending stores and
 *        performs the initial fill before the first read is available.
 *
 *        The value is measured rather than derived. Subtracting the modelled
 *        loop cycles from the Loki cycle count leaves exactly 44 on C7504 and
 *        exactly 43 on C7524, identically for every one of the 56 test cases in
 *        both data types, so the overhead is a constant and not a function of
 *        buffer size. 44 is used for both devices, which leaves the C7524
 *        estimate one cycle high.
 *
 *        Re-measure this if the execution function gains or loses work outside
 *        the loop; the loop itself is modelled by the SP and DP macros above.
 *
 */

#define AUDIOLIB_FLOOR_CALL_OVERHEAD_CYCLES (44)

/**
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_floor_exec_ci
 *         and @ref AUDIOLIB_floor_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_floor_exec)(AUDIOLIB_kernelHandle handle,
                                                   void *restrict pIn,
                                                   void *restrict pOut);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_floor_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          hardware resources based on the function call parameters, and the
 *          configuration is saved in bufPBlock array. In the kernel call
 *          sequence, @ref AUDIOLIB_floor_exec_ci would be called later
 *          independently by the application. When
 *          @ref AUDIOLIB_floor_exec_ci runs, it merely retrieves the
 *          configuration from the bufPBlock and uses it to set up the hardware
 *          resources. This arrangement is so that
 *          @ref AUDIOLIB_floor_exec_ci does not lose cycles to determine the
 *          hardware configuration.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_floor_init_ci(AUDIOLIB_kernelHandle          handle,
                                              const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                              const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                              const AUDIOLIB_floor_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_floor_exec.
 *
 * @details The input buffer is read through one streaming engine and the result
 *          is written back through a stream address generator, whose predicate
 *          masks the partial last vector. The compute body is branch free. The
 *          loop is unrolled by two; @ref AUDIOLIB_floor_exec_ci_unroll4 is the
 *          wider single precision variant used for longer buffers.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
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
extern AUDIOLIB_STATUS AUDIOLIB_floor_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief Execution variant of @ref AUDIOLIB_floor_exec_ci unrolled by four,
 *         instantiated for single precision only. Selected by
 *         @ref AUDIOLIB_floor_init when the buffer holds at least
 *         @ref AUDIOLIB_FLOOR_SP_UNROLL4_MIN_VECS vectors.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_floor_exec_ci_unroll4(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_floor_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_floor_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /** @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_floor_exec_cn and
    *         @ref AUDIOLIB_floor_exec_ci.                        */
   pFxnAUDIOLIB_floor_exec execute;

   uint32_t nVecs;     /**< \brief Number of vector to be processed for the given input buffer */
   uint32_t dim_x;     /**< \brief Width of buffer in X dimension in elements. */
   uint32_t dim_y;     /**< \brief Height of buffer in Y dimension in elements. */
   int32_t  inStride;  /**< \brief Stride of input buffer in elements. */
   int32_t  outStride; /**< \brief Stride of output buffer in elements. */
   uint32_t dataType;  /**< \brief Data type of the buffer elements. */

   uint8_t bufPblock[AUDIOLIB_FLOOR_IXX_IXX_OXX_PBLOCK_SIZE]; /**< \brief bufPblock array to store SE/SA template */
} AUDIOLIB_floor_PrivArgs;

#endif /* AUDIOLIB_FLOOR_IXX_IXX_OXX_PRIV_H_ */
