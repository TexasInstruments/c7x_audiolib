// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SSRC_H_
#define AUDIOLIB_SSRC_H_

#include "../common/AUDIOLIB_types.h"
#ifdef C7X
#include <dsplib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_ssrc AUDIOLIB_ssrc
 * @brief **Synchronous Sample Rate Conversion (SSRC)**
 *
 * @details
 * The SSRC kernel performs synchronous sample rate conversion of discrete input signals, converting audio data from
 * one sample rate to another using fixed integer ratios. It is designed for high-performance, multi-channel audio
 processing and supports both interleaved and non-interleaved data formats.
 *
 * **Key Features:**
 * - Converts input audio streams from an input sample rate to a different output sample rate using fixed ratios (2:1,
 4:1 or 1:2, 1:4).
 * - Supports both channel interleaved and non-interleaved input data formats.
 * - Handles multiple audio channels in a single kernel call.
 * @if C7X
 * - Supports ping-pong buffer management in circular buffer mode @ref AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR
 *   or linear buffer management when buffer format is @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR.
 * @else
 * @if ARM_A53
 * - Supports linear buffer management @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR.
 * @endif
 * @endif
 * - Provides utility functions to calculate buffer sizes for input, state, coefficients and output data.
 * - Implements efficient half band polyphase filtering for high-quality sample rate conversion.
 * @if C7X
 * - Supports the use of MMA (Matrix Multiply Accelerator) unit for high-performance processing. Note:- Performance of
 the MMA unit
 *   can only be seen for larger input sample counts (block sizes) and multiple channels in the @ref
 AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR buffer format.
 * - For smaller input sample counts (block sizes) and channels counts @ref
 AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR buffer format provides the best performance.
 * @endif
 * - Filters used in SSRC are designed for noise components to be lower than -140 dB.
 *
 * **Algorithm Overview:**
 * 1. **Initialization:**
 *    The kernel is initialized with user-specified parameters such as input/output sample rates, number of channels,
 * @if C7X
 * enableMMA, data format and buffer format using @ref AUDIOLIB_ssrc_InitArgs.
 * @else
 * @if ARM_A53
 * data format and buffer format using @ref AUDIOLIB_ssrc_InitArgs.
 * @endif
 * @endif
 *
 * 2. **Buffer Preparation:**
 *    The application allocates buffers based on the calculated sizes using
 * utility functions like @ref AUDIOLIB_ssrc_getOutBufferLength, @ref AUDIOLIB_ssrc_getFilterLength,
 * @if C7X
 * @ref AUDIOLIB_ssrc_getCircularBufferParams,
 * @endif
 * @ref AUDIOLIB_ssrc_getLinearInputBufferSize, @ref AUDIOLIB_ssrc_getStateBufferParams.
 *
 * 3. **Configuration:**
 *    The kernel is validated and configured using @ref AUDIOLIB_ssrc_init_checkParams and @ref AUDIOLIB_ssrc_init APIs.
 *
 * 4. **Filter State Reset:**
 *    The filter state is reset using @ref AUDIOLIB_ssrc_set with @ref AUDIOLIB_SSRC_MODE_RESET mode.
 *    After each @ref AUDIOLIB_ssrc_init call, the SSRC state must be first reset using @ref AUDIOLIB_ssrc_set before
 * executing the conversion.
 *
 * 5. **Execution:**
 *    The main processing is performed by @ref AUDIOLIB_ssrc_exec, which reads input samples, applies the sample rate
 * conversion algorithm (using polyphase filtering and interpolation or decimation), and writes the converted output
 samples.
 *
 * 6. **Output:**
 *    The number of output samples generated is fixed and depends on the input and output sample rates. If the input
 * data format is channel interleaved, the output samples will also be in interleaved format. If the input data format
 * is non-interleaved, the output will be in non-interleaved format.
 *
 * **Performance Considerations:**
 * @if C7X
 * - Buffers should be allocated in L2 memory and aligned for optimal performance.
 * - The kernel is primarily optimized for batch processing of multiple channels and large blocks of audio data.
 * - There are special optimizations developed to get best performance with small blocks of data..
 * @else
 * @if ARM_A53
 * - Buffers should be allocated in **DDR memory** and aligned for optimal performance on ARM Cortex-A53 targets.
 * - The kernel supports linear buffer format only and is optimized for batch processing of multiple channels
 * and large blocks of audio data on ARM Cortex-A53.
 * @endif
 * @endif
 *
 * **Usage Flow:**
 * @if C7X
 *  - Determine optimal buffer format using @c AUDIOLIB_ssrc_optimalBufferFormat based on input/output sample rates,
 *    input sample count, number of channels, data format, and MMA enablement. This function returns the recommended
 *    buffer format (@ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR or @ref AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR)
 *    for optimal performance based on the specific use case parameters. Note: running this function is not mandatory
 *    and it's best that user trys both buffer formats and selects the one that works best for the specific use case.
 *    @c AUDIOLIB_ssrc_optimalBufferFormat is developed using prerecorded performance data which might be different
 for
 *    user's application due to instruction cache behavior.
 *  - Allocate memory to all required buffers and third party kernels used by SSRC kernel. Ex:- fir, 2d block copy.
 * @else
 * @if ARM_A53
 *  - Allocate memory to all required buffers.
 * @endif
 * @endif
 *  - Validate and Configure/Initialize parameters using @ref AUDIOLIB_ssrc_init_checkParams and @ref
 * AUDIOLIB_ssrc_init.
 *  - Reset SSRC state using @ref AUDIOLIB_ssrc_set's @ref AUDIOLIB_SSRC_MODE_RESET mode.
 *  - For each input block:
 *      - Execute conversion for each input block @ref AUDIOLIB_ssrc_exec.
 *      - Retrieve output samples.
 *
 * **Input/Output Data Formats:**
 * - **Interleaved Format:**
 *   - Input: Channels are stored in a single array with samples for each channel interleaved.
 * @if C7X
 *   - The user has to manage the ping pong buffering when the data format is @c AUDIOLIB_DATA_FORMAT_INTERLEAVED and
 *     the buffer format is @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR.
 *     - Ex: Maintain two \a pIn buffers. Once the execution is started on the first \a pIn buffer, the user can start
 * filling the second \a pIn buffer with new data.
 *   - Once the execution is done on the first \a pIn buffer, submit the second \a pIn buffer to the execution
 * function and the user can start filling the first \a pIn buffer with new data while the second \a pIn buffer is
 * being processed.
 *   - If the user selects @ref AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR as the buffer format, the input buffer
 *     \a pIn should be used as the ping pong circular buffer. No need to allocate other \a pIn buffers.
 *   - The format of channel interleaved data is shown in the diagram below:
 * \image html AUDIOLIB_asrc_channel_interleaved_fmt.svg "Channel Interleaved Format Data Organization"
 * @else
 * @if ARM_A53
 *    - The user has to manage the ping pong buffering when the data format is @c AUDIOLIB_DATA_FORMAT_INTERLEAVED
 * since only @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR buffer format is supported on ARM Cortex-A53.
 *     - Ex: Maintain two \a pIn buffers. Once the execution is started on the first \a pIn buffer, the user can start
 * filling the second \a pIn buffer with new data.
 *     - Once the execution is done on the first \a pIn buffer, submit the second \a pIn buffer to the execution
 * function and the user can start filling the first \a pIn buffer with new data while the second \a pIn buffer is
 * being processed.
 * @endif
 * @endif
 *
 * - **Non-Interleaved Format:**
 *  - Input: Each channel has its samples first before the next channel's samples.
 *  - Output: Same as input, with converted sample rates.
 * @if C7X
 *  - The format of non interleaved data is shown in the diagram below:
 * \image html AUDIOLIB_asrc_non_interleaved_fmt.svg "Non-Interleaved Format Data Organization"
 *
 *  - If the user is using @ref AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR buffer format,
 *     the user has to increment the write pointer for each input block and make sure to wrap around the buffer.
 *     If the input block is bigger than remaining buffer size, the input block should be broken into two blocks and
 *     first one has to be written to the end of the buffer and the second one should be written to the beginning of the
 *     buffer.
 * @else
 * @if ARM_A53
 *   - Only @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR buffer format is supported on ARM Cortex-A53.
 *     The user simply writes new input samples to \a pIn for each frame. No circular write offset management
 *     or block tracking is required.
 * @endif
 * @endif
 *
 * **Other Considerations:**
 *   - The user can utilize @ref AUDIOLIB_ssrc_copyFilterCoeffs function to load the correct filter coefficients in to
 pFiltCoeffs buffer.
 *   - Filter coefficient header files are in the \a $AUDOLIB/src/AUDIOLIB_ssrc/filt_coeffs directory.
 *   - SSRC operates with fixed integer ratios (2:1 or 4:1) between input and output sample rates.
 *   - Unlike asynchronous sample rate conversion (ASRC), SSRC does not support variable ratios or continuous
 adjustment.
 *   - The conversion ratio is determined by the input and output sample rates specified during initialization.
 *
 * @ingroup   AUDIOLIB
 */
