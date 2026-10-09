// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_preEmphasis_IXX_IXX_OXX_H_
#define AUDIOLIB_preEmphasis_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_preEmphasis AUDIOLIB_preEmphasis
 * @brief Kernel applying a first-order pre-emphasis (FIR high-pass) filter to multichannel audio.
 * @details
 * The following datatype combinations are supported
 *
 *  | Case    | Input   | Output  |
 *  |  ----:  | :----:  | :----   |
 *  |    1    | float   | float   |
 *
 * @par Method
 * \f[
 * H_{pre}(z) = 1 - \alpha z^{-1} \qquad\Longleftrightarrow\qquad y[n] = x[n] - \alpha \, x[n-1]
 * \f]
 *
 * Each channel is filtered independently with the feedback-free first-order difference
 * above, which lifts high frequencies relative to low ones. @ref AUDIOLIB_deEmphasis applies
 * the exact inverse.
 *
 * - The last input sample of every channel, \f$ x[-1] \f$ for the next call, is carried in
 *   the handle, so a stream may be processed in arbitrary block sizes without a seam.
 *   @ref AUDIOLIB_preEmphasis_init clears it, so the first block behaves as if preceded by
 *   silence.
 * - \f$ \alpha \f$ must lie in \f$ [0, 1] \f$. \f$ \alpha = 0 \f$ passes the input through
 *   unchanged; \f$ \alpha = 1 \f$ is a pure first difference with a zero at DC.
 *
 * **Layouts** (selected by @c isInterleave):
 *
 *  | isInterleave | dimX    | dimY    | Contiguous axis |
 *  | :----:       | :----    | :----    | :----           |
 *  | 0            | samples  | channels | samples         |
 *  | 1            | channels | samples  | channels        |
 *
 * ### Implementation Details
 *
 * The optimized C7x variant is branch free. It needs \f$ x[n] \f$ and \f$ x[n-1] \f$ for
 * every output, and obtains the delayed input by opening a second streaming engine on the
 * same input buffer, offset by one sample, rather than by shifting registers. Each output
 * vector is then one multiply and one subtract:
 * \f[
 * \mathbf{y} = \mathbf{x}_{SE0} - \alpha \, \mathbf{x}_{SE1}
 * \f]
 *
 * - **Interleaved.** A vector holds one sample of up to 8 channels, and SE1 trails SE0 by
 *   one row. Channel groups smaller than a vector are masked by the streaming engine and
 *   store predication.
 * - **Non-interleaved.** A vector holds 8 consecutive samples of one channel, and SE1
 *   starts 7 samples into the row so that its vector \f$ j \f$ holds
 *   \f$ x[8j-1] \ldots x[8j+6] \f$. For the first vector of each row, \f$ x[n-1] \f$ is
 *   built in one instruction by shifting the row's first vector up one lane and merging
 *   the carried sample into lane 0. Rows of at most 8 samples are a single vector; they
 *   take a separate variant that does not open SE1 and retires one row every 2 cycles.
 *
 * The inner loop retires one output vector per cycle, which is the single-store-per-cycle
 * limit of the device, so the kernel runs at 1 cycle per 8 output samples.
 *
 * @par Preconditions
 *  - At most 64 channels. The filter state is a fixed-size array in the handle, and
 *    @ref AUDIOLIB_preEmphasis_init_checkParams rejects larger counts.
 *  - @c pIn and @c pOut must not overlap. In-place operation is not supported: the
 *    non-interleaved variant reads the last input sample of a row after that row's output
 *    has been written.
 *
 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t   funcStyle;
   uint8_t  isInterleave; /**< 0 = non-interleaved (planar), 1 = interleaved. */
   float    alpha;        /**< Pre-emphasis coefficient, range [0, 1]. */
} AUDIOLIB_preEmphasis_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_preEmphasis_getHandleSize(AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_preEmphasis_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine, and clearing the filter state. The results
 *               of these operations are stored in the handle.
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
AUDIOLIB_STATUS AUDIOLIB_preEmphasis_init(AUDIOLIB_kernelHandle                handle,
                                          const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                          const AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                          const AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_preEmphasis_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_preEmphasis_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_preEmphasis_init is called.
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
 *   - @ref AUDIOLIB_ERR_NULL_POINTER       if any argument is NULL
 *   - @ref AUDIOLIB_ERR_INVALID_TYPE       if the data type is not float, or input and output types differ
 *   - @ref AUDIOLIB_ERR_INVALID_VALUE      if @c alpha is outside [0, 1]
 *   - @ref AUDIOLIB_ERR_INVALID_DIMENSION  if the channel count exceeds 64
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_init_checkParams(AUDIOLIB_kernelHandle                handle,
                                      const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                      const AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                      const AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_preEmphasis_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_preEmphasis_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_preEmphasis_exec is called.
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
AUDIOLIB_STATUS AUDIOLIB_preEmphasis_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                      const void *restrict pIn,
                                                      const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Applies \f$ y[n] = x[n] - \alpha x[n-1] \f$ per channel, resuming
 *               from the state left by the previous call. Please refer to details
 *               under @ref AUDIOLIB_preEmphasis
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 * @par Memory Requirements
 * | Buffer | dimY | dimX |  Comments                              |
 * | :----: | :--: | :--: | :--------:                             |
 * | pIn    | M    | N    | Input Buffer; stride from @c bufParamsIn  |
 * | pOut   | M    | N    | Output Buffer; stride from @c bufParamsOut |
 *
 * M x N is channels x samples when non-interleaved and samples x channels when
 * interleaved. The input and output strides may differ.
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - @c pIn and @c pOut do not overlap.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_preEmphasis_init. This ensures resource configuration
 *               and error checks are done only once for several invocations of
 *               this function.
 */
AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This function is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [out] archCycles     :  Arch compute cycles obtained from asm
 *  @param [out] estCycles      :  Cycles estimated for the configured case
 *
 *  @return      Void.
 *
 *  @remarks     None
 */
void AUDIOLIB_preEmphasis_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_preEmphasis_IXX_IXX_OXX_H_ */
