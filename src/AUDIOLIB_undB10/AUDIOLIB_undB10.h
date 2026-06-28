// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_UNDB10_IXX_IXX_OXX_H_
#define AUDIOLIB_UNDB10_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_undB10 AUDIOLIB_undB10
 * @brief Kernel for calculating elementwise inverse dB transformation of input vector data scaled down by a factor
 *of 20.
 *
 * @details
 *  | Case    | Input   | Output  |
 *  |  ----:  | :----:  | :----   |
 *  |    1    | float   | float   |
 *
 ** @par Method
 * \[
 * \text{undB10}(y) = 10^{\frac{y}{20}}
 * \]
 *
 * @ingroup  AUDIOLIB
 *
 * @details
 *          -  Function can be overloaded with float , and the appropriate
 * precision is employed to compute elementwise inverse dB of the input vector.
 *
 *   **Logic for scalar function**
 *
 * @brief Get N such that |N - x * 16 / ln(2)| is minimized.
 *
 * Find the integer \(N\) that minimizes:
 * @f[
 * \left| N - \frac{16 \cdot x}{\ln(2)} \right|
 * @f]
 *
 * @brief Perform argument reduction, compute \(r\), and polynomial approximation \(\text{pol}(r)\).
 *
 * Argument reduction:
 * @f[
 * r = x - N \cdot \frac{\ln(2)}{16}
 * @f]
 *
 * Polynomial approximation:
 * @f[
 * \text{pol}(r) \approx f(r)
 * @f]
 *
 * @brief Get index for ktable and jtable.
 *
 * Compute indices \(k, j\) for lookup tables:
 * @f[
 * k = \left\lfloor \frac{N}{16} \right\rfloor, \quad j = N \bmod 16
 * @f]
 *
 * @brief Scale exponent to adjust for \(2^M\).
 *
 * Adjust the exponent by:
 * @f[
 * M = k
 * @f]
 *
 * Scaling by:
 * @f[
 * 2^M
 * @f]
 *
 * @brief Early exit for small \(a\).
 *
 * For small input \(a\), return early when:
 * @f[
 * |a| < \epsilon
 * @f]
 *
 * where \(\epsilon\) is a small threshold.
 *
 *      **Logic for vector function**
 *
 * @brief Create variables employed in the exp10 computation.
 *
 * Initialize all necessary variables for the computation of \(\exp_{10}\).
 *
 * @brief Get \(N\) such that \(|N - \text{inVec} \cdot 16 / \ln(2)|\) is minimized.
 *
 * Find integer \(N\) to minimize:
 * @f[
 * \left| N - \frac{16 \cdot \text{inVec}}{\ln(2)} \right|
 * @f]
 *
 * @brief Calculate Taylor series approximation for \(\exp_{10}\).
 *
 * Use the Taylor series centered around 0 to approximate:
 * @f[
 * \exp_{10}(r) \approx \sum_{n=0}^{k} \frac{r^n}{n!}
 * @f]
 *
 * where \(r\) is the reduced argument after subtracting \(N \cdot \frac{\ln(2)}{16}\).
 *
 * @brief Get index of Lookup Table (LUT) and \(2^M\) values.
 *
 * Compute indices for LUT access and the exponent adjustment:
 * @f[
 * k = \left\lfloor \frac{N}{16} \right\rfloor, \quad j = N \bmod 16
 * @f]
 *
 * where \(M = k\).
 *
 * @brief Create vectors of LUT indices.
 *
 * Prepare vectors containing the indices needed to read LUT values efficiently.
 *
 * @brief Read values from LUT, convert and store as doubles in split vectors.
 *
 * Retrieve LUT values, convert them to double precision, and store in vectorized form.
 *
 * @brief Multiply LUT values.
 *
 * Perform vectorized multiplication of LUT values as part of the approximation process.
 *
 * @brief Scale exponent to adjust for \(2^M\).
 *
 * Apply scaling to the exponent by multiplying by:
 * @f[
 * 2^M
 * @f]
 *
 * @brief Extract upper 32 bits and lower 32 bits of all double-precision \(dT\) values.
 *
 * Split each double \(dT\) into two 32-bit parts:
 * - Upper 32 bits
 * - Lower 32 bits
 *
 * @brief Concatenate adjusted upper 32 bits to lower 32 bits, convert \(dT\) to float.
 *
 * Combine the adjusted upper and lower 32-bit parts and convert the result back to single-precision float.
 *
 * @brief Adjust calculation so that \(DT\) and \(POL\) are doubles, then output as float.
 *
 * Ensure intermediate variables \(DT\) (difference term) and \(POL\) (polynomial result) are kept as double precision
 * to improve accuracy, and cast the final output to float precision.
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;

} AUDIOLIB_undB10_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */

int32_t AUDIOLIB_undB10_getHandleSize(AUDIOLIB_undB10_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_undB10_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_undB10_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_undB10_init is called.
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
AUDIOLIB_undB10_init_checkParams(AUDIOLIB_kernelHandle           handle,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                 const AUDIOLIB_undB10_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_undB10_exec function is called. This
 *               function takes care of any one-time operations such as setting
 * up the configuration of required hardware resources such as the MMA
 *               accelerator and the streaming engine.  The results of these
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

AUDIOLIB_STATUS AUDIOLIB_undB10_init(AUDIOLIB_kernelHandle           handle,
                                     AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                     AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                     const AUDIOLIB_undB10_InitArgs *pKerInitArgs);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  dataType       :  Datatype of purticular test case
 *  @param [in]  archCycles     :  Arch cycles used in that purticluar kernel
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *
 *  @return      Void.
 *
 *  @remarks     None
 */

void AUDIOLIB_undB10_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_undB10_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_undB10_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_undB10_exec is called.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  pIn  :  Pointer to the structure input buffer
 *  @param [out] pout :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_undB10_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_undB10_exec
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
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
 *               @ref AUDIOLIB_undB10_init and
 *               @ref AUDIOLIB_undB10_exec_checkParams functions.
 *               This ensures resource configuration and error checks are done
 * only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_undB10_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_UNDB10_IXX_IXX_OXX_H_ */