/**@{*/

/** @brief Max number of channels supported by the SSRC */
#define AUDIOLIB_SSRC_MAX_NUM_CHANNELS (128U)

/**
 * @brief SSRC set function modes
 */
#define AUDIOLIB_SSRC_MODE_RESET 0

/**
 * @brief Enum containing the allowed sample rates
 */
typedef enum {
   SSRC_SAMPLE_RATE_8000   = 0,
   SSRC_SAMPLE_RATE_11025  = 1,
   SSRC_SAMPLE_RATE_12000  = 2,
   SSRC_SAMPLE_RATE_16000  = 3,
   SSRC_SAMPLE_RATE_22050  = 4,
   SSRC_SAMPLE_RATE_24000  = 5,
   SSRC_SAMPLE_RATE_32000  = 6,
   SSRC_SAMPLE_RATE_44100  = 7,
   SSRC_SAMPLE_RATE_48000  = 8,
   SSRC_SAMPLE_RATE_64000  = 9,
   SSRC_SAMPLE_RATE_88200  = 10,
   SSRC_SAMPLE_RATE_96000  = 11,
   SSRC_SAMPLE_RATE_128000 = 12,
   SSRC_SAMPLE_RATE_176400 = 13,
   SSRC_SAMPLE_RATE_192000 = 14
} ssrc_sample_rate_t;

