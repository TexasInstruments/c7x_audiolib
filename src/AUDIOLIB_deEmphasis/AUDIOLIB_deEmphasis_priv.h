// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_deEmphasis_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_deEmphasis_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_deEmphasis.h"

/**
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_deEmphasis_PrivArgs structure.
 */
#define AUDIOLIB_DEEMPHASIS_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE)

#define SE_PARAM_BASE (0x0000)                                    // Base offset of the parameter block.
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)                       // SE0 template offset.
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE) // SE1 template offset.
#define SE_SA0_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE) // SA0 template offset.
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE) // SA1 template offset.

#define AUDIOLIB_DEEMPHASIS_MAX_NUM_CHANNELS (64)  // Channels the fixed filter state can hold.
#define AUDIOLIB_DEEMPHASIS_VEC_SIZE         (8)   // Float lanes per vector.
#if __C7X_VEC_SIZE_BITS__ == 256
#define AUDIOLIB_DEEMPHASIS_LANES_F32 8
#else
#define AUDIOLIB_DEEMPHASIS_LANES_F32 16
#endif
#define AUDIOLIB_DEEMPHASIS_BITS_PER_FLOAT   (32U) // Lane width, to turn lane shifts into bit shifts.
#define AUDIOLIB_DEEMPHASIS_INTERLEAVE_BLOCK (16U) // Samples per interleaved inner-loop iteration.
#define AUDIOLIB_DEEMPHASIS_PERM_ZERO        (0x40U) // VPERM control bits [7:6] = 01: output byte is zero.


/**
 *  @brief This is a function pointer type that conforms to the
 *         declaration of the AUDIOLIB_deEmphasis exec variants.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_deEmphasis_exec)(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pOut);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_deEmphasis_init.
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
AUDIOLIB_STATUS AUDIOLIB_deEmphasis_init_ci(AUDIOLIB_kernelHandle               handle,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                            const AUDIOLIB_deEmphasis_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel for interleaved data. The function
 *         declaration conforms to the declaration of @ref AUDIOLIB_deEmphasis_exec.
 *
 *  @param [in]  handle : Active handle to the kernel
 *  @param [in]  pIn    : Pointer to buffer holding the input data
 *  @param [out] pOut   : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_interleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel for non-interleaved data with two or
 *         more channels, filtered in pairs. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_deEmphasis_exec.
 *
 *  @tparam hasTailChannel  True for an odd channel count; the channel left over is filtered last.
 *  @param [in]  handle : Active handle to the kernel
 *  @param [in]  pIn    : Pointer to buffer holding the input data
 *  @param [out] pOut   : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType, bool hasTailChannel>
AUDIOLIB_STATUS
AUDIOLIB_deEmphasis_nonInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel for non-interleaved data with a single
 *         channel. The function declaration conforms to the declaration of
 *         @ref AUDIOLIB_deEmphasis_exec.
 *
 *  @param [in]  handle : Active handle to the kernel
 *  @param [in]  pIn    : Pointer to buffer holding the input data
 *  @param [out] pOut   : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_deEmphasis_nonInterleaveSingleChannel_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                       void *restrict pIn,
                                                                       void *restrict pOut);

/**
 *  @brief This function is the natural C implementation of the kernel. The
 *         function declaration conforms to the declaration of @ref AUDIOLIB_deEmphasis_exec.
 *
 *  @param [in]  handle : Active handle to the kernel
 *  @param [in]  pIn    : Pointer to buffer holding the input data
 *  @param [out] pOut   : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_deEmphasis_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   pFxnAUDIOLIB_deEmphasis_exec execute;

   int32_t  samples;      // Number of samples per channel
   uint32_t numChannels;  // Total number of channels
   uint8_t  isInterleave; // Interleave flag
   int32_t  strideIn;     // Stride Y of the input buffer measured in elements.
   int32_t  strideOut;    // Stride Y of output buffer (elements)
   float    alpha;        // De-emphasis feedback coefficient, range [0, 1]
   uint32_t ncols;        // Interleaved: samples. Non-interleaved: vector pairs per row.
   uint32_t nrows;        // Interleaved: channel groups. Non-interleaved: even part of the channel count.

   /** @brief Filter state y[-1] per channel, carried across exec() calls. */
   float yPrev[AUDIOLIB_DEEMPHASIS_MAX_NUM_CHANNELS] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));

   /** @brief Carry weights alpha^(i+1) for lane i, used after the non-interleaved in-vector scan. */
   float alphaPow[AUDIOLIB_DEEMPHASIS_VEC_SIZE] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));

   float alphaPow1;  // alpha^1, scan step 1 weight
   float alphaPow2;  // alpha^2, scan step 2 weight
   float alphaPow4;  // alpha^4, scan step 3 weight
   float alphaPow12; // alpha^12, interleaved carry weight to the end of the third sub-block
   float alphaPow16; // alpha^16, carry weight across a pair of vectors

   /** @brief VPERM control: delay by four lanes, zero-filling lanes 0..3. One .C op instead of two shifts. */
   uint8_t delay4Ctrl[AUDIOLIB_DEEMPHASIS_LANES_F32 * sizeof(float)] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));

   /** @brief VPERM control: broadcast the last lane to every lane. One .C op instead of VGETW + VDUPW. */
   uint8_t bcastLastCtrl[AUDIOLIB_DEEMPHASIS_LANES_F32 * sizeof(float)] __attribute__((aligned(AUDIOLIB_L2DATA_ALIGNMENT)));

   uint8_t bufPblock[AUDIOLIB_DEEMPHASIS_PBLOCK_SIZE]; // Parameter block buffer

} AUDIOLIB_deEmphasis_PrivArgs;

#endif /* AUDIOLIB_deEmphasis_IXX_IXX_OXX_PRIV_H_ */
