// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_GAINOFFSET_IXX_IXX_OXX_H_
#define AUDIOLIB_GAINOFFSET_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_gainOffset AUDIOLIB_gainOffset
 * @brief Kernel applying a scalar gain followed by a scalar offset to multichannel audio.
 *
 * @details
 * The following datatype combinations are supported
 *
 *  | Case    | Input   | Output  | `*pGain`, `*pOffset` |
 *  |  ----:  | :----:  | :----   | :--                  |
 *  |    1    | float   | float   | float                |
 *  |    2    | double  | double  | float                |
 *  |    3    | int32   | int32   | int32_t              |
 *
 * @par Method
 * \f[
 *   Y[i][j] = g \cdot X[i][j] + c
 * \f]
 *
 * Where:
 * - \f$X\f$ = input 2D buffer (dimY rows of dimX samples)
 * - \f$Y\f$ = output 2D buffer of the same dimensions
 * - \f$g\f$ = scalar gain, the same for every channel and sample
 * - \f$c\f$ = scalar offset, the same for every channel and sample
 *
 * @par int32 data
 * \f[
 *   Y[i][j] = \mathrm{sat}_{32}\left( g \cdot X[i][j] + c \right)
 * \f]
 * The gain and offset are plain integers (no fractional bits), as in an integer scale-and-offset
 * block. The product and the sum are formed exactly in 64 bits (\f$|g \cdot X| \le 2^{62}\f$) and
 * clamped **once** to [INT32_MIN, INT32_MAX], so an overflow clips at full scale instead of
 * wrapping to the opposite sign (a full-scale jump, heard as a click on audio). The natural C and
 * the optimized variants are bit-exact. Example: \f$g = 2\f$, \f$c = 0\f$,
 * \f$X = 2^{30}\f$ gives \f$2^{31}\f$, which saturates to INT32_MAX.
 *
 * ---
 * **Example (gain = 2.0, offset = 0.5):**
 *
 * \f[
 *   X =
 *   \begin{bmatrix}
 *     1 & 2 & 3 \\
 *     4 & 5 & 6 \\
 *     7 & 8 & 9
 *   \end{bmatrix}
 *   \quad \xrightarrow{\;g = 2.0,\; c = 0.5\;}
 *   Y =
 *   \begin{bmatrix}
 *     2.5 & 4.5 & 6.5 \\
 *     8.5 & 10.5 & 12.5 \\
 *     14.5 & 16.5 & 18.5
 *   \end{bmatrix}
 * \f]
 *
 * The kernel is stateless, so the layout of channels and samples within the buffer does
 * not matter: interleaved and non-interleaved data are processed identically.
 *
 * ### Implementation Details
 *
 * The optimized C7x variant streams the input through SE0 and writes the output through
 * SA0, one multiply-add per vector. When neither buffer has row padding
 * (stride equal to dimX) the whole buffer is streamed as one 1D block; otherwise the
 * streaming engine walks it as a 2D block and masks the partial vector at the end of
 * every row with store predication.
 *
 * For int32 data the streaming engine sign-extends every sample to 64 bits while reading
 * (SE promotion), so each vector of 64-bit lanes is multiplied by the gain (exact 64-bit
 * product), the offset is added, the lanes are saturated to the int32 range, and the store
 * keeps the low 32 bits of each lane (packing store). No permute is needed, so the loop
 * runs at one vector of 64-bit lanes per cycle.
 *
 * @par Preconditions
 *  - Input and output buffers must have the same data type and dimensions.
 *  - Strides must be a whole number of elements and at least dimX elements.
 *  - @c pIn and @c pOut must not overlap; they are declared @c restrict.
 *
 * @ingroup AUDIOLIB
 */