/**
 * @brief Buffer‑format enumeration
 */
typedef enum {
   AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR             = 0, /**< Simple contiguous buffer   */
   AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR = 1  /**< Ping‑pong / circular buffer */
} AUDIOLIB_ssrc_buffer_format_t;

/**
 * @brief Structure containing buffer parameters
 */
typedef struct {
   /** @brief Size of the buffer in bytes */
   uint32_t bufferSize;
   /** @brief Required alignment of the buffer in bytes */
   uint32_t alignment;
   /** @brief Stride of the buffer in bytes */
   uint32_t stride;
} AUDIOLIB_ssrc_bufferParams_t;

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE    */
   int8_t funcStyle;
   /** @brief Max number of ssrc input samples requested by the user per data block.
    *         This is necessary to allocate the in/out buffer sizes.
    *         User cannot send more data than this value in a single call to exec function.
    * */
   uint32_t inputSampleCount;
   /** @brief Ssrc input sample's data type.
    *         Only float type is supported. */
   uint32_t sampleDataType;
   /** @brief input sample rate set by the user.
    *  Allowed values are 8k, 11.025k, 12k, 16k, 22.05k, 24k, 32k, 44.1k, 48k, 64k, 88.2k, 96k, 128k, 176.4k, 192k Hz
    */
   ssrc_sample_rate_t inputSampleRate;
   /** @brief output sample rate set by the user.
    *  Allowed values are 8k, 11.025k, 12k, 16k, 22.05k, 24k, 32k, 44.1k, 48k, 64k, 88.2k, 96k, 128k, 176.4k, 192k Hz
    */
   ssrc_sample_rate_t outputSampleRate;
   /** @brief number of ssrc channels requested by the user. */
   uint8_t numChannels;
   /** @brief data format of the input samples. This could either be @c AUDIOLIB_DATA_FORMAT_INTERLEAVED = 1 or
    * @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED = 0. */
   uint8_t dataFormat;
   /** @brief Buffer format needed for optimal performance when the data format is @c
    * AUDIOLIB_DATA_FORMAT_INTERLEAVED. This could either be @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR or
    * @ref AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR. When the data format is
    * @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED the bufferFormat should be ONLY be @ref
    * AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR.*/
   AUDIOLIB_ssrc_buffer_format_t bufferFormat;
#ifdef C7X
   /** @brief Flag to indicate whether to use MMA unit (1) or not (0). */
   uint8_t enableMMA;
   /** @brief MMA size parameter for MMA configuration. */
   uint32_t mmaSize;
   /** @brief Kernel handle for the DSPLIB_deinterleave kernel.
    *         Only used when the input data format is channel interleaved and bufferFormat is
    * @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR. **/
   DSPLIB_kernelHandle deinterleaveHandle;
   /** @brief Kernel handle for the DSPLIB_interleave kernel.
    *         Only used when the input data format is channel interleaved and bufferFormat is
    * @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR. **/
   DSPLIB_kernelHandle interleaveHandle;
   /** @brief Kernel handle for the blkCopy2D kernel.
    *         Used for efficient 2D block copy operations for stage 1. **/
   DSPLIB_kernelHandle blkCopy2DHandle1;
   /** @brief Kernel handle for the blkCopy2D kernel.
    *         Used for efficient 2D block copy operations for stage 2. **/
   DSPLIB_kernelHandle blkCopy2DHandle2;
   /** @brief Kernel handle for the blkCopy2D kernel.
    *         Used for efficient 2D block copy operations non interleaved mode. **/
   DSPLIB_kernelHandle blkCopy2DHandle3;
   /** @brief Kernel handle for the fir kernel.
    *         Used for efficient FIR operations in the non interleaved data format for stage 1 filtering.
    */
   DSPLIB_kernelHandle firHandle1;
   /** @brief Kernel handle for the fir kernel.
    *         Used for efficient FIR operations in the non interleaved data format for stage 2 filtering.
    */
   DSPLIB_kernelHandle firHandle2;
#endif
} AUDIOLIB_ssrc_InitArgs;

