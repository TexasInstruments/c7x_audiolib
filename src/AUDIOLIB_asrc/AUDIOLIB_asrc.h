// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ASRC_H_
#define AUDIOLIB_ASRC_H_

#include "../common/AUDIOLIB_types.h"
#ifdef C7X
#include <dsplib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_asrc AUDIOLIB_asrc
 * @brief **Asynchronous Sample Rate Conversion (ASRC)**
 *
 *
 * @details
 * The ASRC kernel performs asynchronous sample rate conversion of discrete input signals, converting audio data from
 * one sample rate to another. It is designed for high-performance, multi-channel audio processing and supports both
 * interleaved and non-interleaved data formats.
 *
 * **Key Features:**
 * - Converts input audio streams from an input sample rate to a different output sample rate.
 * - Supports both channel interleaved and non-interleaved input data formats.
 * - Handles multiple audio channels in a single kernel call.
 *  @if C7X
 * - Supports ping-pong buffer management.
 *  @endif
 * - Provides utility functions to calculate buffer sizes for output, intermediate and remaining data.
 * - Ensures output sample count is a multiple of a configurable frame modulo factor for integrating with different
 * system applications.
 *
 * **Algorithm Overview:**
 * 1. **Initialization:**
 *    The kernel is initialized with user-specified parameters such as input/output sample rates, number of channels,
 * data format, and buffer sizes using @ref AUDIOLIB_asrc_InitArgs.
 *
 * 2. **Buffer Preparation:**
 *    The application allocates output, intermediate and remaining sample buffers based on the calculated sizes using
 * utility functions like
 *    @ref AUDIOLIB_asrc_getOutBufferLength, @ref AUDIOLIB_asrc_getNonInterleavedDataBufSize,
 *    and @ref AUDIOLIB_asrc_getFilterRembufSize.
 *
 * 3. **Configuration:**
 *    The kernel is validated and configured using @ref AUDIOLIB_asrc_init_checkParams and @ref AUDIOLIB_asrc_init APIs.
 *
 * 4. **Ratio Setting / Reset:**
 *    The ASRC ratio (output/input sample rate) is set or the filter state is reset using @ref AUDIOLIB_asrc_set.
 *    After each @ref AUDIOLIB_asrc_init call, the ASRC state must be reset first using @ref AUDIOLIB_asrc_set before
 * executing the conversion.
 *
 * 5. **Execution:**
 *    The main processing is performed by @ref AUDIOLIB_asrc_exec, which reads input samples, applies the sample rate
 * conversion algorithm (using polyphase filtering and interpolation), and writes the converted output samples.
 *
 * 6. **Output:**
 *    The number of output samples generated is provided in @ref AUDIOLIB_asrc_ExecOutArgs. If the input data format is
 * channel interleaved, the output samples will also be in interleaved format. If the input data format is
 * non-interleaved, the output will be in non-interleaved format.
 *
 * **Performance Considerations:**
 * @if C7X
 * - Buffers should be allocated in **L2 memory** and aligned for optimal performance on C7x targets.
 * - The kernel is optimized for batch processing of multiple channels and large blocks of audio data on the C7x DSP.
 * @else
 * @if ARM_A53
 * - Buffers should be allocated in **DDR memory** and aligned for optimal performance on ARM Cortex-A53 targets.
 * - The kernel is optimized for batch processing of multiple channels and large blocks of audio data on ARM Cortex-A53.
 * @else
 * - Buffers should be allocated in the appropriate memory region based on the target platform.
 * - The kernel is optimized for batch processing of multiple channels and large blocks of audio data.
 * @endif
 * @endif
 *
 * **Usage Flow:**
 *  - Allocate memory to all required buffers.
 *  - Validate and Configure/Initialize parameters using @ref AUDIOLIB_asrc_init_checkParams and @ref AUDIOLIB_asrc_init.
 *  - Reset ASRC state using @ref AUDIOLIB_asrc_set's @ref AUDIOLIB_ASRC_MODE_RESET mode.
 *  - Set \a asrcRatio using @ref AUDIOLIB_asrc_set's @ref AUDIOLIB_ASRC_MODE_SET mode (must run at least once).
 *  - For each input block:
 *      - Set \a asrcRatio (optional, using @ref AUDIOLIB_asrc_set's @ref AUDIOLIB_ASRC_MODE_SET mode).
 *      - Execute conversion for each input block @ref AUDIOLIB_asrc_exec.
 *      - Retrieve output samples.
 *
 * **Input/Output Data Formats:**
 * - **Interleaved Format:**
 *   - Input: Channels are stored in a single array with samples for each channel interleaved.
 * @if C7X
 *   - ASRC algorithm is optimized to use non interleaved data. The input data is converted to non interleaved format internally before processing using \a DSPLIB_matTrans kernel. User has to provide the handle to this kernel in @c AUDIOLIB_asrc_InitArgs::matTransHandle and allocate all necessary buffers including the intermediate circular buffer. ASRC uses a novel circular ping-pong buffer technique; the intermediate circular buffer is half the size of the non-interleaved mode circular buffer because the user is already managing ping pong buffering via pIn buffer.
 *   - The output data is converted back to interleaved format once the processing is done.
 *   - The user has to manage the ping pong buffering when the input data is set to the channel interleaved format.
 *   - Ex: Maintain two \a pIn buffers. Once the execution is started on the first \a pIn buffer, the user can start filling the second \a pIn buffer with new data.
 *   - Once the execution is done on the first \a pIn buffer, submit the second \a pIn buffer to the execution function and the user can start filling the first \a pIn buffer with new data while the second \a pIn buffer is being processed.
 *   - The format of channel interleaved data is shown in the diagram below:
 * \image html AUDIOLIB_asrc_channel_interleaved_fmt.svg "Channel Interleaved Format Data Organization"
 * @else
 * @if ARM_A53
 *   - ASRC algorithm is optimized to use non interleaved data. The input data is converted to non interleaved format internally before processing using a standard C implementation. Unlike the C7x target, this conversion does not use \a DSPLIB_matTrans kernel. The interleaved to non interleaved conversion is handled internally using a plain C routine, and the filter history is maintained in the \a pNonInterleavedData linear buffer across frames.
 *   - The output data is converted back to interleaved format once the processing is done.
 *   - The user has to manage the ping pong buffering when the input data is set to the channel interleaved format, since the kernel processes one \a pIn buffer at a time.
 *   - Ex: Maintain two \a pIn buffers. Once the execution is started on the first \a pIn buffer, the user can start filling the second \a pIn buffer with new data.
 *   - Once the execution is done on the first \a pIn buffer, submit the second \a pIn buffer to the execution function and the user can start filling the first \a pIn buffer with new data while the second \a pIn buffer is being processed.
 * @endif
 * @endif
 *
 * - **Non-Interleaved Format:**
 *  - Input: Each channel has its samples first before the next channel's samples.
 *  - Output: Same as input, with converted sample rates.
 * @if C7X
 *  - The format of non interleaved data is shown in the diagram below:
 * \image html AUDIOLIB_asrc_non_interleaved_fmt.svg "Non-Interleaved Format Data Organization"
 * @endif
 *
 * @if C7X
 * In C7x non-interleaved mode, the following additional constraints apply:
 * - The user must write to the correct block of the circular buffer for each new block of data, cycling back to the first block once the last block is filled.
 * - The width of the circular buffer is determined by the maximum sample count per block.
 * - The maximum sample count per block must be a power of 2 to support circular buffering.
 * - If the maximum sample count per block is >= 64, the circular buffer will have 4 blocks of the maximum sample count.
 * - If the maximum sample count per block is < 64, the circular buffer width should be 128 samples.
 * - The circular buffer will have as many rows as the number of channels.
 * - The circular buffer is in L2 memory and MUST be \f$(\text{totalWidthOfCircularBuffer} \times \text{sizeof(float)})\f$-byte aligned.
 * - Ping pong buffering is supported through this circular buffer (no need to allocate 2 buffers).
 * - Example: circular buffer of 128 samples total, each block 32 samples.
 * - Note:
 *
 * \image html AUDIOLIB_asrc_circular_ping_pong_buffer.svg "Circular Ping Pong Buffering for Non-Interleaved Data"
 *
 * - Each block can have multiple rows depending on the number of channels.
 * @else
 * @if ARM_A53
 * - In the non-interleaved mode on ARM Cortex-A53, a plain linear buffer is used. The filter history is maintained internally across frames via the \a pNonInterleavedData linear buffer.
 * - The user simply writes new input samples to \a pIn for each frame; no circular write offset management or block tracking is required.
 * - The \a pNonInterleavedData buffer has as many rows as the number of channels, and the width is determined by the history length plus the maximum input sample count per block.
 * - The \a pNonInterleavedData buffer is allocated in DDR memory.
 * @endif
 * @endif
 * - Output sample buffer is not a circular buffer; it is a single contiguous block. The output data format matches the input data format.
 *
 * **Other Considerations:**
 * - Load the correct filter coefficients into pFiltCoeffs buffer based on the input/output sample rates (see AUDIOLIB_asrc_d.c).
 * - Filter coefficient header files are in the \a $AUDIOLIB/src/AUDIOLIB_asrc/filt_coeffs directory.
 * - In standard operation, initially set \a asrcRatio using @ref AUDIOLIB_asrc_set based on the static input and output sample rates.
 * - Real sample clock frequencies can vary; it is the user's responsibility to frequently calculate the correct \a asrcRatio = fsout / fsin value using a separate calcRatio driver with hardware counters. If this is not done, system-level buffers will eventually overflow.
 * - There are limitations on the asrcRatio that can be set to prevent internal buffer overflow.
 * - This is determined by: \f$\text{maxAsrcRatio} = \text{InitialSetSampleRateRatio} + \frac{(\text{AUDIOLIB\_ASRC\_MAX\_MODULO\_FACTOR} - \text{userSetFrameModuloFactor})}{maxSampleCountPerBlock}\f$
 *
 * - **FFT Plot:**
 * @if C7X
 *  - Follwing plot shows the FFT for 1 kHz Sine Input that was sample rate converted from 44.1KHz to 48KHz.
 *  - Plot is based on output data collected by running ASRC kernel on a C7504 processor.
 *  - Note that noise compomnents are lower than -140 dB which is what our filters are designed for.
 * \image html AUDIOLIB_asrc_output_fft_plot.svg "FFT of 1 kHz Sine Input for Sample Rate Conversion: 44.1 to 48 kHz"
 * @else
 * @if ARM_A53
 *  - Following plot shows the FFT for 1 kHz Sine Input that was sample rate converted from 44.1 kHz to 48 kHz.
 *  - Plot is based on output data collected by running ASRC kernel on an ARM Cortex-A53 processor.
 *  - Note that noise components are lower than -140 dB which is what our filters are designed for.
 * \image html AUDIOLIB_asrc_output_fft_plot.svg "FFT of 1 kHz Sine Input for Sample Rate Conversion: 44.1 to 48 kHz"
 * @endif
 * @endif
 * @ingroup   AUDIOLIB
 */
/**@{*/

/**
 * @brief ASRC's input frame length.
 *        Only allowed values are multiples of 2 starting from 4 (4, 8, 16, 32, 64, 128, 256, etc).
 *        Note that ASRC uses quadruple buffers for ping ponging so for larger block sizes the input buffer size will be
 *        4 times the standard buffer size.
 * */
#define AUDIOLIB_ASRC_DEFAULT_INPUT_FRAMELENGTH (256U)

/** @brief Max number of channels supported by the ASRC */
#define AUDIOLIB_ASRC_MAX_NUM_CHANNELS (128U)

/**
 * @brief ASRC set function modes
 */
#define AUDIOLIB_ASRC_MODE_SET   0 /**< Mode: Set the ASRC ratio. */
#define AUDIOLIB_ASRC_MODE_RESET 1 /**< Mode: Reset the ASRC state. */

/**
 * @brief Enum containing the allowed sample rates
 */
typedef enum { SAMPLE_RATE_NA = 0, SAMPLE_RATE_32000 = 1, SAMPLE_RATE_44100 = 2, SAMPLE_RATE_48000 = 3 } sample_rate_t;

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE    */
   int8_t funcStyle;
   /** @brief Max number of asrc input samples requested by the user per data block.
    *         This is necessary to allocate the in/out buffer sizes.
    *         User cannot send more data than this value in a single call to exec function.
    * */
   uint32_t maxSampleCountPerBlock;
   /** @brief Asrc input sample's data type.
    *         Only float type is supported. */
   uint32_t sampleDataType;
   /** @brief input sample rate set by the user.
    *        Allowed values are 32kHz, 44.1 kHz, and 48 kHz.
    */
   sample_rate_t inputSampleRate;
   /** @brief output sample rate set by the user.
    *         Allowed values are 48 kHz.
    */
   sample_rate_t outputSampleRate;
   /** @brief number of asrc channels requested by the user. */
   uint8_t numChannels;
   /** @brief data format of the input samples. This could either be @c AUDIOLIB_DATA_FORMAT_INTERLEAVED = 1 or
    * @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED = 0. */
   uint8_t dataFormat;
   /** @brief Frame modulo factor, used to calculate the output sample count.
    *         This is used to ensure that the output sample count is a multiple of this factor.
    *         Default value is 4 for low latency. Max is set to 32 for now (AUDIOLIB_ASRC_MAX_MODULO_FACTOR) */
   uint32_t frameModuloFactor;
#ifdef C7X
   /** @brief Kernel handle for the matTrans kernel.
    *         Only used when the input data format is channel interleaved. **/
   DSPLIB_kernelHandle matTransHandle;
#endif
} AUDIOLIB_asrc_InitArgs;

