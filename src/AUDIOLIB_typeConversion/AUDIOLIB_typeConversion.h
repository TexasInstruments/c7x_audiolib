// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_H_
#define AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_typeConversion AUDIOLIB_typeConversion
 * @brief Kernel for type conversion between different audio buffer formats.
 * @ingroup AUDIOLIB
 *
 * This kernel converts input buffers between common fixed-point and floating-point
 * representations (e.g., `float`, `double`, `int16_t`, `int32_t`).
 *
 * ---
 * **Supported Conversions**
 *
 * - **Q15  <--> float / double**  (16-bit fixed-point samples)
 *   \f[
 *     \text{float} = \frac{Q15}{2^{15}}, \qquad
 *     Q15 = \text{round}\left(\text{float} \times 2^{15}\right)
 *   \f]
 *
 * - **Q23 / Q31  <--> float / double**  (32-bit fixed-point samples)
 *   \f[
 *     \text{float} = \frac{Qn}{2^{n}}, \qquad
 *     Qn = \text{round}\left(\text{float} \times 2^{n}\right)
 *   \f]
 *   where the scale exponent `n` is:
 *   - 31 if `TEST_Q31` is set
 *   - 23 if `TEST_Q31` is not set (the 24-bit "Q23" path uses a 2^23 scale)
 *
 * ---
 * @note These conversions preserve numerical scaling by interpreting Q-format
 *       fixed-point values as normalized fractional representations.
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t  funcStyle;
   uint8_t testQ31; /**< Test flag: 1 to enable Q31 fixed-point test mode, 0 for normal operation. */

} AUDIOLIB_typeConversion_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_typeConversion_getHandleSize(AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_typeConversion_exec function is called. This
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
AUDIOLIB_STATUS AUDIOLIB_typeConversion_init(AUDIOLIB_kernelHandle                   handle,
                                             AUDIOLIB_bufParams2D_t                 *bufParamsIn,
                                             AUDIOLIB_bufParams2D_t                 *bufParamsOut,
                                             const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_typeConversion_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_typeConversion_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_typeConversion_init is called.
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
AUDIOLIB_typeConversion_init_checkParams(AUDIOLIB_kernelHandle                   handle,
                                         const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                         const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_typeConversion_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_typeConversion_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_typeConversion_exec is called.
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
AUDIOLIB_typeConversion_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                         const void *restrict pIn,
                                         const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_typeConversion_exec
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
 * *  * @par Memory Requirements

 *    | Buffer   | dimY | dimX | Comments |
 *    | :--      | :--: | :--: | :-----|
 *    | pIn      | M    | N    | 2D Input Buffer |
 *    | pOut     | M    | N    | 2D Output Buffer|
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_typeConversion_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_typeConversion_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  archCycles     :  Arch compute cycles obtained from asm
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *  @param [in]  inDataType     :  Input data type
 *  @param [in]  outDataType    :  Output data type
 *
 *  @remarks     None
 */
void AUDIOLIB_typeConversion_perfEst(AUDIOLIB_kernelHandle handle,
                                     uint64_t             *archCycles,
                                     uint64_t             *estCycles,
                                     uint32_t              inDataType,
                                     uint32_t              outDataType);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_H_ */