/**
 * @brief Utility function to calculate the size of internal
 *        handle
 *
 * @param [in]   pKerInitArgs   : Pointer to structure holding init parameters
 *
 * @return       Size of the buffer in bytes
 *
 * @remarks      Application is expected to allocate buffer of the requested
 *               size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_ssrc_getHandleSize(AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 * @brief Utility function to calculate the output buffer length based on input/output sample rates.
 *
 * This function calculates the required output buffer length based on the input sample count and the
 * ratio between input and output sample rates. For upsampling, the output length is ceil(input length * ratio).
 * For downsampling, the output length is input length divided by the ratio (integer division).
 *
 * @param [in]   inputSampleRate   : Input sample rate
 * @param [in]   outputSampleRate  : Output sample rate
 * @param [in]   inputSampleCount  : Number of input samples
 *
 * @return       Returns the required output buffer length in samples.
 *
 * @remarks      Application is expected to allocate output buffer of sufficient size
 *               based on this calculation before making the @ref AUDIOLIB_ssrc_exec function call.
 */
int32_t AUDIOLIB_ssrc_getOutBufferLength(ssrc_sample_rate_t inputSampleRate,
                                         ssrc_sample_rate_t outputSampleRate,
                                         uint32_t           inputSampleCount);

/**
 * @brief Calculates the sample rate conversion ratio between input and output sample rates.
 *
 * This function calculates the ratio between input and output sample rates, handling both
 * upsampling and downsampling cases. For upsampling, it returns outRate/inRate, and for
 * downsampling, it returns inRate/outRate. The ratio is always positive and returned as a float.
 *
 * @param [in] inputSampleRate   : Input sample rate enum
 * @param [in] outputSampleRate  : Output sample rate enum
 *
 * @return The sample rate conversion ratio as a float (typically 2 or 4)
 */
float AUDIOLIB_ssrc_getSampleRateRatio(ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate);

/**
 * @brief Determines if the sample rate conversion is upsampling.
 *
 * This function checks if the output sample rate is higher than the input sample rate,
 * which indicates an upsampling operation.
 *
 * @param [in] inputSampleRate   : Input sample rate enum
 * @param [in] outputSampleRate  : Output sample rate enum
 *
 * @return true (1) if upsampling, false (0) if downsampling or equal rates
 */
bool AUDIOLIB_ssrc_isUpsampling(ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate);

/**
 * @brief Validates if the ratio between input and output sample rates is supported.
 *
 * This function checks if the ratio between the input and output sample rates is either 2 or 4,
 * which are the only ratios supported by SSRC. It returns AUDIOLIB_SUCCESS if the ratio is valid,
 * or AUDIOLIB_ERR_INVALID_VALUE if the ratio is not supported.
 *
 * @param [in]   inputSampleRate   : Input sample rate
 * @param [in]   outputSampleRate  : Output sample rate
 *
 * @return       Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *               - AUDIOLIB_SUCCESS: The ratio is valid (2 or 4)
 *               - AUDIOLIB_ERR_INVALID_VALUE: The ratio is not supported
 *
 * @remarks      Only ratios of 2 or 4 are supported for both upsampling and downsampling.
 */
AUDIOLIB_STATUS AUDIOLIB_ssrc_checkValidRatio(ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate);

/**
 * @brief Retrieves the filter length used in audio sample rate conversion based on sample rates and stage.
 *
 * This function returns the appropriate filter length value used in the
 * audio sample rate conversion process based on the input and output sample rates
 * and the specific stage. This function internally calculates the required number of stages for
 * sample rate conversion. If the user request the 2nd stage filter length for a 1 stage filtering operation, the
 * function will return 0.
 *
 * @param [in] inputSampleRate   : Input sample rate enum
 * @param [in] outputSampleRate  : Output sample rate enum
 * @param [in] stage             : The specific stage (1 or 2) for which to get the filter length
 *
 * @return uint32_t The filter length determined by the sample rate combination and stage.
 *
 * @remarks This value is used for memory allocation of filter coefficient buffers and copying of filter coefficients.
 * It represents the number of filter taps used in the polyphase filtering implementation of the SSRC algorithm.
 * Different sample rate combinations and stages may require different filter lengths for optimal audio quality and
 * performance.
 */
uint32_t
AUDIOLIB_ssrc_getFilterLength(ssrc_sample_rate_t inputSampleRate, ssrc_sample_rate_t outputSampleRate, uint32_t stage);

/**
 * @brief Retrieves the sample history length used in synchronous sample rate conversion based on sample rates and
 * stage.
 *
 * This function returns the appropriate historical sample count used in the
 * synchronous sample rate conversion process based on the input and output sample rates
 * and the specific stage. This function internally calculates the required number of stages for
 * sample rate conversion. If the user requests the 2nd stage history length for a 1-stage filtering operation, the
 * function will return 0.
 *
 * @param [in] inputSampleRate   : Input sample rate enum
 * @param [in] outputSampleRate  : Output sample rate enum
 * @param [in] stage             : The specific stage (1 or 2) for which to get the sample count history
 *
 * @return uint32_t The sample history length determined by the sample rate combination and stage.
 *
 * @remarks This value is used for memory allocation of input buffer
 *          and internal state buffer. It represents the number of samples stored as history for use
 *          in the polyphase filtering implementation of the SSRC algorithm.
 *          Different sample rate combinations and stages may require different history lengths
 *          for optimal audio quality and performance.
 *          For 2:1 conversion ratios, only stage 1 history is used. For 4:1 conversion ratios,
 *          both stage 1 and stage 2 history are used in a cascaded manner.
 */
