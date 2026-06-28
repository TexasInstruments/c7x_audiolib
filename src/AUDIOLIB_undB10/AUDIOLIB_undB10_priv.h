// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_UNDB10_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_UNDB10_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_undB10.h"

/**
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_undB10_PrivArgs structure.
 *
 */

#define AUDIOLIB_UNDB10_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 1 * AUDIOLIB_PARAM_SIZE + 2 * sizeof(int32_t))
/**
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_undB10_exec_ci
 *         and @ref AUDIOLIB_undB10_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_undB10_exec)(AUDIOLIB_kernelHandle handle,
                                                    void *restrict pIn,
                                                    void *restrict pOut);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_undB10_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_undB10_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_undB10_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_undB10_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_undB10_init_ci(AUDIOLIB_kernelHandle           handle,
                                               const AUDIOLIB_bufParams2D_t   *bufParamsIn,
                                               const AUDIOLIB_bufParams2D_t   *bufParamsOut,
                                               const AUDIOLIB_undB10_InitArgs *pKerInitArgs);

#if defined(__C7524__)
/**
 * @brief Executes the vectorized C7x implementation of the undB10 kernel.
 *        Only available for __C7524__ target.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */

template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_undB10_vector_sp_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
#endif

/**
 * @brief Executes the undB10 scaler kernel for 1x1 input/output using the specified data type.
 *
 * This function applies the undB10 scaling operation on the input data and writes the result to the output buffer.
 *
 * @tparam dataType The data type of the input and output buffers.
 * @param handle Kernel handle for managing the scaler operation.
 * @param pIn Pointer to the input buffer (must be compatible with dataType).
 * @param pOut Pointer to the output buffer (must be compatible with dataType).
 * @return AUDIOLIB_STATUS Status of the scaler execution.
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_undB10_scaler_1x1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Executes the scalar Mx1 C7x implementation of the undB10 kernel.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */
template <typename dataType>
AUDIOLIB_STATUS
AUDIOLIB_undB10_scaler_Mx1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
#if defined(__C7504__)
/**
 * @brief Executes the 1D vectorized C7x implementation of the undB10 scaler kernel.
 *        Only available for __C7504__ target.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */

template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_undB10_scaler_sp_1xN_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Executes the 2D vectorized C7x implementation of the undB10 scaler kernel.
 *        Only available for __C7504__ target.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_undB10_scaler_sp_MxN_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
#endif

/**
 *  @brief This function is the initialization function for the natural C
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_undB10_init.
 *
 * @details
 *
 *  @param [in]  handle                 :  Active handle to the kernel
 *  @param [in]  bufParamsIn            :  Pointer to the structure containing
 * dimensional information of input buffer
 *  @param [out] bufParamsOut           :  Pointer to the structure containing
 * dimensional information of ouput buffer
 *  @param [in]  pKerInitArgs           :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
AUDIOLIB_STATUS
AUDIOLIB_undB10_init_cn(AUDIOLIB_kernelHandle   handle,
                        AUDIOLIB_bufParams2D_t *bufParamsIn,
                        AUDIOLIB_bufParams2D_t *bufParamsOut);
/**
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_undB10_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_undB10_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /** @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_undB10_exec_cn and
    *         @ref AUDIOLIB_undB10_exec_ci.                        */
   pFxnAUDIOLIB_undB10_exec execute;
   /** @brief Size of input buffer for different batches
    *         @ref AUDIOLIB_undB10_init that will be retrieved
    *         and used by @ref AUDIOLIB_undB10_exec                */

   /*! @brief Number of vector to be processed for the given input buffer */
   uint32_t nVecs;

   uint32_t dim_x;     /**< \brief Width of buffer in X dimension in elements. */
   uint32_t dim_y;     /**< \brief Height of buffer in Y dimension in elements. */
   uint32_t inStride;  /**< \brief Stride in X dimension in bytes. */
   uint32_t outStride; /**< \brief Stride in Y dimension in bytes. */
   uint32_t dataType;  /**< \brief Data type of the buffer elements. */

   uint8_t bufPblock[AUDIOLIB_UNDB10_IXX_IXX_OXX_PBLOCK_SIZE];
} AUDIOLIB_undB10_PrivArgs;

#endif /* AUDIOLIB_UNDB10_IXX_IXX_OXX_PRIV_H_ */
