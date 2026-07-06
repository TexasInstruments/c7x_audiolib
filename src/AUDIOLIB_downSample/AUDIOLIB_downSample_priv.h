// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0


#ifndef AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_downSample.h"

/*!
 * \file
 * \brief Header file for kernel's internal use. For the kernel's interface,
 * please see @ref AUDIOLIB_downSample
 */

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_downSample_PrivArgs structure.
 *
 */
#define AUDIOLIB_NUM_STREAMING_ENGINES    (2)

/*! @brief Mask sub-block stored after the two SE/SA template pairs: up to three
 *         32-byte permute controls plus up to six 8-byte select masks (Ratio12
 *         needs the most permutes, Ratio7 the most selects). */
#define AUDIOLIB_DOWNSAMPLE_MASK_BYTES (3 * 32 + 6 * 8)

#define AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_PBLOCK_SIZE                                                                     \
   (2 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE + AUDIOLIB_DOWNSAMPLE_MASK_BYTES)

/*! @brief Byte offsets within bufPblock: the two SE/SA template pairs followed
 *         by the mask sub-block (three 32-byte permutes then six 8-byte selects). */
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_MASK_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + SE_PARAM_SIZE)
#define DS_MASK_PERMUTE_OFFSET(i) (SE_MASK_PARAM_OFFSET + (i) * 32)
#define DS_MASK_SEL_OFFSET(i) (SE_MASK_PARAM_OFFSET + 3 * 32 + (i) * 8)

/*!
 *  @brief This is a function pointer type that conforms to the declaration of
 *         the C7x execution variants (e.g. @ref AUDIOLIB_downSampleRatio2_exec_ci)
 *         and the natural-C variant @ref AUDIOLIB_downSample_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_downSample_exec)(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pIn,
                                                     void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_downSample_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          hardware resources based on the function call parameters, and the
 *          configuration is saved in the bufPblock array. The matching C7x
 *          execution variant (one of the AUDIOLIB_downSample*_exec_ci
 *          functions) is selected in @ref AUDIOLIB_downSample_init and stored in
 *          the handle. When the selected exec variant later runs, it merely
 *          retrieves the configuration from the bufPblock and uses it to set up
 *          the hardware resources, so it does not lose cycles determining the
 *          hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of ouput buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_downSample_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_downSample_InitArgs *pKerInitArgs);

/*!
 *  @brief C7x execution variants of the downsampler.
 *
 * @details The decimation work is split across several specialized exec
 *          functions; @ref AUDIOLIB_downSample_init selects one for the
 *          requested layout / ratio and stores it in the handle's @c execute
 *          pointer. Every variant keeps each downSampleRatio-th input sample
 *          (starting at @c phase) using the Streaming Engine / Streaming Address
 *          generator, but each uses the cheapest scheme for its case:
 *            - Ratio2 / Ratio2TwoCh      : hardware 2:1 decimation (dual stream)
 *            - Ratio4 / Ratio8 / Ratio12 : hardware 2:1 decimation (dual stream)
 *                                     plus select / permute to keep every
 *                                     4th / 8th / 12th sample (float ratios
 *                                     8 and 12)
 *            - Ratio3_6 / Ratio5 / Ratio7 : select + permute gather (Ratio3_6
 *                                     also serves float ratio 6)
 *            - Generic                 : straight stream-and-store (interleaved,
 *                                     deinterleave/transpose, ratio 1, and
 *                                     general ratios)
 *
 *          All variants share the signature below.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
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

/*! @brief C7x exec variant: decimation-by-2 (planar ratio 2, or the special
 *         interleaved cases that decimate by 2). Streams two input/output
 *         streams in parallel and stores both. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio2_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief C7x exec variant for the interleaved 2-channel float decimate-by-2
 *         case: fetches two channels at a time as a double-wide element and
 *         stores them back as float8. Instantiated for float only. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio2TwoCh_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief C7x exec variant for planar decimation-by-3 (and float decimation-by-6)
 *         using select/permute. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio3_6_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief C7x exec variant for planar decimation-by-5 using select/permute. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio5_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief C7x exec variant for planar decimation-by-7 using select/permute. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio7_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief C7x exec variant for planar float decimation-by-8 using hardware 2:1
 *         decimation + select/permute. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief C7x exec variant for planar float decimation-by-12 using hardware 2:1
 *         decimation + select/permute. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio12_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief C7x exec variant for planar decimation-by-4 using hardware 2:1
 *         decimation + permute. */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleRatio4_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*! @brief C7x exec variant for the straight stream-and-store cases: interleaved
 *         layout, deinterleave (transpose), and decimation ratios with no
 *         select/permute (e.g. ratio 1). */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_downSampleGeneric_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel for the non interleave data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_downSample_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_downSample_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel for the interleave data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_downSample_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_downSampleInterLeave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to the selected execution variant: one of the
    *         natural-C (@ref AUDIOLIB_downSample_exec_cn) or C7x
    *         (AUDIOLIB_downSample*_exec_ci) implementations.          */
   pFxnAUDIOLIB_downSample_exec execute;

   uint32_t nVecs;                    /**< Number of vectors processed by the exec loop */
   uint8_t  isInterleave;             /**< 1 = interleaved (samples x channels), 0 = planar (channels x samples) */
   uint32_t downSampleRatio;          /**< Decimation factor n (>= 1): keep every n-th sample */
   uint32_t phase;                    /**< Offset of the first kept sample, 0 <= phase < downSampleRatio */
   uint32_t dim_x;                    /**< Width of buffer in X dimension in elements. */
   uint32_t dim_y;                    /**< Height of buffer in Y dimension in elements. */
   uint32_t strideIn;                 /**< Stride in Y dimension in elements of input data. */
   uint32_t strideOut;                /**< Stride in Y dimension in elements of output data. */
   uint32_t startingIndex;            /**< Element offset into input for the first kept sample
                                        *   (planar: phase; interleaved: phase * strideIn). */
   uint32_t startingIndexSE1;         /**< Element offset into input for the second stream (SE1),
                                        *   used by every dual-stream variant (Ratio2/Ratio2TwoCh,
                                        *   Ratio3_6, Ratio4, Ratio5, Ratio7, Ratio8, Ratio12). */
   uint32_t outSa1;                   /**< Element offset into output for the second store stream (SA1). */
   uint32_t dataType;                 /**< Buffer element type (@ref AUDIOLIB_data_type_e) cached for perfEst. */
   uint8_t  usesRatio2Path;           /**< 1 when an interleaved input is routed to the dual-stream
                                        *   Ratio2/Ratio2TwoCh path instead of the normal interleaved
                                        *   Copy path (contiguous 1-channel, or 2-channel float, ratio-2). */
   /*! @brief bufPblock array holding the two SE/SA template pairs followed by the
    *         selected variant's permute/select masks
    *         ([perm0..2 : 32 B each][sel0..5 : 8 B each]).  init_ci writes the
    *         masks; each exec variant reads them back with an unaligned load, so
    *         the handle is not required to be vector-aligned. */
   uint8_t bufPblock[AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_downSample_PrivArgs;

#endif /* AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_PRIV_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_downSample_priv.h          */
/* ======================================================================== */
