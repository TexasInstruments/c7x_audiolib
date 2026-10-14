// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_FLOOR_IXX_IXX_OXX_H_
#define AUDIOLIB_FLOOR_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_floor AUDIOLIB_floor
 * @brief  Kernel for computing the elementwise floor of an input buffer.
 * @details
 * Supported data types (input and output types match):
 *
 *  | Case | Input  | Output |
 *  | :--: | :----: | :----: |
 *  |  1   | float  | float  |
 *  |  2   | double | double |
 *
 * @par Method
 * \f[
 * floor(x) = \max \{ n \in \mathbb{Z} \mid n \leq x \}
 * \f]
 * For a @c -0.0 input the optimized variant may return @c +0.0; the two compare
 * equal.
 *
 * ### Implementation Details
 * The optimized variant is branch free. It truncates to int32 and converts back,
 * then corrects negative inputs that had a fraction:
 * \f[
 * t = \mathrm{trunc}(x) \qquad
 * y = \begin{cases} t - 1 & \text{if } x < t \\ t & \text{otherwise} \end{cases}
 * \f]
 * The input is streamed through both streaming engines, so the comparison reads
 * its own copy and nothing is held in registers across the conversion. The loop
 * exists unrolled by two and by eight (1.5 and 1.375 cycles per vector), and
 * @ref AUDIOLIB_floor_init picks whichever is faster for the buffer size.
 *
 * @par Preconditions
 * - Every sample must satisfy \f$ |x| < 2^{31} \f$ in both data types, 256 times
 *   24 bit PCM full scale. Larger values saturate the int32 conversion.
 * - Inf and NaN are not supported.
 *
 * The optimized variant does not check either; outside them its output is
 * meaningless and differs from the natural C variant, which calls the C library.
 *
 * @ingroup  AUDIOLIB
 */

/**@{*/

/**
 * @brief Parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant to run, see @ref AUDIOLIB_FUNCTION_STYLE */
   int8_t funcStyle;

} AUDIOLIB_floor_InitArgs;

/**
 * @brief  Returns the size in bytes of the handle the application must allocate.
 * @param [in] pKerInitArgs : Pointer to the init parameters
 * @return Handle size in bytes
 */
int32_t AUDIOLIB_floor_getHandleSize(AUDIOLIB_floor_InitArgs *pKerInitArgs);

/**
 * @brief  Checks the parameters of @ref AUDIOLIB_floor_init. Call it with the same
 *         arguments, before @ref AUDIOLIB_floor_init.
 * @param [in] handle       : Active handle to the kernel
 * @param [in] bufParamsIn  : Dimensions and type of the input buffer
 * @param [in] bufParamsOut : Dimensions and type of the output buffer
 * @param [in] pKerInitArgs : Pointer to the init parameters
 * @return @c AUDIOLIB_SUCCESS, or
 *         - @c AUDIOLIB_ERR_NULL_POINTER if @p handle, @p bufParamsIn or
 *           @p bufParamsOut is NULL
 *         - @c AUDIOLIB_ERR_INVALID_TYPE if the type is not float or double, or the
 *           input and output types differ
 *         - @c AUDIOLIB_ERR_INVALID_DIMENSION if the input and output dimensions
 *           differ
 */
AUDIOLIB_STATUS
AUDIOLIB_floor_init_checkParams(AUDIOLIB_kernelHandle          handle,
                                const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                const AUDIOLIB_floor_InitArgs *pKerInitArgs);

/**
 * @brief  One-time setup before @ref AUDIOLIB_floor_exec: selects the execution
 *         variant and stores the streaming configuration in the handle.
 * @param [in] handle       : Active handle to the kernel
 * @param [in] bufParamsIn  : Dimensions and type of the input buffer
 * @param [in] bufParamsOut : Dimensions and type of the output buffer
 * @param [in] pKerInitArgs : Pointer to the init parameters
 * @return Status value, see @ref AUDIOLIB_STATUS
 */
AUDIOLIB_STATUS AUDIOLIB_floor_init(AUDIOLIB_kernelHandle          handle,
                                    AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                    AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                    const AUDIOLIB_floor_InitArgs *pKerInitArgs);

/**
 * @brief  Estimates the cycles of @ref AUDIOLIB_floor_exec for the configuration in
 *         @p handle.
 * @param [in]  handle     : Active handle, after @ref AUDIOLIB_floor_init
 * @param [out] archCycles : Cycles of the compute loop
 * @param [out] estCycles  : @p archCycles plus the fixed call overhead
 */
void AUDIOLIB_floor_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 * @brief  Checks the parameters of @ref AUDIOLIB_floor_exec. Call it with the same
 *         arguments, before @ref AUDIOLIB_floor_exec.
 * @param [in] handle : Active handle to the kernel
 * @param [in] pIn    : Input buffer
 * @param [in] pOut   : Output buffer
 * @return @c AUDIOLIB_SUCCESS, or @c AUDIOLIB_ERR_NULL_POINTER if any argument is
 *         NULL
 */
AUDIOLIB_STATUS
AUDIOLIB_floor_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 * @brief  Computes the elementwise floor of @p pIn into @p pOut, see
 *         @ref AUDIOLIB_floor. Call @ref AUDIOLIB_floor_init first; the input must meet
 *         the preconditions listed there.
 * @param [in]  handle : Active handle to the kernel
 * @param [in]  pIn    : Input buffer
 * @param [out] pOut   : Output buffer
 *
 * @par Memory Requirements
 * | Buffer | dimY | dimX | Comments      |
 * | :----: | :--: | :--: | :-----------: |
 * | pIn    | M    | N    | Input buffer  |
 * | pOut   | M    | N    | Output buffer |
 *
 * @par Performance Considerations
 * Best with both buffers in L2 and 64-byte aligned.
 *
 * @return Status value, see @ref AUDIOLIB_STATUS
 */
AUDIOLIB_STATUS
AUDIOLIB_floor_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_FLOOR_IXX_IXX_OXX_H_ */
