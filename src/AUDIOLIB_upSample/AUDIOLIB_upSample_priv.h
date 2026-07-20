// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/********************************************************************************
 *  @file     AUDIOLIB_upSample_priv.h
 *  @brief    Private parameters of function AUDIOLIB_upSample
 *  @version  0.1 - 2025 : Initial Version
 ********************************************************************************/

#ifndef AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_upSample.h"
/*******************************************************************************
 *
 * DEFINES
 *
 ******************************************************************************/

#define SE_PARAM_BASE (0x0000)
/* SE: input read stream (input -> registers). */
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
/* SA: primary store (output for the interleaved / per-upFactor paths, or the
 *     whole-output zero-fill for the generic planar path). */
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
/* SA: sample-placement store used by the generic planar path (samples -> output). */
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA2_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + SE_PARAM_SIZE)

/*!
 * @brief Size of the bufPblock scratch. The generic planar path is the largest
 *        user and stores up to four templates: the input-read SE (SE0), the
 *        zero-fill SA (SA0), the sample-placement SA (SA1) and -- when phase > 0 --
 *        the phase-fill SA (SA2). The natural-C path does not use it.
 */
#define AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_PBLOCK_SIZE (4 * SE_PARAM_SIZE)

/*!
 *  @brief Function pointer type matching @ref AUDIOLIB_upSample_exec_cn /
 *         @ref AUDIOLIB_upSampleNonInterleave_exec_ci.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_upSample_exec)(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pOut);

/*!
 *  @brief Natural-C reference execution variant (zero-insertion).
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_upSample_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief Optimized C7x initialization variant. Builds the SE/SA templates for
 *         the execution variant selected in @ref AUDIOLIB_upSample_init and
 *         stashes them in the handle's bufPblock. Handles the interleaved
 *         layout, the per-upFactor planar specializations, and the generic
 *         planar fallback.
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_upSample_init_ci(AUDIOLIB_kernelHandle             handle,
                                                 const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                                 const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                                 const AUDIOLIB_upSample_InitArgs *pKerInitArgs);

/*!
 *  @brief Generic planar C7x execution variant (zero-insertion). Fallback used
 *         for any upFactor that does not have a dedicated specialization below:
 *         zero-fills the output, then streams each input sample into place.
 */
template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_upSampleNonInterleave_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief Per-upFactor C7x execution variants (zero-insertion). Each builds one
 *         output group (upFactor slots) per input sample with permute/select
 *         masks from @ref AUDIOLIB_upSample_masks.h, avoiding a separate
 *         zero-fill pass. @ref AUDIOLIB_upSample_init picks the variant that
 *         matches the configured upFactor and data type.
 */
template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample2_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample3_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample4_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample5_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample6_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample6x2_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample7_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample8X_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief Interleaved-layout C7x execution variant (zero-insertion). Writes the
 *         leading phase zeros, then streams each input row into output row
 *         phase + k*upFactor; the intervening rows are left as zeros.
 */
template <bool HasPhase, typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_upSample_execInterleave_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure reserved for internal use by the kernel (the "handle").
 */

typedef struct {
   /*! @brief Selected execution variant (@ref AUDIOLIB_upSample_exec_cn / _ci). */
   pFxnAUDIOLIB_upSample_exec execute;

   uint32_t dimXIn;            /**< input buffer width  (elements): planar=numSamples, interleaved=numChannels */
   uint32_t dimYIn;            /**< input buffer height (rows):     planar=numChannels, interleaved=numSamples */
   uint32_t upFactor;          /**< upsampling factor n (>= 1)                                                 */
   uint32_t phase;             /**< sample offset, 0 <= phase < upFactor                                       */
   uint32_t isInterleave;      /**< 0 = planar, 1 = interleaved                                                */
   int32_t  strideIn;          /**< input  row stride in ELEMENTS                                              */
   int32_t  strideOut;         /**< output row stride in ELEMENTS                                              */
   uint32_t numZeroFillVecs;   /**< vectors in the generic planar zero-fill pass (whole output)           */
   uint32_t phaseZeroFillVecs; /**< leading elements to zero before the first sample (the phase region)   */
   uint32_t nVecs;    /**< vectors processed by the selected exec variant's main loop            */
   uint32_t dataType; /**< element type: AUDIOLIB_FLOAT32 / AUDIOLIB_FLOAT64                      */
   /*! @brief Scratch holding the SE/SA templates built by @ref AUDIOLIB_upSample_init_ci. */
   uint8_t  bufPblock[AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_PBLOCK_SIZE];
   uint32_t useNonInterleave; /**< 1 = single-channel interleaved handled as planar; 0 otherwise         */
   int32_t  phaseOffset;      /**< element offset of the first kept sample (phase, in output elements)   */
   uint8_t  genericNonInterleaveCase; /**< 1 = generic planar fallback (zero-fill + placement) is used   */

} AUDIOLIB_upSample_PrivArgs;

#endif /* AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_PRIV_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample_priv.h                                   */
/* ======================================================================== */
