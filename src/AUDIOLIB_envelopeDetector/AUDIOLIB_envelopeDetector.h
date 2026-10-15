// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ENVELOPEDETECTOR_H_
#define AUDIOLIB_ENVELOPEDETECTOR_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_envelopeDetector AUDIOLIB_envelopeDetector
 * @brief  Multi-channel envelope detector with separate attack and release time
 *         constants, for use as the level detector of an automatic gain control.
 * @details
 * The following datatype combinations are supported
 *
 *  | Case    | Input   | Output  |
 *  |  ----:  | :----:  | :----   |
 *  |    1    | float   | float   |
 *  |    2    | double  | double  |
 *
 * @par Method
 * The attack and release coefficients are derived in
 * @ref AUDIOLIB_envelopeDetector_set, from the sampling rate \f$ f_s \f$ and the
 * time constants \f$ T_a \f$ and \f$ T_r \f$:
 * \f[
 * \alpha_a = 1 - e^{-1 / (f_s T_a)} \qquad \alpha_r = 1 - e^{-1 / (f_s T_r)}
 * \f]
 *
 * For every channel, each sample then updates that channel's envelope \f$ e \f$ and
 * the updated envelope is the output:
 * \f[
 * \alpha[n] = \begin{cases} \alpha_a & \text{if } |x[n]| > e[n-1] \\
 *                           \alpha_r & \text{otherwise} \end{cases}
 * \qquad
 * e[n] = (1 - \alpha[n])\, e[n-1] + \alpha[n]\, |x[n]| \qquad y[n] = e[n]
 * \f]
 *
 * The envelope therefore rises toward a louder input with the attack time constant
 * and decays toward a quieter one with the release time constant. Each channel
 * keeps its own envelope, which starts at zero after
 * @ref AUDIOLIB_envelopeDetector_init and is carried from one
 * @ref AUDIOLIB_envelopeDetector_exec call to the next, so a signal may be
 * processed in consecutive frames.
 *
 * The coefficients are computed in single precision, and the envelope is kept in
 * single precision between calls, for both data types. Within a call the double
 * precision variant updates the envelope in double precision.
 * 
 * Two variants of the optimized kernel:
 *
 * The envelope of a channel is a serial recurrence, so several independent channels are updated
 * together in one sample loop to fill the cycles each channel waits on its own previous result.
 * Every stream vector is one input sample broadcast to all lanes; the matching address generator
 * addresses the single output sample it produces.
 *
 * Group-of-four variant: one pass. Each sample loop updates four channels and stores
 * envelope directly.
 *
 * Group-of-eight variant: one pass. Each sample loop updates eight channels and stores
 * envelope directly.
 *
 * Leftover channels
 *
 * In both variants the channels are split into numChannelGroups full groups, followed by one to
 * four (group of four) or one to eight (group of eight) leftover channels. The leftover channels
 * are processed as one more full group whose rows are laid out as nested steps: loop channel
 * k = b0 + 2 b1 + 4 b2 reads row b0 * step1 + b1 * step2 + b2 * step3 (step3 only for the group
 * of eight).
 *
 *    leftover channels | step1 | step2 | step3 | rows read by loop channels 0..7
 *    ------------------+-------+-------+-------+---------------------------------
 *            8         |   1   |   2   |   4   |  a b c d e f g h
 *            7         |   1   |   2   |   3   |  a b c d d e f g
 *            6         |   1   |   2   |   2   |  a b c d c d e f
 *            5         |   1   |   2   |   1   |  a b c d b c d e
 *            4         |   1   |   2   |   0   |  a b c d a b c d
 *            3         |   1   |   1   |   0   |  a b b c a b b c
 *            2         |   1   |   0   |   0   |  a b a b a b a b
 *            1         |   0   |   0   |   0   |  a a a a a a a a
 *
 * The group of four uses the first four columns of the rows with up to four leftover channels.
 * A repeated row starts from the same state and reads the same input, so it produces the same
 * output and state and writes them to the same place. The leftover group is therefore never
 * empty, and no channel count needs its own code path.
 *
 *
 * @par Preconditions
 *  - At most 64 channels. The envelope state is a fixed-size array in the handle, and
 *    @ref AUDIOLIB_envelopeDetector_init_checkParams rejects a larger @c dim_y.
 *  - Input and output buffers must not overlap; both are declared @c restrict and
 *    in-place operation is not supported.
 *  - @c stride_y of each buffer must be at least @c dim_x elements, which
 *    @ref AUDIOLIB_envelopeDetector_init_checkParams checks, and a multiple of the
 *    element size, which is not checked.
 *
 * @ingroup  AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function, refer to @ref AUDIOLIB_FUNCTION_STYLE */
   int8_t funcStyle;
   /** @brief Sampling rate of the input in Hz. Must be greater than 0. */
   int32_t samplingRate;
} AUDIOLIB_envelopeDetector_InitArgs;

/**
 * @brief Structure containing the parameters to set the kernel
 */
