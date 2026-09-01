// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_TABLEINTERPOLATE_IXX_IXX_OXX_H_
#define AUDIOLIB_TABLEINTERPOLATE_IXX_IXX_OXX_H_

#include "../common/AUDIOLIB_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup AUDIOLIB_tableInterpolate AUDIOLIB_tableInterpolate
 * @brief Kernel for multichannel table interpolation apply
 *
 * @details
 * This kernel performs **linear interpolation** on multiple audio channels using
 * a shared lookup table. Each input sample is normalized between the minimum and
 * maximum source values and then used to linearly interpolate between adjacent
 * table entries.
 *
 * ---
 * **Mathematical Formulation**
 *
 * \f[
 * \begin{aligned}
 * \textbf{Given:} \quad
 * & \mathbf{x} = [x_0, x_1, \dots, x_{N-1}] && \text{(source samples, float only)} \\
 * & \mathbf{T} = [T_0, T_1, \dots, T_{M-1}] && \text{(lookup table with allocation } M = \text{tableSamples}+1) \\[6pt]
 * \textbf{Parameters:} \quad
 * & \text{minVal} = \min(\mathbf{x}), \quad
 *   \text{maxVal} = \max(\mathbf{x}), \quad
 *   \text{tableSamples} = M \\[8pt]
 * \textbf{Divisor:} \quad
 * & \text{divisor} = \frac{M - 1}{\text{maxVal} - \text{minVal}} \\[8pt]
 * \textbf{Normalized index:} \quad
 * & s_n = (x_n - \text{minVal}) \cdot \text{divisor} \\[8pt]
 * \textbf{Index decomposition:} \quad
 * & i_n = \lfloor s_n \rfloor, \quad f_n = s_n - i_n \\[8pt]
 * \textbf{Piecewise-linear interpolation:} \quad
 * y_n =
 * \begin{cases}
 * T_0, & s_n \le 0 \\[4pt]
 * T_{M-1}, & s_n \ge M - 1 \\[4pt]
 * T_{i_n} (1 - f_n) + T_{i_n + 1} f_n, & \text{otherwise}
 * \end{cases}
 * \\[10pt]
 * \text{for all } n \in [0, N-1]
 * \end{aligned}
 * \f]
 *
 * ---
 * **Parameter Roles**
 *
 * \f[
 * \begin{aligned}
 * \text{minVal:} & \quad \text{Minimum input value; samples } x_n \le \text{minVal} \mapsto T_0 \\[4pt]
 * \text{maxVal:} & \quad \text{Maximum input value; samples } x_n \ge \text{maxVal} \mapsto T_{M-1} \\[4pt]
 * \text{divisor:} & \quad \frac{M - 1}{\text{maxVal} - \text{minVal}}, \text{ scales the input range } [\text{minVal},
 * \text{maxVal}] \to [0, M-1] \\[4pt] f_n: & \quad \text{Fractional index used for interpolation between } T_{i_n}
 * \text{ and } T_{i_n + 1} \\[4pt] \text{Data type:} & \quad \text{Only float is supported} \\[2pt] \text{Table
 * allocation:} & \quad \text{Allocate } M = \text{tableSamples}+1 \text{ to avoid memory access issues.} \end{aligned}
 * \f]
 *
 * ---
 * **Initialization and Configuration**
 *
 * \f[
 * \begin{aligned}
 * \text{Default table size: } & 8~\text{KB} \\[4pt]
 * \text{User-configurable parameter: } & \text{tableInterpolateSize} \\[4pt]
 * \text{Runtime check: } &
 * \begin{cases}
 * \text{If } \text{tableSamples} > \text{tableInterpolateSize}, &
 * \text{use } AUDIOLIB\_tableInterpolate\_unroll\_exec\_ci \\
 * \text{otherwise,} &
 * \text{use } AUDIOLIB\_tableInterpolate\_exec\_ci
 * \end{cases}
 * \end{aligned}
 * \f]
 *
 * ---
 * **Platform-Specific Execution**
 *
 * \f[
 * \begin{aligned}
 * \textbf{AM275:} &
 * \begin{cases}
 * \text{Default: } AUDIOLIB\_tableInterpolate\_exec\_ci, \\
 * \text{Switch to unrolled version if } \text{tableSamples} > \text{configured size.}
 * \end{cases} \\[8pt]
 * \textbf{AM62D:} &
 * \text{Always uses } AUDIOLIB\_tableInterpolate\_unroll\_exec\_ci
 * \end{aligned}
 * \f]
 *
 * ---
 * **Notes**
 *
 * \f[
 * \begin{aligned}
 * &\text{• Implements continuous, piecewise-linear interpolation.} \\[4pt]
 * &\text{• Out-of-range values are clamped: } s_n \le 0 \Rightarrow T_0, \quad s_n \ge M - 1 \Rightarrow T_{M-1}.
 * \\[4pt]
 * &\text{• Supports multi-channel operation with a shared lookup table.} \\[2pt]
 * &\text{• Supports only float data type.} \\[2pt]
 * &\text{• Lookup table allocated as } \text{tableSamples}+1 \text{ entries to avoid memory access issues.}
 * \end{aligned}
 * \f]
 *
 * @ingroup AUDIOLIB
 */