uint32_t AUDIOLIB_ssrc_getSampleHistoryLength(ssrc_sample_rate_t inputSampleRate,
                                              ssrc_sample_rate_t outputSampleRate,
                                              uint32_t           stage);

/**
 * @brief Utility function to get the size of filter coefficients buffer.
 *
 * This function calculates the required size of the filter coefficients buffer based on
 * the input parameters. The size depends on the sample rates, buffer format, and whether MMA is enabled.
 * For 2:1 conversion ratios, only stage 1 filter coefficients are needed. For 4:1 conversion ratios,
 * both stage 1 and stage 2 filter coefficients are needed.
 *
 * @param [in] inputSampleRate   : Input sample rate
 * @param [in] outputSampleRate  : Output sample rate
 * @param [in] bufferFormat      : Buffer format – @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR or
 *                                 @ref AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR
 * @param [in] sampleDataType    : Data type of the samples (e.g., AUDIOLIB_FLOAT32)
 *
 * @return Size of the filter coefficients buffer in bytes
 *
 * @remarks Application is expected to allocate a filter coefficients buffer of the requested
 *          size and provide it as input to the @ref AUDIOLIB_ssrc_exec function.
 *          The filter coefficients can be copied to this buffer using @ref AUDIOLIB_ssrc_copyFilterCoeffs.
 */
int32_t AUDIOLIB_ssrc_getFilterCoeffSize(ssrc_sample_rate_t            inputSampleRate,
                                         ssrc_sample_rate_t            outputSampleRate,
                                         AUDIOLIB_ssrc_buffer_format_t bufferFormat,
#ifdef C7X
                                         uint8_t  enableMMA,
                                         uint32_t mmaSize,
#endif
                                         uint32_t sampleDataType);

/**
 * @brief Utility function to copy filter coefficients to a user-provided buffer.
 *
 * This function copies the appropriate filter coefficients based on the input/output sample rates
 * and whether it's upsampling or downsampling. It first zeros out the destination buffer and then
 * copies the coefficients from the provided arrays. For 2:1 conversion ratios, only stage 1 filter
 * coefficients are copied. For 4:1 conversion ratios, both stage 1 and stage 2 filter coefficients
 * are copied. If any pointer argument is NULL, the function returns AUDIOLIB_ERR_NULL_POINTER.
 *
 * @param [in] inputSampleRate   : Input sample rate
 * @param [in] outputSampleRate  : Output sample rate
 * @param [in] bufferFormat      : Buffer format – @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR or
 *                                 @ref AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR
 * @param [in] sampleDataType    : Data type of the samples (e.g., AUDIOLIB_FLOAT32)
 * @param [in] downsampleStage1  : Pointer to the downsampling stage 1 filter coefficients
 * @param [in] downsampleStage2  : Pointer to the downsampling stage 2 filter coefficients
 * @param [in] upsampleStage1    : Pointer to the upsampling stage 1 filter coefficients
 * @param [in] upsampleStage2    : Pointer to the upsampling stage 2 filter coefficients
 * @param [in] filterCoeffSizeInBytes : Size of the filter coefficients buffer in bytes
 * @param [out] pFiltCoeffs      : Pointer to the destination buffer where coefficients will be copied
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *         - AUDIOLIB_SUCCESS: Coefficients copied successfully
 *         - AUDIOLIB_ERR_NULL_POINTER: One or more pointer arguments are NULL
 *
 * @remarks Application is expected to allocate a filter coefficients buffer of the requested
 *          size using @ref AUDIOLIB_ssrc_getFilterCoeffSize before calling this function.
 *          The filter coefficients are organized in memory based on the buffer format and MMA settings.
 *          For MMA-enabled linear buffers, additional padding is added for alignment.
 */
AUDIOLIB_STATUS AUDIOLIB_ssrc_copyFilterCoeffs(ssrc_sample_rate_t            inputSampleRate,
                                               ssrc_sample_rate_t            outputSampleRate,
                                               AUDIOLIB_ssrc_buffer_format_t bufferFormat,
#ifdef C7X
                                               uint8_t  enableMMA,
                                               uint32_t mmaSize,
#endif
                                               uint32_t     sampleDataType,
                                               const float *downsampleStage1,
                                               const float *downsampleStage2,
                                               const float *upsampleStage1,
                                               const float *upsampleStage2,
                                               int32_t      filterCoeffSizeInBytes,
                                               void        *pFiltCoeffs);

