// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_preEmphasis_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_preEmphasis_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_preEmphasis.h"

/**
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_preEmphasis_PrivArgs structure.
 */
#define AUDIOLIB_PREEMPHASIS_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE)

#define SE_PARAM_BASE (0x0000)                                    // Base offset of the parameter block.
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)                       // SE0 template offset.
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE) // SE1 template offset.
#define SE_SA0_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE) // SA0 template offset.
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE) // SA1 template offset.

#define AUDIOLIB_PREEMPHASIS_MAX_NUM_CHANNELS             (64)  // Channels the fixed filter state can hold.
#if __C7X_VEC_SIZE_BITS__ == 256
#define AUDIOLIB_PREEMPHASIS_LANES_F32 8
#else
#define AUDIOLIB_PREEMPHASIS_LANES_F32 16
#endif
#define AUDIOLIB_PREEMPHASIS_NONINTERLEAVE_HISTORY_OFFSET   (AUDIOLIB_PREEMPHASIS_LANES_F32 - 1)  // SE1 start in each row.
#define AUDIOLIB_PREEMPHASIS_BITS_PER_FLOAT (32U)



/**
 *  @brief This is a function pointer type that conforms to the
 *         declaration of the AUDIOLIB_preEmphasis exec variants.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_preEmphasis_exec)(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pOut);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_preEmphasis_init.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [in]  bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of output buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_preEmphasis_init_ci(AUDIOLIB_kernelHandle                handle,
                                             const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                             const AUDIOLIB_bufParams2D_t        *bufParamsOut,
                                             const AUDIOLIB_preEmphasis_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel for interleaved data. The function
 *         declaration conforms to the declaration of @ref AUDIOLIB_preEmphasis_exec.
 *
 *  @param [in]  handle : Active handle to the kernel
 *  @param [in]  pIn    : Pointer to buffer holding the input data
 *  @param [out] pOut   : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_interleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel for non-interleaved rows longer than one
 *         vector. The function declaration conforms to the declaration of
 *         @ref AUDIOLIB_preEmphasis_exec.
 *
 *  @param [in]  handle : Active handle to the kernel
 *  @param [in]  pIn    : Pointer to buffer holding the input data
 *  @param [out] pOut   : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_nonInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel for non-interleaved rows of at most one
 *         vector. The function declaration conforms to the declaration of
 *         @ref AUDIOLIB_preEmphasis_exec.
 *
 *  @param [in]  handle : Active handle to the kernel
 *  @param [in]  pIn    : Pointer to buffer holding the input data
 *  @param [out] pOut   : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_preEmphasis_nonInterleaveSingleVec_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief This function is the natural C implementation of the kernel. The
 *         function declaration conforms to the declaration of @ref AUDIOLIB_preEmphasis_exec.
 *
 *  @param [in]  handle : Active handle to the kernel
 *  @param [in]  pIn    : Pointer to buffer holding the input data
 *  @param [out] pOut   : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_preEmphasis_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   pFxnAUDIOLIB_preEmphasis_exec execute;

   int32_t  samples;      // Number of samples per channel
   uint32_t numChannels;  // Total number of channels
   uint8_t  isInterleave; // Interleave flag
   int32_t  strideIn;     // Stride Y of the input buffer measured in elements.
   int32_t  strideOut;    // Stride Y of output buffer (elements)
   float    alpha;        // Pre-emphasis coefficient, range [0, 1]
   uint32_t ncols;        // Interleaved: samples. Non-interleaved: vectors per row.
   uint32_t nrows;        // Interleaved: channel groups. Non-interleaved: channels.
   float    xPrev[AUDIOLIB_PREEMPHASIS_MAX_NUM_CHANNELS]; // Fixed buffer for filter state
   uint8_t  bufPblock[AUDIOLIB_PREEMPHASIS_PBLOCK_SIZE]; // Parameter block buffer

} AUDIOLIB_preEmphasis_PrivArgs;

#endif /* AUDIOLIB_preEmphasis_IXX_IXX_OXX_PRIV_H_ */
