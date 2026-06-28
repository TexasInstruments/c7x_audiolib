// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_GAINNCH_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_GAINNCH_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_gainNCh.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_gainNCh_PrivArgs structure.
 *
 */
#define AUDIOLIB_GAINNCH_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE + SA_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_gainNCh_exec_ci
 *         and @ref AUDIOLIB_gainNCh_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_gainNCh_exec)(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pIn,
                                                     void *restrict pGain,
                                                     void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainNCh_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_gainNCh_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_gainNCh_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_gainNCh_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsGain :  Pointer to the structure containing dimensional
 *                                information of gain buffer
 *  @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of ouput buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainNCh_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_gainNCh_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainNCh_exec for the
 *          operation non interleave data.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          convolution computation. Filter data is loaded into B panel of the
 *          MMA from memory using one streaming engine, while the input data is
 *          loaded into A vectors of the MMA using the other streaming engine.
 *          Result of the compute from MMA C panel is stored into memory using
 *          a stream  generator.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [in]  pGain       : Pointer to buffer holding the gain buffer
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
AUDIOLIB_gainNCh_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel for the non interleave data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainNCh_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [in]  pGain       : Pointer to buffer holding the gain buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_gainNCh_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel for the interleave data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainNCh_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [in]  pGain       : Pointer to buffer holding the gain buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_gainNChInterLeave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pGain,
                                                          void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_gainNCh_exec_cn and
    *         @ref AUDIOLIB_gainNCh_exec_ci.                        */
   pFxnAUDIOLIB_gainNCh_exec execute;

   /*! @brief Number of vector to be processed for the given input buffer */
   uint32_t nVecs;
   uint8_t  isInterleave;         /**< Checking the input data is in the interleave format or not */
   uint32_t dim_x;                /**< \brief Width of buffer in X dimension in elements. */
   uint32_t dim_y;                /**< \brief Height of buffer in Y dimension in elements. */
   uint32_t strideIn;             /**< \brief Stride in Y dimension in bytes of input data. */
   uint32_t strideOut;            /**< \brief Stride in Y dimension in bytes of output data. */
   bool     customImplementation; /**< \brief specifies whether to use custom implementation */

   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_GAINNCH_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_gainNCh_PrivArgs;

#endif /* AUDIOLIB_GAINNCH_IXX_IXX_OXX_PRIV_H_ */