/**
 * @brief Utility function to promote a value to the next power of 2.
 *
 * This function takes an integer value and returns the next power of 2 that is greater than or equal to the input
 * value. For example, if the input is 100, the output will be 128. If the input is already a power of 2, the same value
 * is returned. If the input is 0, the function returns 1.
 *
 * @param [in]   value   : The value to be promoted to the next power of 2
 *
 * @return       The next power of 2 value
 *
 * @remarks      This function uses bit manipulation to efficiently find the next power of 2.
 *               It's useful for ensuring proper alignment and optimizing memory access patterns,
 *               particularly for circular buffer allocation where buffer sizes must be powers of 2.
 */
uint32_t AUDIOLIB_ssrc_getNextPowerOf2(uint32_t value);

#ifdef C7X
/**
 * @brief  Recommends which buffer **format** (linear vs circular) should be used
 *         for a particular SSRC configuration based on the prerecorded performance data.
 *         The SSRC kernel works with any buffer format, so it is not a must to use this function.
 *         In fact it is imperative that user runs this kernel in user's system level application
 *         and figure out what buffer format to use for optimal performance. The performance user encounters
 *         in their system might be slightly different from prerecorded performance data used by this function to make a
 *         buffer format recommendation due to instruction cache behavior (L1P). But in general larger input sample
 *         counts (block sizes) and higher channel counts with MMA enabled perform better in the @ref
 *         AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR buffer format and smaller input sample counts (block sizes) and channels
 *         counts perform better in the @ref AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR buffer format. Note:
 *         Circular buffer format does not need MMA acceleration.
 *
 *
 * @param[in]  inputSampleRate   Input sample rate
 * @param[in]  outputSampleRate  Output sample rate
 * @param[in]  inputSampleCount  Number of input samples per block
 *                               (the value passed to @c kerInitArgs.inputSampleCount).
 * @param[in]  numChannels       Number of audio channels.
 * @param[in]  dataFormat        Data format – 1 for interleaved or 0 for non-interleaved.
 * @param[in]  enableMMA         Flag to indicate whether to use MMA unit (1) or not (0).
 *
 * @return  One of @c AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR or
 *          @c AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR.
 *
 */
AUDIOLIB_ssrc_buffer_format_t AUDIOLIB_ssrc_optimalBufferFormat(ssrc_sample_rate_t inputSampleRate,
                                                                ssrc_sample_rate_t outputSampleRate,
                                                                uint32_t           inputSampleCount,
                                                                uint8_t            numChannels,
                                                                uint8_t            dataFormat,
                                                                uint8_t            enableMMA);
#endif

/**
 * @brief        Converts an enum sample rate to an integer sample rate.
 *
 * @param [in]    rate The enum sample rate to convert.
 *
 * @return        The integer sample rate, or 0 if the enum value is invalid.
 */
uint32_t AUDIOLIB_ssrc_convertSampleRateToInt(ssrc_sample_rate_t rate);

/**
 * @brief Utility function to calculate the circular buffer parameters
 *
 * This function calculates the required buffer size, alignment, and stride for circular buffer mode based on ,
 * input/output sample rates, and other parameters. Data format supported @c AUDIOLIB_DATA_FORMAT_INTERLEAVED ONLY
 *
 * @param [in] inputSampleRate    : Input sample rate
 * @param [in] outputSampleRate   : Output sample rate
 * @param [in] inputSampleCount   : Number of input samples
 * @param [in] numChannels        : Number of audio channels
 * @param [in] sampleDataType     : Sample data type (e.g., AUDIOLIB_FLOAT32)
 * @param [out] pBufferParams     : Pointer to store the calculated buffer parameters (size, alignment, stride)
 *
 * @return AUDIOLIB_STATUS indicating success or failure
 */
AUDIOLIB_STATUS AUDIOLIB_ssrc_getCircularBufferParams(ssrc_sample_rate_t            inputSampleRate,
                                                      ssrc_sample_rate_t            outputSampleRate,
                                                      uint32_t                      inputSampleCount,
                                                      uint8_t                       numChannels,
                                                      uint32_t                      sampleDataType,
                                                      AUDIOLIB_ssrc_bufferParams_t *pBufferParams);

/**
 * @brief Utility function to calculate the size of the state buffer
 *
 * This function calculates the required size of the state buffer based on the
 * input parameters. Only intended to use when the buffer format is @ref AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR
 *
 * @param [in] inputSampleRate   : Input sample rate
 * @param [in] outputSampleRate  : Output sample rate
 * @param [in] inputSampleCount  : Number of input samples
 * @param [in] numChannels       : Number of audio channels
 * @param [in] sampleDataType    : Data type of the samples (e.g., AUDIOLIB_FLOAT32)
 * @param [in] dataFormat        : Data format – @c AUDIOLIB_DATA_FORMAT_INTERLEAVED = 1 or
 * @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED = 0.
 * @param [out] pBufferParams     : Pointer to store the calculated buffer parameters (size, alignment, stride)
 *
 * @return Size of the state buffer in bytes
 *
 * @remarks Application is expected to allocate a state buffer of the requested
 *          size and provide it as input to the @ref AUDIOLIB_ssrc_set and @ref AUDIOLIB_ssrc_exec
 *          functions when using linear buffer format.
 */
