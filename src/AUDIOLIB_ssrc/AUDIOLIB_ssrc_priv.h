// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SSRC_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_SSRC_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_ssrc.h"

#define ROUND(x) ((x) + 0.5) /* floor( (x) + 0.5) */

#ifdef C7X
/**
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_ssrc_PrivArgs structure.
 *
 */
#define AUDIOLIB_SSRC_PBLOCK_SIZE (7 * SE_PARAM_SIZE + 6 * SA_PARAM_SIZE)
#endif

/**
 * @brief Maximum supported ratio between input and output sample rates.
 *
 * SSRC only supports fixed integer ratios of 2:1 or 4:1 for both upsampling and downsampling.
 * This is different from ASRC (Asynchronous Sample Rate Conversion) which supports variable ratios.
 */
#define AUDIOLIB_SSRC_MAX_IO_SAMPLE_RATE_RATIO (4)

/**
 * @brief Number of taps in the original filter design for stage 1 of upsampling
 */
#define AUDIOLIB_SSRC_UPSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS (191U)
/**
 * @brief Calculated number of filter taps for stage 1 of upsampling
 *        Derived from stage 1 original filter taps using optimization formula
 */
#define AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS (AUDIOLIB_SSRC_UPSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS + 1U) / 4U
/**
 * @brief Calculated number of filter taps for stage 1 of upsampling with symmetry
 */
#define AUDIOLIB_SSRC_UPSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC                                                          \
   (AUDIOLIB_SSRC_UPSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS + 1U) / 2U
/**
 * @brief Number of taps in the original filter design for stage 2 of upsampling
 */
#define AUDIOLIB_SSRC_UPSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS (39U)
/**
 * @brief Calculated number of filter taps for stage 2 of upsampling
 *        Derived from stage 1 original filter taps using optimization formula
 */
#define AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS + 1U) / 4U
/**
 * @brief Calculated number of filter taps for stage 2 of upsampling with symmetry
 */
#define AUDIOLIB_SSRC_UPSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC                                                          \
   (AUDIOLIB_SSRC_UPSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS + 1U) / 2U
/**
 * @brief Number of taps in the original filter design for stage 1 of downsampling
 */
#define AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS (39U)
/**
 * @brief Calculated number of filter taps for stage 1 of downsampling
 *        Derived from stage 1 original filter taps using optimization formula
 */
#define AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS + 1U) / 4U
/**
 * @brief Calculated number of filter taps for stage 1 of downsampling with symmetry
 */
#define AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_FILTER_TAPS_SYMMETRIC                                                        \
   (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE1_ORIGINAL_FILTER_TAPS + 1U) / 2U

/**
 * @brief Number of taps in the original filter design for stage 2 of downsampling
 */
#define AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS (191U)
/**
 * @brief Calculated number of filter taps for stage 2 of downsampling
 *        Derived from stage 1 original filter taps using optimization formula
 *        Note: This appears to use stage 1 filter taps in calculation instead of stage 2
 */
#define AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS + 1U) / 4U
/**
 * @brief Calculated number of filter taps for stage 2 of downsampling with symmetry
 */
#define AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_FILTER_TAPS_SYMMETRIC                                                        \
   (AUDIOLIB_SSRC_DOWNSAMPLING_STAGE2_ORIGINAL_FILTER_TAPS + 1U) / 2U

/**
 * @brief Number of phases in the prototype filter
 */
#define AUDIOLIB_SSRC_NUMBER_OF_FILTER_PHASES (2U)
/* @brief ping pong factor used for double buffering */
#define AUDIOLIB_SSRC_PING_PONG_FACTOR (2U)
/* @brief Under or equal to this input sample count will run Single Sample Symm FIR for upsampling */
#define AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT (2U)
/* @brief Under or equal to this input sample count will run Single Sample Symm FIR for downsampling */
#define AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT (4U)
#ifdef C7X
/* C7000 minimum circular buffer size in bytes */
#define C7000_MIN_CIRCULAR_BUFFER_SIZE_B (512U)
#endif
/* Output block size in samples */
#define AUDIOLIB_SSRC_OUTPUT_SAMPLES_PER_BLOCK (4U)
/* Unroll factor for the upsample2x kernel's single sample Symm FIR implementation */
#define AUDIOLIB_SSRC_SS_UNROLL_FACTOR (4U)

