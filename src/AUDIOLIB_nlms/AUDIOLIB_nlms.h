// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_NLMS_IXX_IXX_OXX_H_
#define AUDIOLIB_NLMS_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_nlms AUDIOLIB_nlms
 * @brief Kernel for Normalized Least Mean Squares (NLMS) Adaptive Filter
 *
 * @details
 * @brief Performs adaptive filtering to estimate a clean signal from a noisy input.
 *
 * This kernel implements the NLMS algorithm. Based on the test configuration,
 * it is set up to adapt the **Input (Noisy)** signal to match the **Desired (Clean)** signal.
 * The filter minimizes the error between the clean reference and the filtered output.
 *
 * ---
 * **Signal Definitions:**
 *
 * - \f$ x(n) \f$: **Input Signal (Buffer: pIn)**.
 * The noisy signal containing both the target audio and interference.
 *
 * - \f$ d(n) \f$: **Desired Signal (Buffer: pInRef)**.
 * The clean reference signal (ground truth) that the filter attempts to match.
 *
 * - \f$ y(n) \f$: **Output Signal (Buffer: pOut)**.
 * The result of applying the adaptive filter to the input. As the filter converges,
 * \f$ y(n) \f$ approximates the clean signal \f$ d(n) \f$.
 *
 * - \f$ e(n) \f$: **Error Signal (Internal Calculation)**.
 * \f$ e(n) = d(n) - y(n) \f$.
 * The difference between the clean reference and the estimated output. This error
 * drives the adaptation of the filter coefficients \f$ w(n) \f$.
 *
 * ---
 * **Algorithm Mathematics:**
 *
 * 1. **Filtering (Output Generation):**
 * \f[
 * y(n) = \sum_{k=0}^{L-1} w_k(n) \cdot x(n-k)
 * \f]
 *
 * 2. **Error Calculation:**
 * \f[
 * e(n) = d(n) - y(n)
 * \f]
 *
 * 3. **Coefficient Update:**
 * \f[
 * w_k(n+1) = w_k(n) + \mu \cdot \frac{e(n)}{\| \mathbf{x}(n) \|^2 + \delta} \cdot x(n-k)
 * \f]
 *
 * Where:
 * - \f$ \mu \f$: Step size (controls convergence speed).
 * - \f$ \delta \f$: Regularization term (prevents division by zero).
 *
 * ---
 * **Internal Structure of the NLMS Adaptive Filter:**
 *
 * @image html AUDIOLIB_nlms_block_diagram.svg "Internal structure of the NLMS adaptive filter"
 * @image latex AUDIOLIB_nlms_block_diagram.pdf "Internal structure of the NLMS adaptive filter" width=14cm
 *
 * The diagram shows:
 * - Input x[n] feeds the FIR filter and coefficient update mechanism
 * - FIR filter produces output y[n] using adaptive coefficients w[n]
 * - Output y[n] is subtracted from reference d[n] to compute error e[n]
 * - Error e[n] drives the coefficient update mechanism (NLMS adaptation)
 * - Updated coefficients w[n] are fed back to the FIR filter
 *
 * ---
 * **Signal Definitions:**
 * - **x[n]**: Input signal (noisy/contaminated)
 * - **d[n]**: Reference signal (clean reference)
 * - **y[n]**: Output signal (filtered/estimated)
 * - **e[n]**: Error signal = d[n] - y[n]
 * - **w[n]**: Adaptive filter coefficients
 *
 * **NLMS Update Equation:**
 * \f[
 * w[n+1] = w[n] + \mu \cdot \frac{e[n] \cdot x[n]}{\|x[n]\|^2 + \delta}
 * \f]
 *
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
   /** @brief Length of the adaptive filter (number of taps) */
   uint32_t filterLength;
   /** @brief Step size (mu) for coefficient adaptation */
   float stepSize;
   /** @brief Number of audio channels */
   uint32_t numChannels;
} AUDIOLIB_nlms_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_nlms_getHandleSize(AUDIOLIB_nlms_InitArgs *pKerInitArgs);

