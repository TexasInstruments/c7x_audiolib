// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SPLIT_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_SPLIT_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_split.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_split_PrivArgs structure.
 *
 */

#define AUDIOLIB_SPLIT_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_split_exec_ci
 *         and @ref AUDIOLIB_split_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_split_exec)(AUDIOLIB_kernelHandle handle,
                                                   void *restrict pIn,
                                                   void **restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_split_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_split_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_split_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_split_exec_ci does not lose cycles
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
AUDIOLIB_STATUS AUDIOLIB_split_init_ci(AUDIOLIB_kernelHandle          handle,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsIn,
                                       const AUDIOLIB_bufParams2D_t  *bufParamsOut,
                                       const AUDIOLIB_split_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the optimized C7x execution function for splitting
 *         a single input buffer into N output buffers.
 *
 * @details This kernel splits a single input audio buffer containing multiple
 *          channels into several output buffers, distributing channels across
 *          the outputs. It supports both interleaved and non-interleaved data
 *          layouts. The function uses the streaming engine (SE) to read from
 *          the input buffer and the streaming address generator (SA) to write
 *          channels to each output buffer.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to the input buffer containing all channels
 *  @param [out] pOut        : Pointer to array of output buffer pointers
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
AUDIOLIB_STATUS AUDIOLIB_split_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);
/*!
 *  @brief This function is the natural C reference implementation of the
 *         split kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_split_exec.
 *
 * @details This implementation handles all format conversion combinations
 *          (interleaved/non-interleaved for both input and output) using
 *          simple nested loops. It serves as a reference for functional
 *          verification of the optimized C7x implementation.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to the input buffer
 *  @param [out] pOut        : Pointer to array of output buffer pointers
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_split_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the split kernel
 */
typedef struct {
   /*! @brief Function pointer to the selected execution variant between
    *         @ref AUDIOLIB_split_exec_cn and
    *         @ref AUDIOLIB_splitDeinterleaveToDeinterleave_exec_ci or
    *         @ref AUDIOLIB_split_exec_ci. */
   pFxnAUDIOLIB_split_exec execute;

   /*! @brief Number of output buffers to split into */
   uint32_t numOutputs;
   /*! @brief Total number of samples in the input buffer */
   uint32_t numInputSamples;
   /*! @brief Total number of channels in the input buffer */
   uint32_t numInputChannels;
   /*! @brief Total number of output channels per output buffer */
   uint32_t numOutputChannels;

   /*! @brief Number of vectorized loop iterations per output buffer.
    *         For interleaved input:
    *           ceilingDiv(numOutputChannels, eleCount) * numInputSamples
    *         For deinterleaved input:
    *           ceilingDiv(numInputSamples, eleCount) * numOutputChannels
    *         where eleCount is the number of elements per C7x vector. */
   uint32_t iterCount;
   /*! @brief Byte stride between consecutive rows of the input buffer */
   uint32_t strideIn;
   /*! @brief Byte stride between consecutive rows of each output buffer */
   uint32_t strideOut;
   /*! @brief Input data format: 1 for interleaved, 0 for deinterleaved */
   uint32_t isInputInterleave;
   /*! @brief Parameter block array storing SE/SA templates for C7x execution */
   uint8_t bufPblock[AUDIOLIB_SPLIT_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_split_PrivArgs;

#endif /* AUDIOLIB_SPLIT_IXX_IXX_OXX_PRIV_H_ */