/**
 * @brief Structure containing the input parameters to exec function
 */
typedef struct {
   /** @brief Number of asrc input sample count requested by the user.
    *         This should be equal to the AUDIOLIB_asrc_InitArgs::maxSampleCountPerBlock
    *         for all the calls of @ref AUDIOLIB_asrc_exec except for the last call.
    *         In the last call you can set this to any value > 0 and <= AUDIOLIB_asrc_InitArgs::maxSampleCountPerBlock
    */
   uint32_t inputSampleCount;
} AUDIOLIB_asrc_ExecInArgs;

/**
 * @brief Structure containing the output parameters from exec function
 */
typedef struct {
   /** @brief Number of asrc output sample count produced by the @ref AUDIOLIB_asrc_exec on each call. */
   uint32_t outputSampleCount;
} AUDIOLIB_asrc_ExecOutArgs;

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
int32_t AUDIOLIB_asrc_getHandleSize(AUDIOLIB_asrc_InitArgs *pKerInitArgs);

/**
 * @brief Utility function to calculate the size of Non Interleaved Data circular buffer.
 *        This buffer is only required when the input data to ASRC is interleaved.
 *
 * @param [in]   numChannels     : Number of ASRC channels
 * @param [in]   sampleCount     : maximum sample count per block
 * @param [in]   sampleDataType  : Sample's data type
 * @param [in]   dataFormat      : Data format (interleaved/non-interleaved)
 *
 * @return       Returns the size of the buffer in bytes.
 *
 * @remarks      Application is expected to allocate buffers of the requested
 *               size for the Non Interleaved Data circular buffer before
 *               making the @ref AUDIOLIB_asrc_exec function call if the input data format is
 *               interleaved.
 */
