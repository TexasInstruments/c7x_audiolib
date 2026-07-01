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

#define MAX_SE_PARAMS (64)
#define SE_PARAM_BASE (0x0000)
// Single SE template streaming the whole contiguous input (deinterleaved / planar path)
#define SE_SE0_SINGLE_PARAM_OFFSET (SE_PARAM_BASE)
// One SE template per output (interleaved path: each output reads a channel window)
#define SE_SE0_PARAM_OFFSET (SE_SE0_SINGLE_PARAM_OFFSET + SE_PARAM_SIZE)
// One SA template per output (both paths)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + MAX_SE_PARAMS * SE_PARAM_SIZE)
// Per-output input element offsets (interleaved path; one uint32_t per output)
#define SE_INOFFSET_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + MAX_SE_PARAMS * SA_PARAM_SIZE)
// Per-output vector iteration counts (one uint32_t per output)
#define SE_ITERCOUNT_PARAM_OFFSET (SE_INOFFSET_PARAM_OFFSET + MAX_SE_PARAMS * sizeof(uint32_t))

#define AUDIOLIB_SPLIT_IXX_IXX_OXX_PBLOCK_SIZE                                                                         \
   (SE_PARAM_SIZE + MAX_SE_PARAMS * SE_PARAM_SIZE + MAX_SE_PARAMS * SA_PARAM_SIZE +                                    \
    2 * MAX_SE_PARAMS * sizeof(uint32_t))

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
 *  @brief Optimized C7x execution function that loads the whole input with a single
 *         streaming engine opened once.
 *
 * @details A single SE is opened once and streamed across all outputs; only the streaming
 *          address generator (SA) is opened per output. Used for the deinterleaved (planar)
 *          layout, and for the interleaved layout when every output has the same channel
 *          count (a 3D SE folds the output dimension via DIM2/ICNT2). Each output may have a
 *          different channel count in the planar case.
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
 *  @brief Optimized C7x execution function for the interleaved layout when outputs have
 *         different channel counts.
 *
 * @details Each output's channels are non-contiguous in the interleaved input, so one SE
 *          and one SA template are opened per output, reading a channel window of the input
 *          at the output's cumulative channel offset.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to the input buffer containing all channels
 *  @param [out] pOut        : Pointer to array of output buffer pointers
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 *               AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_splitPerOutput_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void **restrict pOut);
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
   /*! @brief Function pointer to the selected execution variant:
    *         @ref AUDIOLIB_split_exec_cn, @ref AUDIOLIB_split_exec_ci, or
    *         @ref AUDIOLIB_splitPerOutput_exec_ci. */
   pFxnAUDIOLIB_split_exec execute;

   /*! @brief Number of output buffers to split into */
   uint32_t numOutputs;
   /*! @brief Total number of samples per channel in the input buffer */
   uint32_t numInputSamples;
   /*! @brief Total number of channels in the input buffer (sum of outChannels) */
   uint32_t numInputChannels;
   /*! @brief Pointer to array of channel counts for each output buffer */
   uint32_t *outChannels;

   /*! @brief Stride in elements between consecutive rows of the input buffer */
   uint32_t strideIn;
   /*! @brief Pointer to array of strides in elements for each output buffer */
   uint32_t *strideOut;
   /*! @brief Input data format: 1 for interleaved, 0 for deinterleaved */
   uint32_t isInputInterleave;
   /*! @brief Flag: 1 if all outputs have the same channel count (enables the single-load
    *         fast path for the interleaved layout), 0 otherwise */
   uint8_t outChannelsUniform;
   /*! @brief Parameter block array storing SE/SA templates for C7x execution */
   uint8_t bufPblock[AUDIOLIB_SPLIT_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_split_PrivArgs;

#endif /* AUDIOLIB_SPLIT_IXX_IXX_OXX_PRIV_H_ */