/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 *
 * @c pGain and @c pOffset point to one value each:
 *
 *  | Buffer data type        | @c pGain / @c pOffset point to |
 *  | :--                     | :--                            |
 *  | @ref AUDIOLIB_FLOAT32   | @c float                       |
 *  | @ref AUDIOLIB_FLOAT64   | @c float (a float gain is far below audible error on double data) |
 *  | @ref AUDIOLIB_INT32     | @c int32_t (plain integer, no fractional bits) |
 *
 * The values are read and copied into the handle by @ref AUDIOLIB_gainOffset_init, so the
 * variables they point to need not outlive the call.
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t      funcStyle;
   const void *pGain;   /**< Scalar gain \f$ g \f$ applied to every sample, of the buffer data type. */
   const void *pOffset; /**< Scalar offset \f$ c \f$ added after the gain, of the buffer data type. */
} AUDIOLIB_gainOffset_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *                                (not read; the handle size is fixed)
 *  @return       Size of the buffer in bytes: the private arguments plus the gain and offset
 *                values, each region rounded up to AUDIOLIB_L2DATA_ALIGNMENT (64 bytes)
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 *                The buffer must be allocated with at least AUDIOLIB_L2DATA_ALIGNMENT
 *                (64-byte) alignment, so the gain and offset regions inside it are aligned.
 */
int32_t AUDIOLIB_gainOffset_getHandleSize(AUDIOLIB_gainOffset_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_gainOffset_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *   - @ref AUDIOLIB_ERR_INVALID_TYPE   if the data type is not float, double or int32
 *   - @ref AUDIOLIB_ERR_INVALID_VALUE  if @c funcStyle is not a supported variant
 *
 *  @remarks     Application is expected to provide a valid handle and to have called
 *               @ref AUDIOLIB_gainOffset_init_checkParams with the same arguments.
 */
AUDIOLIB_STATUS AUDIOLIB_gainOffset_init(AUDIOLIB_kernelHandle               handle,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                         const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gainOffset_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gainOffset_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gainOffset_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *   - @ref AUDIOLIB_ERR_NULL_POINTER       if any argument, @c pKerInitArgs->pGain or @c pKerInitArgs->pOffset is NULL
 *   - @ref AUDIOLIB_ERR_INVALID_TYPE       if the data type is not float, double or int32, or input
 *                                          and output types differ
 *   - @ref AUDIOLIB_ERR_INVALID_DIMENSION  if input and output dimensions differ
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_gainOffset_init_checkParams(AUDIOLIB_kernelHandle               handle,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                     const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_gainOffset_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_gainOffset_exec, and this function
 *               must be called before the
 *               @ref AUDIOLIB_gainOffset_exec is called.
 *
 *  @param [in]  handle    :  Active handle to the kernel
 *  @param [in]  pIn       :  Pointer to the input buffer
 *  @param [in]  pOut      :  Pointer to the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *   - @ref AUDIOLIB_ERR_NULL_POINTER  if any argument is NULL
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Applies \f$ Y = g \cdot X + c \f$ element-wise. Please refer to
 *               details under @ref AUDIOLIB_gainOffset
 *
 *  @param [in]  handle     : Active handle to the kernel
 *  @param [in]  pIn        : Pointer to buffer holding the input data
 *  @param [out] pOut       : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 * @par Memory Requirements
 * | Buffer   | dimY | dimX | Comments |
 * | :--      | :--: | :--: | :-----|
 * | pIn      | M    | N    | 2D Input Buffer; stride from @c bufParamsIn  |
 * | pOut     | M    | N    | 2D Output Buffer; stride from @c bufParamsOut |
 *
 *  @par Assumptions:
 *    - @c pIn and @c pOut do not overlap
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *    - buffers without row padding (stride equal to dimX) are streamed as one
 *      block and avoid a partial vector per row
 *
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_gainOffset_init and
 *               @ref AUDIOLIB_gainOffset_exec_checkParams. This ensures resource
 *               configuration and error checks are done only once for several
 *               invocations of this function.
 */
AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 *  @brief        This function is called to calculate the arch cycles and
 *                estimate cycles of the loop used in the execution kernel.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [out] archCycles     :  Cycles of the compute loop only
 *  @param [out] estCycles      :  Estimated total cycles, including setup and teardown
 *
 *  @return      Void.
 *
 *  @remarks     Valid only after @ref AUDIOLIB_gainOffset_init with the optimized
 *               variant; for the natural C variant the loop count is reported as zero.
 */
void AUDIOLIB_gainOffset_perfEst(AUDIOLIB_kernelHandle handle, uint64_t *archCycles, uint64_t *estCycles);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_GAINOFFSET_IXX_IXX_OXX_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainOffset.h                                     */
/* ======================================================================== */
