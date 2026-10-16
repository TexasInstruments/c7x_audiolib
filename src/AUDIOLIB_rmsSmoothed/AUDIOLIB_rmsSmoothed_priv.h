// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_rmsSmoothed.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_rmsSmoothed_PrivArgs structure.
 *
 */
#define AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE)
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SE1_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_rmsSmoothed_exec_ci and
 *         @ref AUDIOLIB_rmsSmoothed_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_rmsSmoothed_exec)(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_rmsSmoothed_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          based on the function call parameters, and the configuration is
 *          saved in bufPBlock array. In the kernel call sequence,
 *          @ref AUDIOLIB_rmsSmoothed_exec_ci would be called later
 *          independently by the application. When
 *          @ref AUDIOLIB_rmsSmoothed_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_rmsSmoothed_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of src buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_init_ci(AUDIOLIB_kernelHandle                handle,
                                             const AUDIOLIB_bufParams2D_t        *bufParamsIn,
                                             const AUDIOLIB_bufParams1D_t        *bufParamsOut,
                                             const AUDIOLIB_rmsSmoothed_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_rmsSmoothed_exec.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the src buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_rmsSmoothed_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the NATC implementation of the kernel. The function
 * declaration conforms to the declaration of @ref AUDIOLIB_rmsSmoothed_exec.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the src buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_rmsSmoothed_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_rmsSmoothed_exec_cn and
    *         @ref AUDIOLIB_rmsSmoothed_exec_ci.                                        */
   pFxnAUDIOLIB_rmsSmoothed_exec execute;
   uint32_t                      blockSize;            /**< Number of samples per channel */
   uint32_t                      numChannels;          /**< Number of input channels */
   uint32_t                      strideInElements;     /**< Stride between channels of the input, in elements */
   uint32_t                      nVecs;                /**< Iteration count of the C7x accumulation loop */
   uint32_t                      offset;               /**< Start of the SE1 stream in the input, in elements */
   float                         invNumSamples;        /**< 1 / (blockSize * numChannels) */
   float                         energyScale;          /**< C7x sum-of-squares scale (halved if SE1 re-reads SE0) */
   float                         smoothingCoefficient; /**< Smoothing coefficient (alpha) from smoothingTime */
   float                         smoothedEnergy;       /**< Smoothed energy kept across calls, zero after init */
   AUDIOLIB_rmsSmoothed_InitArgs initArgs;             /**< Structure holding initialization parameters  */
   AUDIOLIB_rmsSmoothed_SetArgs  setArgs;              /**< Structure holding the last-set runtime parameters */
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_PBLOCK_SIZE]; /**< Array to hold SE/SA template */
} AUDIOLIB_rmsSmoothed_PrivArgs;

#endif /* AUDIOLIB_RMSSMOOTHED_IXX_IXX_OXX_PRIV_H_ */
