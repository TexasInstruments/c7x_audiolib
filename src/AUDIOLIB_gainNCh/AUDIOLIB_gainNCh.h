// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_GAINNCH_IXX_IXX_OXX_H_
#define AUDIOLIB_GAINNCH_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_gainNCh AUDIOLIB_gainNCh
 * @brief Kernel for multichannel gain apply
 *
 * @details
 * Apply per-channel gain for interleaved and non-interleaved audio buffers.
 *
 * Each channel is scaled by its corresponding gain value.
 *
 * ---
 * **Non-Interleaved Layout:**
 *
 *   \f[
 *     X =
 *     \begin{bmatrix}
 *       x_{0,0} & x_{0,1} & x_{0,2} & \cdots & x_{0,N-1} \\
 *       x_{1,0} & x_{1,1} & x_{1,2} & \cdots & x_{1,N-1} \\
 *       \vdots  & \vdots  & \vdots  & \ddots & \vdots    \\
 *       x_{C-1,0} & x_{C-1,1} & x_{C-1,2} & \cdots & x_{C-1,N-1}
 *     \end{bmatrix}
 *   \f]
 *
 *   where each row corresponds to a channel, and each column corresponds to a sample.
 *
 * ---
 * **Interleaved Layout:**
 *
 *   \f[
 *     X =
 *     \begin{bmatrix}
 *       x_{0,0}, & x_{1,0}, & \dots, & x_{C-1,0}, &
 *       x_{0,1}, & x_{1,1}, & \dots, & x_{C-1,1}, &
 *       \dots,   &
 *       x_{0,N-1}, & x_{1,N-1}, & \dots, & x_{C-1,N-1}
 *     \end{bmatrix}
 *   \f]
 *
 *   where samples are stored sequentially per frame, channel by channel.
 *
 * ---
 * **Per-Channel Gain Application:**
 *
 *   \f[
 *     Y[c][n] = g_c \cdot X[c][n], \quad
 *     0 \leq c < C, \; 0 \leq n < N
 *   \f]
 *
 * Where:
 *   - \f$ C \f$ = number of channels
 *   - \f$ N \f$ = number of samples per channel
 *   - \f$ g_c \f$ = gain applied to channel \f$c\f$
 *   - \f$ X[c][n] \f$ = input sample for channel \f$c\f$ at time \f$n\f$
 *   - \f$ Y[c][n] \f$ = output sample after scaling
 * @ingroup  AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t funcStyle;
   /** @brief Flag indicating interleaved (1) or non-interleaved (0) input layout */
   uint8_t isInterleave;

} AUDIOLIB_gainNCh_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_gainNCh_getHandleSize(AUDIOLIB_gainNCh_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_gainNCh_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of gain buffer
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
AUDIOLIB_STATUS AUDIOLIB_gainNCh_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams1D_t          *bufParamsGain,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_gainNCh_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gainNCh_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gainNCh_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gainNCh_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of gain buffer
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
AUDIOLIB_gainNCh_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_gainNCh_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gainNCh_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gainNCh_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gainNCh_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel buffer
 *  @param [in]  pIn       :  Pointer to buffer holding the input buffer
 *  @param [in]  pGain     :  Pointer to buffer holding the input gain buffer
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS AUDIOLIB_gainNCh_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                  const void *restrict pIn,
                                                  const void *restrict pGain,
                                                  const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_gainNCh_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [in]  pGain      : Pointer to buffer holding the input gain buffer
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
 *    (M = channels, N = samples per channel; one gain value per channel)
 *
 *    De-Interleave case
 *
 *    | Buffer   | dimY | dimX | Comments |
 *    | :--      | :--: | :--: | :-----|
 *    | pIn      | M    | N    | 2D Input Buffer |
 *    | pGain    | 1    | M    | Per-channel gain vector |
 *    | pOut     | M    | N    | 2D Output Buffer|
 *
 *    Interleave case
 *
 *    | Buffer   | dimY | dimX | Comments |
 *    | :--      | :--: | :--: | :-----|
 *    | pIn      | M    | N    | 2D Input Buffer |
 *    | pGain    | 1    | M    | Per-channel gain vector |
 *    | pOut     | M    | N    | 2D Output Buffer|
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_gainNCh_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_gainNCh_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut);

/**
 *  @brief        This funtion is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [out] archCycles     :  Arch compute cycles obtained from asm
 *  @param [out] estCycles      :  Cycles estimated for that particular kernel
 *
 *
 *  @remarks     None
 */
void AUDIOLIB_gainNCh_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_GAINNCH_IXX_IXX_OXX_H_ */
