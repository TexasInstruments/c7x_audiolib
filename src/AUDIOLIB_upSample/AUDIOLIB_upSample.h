/*******************************************************************************
**+--------------------------------------------------------------------------+**
**|                            ****                                          |**
**|                            ****                                          |**
**|                            ******o***                                    |**
**|                      ********_///_****                                   |**
**|                      ***** /_//_/ ****                                   |**
**|                       ** ** (__/ ****                                    |**
**|                           *********                                      |**
**|                            ****                                          |**
**|                            ***                                           |**
**|                                                                          |**
**|        Copyright (c) 2025 Texas Instruments Incorporated                 |**
**|                                                                          |**
**|              All rights reserved not granted herein.                     |**
**|                                                                          |**
**|                         Limited License.                                 |**
**|                                                                          |**
**|  Texas Instruments Incorporated grants a world-wide, royalty-free,       |**
**|  non-exclusive license under copyrights and patents it now or            |**
**|  hereafter owns or controls to make, have made, use, import, offer to    |**
**|  sell and sell ("Utilize") this software subject to the terms herein.    |**
**|  With respect to the foregoing patent license, such license is granted   |**
**|  solely to the extent that any such patent is necessary to Utilize the   |**
**|  software alone.  The patent license shall not apply to any              |**
**|  combinations which include this software, other than combinations       |**
**|  with devices manufactured by or for TI ("TI Devices").  No hardware     |**
**|  patent is licensed hereunder.                                           |**
**|                                                                          |**
**|  Redistributions must preserve existing copyright notices and            |**
**|  reproduce this license (including the above copyright notice and the    |**
**|  disclaimer and (if applicable) source code license limitations below)   |**
**|  in the documentation and/or other materials provided with the           |**
**|  distribution                                                            |**
**|                                                                          |**
**|  Redistribution and use in binary form, without modification, are        |**
**|  permitted provided that the following conditions are met:               |**
**|                                                                          |**
**|    *  No reverse engineering, decompilation, or disassembly of this      |**
**|  software is permitted with respect to any software provided in binary   |**
**|  form.                                                                   |**
**|                                                                          |**
**|    *  any redistribution and use are licensed by TI for use only with    |**
**|  TI Devices.                                                             |**
**|                                                                          |**
**|    *  Nothing shall obligate TI to provide you with source code for      |**
**|  the software licensed and provided to you in object code.               |**
**|                                                                          |**
**|  If software source code is provided to you, modification and            |**
**|  redistribution of the source code are permitted provided that the       |**
**|  following conditions are met:                                           |**
**|                                                                          |**
**|    *  any redistribution and use of the source code, including any       |**
**|  resulting derivative works, are licensed by TI for use only with TI     |**
**|  Devices.                                                                |**
**|                                                                          |**
**|    *  any redistribution and use of any object code compiled from the    |**
**|  source code and any resulting derivative works, are licensed by TI      |**
**|  for use only with TI Devices.                                           |**
**|                                                                          |**
**|  Neither the name of Texas Instruments Incorporated nor the names of     |**
**|  its suppliers may be used to endorse or promote products derived from   |**
**|  this software without specific prior written permission.                |**
**|                                                                          |**
**|  DISCLAIMER.                                                             |**
**|                                                                          |**
**|  THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY      |**
**|  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE       |**
**|  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR      |**
**|  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TI AND TI'S LICENSORS BE      |**
**|  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR     |**
**|  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF    |**
**|  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR         |**
**|  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,   |**
**|  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE    |**
**|  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,       |**
**|  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                      |**
**+--------------------------------------------------------------------------+**
*******************************************************************************/

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
