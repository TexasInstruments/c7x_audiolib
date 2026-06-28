// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_DB20_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_DB20_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_dB20.h"

/**
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_dB20_PrivArgs structure.
 *
 */

#define AUDIOLIB_DB20_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 1 * AUDIOLIB_PARAM_SIZE + 2 * sizeof(int32_t))

/**
 * @brief Macro defining the maximum value representable by a 32-bit float
 *
 */

#define FLT_MAX_DB20 3.402823466E+38

/**
 * @brief Macro defining the maximum value representable by a double.
 *
 */

#define DBL_MAX_DB20 1.7976931348623157e+308

/**
 * @brief Macro defining the minimum constant value representable by a double.
 *
 */

#define CONST_DOUBLE_MIN_DB20 1.843542739450977e+19

/**
 * @brief Macro defining the maximum constant value.
 *
 */

#define CONST_MAX_DB20 308.254715974092

/**
 * @brief Macro defining the minimum constant value representable by a float.
 *
 */

#define CONST_FLOAT_MIN_DB20 4286578688

/**
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_dB20_exec_ci
 *         and @ref AUDIOLIB_dB20_exec_cn.
 */

/*
 * @brief Executes the vectorized C7x implementation of the dB20 kernel.
 *        Only available for __C7524__ target.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_dB20_exec)(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pOut);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_dB20_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_dB20_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_dB20_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_dB20_exec_ci does not lose cycles
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
extern AUDIOLIB_STATUS AUDIOLIB_dB20_init_ci(AUDIOLIB_kernelHandle         handle,
                                             const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                             const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                             const AUDIOLIB_dB20_InitArgs *pKerInitArgs);

/*
 * @brief Executes the vectorized C7x implementation of the dB20 kernel.
 *        Only available for __C7524__ target.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */
#if defined(__C7524__)
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_dB20_vector_sp_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
#endif

/**
 * @brief Executes the dB20 vector split1 kernel for the specified data type.
 *
 * This function processes input data using the dB20 vector split1 algorithm and writes the result to the output buffer.
 *
 * @tparam dataType The type of data to be processed.
 * @param handle Kernel handle used for execution context.
 * @param pIn Pointer to the input data buffer (must be properly aligned and sized).
 * @param pOut Pointer to the output data buffer (must be properly aligned and sized).
 * @return AUDIOLIB_STATUS Status code indicating success or failure of the operation.
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_dB20_vector_dp_split1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*
 * @brief Executes the split2 variant of the vectorized C7x dB20 kernel.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_dB20_vector_dp_split2_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Executes the dB20 scaler kernel for 1x1 input/output using the specified data type.
 *
 * This function applies the dB20 scaling operation on the input data and writes the result to the output buffer.
 *
 * @tparam dataType The data type of the input and output buffers.
 * @param handle Kernel handle for managing the scaler operation.
 * @param pIn Pointer to the input buffer (must be compatible with dataType).
 * @param pOut Pointer to the output buffer (must be compatible with dataType).
 * @return AUDIOLIB_STATUS Status of the scaler execution.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_dB20_scaler_1x1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Executes the scalar Mx1 C7x implementation of the dB20 kernel.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_dB20_scaler_Mx1_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
#if defined(__C7504__)
/**
 * @brief Executes the 1D vectorized C7x implementation of the dB20 scaler kernel.
 *        Only available for __C7504__ target.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */

template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_dB20_scaler_sp_1xN_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Executes the 2D vectorized C7x implementation of the dB20 scaler kernel.
 *        Only available for __C7504__ target.
 *
 * @param handle   Active handle to the kernel
 * @param pIn      Pointer to input buffer
 * @param pOut     Pointer to output buffer
 * @return         Status value indicating success or failure
 */
template <typename dataType>
extern AUDIOLIB_STATUS
AUDIOLIB_dB20_scaler_sp_MxN_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
#endif

/**
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_dB20_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_dB20_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /** @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_dB20_exec_cn and
    *         @ref AUDIOLIB_dB20_exec_ci.                        */
   pFxnAUDIOLIB_dB20_exec execute;

   /*! @brief Number of vector to be processed for the given input buffer */
   uint32_t nVecs;

   uint32_t dim_x;     /**< \brief Width of buffer in X dimension in elements. */
   uint32_t dim_y;     /**< \brief Height of buffer in Y dimension in elements. */
   uint32_t inStride;  /**< \brief Stride in X dimension in bytes. */
   uint32_t outStride; /**< \brief Stride in Y dimension in bytes. */
   uint32_t dataType;  /**< \brief Data type of the buffer elements. */

   uint8_t bufPblock[AUDIOLIB_DB20_IXX_IXX_OXX_PBLOCK_SIZE];
} AUDIOLIB_dB20_PrivArgs;

/**
 *  @brief This function is the initialization function for the natural C
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_dB20_init.
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
AUDIOLIB_dB20_init_cn(AUDIOLIB_kernelHandle   handle,
                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                      AUDIOLIB_bufParams2D_t *bufParamsOut);

#endif /* AUDIOLIB_DB20_IXX_IXX_OXX_PRIV_H_ */