AUDIOLIB_STATUS AUDIOLIB_ssrc_getStateBufferParams(ssrc_sample_rate_t            inputSampleRate,
                                                   ssrc_sample_rate_t            outputSampleRate,
                                                   uint32_t                      inputSampleCount,
                                                   uint8_t                       numChannels,
                                                   uint32_t                      sampleDataType,
                                                   uint8_t                       dataFormat,
                                                   AUDIOLIB_ssrc_bufferParams_t *pBufferParams);

/**
 * @brief Utility function to calculate the size of the linear input buffer.
 *
 * This function calculates the required size of the input buffer based on the
 * input parameters and data format. For interleaved format, it uses matrix transpose
 * row stride calculation to ensure proper alignment for vector operations. This utility function should only be used
 * for linear buffer format ONLY. For circular buffers use @c AUDIOLIB_ssrc_getCircularBufferParams
 *
 * @param [in] inputSampleRate   : Input sample rate
 * @param [in] outputSampleRate  : Output sample rate
 * @param [in] inputSampleCount  : Number of input samples
 * @param [in] numChannels       : Number of audio channels
 * @param [in] sampleDataType    : Data type of the samples (e.g., AUDIOLIB_FLOAT32)
 * @param [in] dataFormat        : Data format – @c AUDIOLIB_DATA_FORMAT_INTERLEAVED = 1 or
 * @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED = 0.
 *
 * @return Size of the input buffer in bytes
 *
 * @remarks Application is expected to allocate an input buffer of the requested
 *          size and provide it as input to the @ref AUDIOLIB_ssrc_set and @ref AUDIOLIB_ssrc_exec
 *          functions.
 */
uint32_t AUDIOLIB_ssrc_getLinearInputBufferSize(ssrc_sample_rate_t inputSampleRate,
                                                ssrc_sample_rate_t outputSampleRate,
                                                uint32_t           inputSampleCount,
                                                uint8_t            numChannels,
                                                uint32_t           sampleDataType,
                                                uint8_t            dataFormat);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_ssrc_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as the MMA
 *               accelerator and the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param[in]   handle          : Active handle to the kernel
 *  @param[in]   bufParamsIn     : Pointer to the structure containing dimensional
 *                                 information of input buffer
 *  @param[out]  bufParamsOut    : Pointer to the structure containing dimensional
 *                                 information of ouput buffer
 *  @param[in]   pKerInitArgs    : Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */
AUDIOLIB_STATUS AUDIOLIB_ssrc_init(AUDIOLIB_kernelHandle   handle,
                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                   AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_ssrc_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_ssrc_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_ssrc_init is called.
 *
 *  @param[in]   handle          : Active handle to the kernel
 *  @param[in]   bufParamsIn     : Pointer to the structure containing dimensional
 *                                 information of input buffer
 *  @param[out]  bufParamsOut    : Pointer to the structure containing dimensional
 *                                 information of ouput buffer
 *  @param[in]   pKerInitArgs    : Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_ssrc_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_ssrc_InitArgs *pKerInitArgs);

/**
 *  @brief       This function resets the filter state when used with @ref AUDIOLIB_SSRC_MODE_RESET.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_ssrc
 *
 *  @param [in]  handle       : Active handle to the kernel
 *  @param [in]  mode         : Operation mode
 *                              - @ref AUDIOLIB_SSRC_MODE_RESET Resets the filter state
 *  @param [in]  pIn          : Pointer to buffer holding the input data.
 *  @param [in]  pState       : Pointer to buffer holding the state data.
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - The kernel handle must be active and valid
 *    - pIn must not be NULL when mode is @ref AUDIOLIB_SSRC_MODE_RESET.
 *    - When the input data format is @c AUDIOLIB_DATA_FORMAT_INTERLEAVED, and the if buffer format is @ref
 * AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR, pIn must be aligned to the size recommended by the @ref
 * AUDIOLIB_ssrc_getCircularBufferParams function.
 *    - If this alignment is not set properly streaming engine circular buffer will not work and the algorithm will
 * break!!!
 *    - When the input data format is @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED, pIn must be aligned to the size of
 * 64-bytes.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - The pIn buffer is expected to be in L2 memory
 *
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_ssrc_init_checkParams and @ref AUDIOLIB_ssrc_init functions.
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 *               This function MUST be called ONCE with mode = @ref AUDIOLIB_SSRC_MODE_RESET
 *               before calling @ref AUDIOLIB_ssrc_exec
 */
AUDIOLIB_STATUS
AUDIOLIB_ssrc_set(AUDIOLIB_kernelHandle handle, uint8_t mode, void *restrict pIn, void *restrict pState);

