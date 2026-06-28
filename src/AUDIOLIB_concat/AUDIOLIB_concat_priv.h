// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_CONCAT_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_CONCAT_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_concat.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_concat_PrivArgs structure.
 *
 */
#define MAX_SE_PARAMS (64)
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + MAX_SE_PARAMS * SE_PARAM_SIZE)
// Offset for the iteration count scalar stored after the SA template
#define SE_ITERCOUNT_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE)

#define AUDIOLIB_CONCAT_IXX_IXX_OXX_PBLOCK_SIZE (MAX_SE_PARAMS * SE_PARAM_SIZE + SA_PARAM_SIZE + sizeof(uint32_t))

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_concat_exec_ci
 *         and @ref AUDIOLIB_concat_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_concat_exec)(AUDIOLIB_kernelHandle handle,
                                                    void **restrict pIn,
                                                    void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_concat_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_concat_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_concat_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_concat_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to array of structures containing dimensional
 *                                information of each input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_concat_init_ci(AUDIOLIB_kernelHandle           handle,
                                        const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                        const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                        const AUDIOLIB_concat_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the optimized C7x execution function for
 *         concatenating multiple input buffers into a single output buffer.
 *
 * @details This kernel supports both interleaved and non-interleaved layouts.
 *          For non-interleaved, it uses a 2D SE per input and a single 2D SA
 *          on the output. For interleaved, it uses a 2D SE per input and a
 *          3D SA that advances the channel offset across inputs.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to array of N input buffer pointers
 *  @param [out] pOut        : Pointer to the aggregated output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_concat_exec_ci(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);
/*!
 *  @brief This function is the natural C reference implementation of the
 *         input aggregator kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_concat_exec.
 *
 * @details This implementation handles all format conversion combinations
 *          (interleaved/non-interleaved for both input and output) using
 *          simple nested loops. It serves as a reference for functional
 *          verification of the optimized C7x implementation.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to array of input buffer pointers
 *  @param [out] pOut        : Pointer to the aggregated output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_concat_exec_cn(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the input aggregator kernel
 */
typedef struct {
   /*! @brief Function pointer to the selected execution variant:
    *         either @ref AUDIOLIB_concat_exec_cn or
    *         @ref AUDIOLIB_concat_exec_ci. */
   pFxnAUDIOLIB_concat_exec execute;

   /*! @brief Number of input buffers to aggregate */
   uint32_t numInputs;
   /*! @brief Flag: 1 if data is in interleaved format, 0 if non-interleaved */
   uint8_t isInterleave;
   /*! @brief Number of channels per input buffer (uniform across all inputs) */
   uint32_t inChannels;
   /*! @brief Number of samples per channel per input buffer */
   uint32_t inSamples;
   /*! @brief Input buffer stride in elements (stride_y / sizeof(element)) */
   uint32_t strideIn;
   /*! @brief Output buffer stride in elements (stride_y / sizeof(element)) */
   uint32_t strideOut;
   /*! @brief Parameter block storing SE/SA templates and iteration count for C7x */
   uint8_t bufPblock[AUDIOLIB_CONCAT_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_concat_PrivArgs;

#endif /* AUDIOLIB_CONCAT_IXX_IXX_OXX_PRIV_H_ */