/**
 * @brief       This function should be called before the @ref AUDIOLIB_nlms_exec
 * function is called. This function takes care of any one-time
 * operations such as setting up the configuration of required
 * hardware resources (Streaming Engine/Address Generators).
 * The results of these operations are stored in the handle.
 *
 * @param [in]  handle        :  Active handle to the kernel
 * @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 * information of input buffer (x)
 * @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 * information of output buffer (y)
 * @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 * @remarks     Application is expected to provide a valid handle.
 */
AUDIOLIB_STATUS AUDIOLIB_nlms_init(AUDIOLIB_kernelHandle         handle,
                                   AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                   const AUDIOLIB_nlms_InitArgs *pKerInitArgs);

/**
 * @brief       This function checks the validity of the parameters passed to
 * @ref AUDIOLIB_nlms_init function.
 *
 * @param [in]  handle        :  Active handle to the kernel
 * @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 * information of input buffer
 * @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 * information of output buffer
 * @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS
AUDIOLIB_nlms_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               AUDIOLIB_bufParams2D_t       *bufParamsIn,
                               AUDIOLIB_bufParams2D_t       *bufParamsOut,
                               const AUDIOLIB_nlms_InitArgs *pKerInitArgs);

/**
 * @brief       This function checks the validity of the parameters passed to
 * @ref AUDIOLIB_nlms_exec function.
 *
 * @param [in]  handle          : Active handle to the kernel
 * @param [in]  pIn             : Pointer to input buffer
 * @param [in]  pInRef          : Pointer to desired buffer
 * @param [in]  pStateBuffer    : Pointer to circular state buffer
 * @param [in]  pScratchBuffer  : Pointer to scratch/accumulator buffer
 * @param [in]  pCoefficients   : Pointer to filter coefficients buffer
 * @param [out] pOut            : Pointer to output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_nlms_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                               void *restrict pIn,
                                               void *restrict pInRef,
                                               void *restrict pStateBuffer,   // Circular State Buffer
                                               void *restrict pScratchBuffer, // Accumulator Buffer
                                               void *restrict pCoefficients,
                                               void *restrict pOut);

/**
 * @brief       This function is the main kernel compute function.
 *
 * @details     Performs NLMS adaptive filtering using C7x Streaming Engine and MMA
 * (in optimized mode) or Natural C (in CN mode).
 *
 * @param [in]  handle          : Active handle to the kernel
 * @param [in]  pIn             : Pointer to input buffer
 * @param [in]  pInRef          : Pointer to reference buffer
 * @param [in]  pStateBuffer    : Pointer to circular state buffer.
 * Size must be power of 2 >= (filterLength + totalSamples).
 * @param [in]  pScratchBuffer  : Pointer to scratch buffer for accumulation.
 * Size: numChannels * strideInElements.
 * @param [in]  pCoefficients   : Pointer to filter coefficients buffer (w).
 * Size: numChannels * strideInElements.
 * @param [out] pOut            : Pointer to output buffer (Estimated signal y).
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 * @par Performance Considerations:
 * For best performance,
 * - The input, state, coefficient, and output buffers are expected to be in L2 memory.
 * - The buffer pointers are assumed to be 64-byte aligned.
 *
 * @par Memory Requirements:
 * The application must allocate persistent memory for pStateBuffer and pCoefficients between calls.
 * pScratchBuffer can be scratch memory.
 */
AUDIOLIB_STATUS
AUDIOLIB_nlms_exec(AUDIOLIB_kernelHandle handle,
                   void *restrict pIn,
                   void *restrict pInRef,
                   void *restrict pStateBuffer,   // Circular State Buffer
                   void *restrict pScratchBuffer, // Accumulator Buffer
                   void *restrict pCoefficients,
                   void *restrict pOut);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  dataType       :  Datatype of purticular test case
 *  @param [in]  archCycles     :  Arch compute cycles obtained from asm
 *  @param [in]  estCycles      :  Cycles estimated for that purticular kenel
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
void AUDIOLIB_nlms_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_NLMS_IXX_IXX_OXX_H_ */
