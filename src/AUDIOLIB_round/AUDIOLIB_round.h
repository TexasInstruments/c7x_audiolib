// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ROUND_H_
#define AUDIOLIB_ROUND_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_round AUDIOLIB_round
 * @brief Kernel that rounds each sample of an audio buffer to the nearest
 *        integral value, with halfway cases rounded to even.
 * @ingroup AUDIOLIB
 *
 * The kernel operates elementwise on a 2D buffer and writes a buffer of the
 * same type and dimensions. The result stays in the input's floating-point
 * type; only the fractional part is discarded. Input and output must both be
 * `AUDIOLIB_FLOAT32` or both be `AUDIOLIB_FLOAT64`.
 *
 * ---
 * **Definition**
 *
 * Each sample is rounded to the nearest integral value. A sample exactly
 * halfway between two integers is rounded to whichever of the two is even.
 * This is IEEE 754 round-to-nearest-even, the mode selected by the reset value
 * of the FPCR `RMODE` field, and it matches `std::rint` in C++ and `np.round`
 * in NumPy:
 *
 * | Input  | Output | Note                                    |
 * | :--    | :--:   | :--                                     |
 * | 2.3    | 2.0    | nearest is below                        |
 * | 2.7    | 3.0    | nearest is above                        |
 * | 0.5    | 0.0    | halfway, 0 is the even neighbour        |
 * | 2.5    | 2.0    | halfway, 2 is the even neighbour        |
 * | 3.5    | 4.0    | halfway, 4 is the even neighbour        |
 * | -2.5   | -2.0   | halfway, sign is immaterial to the rule |
 * | -0.3   | -0.0   | the result keeps the sign of the input  |
 *
 * ---
 * @note Halfway cases round to even, not away from zero. `std::round` in C and
 *       C++ rounds halfway cases away from zero and therefore does **not**
 *       describe this kernel.
 * @note Values whose magnitude is already integral, along with infinities and
 *       NaNs, are returned unchanged. No intermediate integer representation is
 *       used, so the full exponent range of the type is supported.
 */

/**@{*/



/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t  funcStyle;

} AUDIOLIB_round_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_round_getHandleSize(AUDIOLIB_round_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_round_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */
AUDIOLIB_STATUS AUDIOLIB_round_init(AUDIOLIB_kernelHandle                   handle,
                                             AUDIOLIB_bufParams2D_t                 *bufParamsIn,
                                             AUDIOLIB_bufParams2D_t                 *bufParamsOut,
                                             const AUDIOLIB_round_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_round_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_round_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_round_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_round_init_checkParams(AUDIOLIB_kernelHandle                   handle,
                                         const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                         const AUDIOLIB_round_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_round_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_round_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_round_init is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the structure input buffer
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS
AUDIOLIB_round_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                         const void *restrict pIn,
                                         const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_round_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
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
 *  @par Memory Requirements
 *    | Buffer   | dimY | dimX | Comments |
 *    | :--      | :--: | :--: | :-----|
 *    | pIn      | M    | N    | 2D Input Buffer |
 *    | pOut     | M    | N    | 2D Output Buffer|
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_round_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_round_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This function is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [out] archCycles     :  Cycles spent in the software pipelined loop
 *  @param [out] estCycles      :  Loop cycles plus the fixed call overhead
 *  @param [in]  inDataType     :  Input data type. Not read: the handle holds
 *                                 the model of the variant chosen at init
 *  @param [in]  outDataType    :  Output data type. Not read
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
void AUDIOLIB_round_perfEst(AUDIOLIB_kernelHandle handle,
                                     uint64_t             *archCycles,
                                     uint64_t             *estCycles,
                                     uint32_t              inDataType,
                                     uint32_t              outDataType);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_ROUND_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_round.h                   */
/* ======================================================================== */
