// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_deEmphasis_IXX_IXX_OXX_H_
#define AUDIOLIB_deEmphasis_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_deEmphasis AUDIOLIB_deEmphasis
 * @brief Kernel applying a first-order de-emphasis (all-pole) filter to multichannel audio.
 *
 * @details
 * The following datatype combinations are supported
 *
 *  | Case    | Input   | Output  |
 *  |  ----:  | :----:  | :----   |
 *  |    1    | float   | float   |
 *
 * @par Method
 * \f[
 * H_{de}(z) = \frac{1}{1 - \alpha z^{-1}} \qquad\Longleftrightarrow\qquad y[n] = x[n] + \alpha \, y[n-1]
 * \f]
 *
 * De-emphasis is the exact inverse of @ref AUDIOLIB_preEmphasis. Each channel is filtered
 * independently, and \f$ y[-1] \f$ is carried in the handle across calls, so a stream may be
 * processed in blocks of any size; @ref AUDIOLIB_deEmphasis_init clears it.
 *
 * - The filter is un-normalized: the DC gain is \f$ 1/(1-\alpha) \f$ (34 dB at
 *   \f$ \alpha = 0.98 \f$), so the caller must provide headroom.
 * - \f$ \alpha = 1 \f$ is accepted and makes a pure integrator, which accumulates any DC
 *   offset without bound.
 *
 * **Layouts** (selected by @c isInterleave):
 *
 *  | isInterleave | dim_x    | dim_y    | Contiguous axis |
 *  | :----:       | :----    | :----    | :----           |
 *  | 0            | samples  | channels | samples         |
 *  | 1            | channels | samples  | channels        |
 *
 * ### Implementation Details
 *
 * The optimized C7x variant is branch free; @ref AUDIOLIB_deEmphasis_init selects a variant
 * for the channel count, so no loop tests it at run time. The recursion is serial, so each
 * loop keeps one multiply-add on its loop-carried path.
 *
 * - **Interleaved.** Each vector lane is one channel's filter. The loop takes 8 samples per
 *   iteration, combines them pairwise, and forms the last output directly from the carried
 *   state, \f$ y[7] = \sum_{k=0}^{7} \alpha^{k} x[7-k] + \alpha^{8} y[-1] \f$. The last block
 *   is padded with rows that the streaming engine zeroes and neither fetches nor stores, and
 *   the state is read back from the last real output row.
 * - **Non-interleaved.** A three-stage Hillis-Steele scan resolves the recursion inside each
 *   vector of 8 samples, and lane \f$ i \f$ then adds \f$ \alpha^{i+1} y[-1] \f$. Each
 *   iteration retires two vectors per channel, with the carry skipping the pair,
 *   \f$ c' = (s_1 + \alpha^{8} s_0) + \alpha^{16} c \f$. Channels are filtered two at a time,
 *   one per streaming engine.
 *
 *  | Loop                                          | Cycles per vector of 8 samples |
 *  | :----                                         | :----:                         |
 *  | Interleaved                                   | 1.375                          |
 *  | Non-interleaved, channels in pairs            | 4.0                            |
 *  | Non-interleaved, single or left-over channel  | 4.5                            |
 *
 * Both layouts reassociate the recursion, so results can differ from the natural C variant
 * in the last bits.
 *
 * @par Preconditions
 *  - At most 64 channels; @ref AUDIOLIB_deEmphasis_init_checkParams rejects more.
 *  - Interleaved: the filter state is read back as whole vectors from the last output row, so
 *    the output stride must cover the channel count rounded up to 8, as any 64-byte-aligned
 *    stride does.
 *  - Non-interleaved rows are read 16 samples at a time. The device does not fetch the
 *    masked part, but the host-emulation build may, so there the input stride must cover
 *    the row rounded up to 16 samples, as any 64-byte-aligned stride does.
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
   uint32_t numChannels;  /**< Not read by the kernel; the channel count is taken from @c bufParamsIn. */
   float    alpha;        /**< Feedback coefficient, range [0, 1]. */
} AUDIOLIB_deEmphasis_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_deEmphasis_getHandleSize(AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_deEmphasis_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine, precomputing the coefficient powers used
 *               by the scan, and clearing the filter state. The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */
AUDIOLIB_STATUS AUDIOLIB_deEmphasis_init(AUDIOLIB_kernelHandle               handle,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                         const AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_deEmphasis_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_deEmphasis_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_deEmphasis_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
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
AUDIOLIB_deEmphasis_init_checkParams(AUDIOLIB_kernelHandle               handle,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                     const AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_deEmphasis_exec function.
 *               This function must be called before the @ref AUDIOLIB_deEmphasis_exec is called.
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [in]  pOut       : Pointer to buffer holding the output buffer
 * */
AUDIOLIB_STATUS AUDIOLIB_deEmphasis_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                     const void *restrict pIn,
                                                     const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Applies \f$ y[n] = x[n] + \alpha y[n-1] \f$ per channel, resuming
 *               from the state left by the previous call. Please refer to details
 *               under @ref AUDIOLIB_deEmphasis
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input data
 *  @param [out] pOut       : Pointer to buffer holding the output data
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
 *    - @c pIn and @c pOut do not overlap. Both are declared @c restrict, and
 *      in-place operation is not validated.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_deEmphasis_init.
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */
AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

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
void AUDIOLIB_deEmphasis_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_deEmphasis_IXX_IXX_OXX_H_ */
