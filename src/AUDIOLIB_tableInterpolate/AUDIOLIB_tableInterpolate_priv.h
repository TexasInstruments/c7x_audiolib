// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_TABLEINTERPOLATE_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_TABLEINTERPOLATE_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_tableInterpolate.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_tableInterpolate_PrivArgs structure.
 *
 */
#define AUDIOLIB_TABLEINTERPOLATE_IXX_IXX_OXX_PBLOCK_SIZE (1 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE)
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SE_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_tableInterpolate_exec_ci
 *         and @ref AUDIOLIB_tableInterpolate_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_tableInterpolate_exec)(AUDIOLIB_kernelHandle handle,
                                                              void *restrict pIn0,
                                                              void *restrict pIn1,
                                                              void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_tableInterpolate_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_tableInterpolate_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_tableInterpolate_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_tableInterpolate_exec_ci does not lose cycles
 *          to determine the hardware configuration.
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
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_init_ci(AUDIOLIB_kernelHandle                     handle,
                                                  const AUDIOLIB_bufParams1D_t             *bufParamsIn0,
                                                  const AUDIOLIB_bufParams1D_t             *bufParamsIn1,
                                                  const AUDIOLIB_bufParams1D_t             *bufParamsOut,
                                                  const AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_tableInterpolate_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_tableInterpolate_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_tableInterpolate_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_tableInterpolate_exec_ci does not lose cycles
 *          to determine the hardware configuration.
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
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_unroll_init_ci(AUDIOLIB_kernelHandle                     handle,
                                                         const AUDIOLIB_bufParams1D_t             *bufParamsIn0,
                                                         const AUDIOLIB_bufParams1D_t             *bufParamsIn1,
                                                         const AUDIOLIB_bufParams1D_t             *bufParamsOut,
                                                         const AUDIOLIB_tableInterpolate_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_tableInterpolate_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          convolution computation. Filter data is loaded into B panel of the
 *          MMA from memory using one streaming engine, while the input data is
 *          loaded into A vectors of the MMA using the other streaming engine.
 *          Result of the compute from MMA C panel is stored into memory using
 *          a stream tableInterpolateress generator.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn0        : Pointer to buffer holding the src buffer
 *  @param [in]  pIn1        : Pointer to buffer holding the table buffer
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
AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_exec_ci(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn0,
                                                  void *restrict pIn1,
                                                  void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_tableInterpolate_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          convolution computation. Filter data is loaded into B panel of the
 *          MMA from memory using one streaming engine, while the input data is
 *          loaded into A vectors of the MMA using the other streaming engine.
 *          Result of the compute from MMA C panel is stored into memory using
 *          a stream tableInterpolateress generator.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn0        : Pointer to buffer holding the src buffer
 *  @param [in]  pIn1        : Pointer to buffer holding the table buffer
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
AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_unroll_exec_ci(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn0,
                                                         void *restrict pIn1,
                                                         void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the NATC implementation of the kernel. The function
 * declaration conforms to the declaration of @ref AUDIOLIB_tableInterpolate_exec.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn0        : Pointer to buffer holding the src buffer
 *  @param [in]  pIn1        : Pointer to buffer holding the table buffer
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
extern AUDIOLIB_STATUS AUDIOLIB_tableInterpolate_exec_cn(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn0,
                                                         void *restrict pIn1,
                                                         void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_tableInterpolate_exec_cn and
    *         @ref AUDIOLIB_tableInterpolate_exec_ci.                        */
   pFxnAUDIOLIB_tableInterpolate_exec execute;
   uint32_t                           srcSamples;   /**< Number of samples in the src buffer */
   uint32_t                           tableSamples; /**< Number of samples in the table buffer */
   float                              divisor; /**< Divisor to normalize the src index to the given max and min value */
   float                              minVal;  /**< Maximum value of the sec index */
   float                              maxVal;  /**< Minimum value of the sec index */
   int32_t                            nVecs;   /*Number of vectors to be processed in the loop*/
   AUDIOLIB_tableInterpolate_InitArgs initArgs; /**< Structure holding initialization parameters  */
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_TABLEINTERPOLATE_IXX_IXX_OXX_PBLOCK_SIZE]; /*< Array to hold SE/SA template */
} AUDIOLIB_tableInterpolate_PrivArgs;

#endif /* AUDIOLIB_GAIN_IXX_IXX_OXX_PRIV_H_ */