#ifdef C7X
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE2_PARAM_OFFSET (SE_SE1_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE3_PARAM_OFFSET (SE_SE2_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE4_PARAM_OFFSET (SE_SE3_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE5_PARAM_OFFSET (SE_SE4_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SE6_PARAM_OFFSET (SE_SE5_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA0_PARAM_OFFSET (SE_SE6_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA2_PARAM_OFFSET (SE_SA1_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA3_PARAM_OFFSET (SE_SA2_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA4_PARAM_OFFSET (SE_SA3_PARAM_OFFSET + SA_PARAM_SIZE)
#define SE_SA5_PARAM_OFFSET (SE_SA4_PARAM_OFFSET + SA_PARAM_SIZE)
#endif

/**
 *  @brief Function pointer type for SSRC execution functions.
 *
 *  @details This type is used for pointers to all SSRC execution function variants,
 *           including C7x and natural C implementations, for both interleaved and
 *           non-interleaved data formats, and for upsampling and downsampling.
 *           The pointed-to function must match the signature:
 *           @code
 *           AUDIOLIB_STATUS func(AUDIOLIB_kernelHandle handle,
 *                                void *restrict pIn,
 *                                void *restrict pState,
 *                                void *restrict pFiltCoeffs,
 *                                void *restrict pOut);
 *           @endcode
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_ssrc_exec)(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pState,
                                                  void *restrict pFiltCoeffs,
                                                  void *restrict pOut);

/**
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /** @brief Structure holding initialization parameters
    */
   AUDIOLIB_ssrc_InitArgs initArgs;
   /** @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_ssrc_exec_cn_non_interleaved or @ref AUDIOLIB_ssrc_exec_ci_interleaved or
    *         @ref AUDIOLIB_ssrc_exec_ci_non_interleaved or @ref AUDIOLIB_ssrc_exec_ci_interleaved
    */
   pFxnAUDIOLIB_ssrc_exec execute;
   /** @brief Numper of output samples per channel generated for each sample rate conversion */
   int32_t outputSampleCount;
   /** @brief Total width of the input buffer in elements. Set by @ref AUDIOLIB_ssrc_init and used by exec
    * functions. Used in non-interleaved data format as the total width of one buffer out of the rows of
   buffers. In the Interleaved data format this is used to store the total with of the whole buffer. */
   uint32_t inBufferTotalDimX;
   /** @brief Total stride of the full input circular buffer. Initialized by init. Used in non-interleaved data format.
    */
   uint32_t inBufferTotalStrideY;
   /** @brief Address mask for circular buffer calculated based on user set inBuffer dim x and y */
   uint32_t cirBuffAddressMask;
   /** @brief Index of the second sample in history portion of the input buffer for stage 1. Initialized by
    * init and updated by exec. */
   uint32_t stage1BuffStartIndex;
   /** @brief Index of the second sample in history portion of the input buffer for stage 2. Used in
    * multi-stage processing. */
   uint32_t stage2BuffStartIndex;
   /** @brief Starting Index of the scratch buffer 1 used for up/down sampling */
   uint32_t scratch1BuffStartIndex;
   /** @brief Starting Index of the scratch buffer 2 used for up/down sampling */
   uint32_t scratch2BuffStartIndex;
   /** @brief Starting Index of the state buffer used for 2x up/down sampling */
   uint32_t stage1BuffCurrIndex;
   /** @brief Starting Index of the state buffer used for 4x up/down sampling */
   uint32_t stage2BuffCurrIndex;
   /** @brief Stage 1 filter coefficient offset. */
   uint32_t stage1FiltCoeffsOffset;
   /** @brief Stage 2 filter coefficient offset. */
   uint32_t stage2FiltCoeffsOffset;
   /** @brief Number of output blocks per output sample loop for stage 1 */
   int32_t numOutputBlocks1;
   /** @brief Number of output blocks per output sample loop for stage 2 */
   int32_t numOutputBlocks2;
   /** @brief Number of output blocks */
   int32_t numOutputBlocks3;
   /** @brief Number of output blocks */
   int32_t numOutputBlocks4;
   /** @brief Number of output blocks */
   int32_t numOutputBlocks5;
   /** @brief Number of channel blocks per channel loop */
   int32_t numChannelBlocks;
   /** @brief Flag indicating whether the conversion is upsampling (true) or downsampling (false) */
   uint8_t isUpsampling;
   /** @brief Sample rate ratio between input and output (2 or 4) */
   uint32_t sampleRateRatio;
   /** @brief Total state buffer size in bytes (for linear buffer format) */
   uint32_t stateSize;
   /** @brief Buffer parameters for state buffer */
   AUDIOLIB_bufParams2D_t bufParamsState;
#ifdef C7X
   /** @brief Buffer to save SE & SA configuration parameters */
   uint8_t bufPblock[AUDIOLIB_SSRC_PBLOCK_SIZE];
#endif
} AUDIOLIB_ssrc_PrivArgs;

/**
 *  @brief This function is the initialization function for the natural C
 *         implementation of the SSRC kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_ssrc_init.
 *
 * @details This function initializes the SSRC kernel for synchronous sample rate conversion
 *          with fixed integer ratios (2:1 or 4:1). It sets up internal parameters based on
 *          whether the operation is upsampling or downsampling and configures the appropriate
 *          execution function pointer.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of output buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 *                               parameters including sample rates, number of channels,
 *                               and buffer format
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
AUDIOLIB_STATUS AUDIOLIB_ssrc_init_cn(AUDIOLIB_kernelHandle   handle,
                                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the natural C implementation of the main execution function for
 *         2x upsampling in the SSRC kernel.
 *
 * @details This function performs 2x upsampling using a polyphase FIR filter approach.
 *          For each input sample, it produces two output samples. The first output sample
 *          is generated by applying the filter coefficients to the input samples, while
 *          the second output sample is the original input sample. This implementation uses
 *          symmetric FIR filtering to reduce computational complexity.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data (filter history)
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data (twice the size of input)
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_exec_cn(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pState,
                                                        void *restrict pFiltCoeffs,
                                                        void *restrict pOut);

/**
 *  @brief C7x implementation of the main execution function for 2x upsampling.
 *
 * @details This function selects between block and single sample processing based on input sample count and buffer
 * format. For interleaved data, deinterleave/interleave operations are used. For non-interleaved, block copy and FIR
 * filtering. The function handles circular buffer index updates and history block copy for continuity.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_exec_ci(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pState,
                                                        void *restrict pFiltCoeffs,
                                                        void *restrict pOut);

/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 2x upsampling using block processing.
 *
 * @details This function implements block processing for 2x upsampling using C7x streaming engines
 *          and vector operations. It processes multiple samples at once in blocks for improved
 *          performance. This implementation is used when the input sample count is greater than
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT.
 *
 *          The function uses symmetric FIR filtering with optimized memory access patterns:
 *          - Streaming engines fetch input samples from both forward and reverse directions
 *          - Filter coefficients are applied to pairs of input samples (symmetric taps)
 *          - For each input sample, two output samples are produced (2x upsampling)
 *          - The first output is the filtered sample, the second is the original input sample
 *
 *          @note The state buffer (pState) is not used in this function and could be NULL. Input data is accessed in
 * circular buffer mode.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Not used, could be NULL
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data (twice the size of input)
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_block_exec_ci(AUDIOLIB_kernelHandle handle,
                                                              void *restrict pIn,
                                                              void *restrict pState,
                                                              void *restrict pFiltCoeffs,
                                                              void *restrict pOut);

#ifdef C7X
/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 2x upsampling using single sample processing.
 *
 * @details This function implements single sample processing for 2x upsampling using streaming engines.
 *          It is used when the input sample count is less than or equal to
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT.
 *
 *          @note The state buffer (pState) is not used in this function and could be NULL. Input data is accessed in
 * circular buffer mode.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Not used, could be NULL
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients (can be NULL to use default
 * coefficients)
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_ss_exec_ci(AUDIOLIB_kernelHandle handle,
                                                           void *restrict pIn,
                                                           void *restrict pState,
                                                           void *restrict pFiltCoeffs,
                                                           void *restrict pOut);
#endif
/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 2x upsampling using linear buffer format.
 *
 * @details This function implements the processing for interleaved data in linear buffer mode.
 *          It transforms interleaved input to non-interleaved format using matTrans,
 *          processes each channel with FIR filtering, and transforms the output
 *          back to interleaved format.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                               void *restrict pIn,
                                                               void *restrict pState,
                                                               void *restrict pFiltCoeffs,
                                                               void *restrict pOut);

/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 2x upsampling using non-interleaved data format.
 *
 * @details This function implements the processing for non-interleaved data.
 *          It copies input data to the state buffer using blkCopy2D,
 *          processes each channel with FIR filtering, and writes
 *          the output directly to the output buffer. It also copies the last
 *          filter length samples to the beginning of the state buffer for the
 *          next processing block.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                        void *restrict pIn,
                                                                        void *restrict pState,
                                                                        void *restrict pFiltCoeffs,
                                                                        void *restrict pOut);

/**
 *  @brief This function is the natural C implementation of the main execution function for
 *         4x upsampling in the SSRC kernel.
 *
 * @details This function implements 4x upsampling by cascading two 2x upsampling stages:
 *          1. First stage: Performs 2x upsampling using stage1FiltCoeffs filter
 *             - Input: Original samples
 *             - Output: Intermediate buffer with 2x the number of samples
 *          2. Second stage: Performs 2x upsampling on the intermediate buffer using stage2FiltCoeffs filter
 *             - Input: Intermediate buffer from first stage
 *             - Output: Final buffer with 4x the number of samples
 *
 *          Each stage uses symmetric FIR filtering to reduce computational complexity.
 *          The filter coefficients are specifically designed for each stage to maintain
 *          audio quality while preventing aliasing. The two-stage approach allows for
 *          more efficient processing compared to a single-stage 4x upsampling.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data (filter history)
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding both stage1 and stage2 filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data (four times the size of input)
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_exec_cn(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pState,
                                                        void *restrict pFiltCoeffs,
                                                        void *restrict pOut);

/**
 *  @brief C7x implementation of the main execution function for 4x upsampling.
 *
 * @details This function selects between block and single sample processing based on input sample count and buffer
 * format. For interleaved data, deinterleave/interleave operations are used. For non-interleaved, block copy and FIR
 * filtering. The function manages separate buffers and filter coefficients for each stage and handles circular buffer
 * index updates.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding both stage1 and stage2 filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_exec_ci(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pState,
                                                        void *restrict pFiltCoeffs,
                                                        void *restrict pOut);

/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 4x upsampling using linear buffer format.
 *
 * @details This function implements the processing for interleaved data in linear buffer mode.
 *          It transforms interleaved input to non-interleaved format using matTrans,
 *          processes each channel with FIR filtering, and transforms the output
 *          back to interleaved format.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                               void *restrict pIn,
                                                               void *restrict pState,
                                                               void *restrict pFiltCoeffs,
                                                               void *restrict pOut);

/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 4x upsampling using circular buffer format.
 *
 * @details This function implements the processing for interleaved data in circular buffer mode.
 *          It performs symmetric FIR filtering in 2 stages and produces 4x upsampled output.
 *
 *          @note The state buffer (pState) is not used in this function and could be NULL. Input data is accessed in
 * circular buffer mode.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Not used, could be NULL
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_block_exec_ci(AUDIOLIB_kernelHandle handle,
                                                              void *restrict pIn,
                                                              void *restrict pState,
                                                              void *restrict pFiltCoeffs,
                                                              void *restrict pOut);

#ifdef C7X
/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 4x upsampling using single sample processing.
 *
 * @details This function implements single sample processing for 4x upsampling using streaming engines.
 *          It is used when the input sample count is less than or equal to
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT. The function processes each sample
 *          individually in two stages to achieve 4x upsampling.
 *
 *          @note The state buffer (pState) is not used in this function and could be NULL. Input data is accessed in
 * circular buffer mode.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Not used, could be NULL
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_ss_exec_ci(AUDIOLIB_kernelHandle handle,
                                                           void *restrict pIn,
                                                           void *restrict pState,
                                                           void *restrict pFiltCoeffs,
                                                           void *restrict pOut);
#endif
/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 4x upsampling using non-interleaved data format.
 *
 * @details This function implements the processing for non-interleaved data.
 *          It copies input data to the state buffer using blkCopy2D,
 *          processes each channel with two stage filtering, and writes
 *          the output directly to the output buffer.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                        void *restrict pIn,
                                                                        void *restrict pState,
                                                                        void *restrict pFiltCoeffs,
                                                                        void *restrict pOut);

/**
 *  @brief This function is the natural C implementation of the main execution function for
 *         2x downsampling in the SSRC kernel.
 *
 * @details This function performs 2x downsampling using a polyphase FIR filter approach.
 *          For every two input samples, it produces one output sample. The function applies
 *          an anti-aliasing filter to prevent aliasing artifacts that would otherwise occur
 *          during the downsampling process.
 *
 *          The implementation uses symmetric FIR filtering to reduce computational complexity:
 *          - For each output sample, it processes pairs of input samples (symmetric taps)
 *          - The filter is designed to have a cutoff frequency at half the Nyquist frequency
 *            of the output sample rate to prevent aliasing
 *          - The function maintains a history buffer of previous input samples to ensure
 *            proper filter operation across block boundaries
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data (filter history)
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data (half the size of input)
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_exec_cn(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pState,
                                                          void *restrict pFiltCoeffs,
                                                          void *restrict pOut);

/**
 *  @brief C7x implementation of the main execution function for 2x downsampling.
 *
 * @details This function selects between block and single sample processing based on input sample count and buffer
 * format. For interleaved data, deinterleave/interleave operations are used. For non-interleaved, block copy and FIR
 * filtering. The function applies anti-aliasing FIR filtering and manages circular buffer index updates and history
 * block copy.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_exec_ci(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pState,
                                                          void *restrict pFiltCoeffs,
                                                          void *restrict pOut);

/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 2x downsampling using linear buffer format.
 *
 * @details This function implements the processing for interleaved data in linear buffer mode.
 *          It transforms interleaved input to non-interleaved format using deinterleave,
 *          processes each channel with anti-aliasing FIR filtering and decimation,
 *          and transforms the output back to interleaved format.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                 void *restrict pIn,
                                                                 void *restrict pState,
                                                                 void *restrict pFiltCoeffs,
                                                                 void *restrict pOut);

/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 2x upsampling using block processing.
 *
 * @details This function implements block processing for 2x downsampling using C7x streaming engines
 *          and vector operations. It processes multiple samples at once in blocks for improved
 *          performance. This implementation is used when the input sample count is greater than
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *          The function uses symmetric FIR filtering with optimized memory access patterns:
 *          - Streaming engines fetch input samples from both forward and reverse directions
 *          - Filter coefficients are applied to pairs of input samples (symmetric taps)
 *          - For two input samples, one output sample is produced (2x downsampling)
 *
 *          @note The state buffer (pState) is not used in this function and could be NULL. Input data is accessed in
 * circular buffer mode.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Not used, could be NULL
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_block_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                void *restrict pIn,
                                                                void *restrict pState,
                                                                void *restrict pFiltCoeffs,
                                                                void *restrict pOut);

#ifdef C7X
/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 2x downsampling using single sample processing.
 *
 * @details This function implements single sample processing for 2x downsampling using streaming engines.
 *          It is used when the input sample count is less than or equal to
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *          @note The state buffer (pState) is not used in this function and could be NULL. Input data is accessed in
 * circular buffer mode.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Not used, could be NULL
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_ss_exec_ci(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn,
                                                             void *restrict pState,
                                                             void *restrict pFiltCoeffs,
                                                             void *restrict pOut);
#endif
/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 2x downsampling using non-interleaved data format.
 *
 * @details This function implements the processing for non-interleaved data.
 *          It copies input data to the state buffer using blkCopy2D,
 *          processes each channel with FIR filtering, and writes
 *          the output directly to the output buffer. It also copies the last
 *          filter length samples to the beginning of the state buffer for the
 *          next processing block.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                          void *restrict pIn,
                                                                          void *restrict pState,
                                                                          void *restrict pFiltCoeffs,
                                                                          void *restrict pOut);

/**
 *  @brief This function is the natural C implementation of the main execution function for
 *         4x downsampling in the SSRC kernel.
 *
 * @details This function implements 4x downsampling by cascading two 2x downsampling stages:
 *          1. First stage: Performs 2x downsampling using stage1FiltCoeffs filter
 *             - Input: Original samples
 *             - Output: Intermediate buffer with 1/2 the number of samples
 *          2. Second stage: Performs 2x downsampling on the intermediate buffer using stage2FiltCoeffs filter
 *             - Input: Intermediate buffer from first stage
 *             - Output: Final buffer with 1/4 the number of samples
 *
 *          Each stage applies an anti-aliasing filter before decimation to prevent aliasing artifacts.
 *          The two-stage approach allows for more efficient processing compared to a single-stage
 *          4x downsampling, as each filter can be optimized for its specific decimation factor.
 *
 *          The implementation uses symmetric FIR filtering to reduce computational complexity:
 *          - For each output sample, it processes pairs of input samples (symmetric taps)
 *          - The filters are designed with specific cutoff frequencies to prevent aliasing
 *          - The function maintains history buffers for both stages to ensure proper filter
 *            operation across block boundaries
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data (filter history)
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding both stage1 and stage2 filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data (one-fourth the size of input)
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_exec_cn(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pState,
                                                          void *restrict pFiltCoeffs,
                                                          void *restrict pOut);

/**
 *  @brief C7x implementation of the main execution function for 4x downsampling.
 *
 * @details This function selects between block and single sample processing based on input sample count and buffer
 * format. For interleaved data, deinterleave/interleave operations are used. For non-interleaved, block copy and FIR
 * filtering. The function applies anti-aliasing FIR filtering in both stages and manages separate buffers and filter
 * coefficients for each stage. Circular buffer index updates and history block copy are handled for continuity.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding both stage1 and stage2 filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_exec_ci(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pState,
                                                          void *restrict pFiltCoeffs,
                                                          void *restrict pOut);

/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 4x downsampling using linear buffer format.
 *
 * @details This function implements 4x downsampling by cascading two 2x downsampling stages,
 *          optimized for the C7x architecture. It leverages streaming engines, vector operations,
 *          and other C7x-specific optimizations for high performance.
 *
 *          The implementation follows these steps:
 *          1. First stage: Performs 2x downsampling using stage1FiltCoeffs filter
 *             - Input: Original samples from circular buffer
 *             - Output: Intermediate buffer with 1/2 the number of samples
 *          2. Second stage: Performs 2x downsampling on the intermediate buffer using stage2FiltCoeffs filter
 *             - Input: Intermediate buffer from first stage
 *             - Output: Final buffer with 1/4 the number of samples
 *
 *          Each stage uses FIR filtering with optimized memory access patterns:
 *          - Streaming engines fetch input samples efficiently from circular buffers
 *          - Vector operations process multiple samples and channels in parallel
 *          - Filter coefficients are applied to pairs of input samples (symmetric taps)
 *          - The function maintains separate history buffers for both stages to ensure
 *            proper filter operation across block boundaries
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Pointer to buffer holding the state data (used in linear buffer mode)
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding both stage1 and stage2 filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data (one-fourth the size of input)
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_linear_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                 void *restrict pIn,
                                                                 void *restrict pState,
                                                                 void *restrict pFiltCoeffs,
                                                                 void *restrict pOut);

/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 4x downsampling using circular buffer format and block processing.
 *
 * @details This function implements the processing for interleaved data in circular buffer mode.
 *          It performs symmetric FIR filtering in 2 stages and produces 4x downsampled output.
 *          It is used when the input sample count is greater than
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *          @note The state buffer (pState) is not used in this function and could be NULL. Input data is accessed in
 * circular buffer mode.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Not used, could be NULL
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_block_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                void *restrict pIn,
                                                                void *restrict pState,
                                                                void *restrict pFiltCoeffs,
                                                                void *restrict pOut);
#ifdef C7X
/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 4x downsampling using single sample processing.
 *
 * @details This function implements single sample processing for 4x downsampling using streaming engines.
 *          It is used when the input sample count is less than or equal to
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT. The function processes each sample
 *          individually in two stages to achieve 4x downsampling.
 *
 *          @note The state buffer (pState) is not used in this function and could be NULL. Input data is accessed in
 * circular buffer mode.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data in circular buffer format
 *  @param [in]  pState      : Not used, could be NULL
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_ss_exec_ci(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn,
                                                             void *restrict pState,
                                                             void *restrict pFiltCoeffs,
                                                             void *restrict pOut);
#endif
/**
 *  @brief This function is the specialized execution function for the C7x
 *         implementation of 4x downsampling using non-interleaved data format.
 *
 * @details This function implements the processing for non-interleaved data.
 *          It copies input data to the state buffer using blkCopy2D,
 *          processes each channel with 2 stage FIR filtering, and writes
 *          the output directly to the output buffer.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pState      : Pointer to buffer holding the state data
 *  @param [in]  pFiltCoeffs : Pointer to buffer holding the filter coefficients
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_non_interleaved_exec_ci(AUDIOLIB_kernelHandle handle,
                                                                          void *restrict pIn,
                                                                          void *restrict pState,
                                                                          void *restrict pFiltCoeffs,
                                                                          void *restrict pOut);

/**
 *  @brief This function is the main initialization function for the C7x
 *         implementation of 2x upsampling. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_ssrc_init.
 *
 * @details This function selects between block and single sample processing based on input sample count when the
 * bufferFormat is circular. If inputSampleCount > AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT, it uses block
 * processing. Otherwise, it uses single sample processing. If the buffer format is linear, it selects linear buffer
 * processing for either C7x or MMA based optimizations.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 2x upsampling using block processing.
 *
 * @details This function initializes the streaming engines and agents for block processing
 *          of 2x upsampling. It is used when the input sample count is greater than
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_block_init_ci(AUDIOLIB_kernelHandle   handle,
                                                              AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                              AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                              AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

#ifdef C7X
/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 2x upsampling using single sample processing.
 *
 * @details This function initializes the streaming engines and agents for single sample processing
 *          of 2x upsampling. It is used when the input sample count is less than or equal to
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_ss_init_ci(AUDIOLIB_kernelHandle   handle,
                                                           AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                           AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                           AUDIOLIB_ssrc_InitArgs *pKerInitArgs);
#endif
/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 2x upsampling using linear buffer format.
 *
 * @details This function initializes the matTrans and blkCopy2D handles for processing
 *          interleaved data in linear buffer mode. It sets up the necessary parameters
 *          for transforming interleaved data to non-interleaved format, processing each
 *          channel, and transforming back to interleaved format.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                               AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                               AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                               AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 2x upsampling using non-interleaved data format.
 *
 * @details This function initializes the FIR filter and blkCopy2D handles for processing
 *          non-interleaved data. It sets up the necessary parameters for copying input data
 *          to the state buffer, processing each channel with FIR filtering, and copying
 *          history data for the next processing block.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample2x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the main initialization function for the C7x
 *         implementation of 4x upsampling in the SSRC kernel.The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_ssrc_init.
 *
 * @details This function selects between block and single sample processing based on input sample count when the
 * bufferFormat is circular. If inputSampleCount > AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_UPSAMPLE_COUNT, it uses block
 * processing. Otherwise, it uses single sample processing. If the buffer format is linear, it selects linear buffer
 * processing for either C7x or MMA based optimizations.
 *
 *  @param [in]  handle       : Active handle to the kernel
 *  @param [in]  bufParamsIn  : Pointer to the structure containing dimensional
 *                              information of input buffer
 *  @param [out] bufParamsOut : Pointer to the structure containing dimensional
 *                              information of output buffer
 *  @param [in]  pKerInitArgs : Pointer to the structure holding init parameters
 *                              including sample rates, number of channels, and buffer format
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 4x upsampling using linear buffer format.
 *
 * @details This function initializes the streaming engines and agents for processing
 *          interleaved data in linear buffer mode for 4x upsampling. It sets up the
 *          necessary parameters for the two-stage upsampling process (2x followed by 2x).
 *
 *  @param [in]  handle       : Active handle to the kernel
 *  @param [in]  bufParamsIn  : Pointer to the structure containing dimensional
 *                              information of input buffer
 *  @param [out] bufParamsOut : Pointer to the structure containing dimensional
 *                              information of output buffer
 *  @param [in]  pKerInitArgs : Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                               AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                               AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                               AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 4x upsampling using block processing.
 *
 * @details This function initializes the streaming engines and agents for block processing
 *          of 4x upsampling. It is used when the input sample count is greater than
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_block_init_ci(AUDIOLIB_kernelHandle   handle,
                                                              AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                              AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                              AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

#ifdef C7X
/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 4x upsampling using single sample processing.
 *
 * @details This function initializes the streaming engines and agents for single sample processing
 *          of 4x upsampling. It is used when the input sample count is less than or equal to
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_ss_init_ci(AUDIOLIB_kernelHandle   handle,
                                                           AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                           AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                           AUDIOLIB_ssrc_InitArgs *pKerInitArgs);
#endif
/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 4x upsampling using non-interleaved data format.
 *
 * @details This function initializes the FIR filter and blkCopy2D handles for processing
 *          non-interleaved data. It sets up the necessary parameters for copying input data
 *          to the state buffer, processing each channel with two stage FIR filtering.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_upsample4x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                        AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                        AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief Main initialization function for the C7x implementation of 2x downsampling in the SSRC kernel.
 *
 *  @details
 *  Initializes the SSRC kernel for 2x downsampling using C7x-specific optimizations. This includes configuring
 *  streaming engines and agents for efficient data access, setting up buffer parameters, filter coefficient offsets,
 *  and selecting the appropriate execution function pointer for 2x downsampling. The function prepares all internal
 *  state and buffer structures required for high-performance, anti-aliasing FIR filtering and decimation.
 *
 *  @param [in]  handle       Active handle to the kernel
 *  @param [in]  bufParamsIn  Pointer to the structure containing dimensional information of input buffer
 *  @param [out] bufParamsOut Pointer to the structure containing dimensional information of output buffer
 *  @param [in]  pKerInitArgs Pointer to the structure holding initialization parameters (sample rates, channels,
 * format)
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 2x downsampling using linear buffer format.
 *
 * @details This function initializes the streaming engines and agents for processing
 *          interleaved data in linear buffer mode for 2x downsampling. It sets up the
 *          necessary parameters for the anti-aliasing filter and decimation process.
 *
 *  @param [in]  handle       : Active handle to the kernel
 *  @param [in]  bufParamsIn  : Pointer to the structure containing dimensional
 *                              information of input buffer
 *  @param [out] bufParamsOut : Pointer to the structure containing dimensional
 *                              information of output buffer
 *  @param [in]  pKerInitArgs : Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 2x downsampling using block processing.
 *
 * @details This function initializes the streaming engines and agents for block processing
 *          of 2x downsampling. It is used when the input sample count is greater than
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_block_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

#ifdef C7X
/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 2x downsampling using single sample processing.
 *
 * @details This function initializes the streaming engines and agents for single sample processing
 *          of 2x downsampling. It is used when the input sample count is less than or equal to
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_ss_init_ci(AUDIOLIB_kernelHandle   handle,
                                                             AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                             AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                             AUDIOLIB_ssrc_InitArgs *pKerInitArgs);
#endif
/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 2x downsampling using non-interleaved data format.
 *
 * @details This function initializes the FIR filter and blkCopy2D handles for processing
 *          non-interleaved data. It sets up the necessary parameters for copying input data
 *          to the state buffer, processing each channel with FIR decimation filtering, and copying
 *          history data for the next processing block.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample2x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief Main initialization function for the C7x implementation of 4x downsampling in the SSRC kernel.
 *
 *  @details
 *  Initializes the SSRC kernel for 4x downsampling using C7x-specific optimizations. This includes configuring
 *  streaming engines and agents for a two-stage (2x followed by 2x) downsampling process, setting up buffer
 *  parameters, filter coefficient offsets for both stages, and selecting the appropriate execution function pointer.
 *  The function prepares all internal state and buffer structures required for efficient, cascaded anti-aliasing
 *  FIR filtering and decimation, supporting high-performance multi-channel audio processing.
 *
 *  @param [in]  handle       Active handle to the kernel
 *  @param [in]  bufParamsIn  Pointer to the structure containing dimensional information of input buffer
 *  @param [out] bufParamsOut Pointer to the structure containing dimensional information of output buffer
 *  @param [in]  pKerInitArgs Pointer to the structure holding initialization parameters (sample rates, channels,
 * format)
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_init_ci(AUDIOLIB_kernelHandle   handle,
                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 4x downsampling in the SSRC kernel.
 *
 * @details This function initializes the SSRC kernel for 4x downsampling using C7x-specific
 *          optimizations. It configures the streaming engines and agents for the two-stage
 *          downsampling process (2x followed by 2x) and sets up the necessary buffer parameters
 *          and filter coefficient offsets.
 *
 *          The function performs the following setup:
 *          - Configures streaming engines for efficient data access from circular buffers
 *          - Sets up filter coefficient offsets for both stages of anti-aliasing filters
 *          - Initializes buffer indices for the two-stage processing
 *          - Configures the execution function pointer to the optimized C7x implementation
 *          - Sets up parameters for vector operations to process multiple samples and channels
 *
 *  @param [in]  handle       : Active handle to the kernel
 *  @param [in]  bufParamsIn  : Pointer to the structure containing dimensional
 *                              information of input buffer
 *  @param [out] bufParamsOut : Pointer to the structure containing dimensional
 *                              information of output buffer
 *  @param [in]  pKerInitArgs : Pointer to the structure holding init parameters
 *                              including sample rates, number of channels, and buffer format
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_linear_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                 AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                 AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 4x downsampling using block processing.
 *
 * @details This function initializes the streaming engines and agents for block processing
 *          of 4x downsampling. It is used when the input sample count is greater than
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_block_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

#ifdef C7X
/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 4x downsampling using single sample processing.
 *
 * @details This function initializes the streaming engines and agents for single sample processing
 *          of 4x downsampling. It is used when the input sample count is less than or equal to
 *          AUDIOLIB_SSRC_BLOCK_TO_SS_SWITCH_DOWNSAMPLE_COUNT.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_ss_init_ci(AUDIOLIB_kernelHandle   handle,
                                                             AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                             AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                             AUDIOLIB_ssrc_InitArgs *pKerInitArgs);
#endif

/**
 *  @brief This function is the specialized initialization function for the C7x
 *         implementation of 4x downsampling using non-interleaved data format.
 *
 * @details This function initializes the FIR filter and blkCopy2D handles for processing
 *          non-interleaved data. It sets up the necessary parameters for copying input data
 *          to the state buffer, processing each channel with two stage FIR decimation filtering, and copying
 *          history data for the next processing block.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_ssrc_downsample4x_non_interleaved_init_ci(AUDIOLIB_kernelHandle   handle,
                                                                          AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                                          AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                                          AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

#endif /* AUDIOLIB_SSRC_IXX_IXX_OXX_PRIV_H_ */