typedef struct {
   /** @brief Attack time constant in seconds. Must be greater than 0. */
   float attackTime;
   /** @brief Release time constant in seconds. Must be greater than 0. */
   float releaseTime;
} AUDIOLIB_envelopeDetector_SetArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters.
 *                                Not read; the handle size is fixed.
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_envelopeDetector_getHandleSize(AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_envelopeDetector_set and
 *               @ref AUDIOLIB_envelopeDetector_exec functions are called. It
 *               sets every channel's envelope state to zero, and configures the streaming
 *               engine and address generator for the selected variant. The
 *               results are stored in the handle.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [in]  bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of output buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 *  @remarks     The parameters are not validated here; application is expected to
 *               call @ref AUDIOLIB_envelopeDetector_init_checkParams first. Calling
 *               this function again on the same handle resets the envelope state.
 */
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_init(AUDIOLIB_kernelHandle                     handle,
                                               const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                               const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                               const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_envelopeDetector_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_envelopeDetector_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_envelopeDetector_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *               - @c AUDIOLIB_ERR_NULL_POINTER : @p handle, @p bufParamsIn,
 *                 @p bufParamsOut or @p pKerInitArgs is NULL
 *               - @c AUDIOLIB_ERR_INVALID_TYPE : the input type is neither
 *                 @c AUDIOLIB_FLOAT32 nor @c AUDIOLIB_FLOAT64, or the output type
 *                 differs from the input type
 *               - @c AUDIOLIB_ERR_INVALID_DIMENSION : @c dim_x or @c dim_y is zero,
 *                 @c dim_y exceeds 64, the output dimensions differ from the
 *                 input dimensions, or a @c stride_y is less than @c dim_x elements
 *               - @c AUDIOLIB_ERR_INVALID_VALUE : @c samplingRate is not greater than zero
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_init_checkParams(AUDIOLIB_kernelHandle                     handle,
                                           const AUDIOLIB_bufParams2D_t             *bufParamsIn,
                                           const AUDIOLIB_bufParams2D_t             *bufParamsOut,
                                           const AUDIOLIB_envelopeDetector_InitArgs *pKerInitArgs);

/**
 * @brief Sets the attack and release time constants of the kernel.
 *
 * @param [in] handle      : Active handle to the kernel.
 * @param [in] pKerSetArgs : Pointer to the structure containing the attack and release parameters
 *
 * @details This function validates the parameters, stores a copy of them in the
 *          handle and derives the attack and release coefficients used by
 *          @ref AUDIOLIB_envelopeDetector_exec. The envelope state of the channels
 *          is not reset, so the time constants can be changed between calls of
 *          @ref AUDIOLIB_envelopeDetector_exec.
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *         - @c AUDIOLIB_ERR_NULL_POINTER : @p handle or @p pKerSetArgs is NULL
 *         - @c AUDIOLIB_ERR_INVALID_VALUE : @c attackTime or @c releaseTime is not
 *           greater than zero
 *
 * @remarks Must be called after @ref AUDIOLIB_envelopeDetector_init and before
 *          @ref AUDIOLIB_envelopeDetector_exec.
 */
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_set(AUDIOLIB_kernelHandle                    handle,
                                              const AUDIOLIB_envelopeDetector_SetArgs *pKerSetArgs);

/**
 * @brief Retrieves the attack and release parameters last set on the kernel.
 *
 * @param [in]  handle      : Active handle to the kernel.
 * @param [out] pKerSetArgs : Pointer to the structure where the current parameters will be stored.
 *
 * @details This function copies the parameters last passed to
 *          @ref AUDIOLIB_envelopeDetector_set into the provided `pKerSetArgs` structure.
 *
 * @return Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *         - @c AUDIOLIB_ERR_NULL_POINTER : @p handle or @p pKerSetArgs is NULL
 */
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_get(AUDIOLIB_kernelHandle              handle,
                                              AUDIOLIB_envelopeDetector_SetArgs *pKerSetArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_envelopeDetector_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_envelopeDetector_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_envelopeDetector_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the input buffer
 *  @param [in]  pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *               - @c AUDIOLIB_ERR_NULL_POINTER : @p handle, @p pIn or @p pOut is NULL
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_envelopeDetector_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                           const void *restrict  pIn,
                                           const void *restrict  pOut);

/**
 *  @brief       This function is the main kernel compute function. It processes
 *               one frame of every channel and carries each channel's envelope
 *               state over to the next call.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_envelopeDetector
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input signal (M x N)
 *  @param [out] pOut       : Pointer to buffer holding the output envelope (M x N)
 *
 *  @par Memory Requirements
 * | Buffer |  Comments                                |
 * | :----: | :--------:                               |
 * | pIn    | Input Buffer, one channel per row        |
 * | pOut   | Output Buffer, envelope of each channel  |
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *               - @c AUDIOLIB_ERR_NULL_POINTER : @p handle, @p pIn or @p pOut is NULL
 *
 *  @par Assumptions:
 *    - @p pIn and @p pOut do not overlap
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_envelopeDetector_init and
 *               @ref AUDIOLIB_envelopeDetector_set. This ensures resource
 *               configuration and error checks are done only once for several
 *               invocations of this function.
 */
AUDIOLIB_STATUS AUDIOLIB_envelopeDetector_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This function is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [out] archCycles     :  Cycles of the sample loops, modelled from the
 *                                 compiled code
 *  @param [out] estCycles      :  Cycles estimated for the whole call: the sample
 *                                 loops plus the per-channel and per-call overheads
 *
 *  @return      Void.
 *
 *  @remarks     None
 */
void AUDIOLIB_envelopeDetector_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_ENVELOPEDETECTOR_H_ */
