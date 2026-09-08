// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_panning_priv.h
*******************************************************************************
*/

#ifndef AUDIOLIB_PANNING_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_PANNING_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_panning.h"

/*!
 * \file
 * \brief Header file for kernel's internal use. For the kernel's interface,
 * please see @ref AUDIOLIB_panning
 */

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_panning_PrivArgs structure.
 *
 */
#define AUDIOLIB_PANNING_IXX_IXX_OXX_PBLOCK_SIZE (3 * SE_PARAM_SIZE + 4 * SA_PARAM_SIZE)

/*! @brief Byte offsets of each stream's template within the bufPblock array.
 *         @ref AUDIOLIB_panning_init_ci writes them and the exec variants read
 *         them back, so no exec call spends cycles deriving a configuration.
 *         SE0 streams the input and SA0 addresses the output.
 *         SA1: interleaved output, writes the running gains back; planar
 *         output, addresses the odd channel rows for the planar smoothing
 *         variant (the two layouts never share a handle).
 *         SE0_PAIR / SA0_PAIR: planar smoothing, which runs channels in pairs.
 *         SE0_PAIR reads the input ceil(numChannels / 2) times (once per pair,
 *         plus once for an odd last channel), SA0_PAIR writes the even channel
 *         rows plus that last channel, SA1 the odd rows.
 *         SE0_BYPASS / SA0_BYPASS: the bypass variant, same layout as SE0/SA0
 *         but over the exact vector count, since bypass is not unrolled and
 *         needs no padding to the smoothing loop's four-vector iteration. */
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SE0_PAIR_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA0_PAIR_PARAM_OFFSET (SE_SE0_PAIR_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE0_BYPASS_PARAM_OFFSET (SE_SA0_PAIR_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA0_BYPASS_PARAM_OFFSET (SE_SE0_BYPASS_PARAM_OFFSET + SE_PARAM_SIZE)

/*! @brief Pi, as a float, for the degree-to-radian conversions in this kernel. */
#define AUDIOLIB_PANNING_PI (3.14159265358979323846f)

/*! @brief An adjacent pair whose |det| falls below this is unusable: the two
 *         directions are antipodal, so no unique VBAP solution exists. */
#define AUDIOLIB_PANNING_DET_EPSILON (1e-6f)

/*! @brief Number of float lanes in one vector, and hence the length of the
 *         alphaCoeff table, which holds one smoothing coefficient per lane. */
#if __C7X_VEC_SIZE_BITS__ == 256
#define AUDIOLIB_PANNING_LANES_F32 8
#else
#define AUDIOLIB_PANNING_LANES_F32 16
#endif

/*! @brief Fixed cycles per outer iteration of the normal smoothing loops (row
 *         setup plus loop prolog and epilog), used by the cost models. */
#define AUDIOLIB_PANNING_SMOOTH_ROW_OVERHEAD (12U)

/*!
 *  @brief This is a function pointer type that conforms to the declaration of
 *         @ref AUDIOLIB_panning_exec_cn and of each of the C7x execution
 *         variants declared below.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_panning_exec)(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pIn,
                                                     void *restrict pGain,
                                                     void *restrict pOut);

/*!
 *  @brief Fetches one vector of target gains for the outer loop iteration @c i.
 *
 *  @details The two output layouts need different fetches, so the choice is made
 *           at compile time through the @c interleave template parameter and no
 *           branch reaches the loop. See the specializations below.
 */
template <bool interleave, typename vec, typename dataType> struct TargetGainFetcher;

/*! @brief Planar output: iteration @c i is one channel, so every lane of the
 *         vector is that channel's gain broadcast across the samples. */
template <typename vec, typename dataType> struct TargetGainFetcher<false, vec, dataType> {
   static inline vec get(const dataType *pGainLocal, size_t i) { return vec(pGainLocal[i]); }
};

/*! @brief Interleaved output: iteration @c i is one group of @c eleCount
 *         channels, so the vector is loaded straight from the gain buffer. */
template <typename vec, typename dataType> struct TargetGainFetcher<true, vec, dataType> {
   static inline vec get(const dataType *pGainLocal, size_t i)
   {
      uint32_t eleCount = c7x::element_count_of<vec>::value;
      return (*stov_ptr(vec, pGainLocal + (eleCount * i)));
   }
};

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_panning_init.
 *
 * @details This function determines the streaming engine (SE) and streaming
 *          address generator (SA) configuration from the buffer geometry recorded
 *          in the handle, and saves it in the bufPBlock array. The sample axis is
 *          padded to a whole number of smoothing-loop iterations (four vectors);
 *          DECDIM zero-fills and store-masks the padding. In the kernel
 *          call sequence, the execution variant chosen by
 *          @ref AUDIOLIB_panning_set is called later independently by the
 *          application. When that variant runs, it merely retrieves the
 *          configuration from the bufPBlock and uses it to set up the hardware
 *          resources. This arrangement is so that the exec path does not lose
 *          cycles to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of the per-channel gain buffer
 *  @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  The geometry is taken from the handle, which @ref AUDIOLIB_panning_init fills
 *  in first; the buffer parameters are kept for signature compatibility with the
 *  other AUDIOLIB kernels.
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_panning_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_panning_InitArgs *pKerInitArgs);

/*!
 *  @brief These are the execution functions for the C7x implementation of the
 *         kernel. Each declaration conforms to @ref pFxnAUDIOLIB_panning_exec,
 *         and @ref AUDIOLIB_panning_set selects the one matching the configured
 *         smoothing and interleave settings.
 *
 * @details Each function executes the C7x implementation of the panning kernel
 *          using the configuration stored in the bufPBlock array. The
 *          bufPBlock array is populated by the @ref AUDIOLIB_panning_init_ci
 *          function.
 *
 *          @c byPassSmoothing applies the target gains directly and serves both
 *          output layouts from one loop nest, selected at compile time by its
 *          @c isInterleave template parameter (see @ref TargetGainFetcher). The
 *          two @c smooth variants additionally advance the running gain towards
 *          the target, and are separate functions because the interleaved case
 *          must also write the per-channel gain state back through SA1.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  pIn          :  Pointer to the mono input audio data
 *  @param [in]  pGain        :  Pointer to the per-channel target gains
 *  @param [out] pOut         :  Pointer to the panned multichannel output
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType, bool isInterleave>
AUDIOLIB_STATUS AUDIOLIB_panning_byPassSmoothing_exec_ci(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pGain,
                                                         void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_panning_smooth_nonInterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                              void *restrict pIn,
                                                              void *restrict pGain,
                                                              void *restrict pOut);
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_panning_smooth_interleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                           void *restrict pIn,
                                                           void *restrict pGain,
                                                           void *restrict pOut);

/*!
 *  @brief This function is the execution function for the Natural C
 *         implementation of the panning kernel. The function declaration
 *         conforms to the declaration of @ref AUDIOLIB_panning_exec.
 *
 * @details This function executes the Natural C implementation of the panning kernel
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  pIn          :  Pointer to the mono input audio data
 *  @param [in]  pGain        :  Pointer to the per-channel target gains
 *  @param [out] pOut         :  Pointer to the panned multichannel output
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_panning_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut);

/*!
 * @brief One distinct speaker direction, together with the inverse matrix of the
 *        adjacent pair that begins at it.
 *
 * @details Channels sharing an azimuth are collapsed into a single node, so that
 *          a pair is never formed from two co-located speakers (which would make
 *          the matrix singular). Nodes and adjacent pairs correspond 1:1, because
 *          the last node pairs back round to the first. The inverse is
 *          precomputed so that @ref AUDIOLIB_panning_set solves the VBAP gains
 *          with two multiply-adds instead of a matrix inversion.
 */
typedef struct {
   /*! @brief Inverse of the [node i; node i+1] unit-vector matrix. Zeroed when
    *         that pair is degenerate, which routes @ref AUDIOLIB_panning_set to
    *         its nearest-node fallback instead. */
   float    invL[2][2];
   float    azimuth;      /**< This node's azimuth, in degrees */
   uint32_t firstSpeaker; /**< Index of this node's first entry in channelMap */
   uint32_t speakerCount; /**< Channels co-located at this azimuth, always >= 1 */
} AUDIOLIB_vbapNode_t;

/*!
 * @brief One entry of the azimuth-sorted list of directional speakers.
 *
 * @details Co-located speakers each keep their own entry, so that the gain of the
 *          node covering them can be fanned back out to every channel. Equal
 *          azimuths are adjacent after sorting, which is what lets a node address
 *          its channels as a contiguous slice.
 */
typedef struct {
   float    azimuth;     /**< Speaker azimuth in degrees, from @ref gChannelPositionMap */
   uint32_t configIndex; /**< Index of this speaker's channel in the caller's buffers */
} AUDIOLIB_channelConfig_t;

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Execution variant selected by @ref AUDIOLIB_panning_set, matching the
    *         configured function style, smoothing setting and output layout. */
   pFxnAUDIOLIB_panning_exec execute;
   int8_t                    funcStyle;

   int32_t  samples;           /**< Number of samples in the input buffer */
   uint32_t channels;          /**< Number of channels in the output buffer */
   int32_t  strideInElements;  /**< Stride in Y dimension for input*/
   int32_t  strideOutElements; /**< Stride in Y dimension for output */
   /*! @brief Set when the layout contains an LFE channel. Recorded for the benefit
    *         of callers inspecting the handle; panning itself excludes the LFE. */
   uint8_t isLFE;
   /*! @brief Number of directional (non-LFE) entries in @c channelMap, i.e. its
    *         valid extent. The nodes collectively span exactly this many entries. */
   uint32_t validChannels;
   uint8_t  isInterleave;         /**< 1 when the output is interleaved, 0 when planar */
   uint8_t  bypassSmoothing;      /**< Set when smoothingTime is 0 and target gains apply directly */
   float    smoothingCoefficient; /**< Per-sample smoothing factor alpha */
   float   *currentGain;          /**< Running gain per channel, carried across exec() calls */
   /*! @brief Powers of alpha across the lanes of one vector, so a whole vector of
    *         gains can be formed in one step. Holds alpha^1..alpha^N for planar
    *         output, where consecutive lanes are consecutive samples; for
    *         interleaved output every lane is a different channel at the same
    *         instant, so all entries are alpha. */
   float alphaCoeff[AUDIOLIB_PANNING_LANES_F32];
   /*! @brief alpha raised to the samples one vector covers (eleCount for planar
    *         output, 1 for interleaved), used to advance the smoothing state from
    *         one vector to the next. */
   float alphaMultiplier;
   /*! @brief alphaMultiplier^4, computed with one powf: the step of each decay
    *         chain per smoothing-loop iteration (four vectors). */
   float alphaMultiplierQuad;
   /*! @brief alphaMultiplier^2, computed with one powf: the step of each decay
    *         chain per iteration of the planar pair loop (two vectors per
    *         channel). */
   float alphaMultiplierPair;
   
   /*! @brief Outer loop trip count: channel groups for interleaved output,
    *         individual channels for planar output. */
   uint32_t numRowVec;
   /*! @brief Input vectors per outer iteration, padded by init_ci to a multiple
    *         of four (one smoothing-loop iteration). */
   uint32_t numColVec;
   /*! @brief Input vectors per outer iteration for the bypass variant: the exact
    *         count (samples for interleaved, ceil(samples / eleCount) for
    *         planar), with no padding. */
   uint32_t numColVecBypass;
   /*! @brief alpha raised to the frame length, used to jump the running gain
    *         directly to its end-of-frame value. */
   float                     alphaN;

   AUDIOLIB_channelConfig_t *channelMap; /**< Directional speakers, sorted by ascending azimuth */
   AUDIOLIB_vbapNode_t      *nodes;      /**< Distinct speaker directions and their pair inverses */
   uint32_t                  nodeCount;  /**< Number of distinct azimuths; <= validChannels */
   /*! @brief Copy of the arguments from the most recent successful
    *         @ref AUDIOLIB_panning_set, returned by @ref AUDIOLIB_panning_get. */
   AUDIOLIB_panning_SetArgs setArgs;
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_PANNING_IXX_IXX_OXX_PBLOCK_SIZE]; /*< Array to hold SE/SA template */
} AUDIOLIB_panning_PrivArgs;

#endif /* AUDIOLIB_PANNING_IXX_IXX_OXX_PRIV_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_panning_priv.h                                   */
/* ======================================================================== */
