// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_H_
#define AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_rmsSmoothed AUDIOLIB_rmsSmoothed
 * @brief Kernel for the smoothed RMS of a multichannel block
 *
 * @details
 * This kernel computes one smoothed RMS value per block, pooled over all
 * channels of the input. The average energy of the block is passed through a
 * first-order lowpass filter that runs once per block, and the square root of
 * the filter output is written to the output.
 *
 * ---
 * **Mathematical Formulation**
 *
 * \f[
 * \begin{aligned}
 * \textbf{Average energy of block } k: \quad
 * & E[k] = \frac{1}{B\,C}\sum_{c=0}^{C-1}\sum_{n=0}^{B-1} x_c[n]^2 \\[6pt]
 * \textbf{Smoothing:} \quad
 * & y[k] = \alpha\,y[k-1] + (1 - \alpha)\,E[k] \\[6pt]
 * \textbf{Output:} \quad
 * & out[k] = \sqrt{y[k]}
 * \end{aligned}
 * \f]
 *
 * where B is the number of samples per channel and C the number of channels.
 *
 * ---
 * **Smoothing Coefficient**
 *
 * The smoothing coefficient \f$\alpha\f$ is computed in @ref AUDIOLIB_rmsSmoothed_set
 * from the sampling rate fs, the block size B and the smoothing time
 * \f$\tau = \text{smoothingTime} / 1000\f$ (smoothingTime in milliseconds, \f$\tau\f$ in seconds):
 *
 * \f[
 * \alpha = e^{-2\pi B / (\tau f_s)}
 * \f]
 *
 * A smoothing time of 0 disables the smoothing and it gives the instantaneous output (\f$\alpha = 0\f$).
 *
 * ---
 * **Notes**
 * - The input is planar: one row of B samples per channel, with a row stride.
 * - The filter state is kept in the handle across calls and is reset by
 *   @ref AUDIOLIB_rmsSmoothed_init. @ref AUDIOLIB_rmsSmoothed_set changes the smoothing time
 *   without resetting it; until it is called, the output is not smoothed.
 * - The kernel supports FLOAT32 and FLOAT64 data types; input and output must have the same type.
 *
 * @ingroup AUDIOLIB
 */
/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t  funcStyle;
   int32_t samplingRate; /**< Audio sampling rate in Hz */

} AUDIOLIB_rmsSmoothed_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {
   float smoothingTime; /**< Smoothing time in milliseconds, must not be negative; 0 disables smoothing (instantaneous
                           output) */

} AUDIOLIB_rmsSmoothed_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_rmsSmoothed_getHandleSize(AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_rmsSmoothed_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine, computing the smoothing filter
 *               coefficients and resetting the filter state.  The results of
 *               these operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of src buffer
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

AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_init(AUDIOLIB_kernelHandle                handle,
                                          const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                          const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                          const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_rmsSmoothed_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_rmsSmoothed_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_rmsSmoothed_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of src buffer
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
AUDIOLIB_rmsSmoothed_init_checkParams(AUDIOLIB_kernelHandle                handle,
                                      const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                      const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                      const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_rmsSmoothed_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_rmsSmoothed_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_rmsSmoothed_exec is called.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the src buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 * */
AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                      const void *restrict pIn,
                                      const void *restrict pOut);

/**
 * @brief Sets the runtime parameters for the rmsSmoothed kernel.
 *
 * @param [in] handle       A valid handle to the kernel instance.
 * @param [in] pKerSetArgs  Pointer to the structure containing the smoothing parameters.
 *
 * @details This function computes the smoothing coefficient from the smoothing time, the sampling
 * rate and the block size. The smoothed energy is kept, so the smoothing time can be changed while
 * the kernel is running. For the formula, please see the @ref AUDIOLIB_rmsSmoothed group description.
 *
 * @return Status code indicating success or failure. See @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_set(AUDIOLIB_kernelHandle handle, AUDIOLIB_rmsSmoothed_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the current runtime parameters from the rmsSmoothed kernel.
 *
 * @param [in]  handle       A valid handle to the kernel instance.
 * @param [out] pKerSetArgs  Pointer to a structure where the current smoothing parameters will be stored.
 * @return Status code indicating success or failure. See @ref AUDIOLIB_STATUS.
 */
AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_rmsSmoothed_SetArgs *pKerSetArgs);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_rmsSmoothed
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the src buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
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
 *               @ref AUDIOLIB_rmsSmoothed_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief Estimates the performance of the rmsSmoothed kernel.
 *
 *  @param [in]  handle           : Active handle to the kernel
 *  @param [out] archCycles       : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles        : Pointer to store the estimated cycles for execution
 *  @param [in]  data_type        : Data type of the processing input
 *  @param [in]  pKerInitArgs     : Pointers to the structure holding init
 *
 *  @details This function provides an estimation of the performance characteristics
 *           of the rmsSmoothed kernel
 * */
void AUDIOLIB_rmsSmoothed_perfEst(AUDIOLIB_kernelHandle                handle,
                                  uint64_t                            *archCycles,
                                  uint64_t                            *estCycles,
                                  uint32_t                             data_type,
                                  const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs);
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_H_ */
