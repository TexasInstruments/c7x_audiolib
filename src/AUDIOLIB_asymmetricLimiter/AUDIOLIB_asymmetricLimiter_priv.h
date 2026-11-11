// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ASYMMETRICLIMITER_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_ASYMMETRICLIMITER_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_asymmetricLimiter.h"

/*!
 * \file
 * \brief Header file for kernel's internal use. For the kernel's interface,
 * please see @ref AUDIOLIB_asymmetricLimiter
 */

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_asymmetricLimiter_PrivArgs structure.
 *
 */

#define AUDIOLIB_ASYMMETRICLIMITER_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_asymmetricLimiter_exec_ci
 *         and @ref AUDIOLIB_asymmetricLimiter_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_asymmetricLimiter_exec)(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_asymmetricLimiter_init.
 *
 * @details This function determines the configuration for the streaming engines
 *          and stream generators based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_asymmetricLimiter_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_asymmetricLimiter_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_asymmetricLimiter_exec_ci does not lose cycles
 *          to determine the hardware configuration.
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
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_asymmetricLimiter_init_ci(AUDIOLIB_kernelHandle         handle,
                                      const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      const AUDIOLIB_asymmetricLimiter_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_asymmetricLimiter_exec.
 *
 * @details The function uses the C7x vector core and streaming engines.
 *          Input data is loaded into vector registers using a streaming engine,
 *          clamped against the lower and upper bounds using vector min/max
 *          instructions, and the result is written to memory using a
 *          stream generator.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
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
AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_asymmetricLimiter_exec.
 *
 * @details This function processes elements sequentially across all specified channels
 *          using the natural C implementation.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_asymmetricLimiter_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_asymmetricLimiter_exec_cn and
    *         @ref AUDIOLIB_asymmetricLimiter_exec_ci.                        */
   pFxnAUDIOLIB_asymmetricLimiter_exec execute;

   /*! @brief Number of vector to be processed for the given input buffer */
   uint32_t nVecs;

   uint32_t dim_x;     /**< \brief Width of buffer in X dimension in elements. */
   uint32_t dim_y;     /**< \brief Height of buffer in Y dimension in elements. */
   int32_t  strideIn;  /**< \brief Input stride in Y dimension in elements. */
   int32_t  strideOut; /**< \brief Output stride in Y dimension in elements. */
   AUDIOLIB_asymmetricLimiter_SetArgs setArgs; /**< A copy of the last-set run-time parameters. */

   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_ASYMMETRICLIMITER_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_asymmetricLimiter_PrivArgs;

#endif /* AUDIOLIB_ASYMMETRICLIMITER_IXX_IXX_OXX_PRIV_H_ */
