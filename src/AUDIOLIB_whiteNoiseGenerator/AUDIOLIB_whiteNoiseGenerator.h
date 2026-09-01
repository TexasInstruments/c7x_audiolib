// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_H_
#define AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_whiteNoiseGenerator AUDIOLIB_whiteNoiseGenerator
 * @brief Kernel for generating a frame of white noise.
 *
 * @details
 * This kernel generates a frame of pseudo-random white noise using a 32-bit
 * **Linear Congruential Generator (LCG)**. The LCG generates the next integer
 * state \f$X_{n+1}\f$ from the current state \f$X_n\f$ using the formula:
 * \f[ X_{n+1} = (a \cdot X_n + c) \pmod{m} \f]
 *
 * The kernel uses the following standard constants:
 * - \f$ a = 1664525 \f$ (multiplier)
 * - \f$ c = 1013904223 \f$ (increment)
 * - \f$ m = 2^{32} \f$ (modulus, implicit in 32-bit unsigned arithmetic)
 *
 * Each 32-bit integer \f$X_n\f$ is normalized to a floating-point value in
 * the range `[0.0, 1.0)`:
 * \f[ \text{normalized} = \frac{X_n}{2^{32}} \f]
 *
 * This value is then scaled to the desired output range `[-range, +range]`:
 * \f[ y[n] = (\text{normalized} \cdot 2 \cdot \text{range}) - \text{range} \f]
 *
 * ### State Continuity
 * The kernel is stateful and maintains the LCG state across calls. When `exec`
 * is called, it begins generating noise using the state (seed) that was saved
 * from the *previous* call. This ensures a continuous, unbroken sequence of
 * pseudo-random noise across frame boundaries.
 *
 * ### C7x Optimized Implementation
 * The C7x implementation uses 8 parallel LCG states (all derived from the
 * initial seed) to generate 8 samples simultaneously. The main loop is unrolled
 * 4x to process **32 samples per iteration** for high throughput.
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
   /** @brief The initial 32-bit seed for the LCG. */
   uint32_t seed;
   /** @brief The output range. Noise will be in [-range, +range]. */
   float range;
} AUDIOLIB_whiteNoiseGenerator_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_whiteNoiseGenerator_getHandleSize(AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_whiteNoiseGenerator_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
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

AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_init(AUDIOLIB_kernelHandle                        handle,
                                                  AUDIOLIB_bufParams1D_t                      *bufParamsOut,
                                                  const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs);

/**
 * @brief       This function checks the validity of the parameters passed to
 * @ref AUDIOLIB_whiteNoiseGenerator_init function. This function
 * is called with the same parameters as the
 * @ref AUDIOLIB_whiteNoiseGenerator_init, and this function
 * must be called before the
 * @ref AUDIOLIB_whiteNoiseGenerator_init is called.
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
 * @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_whiteNoiseGenerator_init_checkParams(AUDIOLIB_kernelHandle                        handle,
                                              const AUDIOLIB_bufParams1D_t                *bufParamsOut,
                                              const AUDIOLIB_whiteNoiseGenerator_InitArgs *pKerInitArgs);

/** * @brief Estimates the performance of the whiteNoiseGenerator kernel.
 *  @param [in]  handle        : Active handle to the kernel
 *  @param [out] archCycles      : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles       : Pointer to store the estimated cycles for execution
 * *  @details This function provides an estimation of the performance characteristics
 *           of the whiteNoiseGenerator kernel
 * */
void AUDIOLIB_whiteNoiseGenerator_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 * @brief       This function checks the validity of the parameters passed to
 * @ref AUDIOLIB_whiteNoiseGenerator_exec function. This function
 * is called with the same parameters as the
 * @ref AUDIOLIB_whiteNoiseGenerator_exec, and this function
 * must be called before the
 * @ref AUDIOLIB_whiteNoiseGenerator_exec is called.
 *
 * @param [in]  handle    :  Active handle to the kernel
 * @param [out] pOut      :  Pointer to the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 * @remarks     None
 */
AUDIOLIB_STATUS AUDIOLIB_whiteNoiseGenerator_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pOut);

/**
 * @brief       This function is the main kernel compute function.
 *
 * @details     This function generates a frame of white noise. Please refer
 * to @ref AUDIOLIB_whiteNoiseGenerator for algorithm details.
 *
 * @param [in]  handle     : Active handle to the kernel
 * @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 * @par Assumptions:
 * - `handle` has been successfully initialized by @ref AUDIOLIB_whiteNoiseGenerator_init.
 *
 * @par Performance Considerations:
 * For best performance,
 * - the output data buffer is expected to be in L2 memory
 * - the buffer pointers are assumed to be 64-byte aligned
 *
 * @remarks     Before calling this function, application is expected to call
 * @ref AUDIOLIB_whiteNoiseGenerator_init.
 * This ensures resource configuration and error checks are done
 * only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_whiteNoiseGenerator_exec(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_WHITENOISEGENERATOR_IXX_IXX_OXX_H_ */
