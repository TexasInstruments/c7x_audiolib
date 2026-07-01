// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_CONCAT_IXX_IXX_OXX_H_
#define AUDIOLIB_CONCAT_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_concat AUDIOLIB_concat
 * @brief Kernel for aggregating multiple input buffers into a single output buffer
 *
 * @details
 * This kernel concatenates multiple input audio buffers, each with a
 * potentially different channel count, into a single unified output buffer. It
 * supports both interleaved and non-interleaved data layouts (output format
 * matches input).
 *
 * ---
 * **Input Aggregation:**
 *
 * Given \f$ M \f$ input buffers where input \f$ i \f$ has \f$ C_i \f$ channels:
 *
 *   \f[
 *     \text{Input}_0: C_0 \text{ channels}, \quad
 *     \text{Input}_1: C_1 \text{ channels}, \quad
 *     \ldots, \quad
 *     \text{Input}_{M-1}: C_{M-1} \text{ channels}
 *   \f]
 *
 * The output buffer contains all channels concatenated:
 *
 *   \f[
 *     \text{Output}: C_{\text{total}} = \sum_{i=0}^{M-1} C_i \text{ channels}
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
 * **Supported Formats:**
 *   - Non-interleaved (planar) input → Non-interleaved output
 *   - Interleaved input → Interleaved output
 *
 * Where:
 *   - \f$ M \f$ = number of input buffers
 *   - \f$ C_i \f$ = number of channels in input buffer \f$ i \f$
 *   - \f$ N \f$ = number of samples per channel (same for all inputs)
 *   - \f$ C_{\text{total}} = \sum_{i=0}^{M-1} C_i \f$ = total output channels
 *
 * @ingroup  AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the concat kernel
 */
typedef struct {
   /** @brief Variant of the function, refer to @ref AUDIOLIB_FUNCTION_STYLE */
   int8_t funcStyle;
   /** @brief Pointer to array containing the channel count for each input buffer
    *         (each input may have a different channel count) */
   uint32_t *inChannels;
   /** @brief Number of input buffers to aggregate */
   uint32_t numInputs;
   /** @brief Total number of channels across all input buffers (sum of inChannels) */
   uint32_t totalInChannels;
   /** @brief Flag indicating if data is in interleaved format (1) or non-interleaved (0). Output format matches input
    * format */
   uint8_t isInterleave;
} AUDIOLIB_concat_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_concat_getHandleSize(AUDIOLIB_concat_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_concat_exec function is called. This
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
AUDIOLIB_STATUS AUDIOLIB_concat_init(AUDIOLIB_kernelHandle           handle,
                                     AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                     AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                     const AUDIOLIB_concat_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_concat_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_concat_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_concat_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the array of structures containing dimensional
 *                                information of each input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 *                                parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_concat_init_checkParams(AUDIOLIB_kernelHandle           handle,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                 const AUDIOLIB_concat_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_concat_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_concat_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_concat_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel buffer
 *  @param [in]  pIn       :  Pointer to array of input buffer pointers
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_concat_exec_checkParams(AUDIOLIB_kernelHandle handle, const void **restrict pIn, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function that aggregates
 *               multiple input buffers into a single output buffer.
 *
 *  @details     This function reads from multiple input buffers, each with the same
 *               channel count, and writes all channels into a single
 *               unified output buffer. The output preserves the data layout
 *               (interleaved or non-interleaved) selected during initialization.
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to array of input buffer pointers
 *  @param [out] pOut       : Pointer to the aggregated output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - All input buffers have the same number of samples per channel
 *    - Handle must be initialized via @ref AUDIOLIB_concat_init
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_concat_init.
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */
AUDIOLIB_STATUS
AUDIOLIB_concat_exec(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);

/**
 *  @brief        This function estimates the arch and est cycles for the
 *                execution kernel loop.
 *
 *  @param [in]   handle      :  Active handle to the kernel
 *  @param [out]  archCycles  :  Pointer to store architectural compute cycles
 *  @param [out]  estCycles   :  Pointer to store estimated total cycles
 *
 *  @remarks      Handle must be initialized via @ref AUDIOLIB_concat_init.
 */
void AUDIOLIB_concat_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_CONCAT_IXX_IXX_OXX_H_ */
