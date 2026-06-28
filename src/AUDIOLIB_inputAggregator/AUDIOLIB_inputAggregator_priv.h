// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_INPUTAGGREGATOR_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_INPUTAGGREGATOR_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_inputAggregator.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_inputAggregator_PrivArgs structure.
 *
 */
#define MAX_SE_PARAMS (64)
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + MAX_SE_PARAMS * SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE)
// Add offset for the pOutOffset array
// This block will store [MAX_SE_PARAMS] elements of type uint32_t
#define SE_OUTOFFSET_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + MAX_SE_PARAMS * SA_PARAM_SIZE)

// Add offset for the totalVectorIterations array
// This block will also store [MAX_SE_PARAMS] elements of type uint32_t
#define SE_ITERCOUNT_PARAM_OFFSET (SE_OUTOFFSET_PARAM_OFFSET + MAX_SE_PARAMS * sizeof(uint32_t))

#define AUDIOLIB_INPUTAGGREGATOR_IXX_IXX_OXX_PBLOCK_SIZE                                                               \
   (MAX_SE_PARAMS * SE_PARAM_SIZE + SA_PARAM_SIZE + MAX_SE_PARAMS * SA_PARAM_SIZE +                                    \
    2 * MAX_SE_PARAMS * sizeof(uint32_t))

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_inputAggregator_exec_ci
 *         and @ref AUDIOLIB_inputAggregator_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_inputAggregator_exec)(AUDIOLIB_kernelHandle handle,
                                                             void **restrict pIn,
                                                             void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_inputAggregator_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_inputAggregator_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_inputAggregator_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_inputAggregator_exec_ci does not lose cycles
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
AUDIOLIB_STATUS AUDIOLIB_inputAggregator_init_ci(AUDIOLIB_kernelHandle                    handle,
                                                 const AUDIOLIB_bufParams2D_t            *bufParamsIn,
                                                 const AUDIOLIB_bufParams2D_t            *bufParamsOut,
                                                 const AUDIOLIB_inputAggregator_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the optimized C7x execution function for aggregating
 *         N non-interleaved input buffers into a single contiguous output buffer.
 *
 * @details This kernel aggregates multiple input audio buffers, each potentially
 *          having different channel counts, into a single unified output buffer.
 *          It is designed as a preprocessing stage for downstream kernels like
 *          TISP mixer, which requires all input channels in a single contiguous
 *          buffer. The function uses streaming engine (SE) to read from each
 *          input buffer and streaming address generator (SA) to write channels
 *          sequentially to the output buffer.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to array of N input buffer pointers
 *  @param [out] pOut        : Pointer to the aggregated output buffer containing
 *                             all channels from all inputs
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
AUDIOLIB_STATUS AUDIOLIB_inputAggregatorDeinterleaveToDeinterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                           void **restrict pIn,
                                                                           void *restrict pOut);

/*!
 *  @brief This function is the optimized C7x execution function for
 *         generic input/output format combinations (handles interleaved formats).
 *
 * @details This variant handles format conversions where at least one of input
 *          or output is interleaved. Uses streaming engine for reading and
 *          streaming address generator for writing with appropriate transpose
 *          operations for format conversion.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_inputAggregatorGeneric_exec_ci(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);
/*!
 *  @brief This function is the natural C reference implementation of the
 *         input aggregator kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_inputAggregator_exec.
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
extern AUDIOLIB_STATUS
AUDIOLIB_inputAggregator_exec_cn(AUDIOLIB_kernelHandle handle, void **restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the input aggregator kernel
 */
typedef struct {
   /*! @brief Function pointer to the selected execution variant between
    *         @ref AUDIOLIB_inputAggregator_exec_cn and
    *         @ref AUDIOLIB_inputAggregatorDeinterleaveToDeinterleave_exec_ci or
    *         @ref AUDIOLIB_inputAggregatorGeneric_exec_ci. */
   pFxnAUDIOLIB_inputAggregator_exec execute;

   /*! @brief Number of input buffers to aggregate */
   uint32_t numInputs;
   /*! @brief Number of vectors to be processed (used internally) */
   uint32_t nVecs;
   /*! @brief Flag: 1 if input data is in interleaved format, 0 if non-interleaved */
   uint8_t isInputInterleave;
   /*! @brief Flag: 1 if output data is in interleaved format, 0 if non-interleaved */
   uint8_t isOutputInterleave;
   /*! @brief Pointer to array of sample counts for each input buffer */
   uint32_t *numInputSamples;
   /*! @brief Pointer to array of channel counts for each input buffer */
   uint32_t *numInputChannels;
   /*! @brief Pointer to array of output channel offsets for writing each input's data */
   uint32_t *restrict pOutOffset;
   /*! @brief Pointer to array of vector iteration counts for each input buffer */
   uint32_t *restrict totalVectorIterations;
   /*! @brief Total number of samples in the output buffer */
   uint32_t numOutputSamples;
   /*! @brief Total number of channels in the output buffer (sum of all input channels) */
   uint32_t numOutputChannels;
   /*! @brief Stride in Y dimension in elements for input buffers */
   uint32_t *strideIn;
   /*! @brief Stride in Y dimension in elements for output buffer */
   uint32_t strideOut;
   /*! @brief Parameter block array storing SE/SA templates for C7x execution */
   uint8_t bufPblock[AUDIOLIB_INPUTAGGREGATOR_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_inputAggregator_PrivArgs;

#endif /* AUDIOLIB_INPUTAGGREGATOR_IXX_IXX_OXX_PRIV_H_ */
