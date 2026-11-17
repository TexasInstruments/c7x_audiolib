// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_H_
#define AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_upSample AUDIOLIB_upSample
 * @brief Multichannel integer upSampler by zero insertion (MATLAB upSample()).
 *
 * @details
 * Increases the sample rate by inserting (upFactor - 1) zeros between samples,
 * optionally offset by @c phase samples.  This is pure zero-insertion -- there
 * is NO anti-imaging / interpolation filtering (use @ref AUDIOLIB_ssrc or
 * @ref AUDIOLIB_asrc for filtered rate conversion).  Operates on
 * @c AUDIOLIB_FLOAT32 and @c AUDIOLIB_FLOAT64 data.
 *
 *   upSample([1 2 3 4], 3)     -> [1 0 0 2 0 0 3 0 0 4 0 0]
 *   upSample([1 2 3 4], 3, 1)  -> [0 1 0 0 2 0 0 3 0 0 4 0]
 *
 * For each channel the input samples are placed at output sample indices
 * phase, phase+upFactor, phase+2*upFactor, ... and all other positions are 0.
 * The number of output samples per channel is upFactor * numSamples.
 *
 * @image html AUDIOLIB_upSample_zeroInsertion.svg "Zero insertion by an integer factor (upFactor = 3)"
 * @image latex AUDIOLIB_upSample_zeroInsertion.pdf "Zero insertion by an integer factor (upFactor = 3)" width=15cm
 *
 * **Layouts** (selected by @c isInterleave):
 *  - planar      (0): input  (numChannels x numSamples),
 *                     output (numChannels x upFactor*numSamples)
 *  - interleaved (1): input  (numSamples  x numChannels),
 *                     output (upFactor*numSamples x numChannels)
 *
 * @ingroup AUDIOLIB
 */
/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   int8_t   funcStyle;    /**< Variant of the function, see @ref AUDIOLIB_FUNCTION_STYLE */
   uint32_t upFactor;     /**< Upsampling factor n (>= 1)                                */
   uint32_t phase;        /**< Sample offset of the kept samples, 0 <= phase < upFactor  */
   uint32_t isInterleave; /**< 0 = planar, 1 = interleaved                               */
} AUDIOLIB_upSample_InitArgs;

/**
 *  @brief Query the size (bytes) of the kernel handle the application must allocate.
 */
int32_t AUDIOLIB_upSample_getHandleSize(AUDIOLIB_upSample_InitArgs *pKerInitArgs);

/**
 *  @brief Validate the parameters passed to @ref AUDIOLIB_upSample_init. Must be
 *         called before @ref AUDIOLIB_upSample_init.
 */
AUDIOLIB_STATUS AUDIOLIB_upSample_init_checkParams(AUDIOLIB_kernelHandle             handle,
                                                   const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                                   const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                                   const AUDIOLIB_upSample_InitArgs *pKerInitArgs);

/**
 *  @brief One-time setup; records dimensions/strides and selects the execution
 *         variant. Results are stored in the handle.
 */
AUDIOLIB_STATUS AUDIOLIB_upSample_init(AUDIOLIB_kernelHandle             handle,
                                       AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                       AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                       const AUDIOLIB_upSample_InitArgs *pKerInitArgs);

/**
 *  @brief Validate the buffers passed to @ref AUDIOLIB_upSample_exec.
 */
AUDIOLIB_STATUS
AUDIOLIB_upSample_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 *  @brief Main compute function: writes the zero-inserted output.
 *
 *  @param [in]  handle   : Active handle to the kernel
 *  @param [in]  pIn      : Pointer to the input buffer
 *  @param [out] pOut     : Pointer to the output buffer
 *
 *  @par Performance Considerations:
 *    For best performance the input/output buffers should be in L2 and the
 *    buffer pointers 64-byte aligned.
 */
AUDIOLIB_STATUS
AUDIOLIB_upSample_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief Estimate arch/loop cycles for the configured case.
 */
void AUDIOLIB_upSample_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample.h                                        */
/* ======================================================================== */
