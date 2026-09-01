// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_DB10_IXX_IXX_OXX_H_
#define AUDIOLIB_DB10_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_dB10 AUDIOLIB_dB10
 * @brief  Kernel for computing the elementwise base-10 logarithm of an input vector, scaled by a factor of 10.
 * @details
 * The following datatype combinations are supported
 *
 *  | Case    | Input   | Output  |
 *  |  ----:  | :----:  | :----   |
 *  |    1    | float   | float   |
 *  |    2    | double  | double  |

 * @par Method
 * \f[
 * dB10(x) = 10 \cdot \log_{10}(x)
 * \f]
 *
 * @details
 *          -  Function can be overloaded with float and double pointers, and the appropriate
 * precision is employed to compute elementwise log base 10 of the input vector.
 *
 *   **Logic for vector function of data type float is**
 *
 * Steps:
 * 1. Split input vector and compute reciprocal approximations
 *    - Convert input floats to double precision halves (odd/even).
 *    - Calculate reciprocal of each half with double precision.
 *    - Prepare reciprocal approximations by bit masking and rearranging.
 *
 * 2. Calculate variable 'r' for Taylor series expansion
 *    - Compute r = (reciprocal_approx * input) - 1.0 in double precision.
 *    - Convert 'r' to float and compute powers r^1, r^2, r^3, r^4.
 *
 * 3. Compute polynomial correction term 'pol'
 *    - Use coefficients C1 to C5 to calculate a polynomial in r (Taylor series).
 *    - Multiply polynomial result by base (log10(e)) constant.
 *
 * 4. Calculate index N and retrieve LUT values
 *    - Compute exponent part N from reciprocal approximation.
 *    - Use N to index into LUT tables to get correction values (split vectors).
 *    - Adjust LUT values using N and constants ln(2) and base.
 *
 * 5. Combine LUT values, polynomial, and apply bounds checking
 *    - Combine LUT corrections with Taylor polynomial.
 *    - Check for invalid inputs (≤0) and clamp outputs for large values.
 *    - Store final log10 results to output vector
 *
 *   **For double precision**
 *
 *  **For length greater than 1 and less than 33 use split - 1 function for better optimization**
 * 1. Floating-point decomposition:
 *    Extract the exponent and normalize the mantissa:
 *    @f[
 *    Y = Z \cdot 2^{E - 1023}
 *    @f]
 *    where:
 *    - @f$ Y @f$ is the original input
 *    - @f$ Z @f$ is the normalized mantissa in the range @f$ [0.5, 2.0) @f$
 *    - @f$ E @f$ is the biased exponent extracted from the IEEE-754 double format
 *
 * 2. Range reduction for polynomial approximation:
 *    Use a change of variable to improve approximation accuracy:
 *    @f[
 *    t = \frac{Z - 1}{Z + 1} \quad \text{or} \quad t = \frac{Z - c}{Z + c}
 *    @f]
 *    Then:
 *    @f[
 *    W = t^2
 *    @f]
 *
 * 3. Polynomial approximation of ln(Z) using a rational function:
 *    @f[
 *    \ln(Z) \approx t + t \cdot \frac{P(W)}{Q(W)}
 *    @f]
 *    Where:
 *    - @f$ P(W) = a_0 + a_1 W + a_2 W^2 @f$
 *    - @f$ Q(W) = 1 + b_0 W + b_1 W^2 + b_2 W^3 @f$
 *
 * 4. Compute ln(Y) by adding the exponent contribution:
 *    Use high-precision split of ln(2):
 *    @f[
 *    \ln(Y) = \ln(Z) + (E - 1023) \cdot \ln(2)
 *    @f]
 *    where:
 *    @f[
 *    \ln(2) \approx c_1 + c_2
 *    @f]
 *
 * 5. Convert to base-10 logarithm:
 *    @f[
 *    \log_{10}(Y) = \frac{\ln(Y)}{\ln(10)} = \ln(Y) \cdot \left(\frac{1}{\ln(10)}\right)
 *    @f]
 *
 * @note
 * Constants used:
 * - @f$ c_1 = 0.693359375 @f$ (coarse ln(2))
 * - @f$ c_2 = -2.1219444005469058 \times 10^{-4} @f$ (residual)
 * - @f$ \frac{1}{\ln(10)} = 0.4342944819032518 @f$
 *
 * @param Y  Input vector of double-precision floating-point values
 * @return   Vector of base-10 logarithms for each element of Y
 *
 *
 *  **For length greater than 32 use split - 2 function for better optimization**
 *
 * This kernel operates in **three stages**, performing a high-accuracy computation of:
 *
 * @f[
 * \log_{10}(Y) = \left( \ln(Z) + (E - 1023) \cdot \ln(2) \right) \cdot \frac{1}{\ln(10)}
 * @f]
 *
 * Where:
 * - @f$ Y @f$ is the input value
 * - @f$ Z @f$ is the normalized mantissa of @f$ Y @f$
 * - @f$ E @f$ is the biased exponent from IEEE-754 format
 * - Constants:
 *   - @f$ \ln(2) = c_1 + c_2 @f$  (split for numerical accuracy)
 *   - @f$ \frac{1}{\ln(10)} = c_{10e} @f$
 *
 * ---
 *
 *Stage 1: Compute X = (Z - 1)/(Z + 1) or alternative form
 *
 * - Decompose input @f$ Y = Z \cdot 2^{E - 1023} @f$
 * - Normalize @f$ Z @f$ to range [0.5, 2.0)
 * - Compute rational approximation input variable @f$ X @f$
 *
 * @f[
 * X =
 * \begin{cases}
 * \frac{Z - 1}{Z + 1} & \text{if } Z \geq \sqrt{0.5} \\
 * \frac{Z - 0.5}{0.5 \cdot Z + 0.5 - 0.25} & \text{otherwise}
 * \end{cases}
 * @f]
 *
 * This prepares input for rational approximation of @f$ \ln(Z) @f$.
 *
 * ---
 *
 * Stage 2: Rational polynomial approximation of ln(Z)
 *
 * - Let @f$ W = X^2 @f$
 * - Evaluate minimax polynomials:
 *
 * @f[
 * P(W) = a_0 + a_1 W + a_2 W^2 \\
 * Q(W) = 1 + b_0 W + b_1 W^2 + b_2 W^3
 * @f]
 *
 * - Compute rational log approximation:
 *
 * @f[
 * \ln(Z) \approx X + X \cdot \frac{P(W)}{Q(W)}
 * @f]
 *
 * This result is stored in vector `Sa`.
 *
 * ---
 *
 * stage 3: Combine exponent and convert to log base 10
 *
 * - Extract exponent @f$ E @f$ and compute:
 *
 * @f[
 * N = E - 1022
 * @f]
 * (Note: Offset is 1022 instead of 1023 because mantissa was normalized to 0x3FE = 0.5)
 *
 * - Adjust exponent for low Z:
 *
 * @f[
 * \text{if } Z < \sqrt{0.5} \Rightarrow N = N - 1
 * @f]
 *
 * - Combine exponent and mantissa parts:
 *
 * @f[
 * \ln(Y) = \ln(Z) + N \cdot (c_1 + c_2)
 * @f]
 *
 * - Convert to log base 10:
 *
 * @f[
 * \log_{10}(Y) = \ln(Y) \cdot c_{10e}
 * @f]
 *
 * ---
 *
 *
 * To ensure stable output:
 *
 * - Clamp @f$ Y < \text{MINe} @f$:
 *   @f[
 *   \log_{10}(Y) = -\text{MAXe}
 *   @f]
 *
 * - Clamp @f$ Y > \text{MAXe} @f$:
 *   @f[
 *   \log_{10}(Y) = 308.254715974092
 *   @f]
 *
 * @note Constants used:
 *   - @f$ c_1 = 0.693359375 @f$ (coarse ln(2))
 *   - @f$ c_2 = -2.1219444005469058 \times 10^{-4} @f$ (residual)
 *   - @f$ c_{10e} = 0.4342944819032518 @f$ (1 / ln(10))
 *   - @f$ \text{MINe} = 2.2250738585072014 \times 10^{-308} @f$
 *   - @f$ \text{MAXe} = 1.7976931348623157 \times 10^{308} @f$
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
   /** @brief Size of input data                                              */

} AUDIOLIB_dB10_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */

int32_t AUDIOLIB_dB10_getHandleSize(AUDIOLIB_dB10_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_dB10_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_dB10_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_dB10_init is called.
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
AUDIOLIB_dB10_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_dB10_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_dB10_exec function is called. This
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

AUDIOLIB_STATUS AUDIOLIB_dB10_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_dB10_InitArgs *pKerInitArgs);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  archCycles     :  Arch cycles used in that purticluar kernel
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *
 *
 *  @remarks     None
 */

void AUDIOLIB_dB10_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_dB10_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_dB10_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_dB10_exec is called.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  pIn  :  Pointer to the structure input buffer
 *  @param [out] pOut :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_dB10_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_dB10_exec
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
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS
AUDIOLIB_dB10_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_DB10_IXX_IXX_OXX_H_ */
