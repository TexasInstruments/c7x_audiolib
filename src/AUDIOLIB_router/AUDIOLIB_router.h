// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ROUTER_IXX_IXX_OXX_H_
#define AUDIOLIB_ROUTER_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_router AUDIOLIB_router
 * @brief Kernel for selecting specific channels from the input buffer.
 *
 * @details
 * The Router takes an input buffer with multiple channels and outputs
 * only the selected ones in the desired order.
 *
 * ### Mathematical Representation
 * \f[
 * \text{Input Buffer: }
 * \begin{bmatrix}
 * CH_1 & CH_2 & CH_3 & CH_4 & CH_5
 * \end{bmatrix}
 * \;\;\;\xrightarrow{\;\;\text{Router (Channel Select)}\;\;}\;\;\;
 * \text{Output Buffer: }
 * \begin{bmatrix}
 * CH_2 & CH_5 & CH_1
 * \end{bmatrix}
 * \f]
 *
 * ### ASCII Block Diagram
 * \verbatim
 * +-----------------------------------+
 * |           Input Buffer            |
 * |   CH1   CH2   CH3   CH4   CH5     |
 * +-----------------------------------+
 *                |
 *                v
 *        +-----------------+
 *        |     Router      |
 *        | (channel select)|
 *        +-----------------+
 *                |
 *                v
 * +-----------------------------------+
 * |          Output Buffer            |
 * |   CH2   CH5   CH1   (selected)    |
 * +-----------------------------------+
 * \endverbatim
 *
 * ### Graphviz Block Diagram
 * \dot
 * digraph router {
 *   rankdir=LR;
 *   node [shape=record, style=filled, fillcolor=lightgray];
 *
 *   Input  [label="{ Input Buffer | CH1 | CH2 | CH3 | CH4 | CH5 }"];
 *   Router [label="Router\n(channel select)", shape=box, style=filled, fillcolor=white];
 *   Output [label="{ Output Buffer | CH2 | CH5 | CH1 }"];
 *
 *   Input -> Router -> Output;
 * }
 * \enddot
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
   /** @brief Flag indicating interleaved (1) or non-interleaved (0) input layout */
   uint8_t isInterleave;
   /** @brief Number of output channels selected by the router */
   uint32_t numOutputs;

} AUDIOLIB_router_InitArgs;

/**
 * @brief Calculates the estimated and actual cycles used by the router kernel.
 *
 * The estimated cycles are the cycles used by the router kernel for the
 * given input parameters plus the overhead cycles used by the kernel.
 * The actual cycles used by the kernel are the cycles used by the kernel
 * without any overhead.
 *
 * @param handle The kernel handle allocated via AUDIOLIB_router_getHandleSize and configured by AUDIOLIB_router_init.
 * @param archCycles The actual loop  cycles used by the router kernel.
 * @param estCycles The estimated cycles used by the router kernel.
 */
void AUDIOLIB_router_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_router_getHandleSize(AUDIOLIB_router_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_router_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle                   :  Active handle to the kernel
 *  @param [in]  bufParamsIn              :  Pointer to the structure containing dimensional
 *                                           information of input buffer
 *  @param [in]  bufParamsInChannelIndex  :  Pointer to the structure containing channel index
 *                                           information of router
 *  @param [out] bufParamsOut             :  Pointer to the structure containing dimensional
 *                                           information of output buffer
 *  @param [in]  pKerInitArgs             :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */
AUDIOLIB_STATUS AUDIOLIB_router_init(AUDIOLIB_kernelHandle           handle,
                                     AUDIOLIB_bufParams2D_t         *bufParamsIn,
                                     AUDIOLIB_bufParams1D_t         *bufParamsInChannelIndex,
                                     AUDIOLIB_bufParams2D_t         *bufParamsOut,
                                     const AUDIOLIB_router_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_router_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_router_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_router_init is called.
 *
 *  @param [in]  handle                   :  Active handle to the kernel
 *
 *  @param [in]  bufParamsIn              :  Pointer to the structure containing dimensional
 *                                           information of input buffer
 *  @param [in]  bufParamsInChannelIndex  :  Pointer to the structure containing dimensional
 *                                           information of channel index buffer
 *  @param [out] bufParamsOut             :  Pointer to the structure containing dimensional
 *                                           information of output buffer
 *  @param [in]  pKerInitArgs             :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_router_init_checkParams(AUDIOLIB_kernelHandle           handle,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                 const AUDIOLIB_bufParams1D_t   *bufParamsInChannelIndex,
                                 const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                 const AUDIOLIB_router_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_router_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_router_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_router_exec is called.
 *
 *  @param [in]  handle             :  Active handle to the kernel
 *  @param [in]  pIn                :  Pointer to the input buffer
 *  @param [in]  pOutChannels       :  Pointer to the input channel index buffer
 *  @param [out] pOut               :  Pointer to the output buffer
 *  @param [out] pOutScratch        :  Pointer to the intermediate output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS AUDIOLIB_router_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                 const void *restrict pIn,
                                                 const void *restrict pOutChannels,
                                                 const void *restrict pOut,
                                                 const void *restrict pOutScratch);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_router_exec
 *
 *  @param [in]  handle             : Active handle to the kernel
 *  @param [in]  pIn                : Pointer to buffer holding the input buffer
 *  @param [out] pOut               : Pointer to buffer holding the output buffer
 *  @param [in]  pOutScratch        : Pointer to the intermediate output buffer
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
 *               @ref AUDIOLIB_router_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_router_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut, void *restrict pOutScratch);

/**
 *  @brief       This function is used to set the gain and channel dependency for the router kernel.
 *
 *  @param [in]  handle               : Active handle to the kernel
 *  @param [in]  pInChannelIndex      : Pointer to the channel index buffer
 *  @param [in]  pInChannels          : Pointer to the input channel mask buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     This function is used to find the dependency between the selected nput channels,
 *                according to that finding the seperate buffer to be passed to the router kernel
 *                for the continous run for the inner loop to get the maximuum read efficiency.
 *
 */

AUDIOLIB_STATUS AUDIOLIB_router_set(AUDIOLIB_kernelHandle handle, void *restrict pInChannelIndex);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_ROUTER_IXX_IXX_OXX_H_ */
