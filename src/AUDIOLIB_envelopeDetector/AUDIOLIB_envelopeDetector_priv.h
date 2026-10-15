// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ENVELOPEDETECTOR_PRIV_H_
#define AUDIOLIB_ENVELOPEDETECTOR_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_envelopeDetector.h"

// SE0/SA0 templates for the full channel groups and SE1/SA1 templates for the leftover channels.
#define AUDIOLIB_ENVELOPEDETECTOR_IXX_IXX_OXX_PBLOCK_SIZE (4 * SE_PARAM_SIZE)
#define AUDIOLIB_ENVELOPEDETECTOR_MAX_NUM_CHANNELS (64) // Maximum number of audio channels supported by the kernel.

/** @brief Channels updated together by one sample loop of each optimized variant */
#define AUDIOLIB_ENVELOPEDETECTOR_GROUP4_CHANNELS (4U)
#define AUDIOLIB_ENVELOPEDETECTOR_GROUP8_CHANNELS (8U)

/*
 * Cycle models of the two optimized variants.
 *
 * CALL_CYCLES      : entry and exit blocks
 * CYCLES_PER_SAMPLE: ii of the sample loop, which updates one full group of channels
 * GROUP_CYCLES     : per full channel group, outside its ii * numSamples: envelope state loads and
 *                    stores, pipeline prolog and epilog, group loop control, and one extra kernel
 *                    iteration where the collapsed prolog and epilog stages outnumber the pipeline
 *                    stages (single precision group of eight, double precision group of four)
 * LEFTOVER_CYCLES  : the same for the leftover group
 */
#define AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP4_CALL_CYCLES (29)       // FP32 group-of-4: entry + exit block cycles
#define AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP4_CYCLES_PER_SAMPLE (10) // FP32 group-of-4: sample loop ii
#define AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP4_GROUP_CYCLES (27)      // FP32 group-of-4: per-full-group overhead
#define AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP4_LEFTOVER_CYCLES (20)   // FP32 group-of-4: leftover-group overhead

#define AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP4_CALL_CYCLES (36)       // FP64 group-of-4: entry + exit block cycles
#define AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP4_CYCLES_PER_SAMPLE (10) // FP64 group-of-4: sample loop ii
#define AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP4_GROUP_CYCLES (39)      // FP64 group-of-4: per-full-group overhead
#define AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP4_LEFTOVER_CYCLES (27)   // FP64 group-of-4: leftover-group overhead

#define AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP8_CALL_CYCLES (38)       // FP32 group-of-8: entry + exit block cycles
#define AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP8_CYCLES_PER_SAMPLE (11) // FP32 group-of-8: sample loop ii
#define AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP8_GROUP_CYCLES (55)      // FP32 group-of-8: per-full-group overhead
#define AUDIOLIB_ENVELOPEDETECTOR_SP_GROUP8_LEFTOVER_CYCLES (36)   // FP32 group-of-8: leftover-group overhead

#define AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP8_CALL_CYCLES (39)       // FP64 group-of-8: entry + exit block cycles
#define AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP8_CYCLES_PER_SAMPLE (13) // FP64 group-of-8: sample loop ii
#define AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP8_GROUP_CYCLES (46)      // FP64 group-of-8: per-full-group overhead
#define AUDIOLIB_ENVELOPEDETECTOR_DP_GROUP8_LEFTOVER_CYCLES (29)   // FP64 group-of-8: leftover-group overhead

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_envelopeDetector_group4_exec_ci,
 *         @ref AUDIOLIB_envelopeDetector_group8_exec_ci and
 *         @ref AUDIOLIB_envelopeDetector_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_envelopeDetector_exec)(AUDIOLIB_kernelHandle handle,
                                                              void *restrict pIn,
                                                              void *restrict pOut);