int32_t AUDIOLIB_asrc_getNonInterleavedDataBufSize(uint8_t  numChannels,
                                                   uint32_t sampleCount,
                                                   uint32_t sampleDataType,
                                                   uint32_t dataFormat);

/**
 * @brief Utility function to calculate the ssize of the polyphase filter coefficients.
 *
 * @param [in]   sampleDataType  : Sample's data type
 *
 * @return       Returns the size of the buffer in bytes.
 *
 * @remarks      Application is expected to allocate buffers of the requested
 *               size for the filter coefficients buffer before
 *               making the @ref AUDIOLIB_asrc_exec function call.
 */
int32_t AUDIOLIB_asrc_getFilterCoeffSize(uint32_t sampleDataType);

/**
 * @brief Utility function to calculate the size of filter remaining buffer.
 *
 * @param [in]   numChannels     : Number of ASRC channels
 * @param [in]   sampleDataType  : Sample's data type
 * @param [in]   frameModuloFactor: Frame modulo factor
 *
 * @return       Returns the size of the buffer in bytes.
 *
 * @remarks      Application is expected to allocate buffers of the requested
 *               size for the filter remaining buffer before
 *               making the @ref AUDIOLIB_asrc_exec function call.
 */
int32_t AUDIOLIB_asrc_getFilterRembufSize(uint8_t numChannels, uint32_t sampleDataType, uint32_t frameModuloFactor);

