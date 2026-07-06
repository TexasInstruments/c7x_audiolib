// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0


#ifndef AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_H_
#define AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_downSample AUDIOLIB_downSample
 * @brief Multichannel downsampler by integer-factor decimation.
 *
 * @details
 * Decreases the sample rate by keeping every @c downSampleRatio-th sample,
 * starting at offset @c phase.  This is pure decimation -- there is NO
 * anti-aliasing / low-pass filtering (use @ref AUDIOLIB_ssrc for filtered
 * rate conversion).  Operates on @c AUDIOLIB_FLOAT32 and @c AUDIOLIB_FLOAT64
 * data.
 *
 *   downsample([1 2 3 4 5 6 7 8 9], 3)     -> [1 4 7]
 *   downsample([1 2 3 4 5 6 7 8 9], 3, 1)  -> [2 5 8]
 *
 * For each channel the kept samples are taken from input sample indices
 * phase, phase+downSampleRatio, phase+2*downSampleRatio, ...  The number of
 * output samples per channel is ceil((inSamples - phase) / downSampleRatio).
 *
 * @verbatim
 *   Decimation (downSampleRatio = 3, phase = 0), single channel:
 *
 *     index :   0    1    2    3    4    5    6    7    8
 *     input :  s0   s1   s2   s3   s4   s5   s6   s7   s8
 *               |              |              |
 *               v              v              v         (keep every 3rd)
 *     output:  s0             s3             s6
 *
 *   phase = 1 starts the pattern one sample later -> s1, s4, s7
 * @endverbatim
 *
 * **Layouts** (selected by @c isInterleave), shown for C channels x N samples:
 *
 * @verbatim
 *   planar / non-interleaved (isInterleave = 0): each row is one channel
 *
 *     ch0 | s0 s1 s2 s3 ...      memory order (row by row):
 *     ch1 | s0 s1 s2 s3 ...        ch0:s0 ch0:s1 ... ch0:sN-1
 *     ch2 | s0 s1 s2 s3 ...        ch1:s0 ch1:s1 ... ch1:sN-1 ...
 *
 *   interleaved (isInterleave = 1): each row is one time sample
 *
 *     s0  | ch0 ch1 ch2 ...     memory order (sample by sample):
 *     s1  | ch0 ch1 ch2 ...       s0:ch0 s0:ch1 ... s0:chC-1
 *     s2  | ch0 ch1 ch2 ...       s1:ch0 s1:ch1 ... s1:chC-1 ...
 *
 *   Decimation drops whole samples and keeps every channel: columns
 *   above are dropped in the planar layout, rows in the interleaved one.
 * @endverbatim
 *
 *  - planar      (0): input  (numChannels x numSamples),
 *                     output (numChannels x outSamples)
 *  - interleaved (1): input  (numSamples  x numChannels),
 *                     output (outSamples  x numChannels)
 *
 * @ingroup  AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE       */
   int8_t   funcStyle;
   /** @brief 0 = planar (channels x samples), 1 = interleaved (samples x channels) */
   uint8_t  isInterleave;
   /** @brief Decimation factor n (>= 1): keep every n-th sample                  */
   uint32_t downSampleRatio;
   /** @brief Offset of the first kept sample, 0 <= phase < downSampleRatio        */
   uint32_t phase;

} AUDIOLIB_downSample_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_downSample_getHandleSize(AUDIOLIB_downSample_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_downSample_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */
AUDIOLIB_STATUS AUDIOLIB_downSample_init(AUDIOLIB_kernelHandle            handle,
                                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                      const AUDIOLIB_downSample_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_downSample_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_downSample_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_downSample_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_downSample_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_downSample_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_downSample_exec function. This function
 *               is called with the same parameters as
 *               @ref AUDIOLIB_downSample_exec, and must be called
 *               before @ref AUDIOLIB_downSample_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the input buffer
 *  @param [out] pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */

AUDIOLIB_STATUS AUDIOLIB_downSample_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                  const void *restrict pIn,
                                                  const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_downSample_exec
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input buffer
 *  @param [out] pOut       : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Assumptions:
 *    - None
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 *  @par Memory Requirements
 *    Let O = ceil((N - phase) / downSampleRatio) be the output samples per channel.
 *
 *    Planar (non-interleaved) case
 *
 *    | Buffer   | dimY | dimX | Comments |
 *    | :--      | :--: | :--: | :-----|
 *    | pIn      | M    | N    | 2D Input Buffer (channels x samples)  |
 *    | pOut     | M    | O    | 2D Output Buffer (channels x O)       |
 *
 *    Interleaved case
 *
 *    | Buffer   | dimY | dimX | Comments |
 *    | :--      | :--: | :--: | :-----|
 *    | pIn      | N    | M    | 2D Input Buffer (samples x channels)  |
 *    | pOut     | O    | M    | 2D Output Buffer (O x channels)       |
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_downSample_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_downSample_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This function is called to calculate the architecture cycles
 *                and estimated cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [out] archCycles     :  Architecture compute cycles (loop body only)
 *  @param [out] estCycles      :  Estimated total cycles including overhead
 *
 *  @remarks     None
 */
void AUDIOLIB_downSample_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_downSample.h                   */
/* ======================================================================== */