/**@{*/

/**
 * @brief Structure containing the parameters to initialize the kernel
 */
typedef struct {
   /** @brief Variant of the function refer to @ref AUDIOLIB_FUNCTION_STYLE     */
   int8_t   funcStyle;
   float    minVal;               /**< Minimum input value corresponding to the first table entry. */
   float    maxVal;               /**< Maximum input value corresponding to the last table entry. */
   uint32_t tableInterpolateSize; /**< Number of entries in the interpolation table. */

} AUDIOLIB_tableInterpolate_InitArgs;

/**
 *  @brief        This is a query function to calculate the size of internal
 *                handle
 *  @param [in]   pKerInitArgs  : Pointer to structure holding init parameters
 *  @return       Size of the buffer in bytes
 *  @remarks      Application is expected to allocate buffer of the requested
 *                size and provide it as input to other functions requiring it.
 */
int32_t AUDIOLIB_tableInterpolate_getHandleSize(AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs);

/**
 *  @brief       This function should be called before the
 *               @ref AUDIOLIB_tableInterpolate_exec function is called. This
 *               function takes care of any one-time operations such as setting
 *               up the configuration of required hardware resources such as
 *               the streaming engine.  The results of these
 *               operations are stored in the handle.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn0  :  Pointer to the structure containing dimensional
                                  information of src buffer
 *  @param [in]  bufParamsIn1  :  Pointer to the structure containing dimensional
 *                                information of table buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of ouput buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     Application is expected to provide a valid handle.
 */

AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_init(AUDIOLIB_kernelHandle                     handle,
                                               const AUDIOLIB_bufParams1D_t             *bufParamsIn0,
                                               const AUDIOLIB_bufParams1D_t             *bufParamsIn1,
                                               const AUDIOLIB_bufParams1D_t             *bufParamsOut,
                                               const AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_tableInterpolate_init function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_tableInterpolate_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_tableInterpolate_init is called.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn0  :  Pointer to the structure containing dimensional
                                  information of src buffer
 *  @param [in]  bufParamsIn1  :  Pointer to the structure containing dimensional
 *                                information of table buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of ouput buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 */
AUDIOLIB_STATUS
AUDIOLIB_tableInterpolate_init_checkParams(AUDIOLIB_kernelHandle                     handle,
                                           const AUDIOLIB_bufParams1D_t             *bufParamsIn0,
                                           const AUDIOLIB_bufParams1D_t             *bufParamsIn1,
                                           const AUDIOLIB_bufParams1D_t             *bufParamsOut,
                                           const AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs);

/**
 *  @brief       This function checks the validity of the parameters passed to
 *               @ref AUDIOLIB_tableInterpolate_exec function. This function
 *               is called with the same parameters as the
 *               @ref AUDIOLIB_tableInterpolate_init, and this function
 *               must be called before the
 *               @ref AUDIOLIB_tableInterpolate_init is called.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn0        : Pointer to buffer holding the src buffer
 *  @param [in]  pIn1        : Pointer to buffer holding the table buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @remarks     None
 * */
AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                           const void *restrict pIn0,
                                                           const void *restrict pIn1,
                                                           const void *restrict pOut);

/** @brief Set the interpolation table pointer for the C-intrinsic variant.
 *  @param [in] handle Active handle to the kernel.
 *  @param [in] pIn1   Pointer to the interpolation table buffer.
 */
void AUDIOLIB_tableInterpolate_set_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn1);

/**
 *  @brief       This function is the main kernel compute function.
 *
 *  @details     Please refer to details under
 *               @ref AUDIOLIB_tableInterpolate_exec
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn0        : Pointer to buffer holding the src buffer
 *  @param [in]  pIn1        : Pointer to buffer holding the table buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
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
 *  @remarks     Before calling this function, application is expected to call
 *               @ref AUDIOLIB_tableInterpolate_init and
 *               This ensures resource configuration and error checks are done
 *               only once for several invocations of this function.
 */

AUDIOLIB_STATUS
AUDIOLIB_tableInterpolate_exec(AUDIOLIB_kernelHandle handle,
                               void *restrict pIn0,
                               void *restrict pIn1,
                               void *restrict pOut);

/** * @brief Estimates the performance of the tableInterpolate kernel.
 *
 *  @param [in]  handle           : Active handle to the kernel
 *  @param [out] archCycles       : Pointer to store the estimated architecture cycles
 *  @param [out] estCycles        : Pointer to store the estimated cycles for execution
 *  @param [in]  data_type        : Data type of the processing input
 *  @param [in]  tableInterpolateSize  : Table lookup size set by the user
 *  @param [in]  pKerInitArgs     :  Pointer to the structure holding init
 *
 * *  @details This function provides an estimation of the performance characteristics
 *           of the tableInterpolate kernel
 * */
void AUDIOLIB_tableInterpolate_perfEst(AUDIOLIB_kernelHandle                     handle,
                                       uint64_t                                 *archCycles,
                                       uint64_t                                 *estCycles,
                                       uint32_t                                  data_type,
                                       uint32_t                                  tableInterpolateSize,
                                       const AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs);
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AUDIOLIB_TABLEINTERPOLATE_IXX_IXX_OXX_H_ */
