// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_CEIL_IXX_IXX_OXX_H_
#define AUDIOLIB_CEIL_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_ceil AUDIOLIB_ceil
 * @brief  Kernel for computing the elementwise ceiling of an input vector.
 * @details
 * The following datatype combinations are supported
 *
 *  | Case    | Input   | Output  |
 *  |  ----:  | :----:  | :----   |
 *  |    1    | float   | float   |
 *  |    2    | double  | double  |
 *
 * @par Method
 * \f[
 * ceil(x) = \min \{ n \in \mathbb{Z} \mid n \geq x \}
 * \f]
 *
 * The result is the smallest integral value that is not less than the input,
 * returned in the input data type. The sign of a zero input is preserved by the
 * natural C variant; the optimized variant may return @c +0.0 for a @c -0.0
 * input, which compares equal numerically.
 *
 * ### Implementation Details
 *
 * The optimized C7x variant is branch free. The two data types take different
 * routes, because the device has a truncating conversion to signed 32 bit
 * integer but none to 64 bit.
 *
 * **Single precision.** The integral part is obtained by converting to signed
 * 32 bit integer with truncation and back. Truncation is toward zero, so it
 * already equals the ceiling for negative input and needs one correction when
 * a positive input had a fractional part:
 * \f[
 * t = \mathrm{trunc}(x) \qquad
 * y = \begin{cases} t + 1 & \text{if } x > t \\ t & \text{otherwise} \end{cases}
 * \f]
 *
 * **Double precision.** No such conversion exists, so a round-to-nearest
 * integral value is produced by adding and then subtracting
 * \f$ M = 2^{52} \f$, carrying the sign of the input so that the intermediate
 * sum always lands in the binade whose spacing is exactly 1.0. Rounding to
 * nearest undershoots by at most one, so a single predicated correction
 * produces the ceiling:
 * \f[
 * m = \begin{cases} -M & \text{if } x < 0 \\ +M & \text{otherwise} \end{cases}
 * \qquad r = (x + m) - m \qquad
 * y = \begin{cases} r + 1 & \text{if } r < x \\ r & \text{otherwise} \end{cases}
 * \f]
 *
 * @par Precondition on the input range
 * The optimized variant does not range check its input. The caller must keep
 * the magnitude of every sample below the bound for its data type:
 *
 *  | Data type | Bound on magnitude | Inf and NaN          |
 *  | :----     | :----              | :----                |
 *  | float     | \f$ 2^{31} \f$     | not supported        |
 *  | double    | \f$ 2^{52} \f$     | propagated unchanged |
 *
 * At or above the bound the result is meaningless rather than merely
 * imprecise: the single precision conversion saturates, and the double
 * precision bias can shift an already integral value into a coarser binade.
 * Single precision additionally cannot carry Inf or NaN through the integer
 * conversion, so those inputs return a finite garbage value, whereas double
 * precision is native floating point throughout and propagates both. The
 * natural C variant calls the C library and is correct over the whole range in
 * both types, so the two variants disagree outside these bounds by design.
 *
 * Both bounds sit far outside audio sample data; \f$ 2^{31} \f$ is 256 times
 * full scale for 24 bit PCM.
 *
 * @ingroup  AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;

} AUDIOLIB_ceil_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */

int32_t AUDIOLIB_ceil_getHandleSize(AUDIOLIB_ceil_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_ceil_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_ceil_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_ceil_init is called.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of output buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS
AUDIOLIB_ceil_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_ceil_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_ceil_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
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
 *  @remarks     Application is expected to provide a valid handle.
 */

AUDIOLIB_STATUS AUDIOLIB_ceil_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_ceil_InitArgs *pKerInitArgs);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  archCycles     :  Arch cycles used in that purticluar kernel
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *
 *  @return      Void.
 *
 *  @remarks     None
 */

void AUDIOLIB_ceil_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_ceil_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_ceil_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_ceil_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the input buffer
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_ceil_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_ceil
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 * @par Memory Requirements
 * | Buffer | dimY | dimX |  Comments     |
 * | :----: | :--: | :--: | :--------:    |
 * | pIn    | M    | N    | Input Buffer  |
 * | pOut   | M    | N    | Output Buffer |
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - None
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_ceil_init. This ensures resource configuration
 *               and error checks are done only once for several invocations of
 *               this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_ceil_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_CEIL_IXX_IXX_OXX_H_ */
