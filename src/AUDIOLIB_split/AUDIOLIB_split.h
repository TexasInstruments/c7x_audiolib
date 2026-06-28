// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SPLIT_IXX_IXX_OXX_H_
#define AUDIOLIB_SPLIT_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_split AUDIOLIB_split
 * @brief Kernel for splitting a single input buffer into multiple output buffers
 *
 * @details
 * This kernel splits a single input audio buffer containing multiple channels into
 * multiple output buffers, each with the same channel count (\f$ C_{\text{total}} / M \f$).
 * It supports interleaved and non-interleaved layouts; each output buffer uses the same
 * layout as the input (the kernel does not convert between layouts).
 *
 * ---
 * **Input Splitting:**
 *
 * Given a single input buffer with \f$ C_{\text{total}} \f$ total channels split evenly into
 * \f$ M \f$ output buffers, each with \f$ C = C_{\text{total}} / M \f$ channels:
 *
 *   \f[
 *     \text{Input}: C_{\text{total}} = M \times C \text{ channels}
 *   \f]
 *
 * The output buffers each receive an equal, contiguous share of channels:
 *
 *   \f[
 *     \text{Output}_0: C \text{ channels}, \quad
 *     \text{Output}_1: C \text{ channels}, \quad
 *     \ldots, \quad
 *     \text{Output}_{M-1}: C \text{ channels}
 *   \f]
 *
 * ---
 * **Non-Interleaved Layout (planar):**
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
 *   where samples are stored sequentially, channel by channel.
 *
 * ---
 * **Supported Layouts (each output uses the same layout as the input):**
 *   - Non-interleaved input to Non-interleaved outputs
 *   - Interleaved input to Interleaved outputs
 *
 * Where:
 *   - \f$ M \f$ = number of output buffers
 *   - \f$ C \f$ = channels per output buffer (\f$ C = C_{\text{total}} / M \f$, same for all outputs)
 *   - \f$ N \f$ = number of samples per channel (same across input and all outputs)
 *   - \f$ C_{\text{total}} \f$ = total input channels
 *
 * @ingroup  AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the split kernel
 */
typedef struct {
   /** @brief Variant of the function, refer to @ref AUDIOLIB_FUNCTION_STYLE */
   int8_t funcStyle;
   /** @brief channel count for each output buffer */
   uint32_t outChannels;
   /** @brief Number of output buffers to split into */
   uint32_t numOutputs;
   /** @brief Flag indicating if input data is in interleaved format (1) or non-interleaved (0) */
   uint8_t isInputInterleave;
} AUDIOLIB_split_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_split_getHandleSize(AUDIOLIB_split_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_split_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the array of structures containing dimensional
 *                                information of each output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */
AUDIOLIB_STATUS AUDIOLIB_split_init(AUDIOLIB_kernelHandle          handle,
                                    AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                    AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                    const AUDIOLIB_split_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_split_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_split_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_split_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the array of structures containing dimensional
 *                                information of each output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 *                                parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_split_init_checkParams(AUDIOLIB_kernelHandle          handle,
                                const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                const AUDIOLIB_split_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_split_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_split_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_split_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel buffer
 *  @param [in]  pIn       :  Pointer to the input buffer
 *  @param [out] pOut      :  Pointer to array of output buffer pointers
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_split_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void **restrict pOut);

/**
 *  @brief       This function is the main kernel compute function that splits
 *               a single input buffer into multiple output buffers.
 *
 *  @details     This function reads from a single input buffer and distributes the channels
 *               evenly into multiple output buffers, each with the same channel count.
 *               Each output buffer uses the same layout (interleaved or non-interleaved)
 *               as the input, as selected during initialization.
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to the input buffer
 *  @param [out] pOut       : Pointer to array of output buffer pointers
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - All output buffers will have the same number of samples per channel as input
 *    - Handle must be initialized via @ref AUDIOLIB_split_init
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_split_init.
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */
AUDIOLIB_STATUS
AUDIOLIB_split_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);

/**
 *  @brief        This function computes the architectural and estimated
 *                cycle counts for the core loop of the execution kernel.
 *
 *  @param [in]   handle         :  Active handle to the kernel
 *  @param [out]  archCycles     :  Pointer to store the architectural cycle count
 *  @param [out]  estCycles      :  Pointer to store the estimated cycle count
 *
 *  @remarks      Must be called after @ref AUDIOLIB_split_init.
 */
void AUDIOLIB_split_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_SPLIT_IXX_IXX_OXX_H_ */
