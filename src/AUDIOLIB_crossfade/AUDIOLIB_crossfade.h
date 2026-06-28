// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_CROSSFADE_IXX_IXX_OXX_H_
#define AUDIOLIB_CROSSFADE_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_crossfade AUDIOLIB_crossfade
 * @brief Kernel for multichannel crossfade apply
 *
 * @details
 *          - Kernel for applying same crossfade on multiple channels
 *
 *
 * Audio buffer element-wise combination with gain factors.
 *
 * This operation computes the output sample as a weighted sum of the previous
 * and next buffers using cosine and sine gains.
 *
 * Non-interleaved (planar) layout:
 * \f[
 * output_{i,j} = prev_{i,j} * gainCos_j + next_{i,j} * gainSine_j
 * \f]
 *
 * Interleaved layout:
 * \f[
 * output_{j,i} = prev_{j,i} * gainCos_j + next_{j,i} * gainSine_j
 * \f]
 *
 * Index ranges:
 * - i = 0, 1, ..., channels-1
 * - j = 0, 1, ..., samples-1
 *
 * Where:
 * - prev = input buffer from previous data
 * - next = input buffer from next data
 * - gainCos_j = cosine gain factor for sample j
 * - gainSine_j = sine gain factor for sample j

 * @ingroup  AUDIOLIB
 */
/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t  funcStyle;
   uint8_t isInterleave;

} AUDIOLIB_crossfade_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_crossfade_getHandleSize(AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_crossfade_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn0   :  Pointer to the structure containing dimensional
 *                                information of previous input buffer
 *  @param [in]  bufParamsIn1  :  Pointer to the structure containing dimensional
 *                                information of next input buffer
 *  @param [in]  bufParamsIn2  :  Pointer to the structure containing dimensional
 *                                information of cos gain buffer
 *  @param [in]  bufParamsIn3  :  Pointer to the structure containing dimensional
 *                                information of sine gain buffer
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

AUDIOLIB_STATUS AUDIOLIB_crossfade_init(AUDIOLIB_kernelHandle              handle,
                                        const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                        const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                        const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                        const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                        const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                        const AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_crossfade_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_crossfade_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_crossfade_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn0   :  Pointer to the structure containing dimensional
 *                                information of previous input buffer
 *  @param [in]  bufParamsIn1  :  Pointer to the structure containing dimensional
 *                                information of next input buffer
 *  @param [in]  bufParamsIn2  :  Pointer to the structure containing dimensional
 *                                information of cos gain buffer
 *  @param [in]  bufParamsIn3  :  Pointer to the structure containing dimensional
 *                                information of sine gain buffer
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
AUDIOLIB_crossfade_init_checkParams(AUDIOLIB_kernelHandle              handle,
                                    const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                    const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                    const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                    const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                    const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                    const AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

/** * @brief Estimates the performance of the crossfade kernel.
 *  @param [in]  handle          : Active handle to the kernel
 *
 *  @param [out] archCycles      : Pointer to store the estimated architecture cycles
 *
 *  @param [out] estCycles       : Pointer to store the estimated cycles for execution
 *
 *  @param [in] data_type        : Data type of the input data
 *
 * *  @details This function provides an estimation of the performance characteristics
 *           of the crossfade kernel
 * */
void AUDIOLIB_crossfade_perfEst(AUDIOLIB_kernelHandle handle,
                                uint64_t             *archCycles,
                                uint64_t             *estCycles,
                                uint32_t              data_type);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_crossfade_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_crossfade_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_crossfade_init is called.
 *
 *  @param [in]  handle :  Active handle to the kernel
 *
 *  @param [in]  pIn0   :  Pointer to the buffer containing dimensional
 *                                information of previous input
 *  @param [in]  pIn1   :  Pointer to the buffer containing dimensional
 *                                information of next input
 *  @param [in]  pIn2   :  Pointer to the buffer containing dimensional
 *                                information of cos gain
 *  @param [in]  pIn3   :  Pointer to the buffer containing dimensional
 *                                information of sine gain
 *  @param [out] pOut   :  Pointer to the buffer containing dimensional
 *                                information of output
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 *
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 * */
AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                    const void *restrict pIn0,
                                                    const void *restrict pIn1,
                                                    const void *restrict pIn2,
                                                    const void *restrict pIn3,
                                                    const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_crossfade_exec
 *
 *  @param [in]  handle :  Active handle to the kernel
 *  @param [in]  pIn0   :  Pointer to the buffer containing dimensional
 *                                information of previous input
 *  @param [in]  pIn1   :  Pointer to the buffer containing dimensional
 *                                information of next input
 *  @param [in]  pIn2   :  Pointer to the buffer containing dimensional
 *                                information of cos gain
 *  @param [in]  pIn3   :  Pointer to the buffer containing dimensional
 *                                information of sine gain
 *  @param [out] pOut   :  Pointer to the buffer containing dimensional
 *                                information of output
 *
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
 *               @ref AUDIOLIB_crossfade_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_crossfade_exec(AUDIOLIB_kernelHandle handle,
                        void *restrict pIn0,
                        void *restrict pIn1,
                        void *restrict pIn2,
                        void *restrict pIn3,
                        void *restrict pOut);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_CROSSFADE_IXX_IXX_OXX_H_ */