/**
 * @brief Utility function to calculate the dimention x of output buffer.
 *
 * @param [in]   inputSampleRate   : The input sample rate.
 * @param [in]   outputSampleRate  : The output sample rate.
 * @param [in]   sampleCount       : The number of input samples.
 *
 * @return       Returns the length of the output buffer in element count.
 *
 * @remarks      Application is expected to allocate buffers of the requested
 *               size for the output buffer before
 *               making the @ref AUDIOLIB_asrc_exec function call.
 */
int32_t
AUDIOLIB_asrc_getOutBufferLength(sample_rate_t inputSampleRate, sample_rate_t outputSampleRate, uint32_t sampleCount);

/**
 * @brief Retrieves the filter length used in audio sample rate conversion.
 *
 * This function returns the predefined filter length value used in the
 * audio sample rate conversion process.
 *
 * @return uint32_t The filter length, which is set to 64.
 */
uint32_t AUDIOLIB_asrc_getFilterLength(void);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_asrc_exec function is called. This
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
AUDIOLIB_STATUS AUDIOLIB_asrc_init(AUDIOLIB_kernelHandle   handle,
                                   AUDIOLIB_bufParams2D_t *bufParamsIn,
                                   AUDIOLIB_bufParams2D_t *bufParamsOut,
                                   AUDIOLIB_asrc_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_asrc_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_asrc_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_asrc_init is called.
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
AUDIOLIB_asrc_init_checkParams(AUDIOLIB_kernelHandle         handle,
                               const AUDIOLIB_bufParams2D_t *bufParamsIn,
                               const AUDIOLIB_bufParams2D_t *bufParamsOut,
                               const AUDIOLIB_asrc_InitArgs *pKerInitArgs);

/**
 *  @brief       This function does 2 operations based on mode parameter
 *                - mode = @ref AUDIOLIB_ASRC_MODE_RESET resets the filter state
 *                - mode = @ref AUDIOLIB_ASRC_MODE_SET sets the asrcRatio
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_asrc
 *
 *  @param [in]  handle       : Active handle to the kernel
 *  @param [in]  mode         : Operation mode
 *                              - @ref AUDIOLIB_ASRC_MODE_RESET
 *                              - @ref AUDIOLIB_ASRC_MODE_SET
 *  @param [in]  asrcRatio     : Outputs to input sample rate ratio (used when mode is @ref AUDIOLIB_ASRC_MODE_SET)
 *  @param [in]  pNonInterleavedData : Pointer to buffer holding the intermediate non interleaved data.
 *  @param [in]  pIn          : Pointer to buffer holding the input data.
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - The kernel handle must be active and valid
 *    - pNonInterleavedData, pIn and pFiltCoeffs must not be NULL when mode is @ref AUDIOLIB_ASRC_MODE_RESET.
 *    - The buffer pointers are assumed to be aligned to the size of circular buffer in bytes. To be precise,
 *      - When the input data format is @c AUDIOLIB_DATA_FORMAT_INTERLEAVED, pIn must be aligned to the size of
 * 64-bytes and the pNonInterleavedData must be aligned to the size of circular buffer in bytes.
 *      - When the input data format is @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED, pIn must be aligned to the size of
 * circular buffer in bytes.
 *      - If this alignment is not set properly streming engine circular buffer will not work and the algorithm will
 * break!!!
 *      - pFiltCoeffs must be aligned to the size of 64-bytes.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - The pNonInterleavedData, pIn and pFiltCoeffs buffers are expected to be in L2 memory
 *
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_asrc_init function.
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 *               This function MUST be called ONCE with mode = @ref AUDIOLIB_ASRC_MODE_RESET
 *               before calling @ref AUDIOLIB_asrc_exec
 */
AUDIOLIB_STATUS AUDIOLIB_asrc_set(AUDIOLIB_kernelHandle handle,
                                  uint8_t               mode,
                                  double                asrcRatio,
                                  void *restrict pNonInterleavedData,
                                  void *restrict pIn);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_asrc
 *
 *  @param [in]  handle                : Active handle to the kernel
 *  @param [in]  pIn                   : Pointer to buffer holding the input data
 *  @param [in]  pNonInterleavedData   : Pointer to buffer holding the intermediate non interleaved data
 *  @param [in]  pFiltCoeffs           : Pointer to buffer holding polyphase filter cefficients.
 *  @param [in]  pFilterRembuf         : Pointer to buffer holding the filter remaining buffer
 *  @param [out] pOut                  : Pointer to buffer holding the output data
 *  @param [in]  pKerInArgs            : Pointer to structure holding input Arguments
 *  @param [out] pKerOutArgs           : Pointer to structure holding output Arguments
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - Memory pointed by \a pIn, \a pNonInterleavedData, \a pFiltCoeffs, \a pFilterRembuf, \a pOut shall NOT be shared.
 *      Library code is using the restrict keyword to optimize the kernel. So any of these pointer should not point to
 *      the same memory region at any time.
 *    - When the input data format is @c AUDIOLIB_DATA_FORMAT_INTERLEAVED, pIn must be aligned to the size of 64-bytes
 * and the pNonInterleavedData must be aligned to the size of circular buffer in bytes.
 *    - When the input data format is @c AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED, pIn must be aligned to the size of
 * circular buffer in bytes and pNonInterleavedData is unsed.
 *    - If this alignment is not set properly streming engine circular buffer will not work and the algorithm will
 * break!!!
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - The input, output and intermediate data buffers are expected to be in L2 memory
 *    - The buffer pointers are assumed to be 64-byte aligned unless it's a circular buffer which needs and alignment
 * equal to the size of the circular buffer in bytes.
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_asrc_init,
 *               @ref AUDIOLIB_asrc_set in @ref AUDIOLIB_ASRC_MODE_RESET mode and
 *               @ref AUDIOLIB_asrc_exec_checkParams functions.
 *               This ensures resource configuration and error checks are done
 * only once for several invocations of this function.
 */
AUDIOLIB_STATUS
AUDIOLIB_asrc_exec(AUDIOLIB_kernelHandle handle,
                   void *restrict pIn,
                   void *restrict pNonInterleavedData,
                   void *restrict pFiltCoeffs,
                   void *restrict pFilterRembuf,
                   void *restrict pOut,
                   const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                   AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_asrc_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_asrc_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_asrc_exec is called.
 *
 *  @param [in]  handle                : Active handle to the kernel
 *  @param [in]  pIn                   : Pointer to buffer holding the input data
 *  @param [in]  pNonInterleavedData   : Pointer to buffer holding the intermediate non interleaved data
 *  @param [in]  pFiltCoeffs           : Pointer to buffer holding polyphase filter cefficients.
 *  @param [in]  pFilterRembuf         : Pointer to buffer holding the filter remaining buffer
 *  @param [out] pOut                  : Pointer to buffer holding the output matrix
 *  @param [in]  pKerInArgs            : Pointer to structure holding input Arguments
 *  @param [out] pKerOutArgs           : Pointer to structure holding output Arguments
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS AUDIOLIB_asrc_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                               const void *restrict pIn,
                                               const void *restrict pNonInterleavedData,
                                               const void *restrict pFiltCoeffs,
                                               const void *restrict pFilterRembuf,
                                               const void *restrict pOut,
                                               const AUDIOLIB_asrc_ExecInArgs  *pKerInArgs,
                                               const AUDIOLIB_asrc_ExecOutArgs *pKerOutArgs);

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
void AUDIOLIB_asrc_perfEst(AUDIOLIB_kernelHandle         handle,
                           const AUDIOLIB_bufParams2D_t *bufParamsIn,
                           const AUDIOLIB_bufParams2D_t *bufParamsOut,
                           uint64_t                     *archCycles,
                           uint64_t                     *estCycles);

/**
 * @brief        Converts an enum sample rate to an integer sample rate.
 *
 * @param [in]    rate The enum sample rate to convert.
 *
 * @return        The integer sample rate, or 0 if the enum value is invalid.
 */
uint32_t convertSampleRateToInt(sample_rate_t rate);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_H_ */