/*!
 *  @brief This function is the init function for the C intrinsic
 *         implementation of the kernel. It models the cycles of both optimized
 *         variants, keeps the cheaper one, writes its stream templates and sets
 *         the execute function pointer.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  bufParamsIn : Input buffer parameters
 *  @param [in]  bufParamsOut: Output buffer parameters
 *  @param [in]  pKerInitArgs: Kernel initialization arguments
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_group8_init_ci(AUDIOLIB_kernelHandle                     handle,
                                                         const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                                         const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_group4_init_ci(AUDIOLIB_kernelHandle                     handle,
                                                         const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                                         const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

/*!
 *  @brief Execution function of the group-of-four optimized variant: one pass, four channels
 *         per sample loop.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group4_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief Execution function of the group-of-eight optimized variant: an envelope pass with
 *         eight channels per sample loop.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_group8_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This function is called to calculate the estimate cycles of @ref
 * AUDIOLIB_envelopeDetector_group4_exec_ci..
 *
 *  @param [in]  dataType         :  Datatype of Input/Output.
 *  @param [in]  numSamples       :  Number of input samples.
 *  @param [in]  numChannels      :  Number of input channels.
 *
 *  @return      Estimated cycles (uint64_t).
 *
 *  @remarks     None
 */
uint64_t AUDIOLIB_envelopeDetector_group4Cycles(uint32_t dataType, uint32_t numSamples, uint32_t numChannels);

/**
 *  @brief        This function is called to calculate the estimate cycles of @ref
 * AUDIOLIB_envelopeDetector_group8_exec_ci..
 *
 *  @param [in]  dataType         :  Datatype of Input/Output.
 *  @param [in]  numSamples       :  Number of input samples.
 *  @param [in]  numChannels      :  Number of input channels.
 *
 *  @return      Estimated cycles (uint64_t).
 *
 *  @remarks     None
 */
uint64_t AUDIOLIB_envelopeDetector_group8Cycles(uint32_t dataType, uint32_t numSamples, uint32_t numChannels);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_envelopeDetector_exec_cn and
    *         the two optimized variants.
    */
   pFxnAUDIOLIB_envelopeDetector_exec execute;

   AUDIOLIB_data_type_e dataType;          /**< \brief Data type (AUDIOLIB_FLOAT32 or AUDIOLIB_FLOAT64) */
   uint32_t             numSamples;        /**< \brief Number of samples per channel */
   uint32_t             numChannels;       /**< \brief Number of channels */
   int32_t              strideInElements;  /**< \brief Input row pitch in elements */
   int32_t              strideOutElements; /**< \brief Output row pitch in elements */

   /* Channel split used by the optimized variants: numChannelGroups full groups of
    * channelsPerGroup, then the leftover channels processed as one more full group */
   uint32_t channelsPerGroup;     /**< \brief Channels per sample loop of the chosen variant, 4 or 8 */
   uint32_t numChannelGroups;     /**< \brief Full groups before the leftover channels */
   uint32_t leftoverFirstChannel; /**< \brief First leftover channel */
   uint32_t leftoverRowStep1;     /**< \brief Rows between leftover loop channels k and k + 1, k even */
   uint32_t leftoverRowStep2;     /**< \brief Rows between leftover loop channels k and k + 2, k = 0, 1, 4, 5 */
   uint32_t leftoverRowStep3;     /**< \brief Rows between leftover loop channels k and k + 4, k = 0..3 */

   AUDIOLIB_envelopeDetector_SetArgs setArgs; /**< \brief A copy of the last-set runtime parameters */
   float samplingRate; /**< \brief Sampling rate in Hz */

   float attackCoefficient;  /**< \brief Attack coefficient */
   float releaseCoefficient; /**< \brief Release coefficient */
   /** \brief Envelope state of each channel, carried across exec() calls */
   float envelopeStates[AUDIOLIB_ENVELOPEDETECTOR_MAX_NUM_CHANNELS];

   /*! @brief bufPblock array to store SE/SA template (C7x) */
   uint8_t bufPblock[AUDIOLIB_ENVELOPEDETECTOR_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_envelopeDetector_PrivArgs;

#endif /* AUDIOLIB_ENVELOPEDETECTOR_PRIV_H_ */