/**
 *  @brief       This function is the main kernel compute function that performs the synchronous sample rate conversion.
 *
 *  @details     This function performs the actual sample rate conversion using polyphase filtering techniques.
 *               For 2:1 conversion ratios, a single-stage filtering process is used.
 *               For 4:1 conversion ratios, a two-stage cascaded filtering process is used.
 *               The function handles both upsampling and downsampling operations based on the input and output sample
 * rates specified during initialization. Please refer to details under @ref AUDIOLIB_ssrc for more information.
 *
 *  @param [in]  handle                : Active handle to the kernel
 *  @param [in]  pIn                   : Pointer to buffer holding the input data
 *  @param [in]  pState                : Pointer to buffer holding the state data (required for linear buffer format,
 * can be NULL for circular buffer format)
 *  @param [in]  pFiltCoeffs           : Pointer to buffer holding filter coefficients
 *  @param [out] pOut                  : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - Memory pointed by \a pIn, \a pOut shall NOT be shared.
 *      Library code is using the restrict keyword to optimize the kernel. So any of these pointer should not point to
 *      the same memory region at any time.
 *    - When the input data format is @c AUDIOLIB_DATA_FORMAT_INTERLEAVED, and the if buffer format is @ref
 * AUDIOLIB_SSRC_BUFFER_FORMAT_PING_PONG_CIRCULAR, pIn must be aligned to the size recommended by the @ref
 * AUDIOLIB_ssrc_getCircularBufferParams function.
 *    - If this alignment is not set properly streaming engine circular buffer will not work and the algorithm will
 * break!!!
 *    - When the input data format is @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED, and the if buffer format is @ref
 * AUDIOLIB_SSRC_BUFFER_FORMAT_LINEAR, pIn must be aligned to the size of 64-bytes.
 *    - When the input data format is @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED, pIn must be aligned to the size of
 * 64-bytes.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - The input, output and intermediate data buffers are expected to be in L2 memory
 *    - The buffer pointers are assumed to be 64-byte aligned unless it's a circular buffer which needs special
 * alignment.
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_ssrc_init,
 *               @ref AUDIOLIB_ssrc_set in @ref AUDIOLIB_SSRC_MODE_RESET mode and
 *               @ref AUDIOLIB_ssrc_exec_checkParams functions.
 *               This ensures resource configuration and error checks are done
 * only once for several invocations of this function.
 */
AUDIOLIB_STATUS
AUDIOLIB_ssrc_exec(AUDIOLIB_kernelHandle handle,
                   void *restrict pIn,
                   void *restrict pState,
                   void *restrict pFiltCoeffs,
                   void *restrict pOut);

/**
 * @brief Checks the validity of parameters for SSRC execution.
 *
 * This function validates the pointers and buffer requirements before executing the SSRC kernel.
 * It checks for NULL pointers for the kernel handle, input, output, and filter coefficients buffers.
 * For linear buffer format, it also checks that the state buffer pointer is not NULL.
 *
 * @param[in]  handle       Active handle to the SSRC kernel
 * @param[in]  pIn          Pointer to input buffer
 * @param[in]  pState       Pointer to state buffer (required for linear buffer format)
 * @param[in]  pFiltCoeffs  Pointer to filter coefficients buffer
 * @param[in]  pOut         Pointer to output buffer
 *
 * @return     Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *             - AUDIOLIB_SUCCESS: All parameters are valid
 *             - AUDIOLIB_ERR_NULL_POINTER: One or more required pointers are NULL
 *
 * @remarks    This function should be called before @ref AUDIOLIB_ssrc_exec to ensure all required buffers
 *             are properly allocated and valid for the selected buffer format.
 */
AUDIOLIB_STATUS
AUDIOLIB_ssrc_exec_checkParams(AUDIOLIB_kernelHandle handle,
                               const void *restrict pIn,
                               const void *restrict pState,
                               const void *restrict pFiltCoeffs,
                               const void *restrict pOut);

/**
 *  @brief       This is a utility function that gives an estimate of the cycles
 *               consumed for the kernel execution.
 *
 *  @param [in]   handle               : Active handle to the kernel
 *  @param [in]   bufParamsIn          : Pointer to the structure containing
 *                                       dimensional information of the input buffer
 *  @param [in]   bufParamsOut         : Pointer to the structure containing
 *                                       dimensional information of the output buffer
 *  @param [out]  archCycles           : Cycles estimated for the compute, startup and
 *                                       teardown
 *  @param [out]  estCycles            : Cycles estimated for the compute, startup,
 *                                       teardown and any associated overhead
 *
 *  @remarks     None
 */
void AUDIOLIB_ssrc_perfEst(AUDIOLIB_kernelHandle         handle,
                           const AUDIOLIB_bufParams2D_t *bufParamsIn,
                           const AUDIOLIB_bufParams2D_t *bufParamsOut,
                           uint64_t                     *archCycles,
                           uint64_t                     *estCycles);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_SSRC_H_ */
