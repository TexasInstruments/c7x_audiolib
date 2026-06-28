// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_typeConversion.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_typeConversion_PrivArgs structure.
 *
 */
#define AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE + 1 * sizeof(uint32_t))

/*******************************************************************************
 *
 * Q-FORMAT SCALING CONSTANTS
 *
 ******************************************************************************/
static constexpr double Q15_SCALE_FACTOR     = 32768.0;            // 2^15
static constexpr double Q15_SCALE_FACTOR_INV = 1.0 / 32768.0;      // 1/2^15
static constexpr double Q23_SCALE_FACTOR     = 8388608.0;          // 2^23
static constexpr double Q23_SCALE_FACTOR_INV = 1.0 / 8388608.0;    // 1/2^23
static constexpr double Q31_SCALE_FACTOR     = 2147483648.0;       // 2^31
static constexpr double Q31_SCALE_FACTOR_INV = 1.0 / 2147483648.0; // 1/2^31

static constexpr int32_t Q15_MAX = 32767;
static constexpr int32_t Q15_MIN = -32768;
static constexpr int32_t Q23_MAX = 8388607;
static constexpr int32_t Q23_MIN = -8388608;

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of the natural and optimized type conversion functions.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_typeConversion_exec)(AUDIOLIB_kernelHandle handle,
                                                            void *restrict pIn,
                                                            void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_q15ToFloat_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_q15ToFloat_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_q15ToFloat_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
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
AUDIOLIB_STATUS AUDIOLIB_q15ToFloat_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_floatToQ15_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_floatToQ15_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_floatToQ15_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
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
AUDIOLIB_STATUS AUDIOLIB_floatToQ15_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_q23ToFloat_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_q23ToFloat_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_q23ToFloat_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
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
AUDIOLIB_STATUS AUDIOLIB_q23ToFloat_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_floatToQ23_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_floatToQ23_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_floatToQ23_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
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
AUDIOLIB_STATUS AUDIOLIB_floatToQ23_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_typeConversion_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          type conversion computation. Input data is loaded into B panel of
 *          the MMA from memory using one streaming engine, while the output
 *          data is loaded into A vectors of the MMA using the other streaming
 *          engine. Result of the compute from MMA C panel is stored into
 *          memory using a stream typeConversion generator.
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
AUDIOLIB_STATUS AUDIOLIB_q15ToFloat_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          type conversion computation. Input data is loaded into B panel of
 *          the MMA from memory using one streaming engine, while the output
 *          data is loaded into A vectors of the MMA using the other streaming
 *          engine. Result of the compute from MMA C panel is stored into
 *          memory using a stream typeConversion generator.
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
AUDIOLIB_STATUS AUDIOLIB_floatToQ15_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          type conversion computation. Input data is loaded into B panel of
 *          the MMA from memory using one streaming engine, while the output
 *          data is loaded into A vectors of the MMA using the other streaming
 *          engine. Result of the compute from MMA C panel is stored into
 *          memory using a stream typeConversion generator.
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
AUDIOLIB_STATUS AUDIOLIB_q23ToFloat_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          type conversion computation. Input data is loaded into B panel of
 *          the MMA from memory using one streaming engine, while the output
 *          data is loaded into A vectors of the MMA using the other streaming
 *          engine. Result of the compute from MMA C panel is stored into
 *          memory using a stream typeConversion generator.
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
AUDIOLIB_STATUS AUDIOLIB_q31ToFloat_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          type conversion computation. Input data is loaded into B panel of
 *          the MMA from memory using one streaming engine, while the output
 *          data is loaded into A vectors of the MMA using the other streaming
 *          engine. Result of the compute from MMA C panel is stored into
 *          memory using a stream typeConversion generator.
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
AUDIOLIB_STATUS AUDIOLIB_floatToQ23_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          type conversion computation. Input data is loaded into B panel of
 *          the MMA from memory using one streaming engine, while the output
 *          data is loaded into A vectors of the MMA using the other streaming
 *          engine. Result of the compute from MMA C panel is stored into
 *          memory using a stream typeConversion generator.
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
AUDIOLIB_STATUS AUDIOLIB_floatToQ31_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details
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
AUDIOLIB_q15ToFloat_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details
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
AUDIOLIB_floatToQ15_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details
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
AUDIOLIB_q23ToFloat_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details
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
AUDIOLIB_q31ToFloat_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details
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
AUDIOLIB_floatToQ23_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_typeConversion_exec.
 *
 * @details
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
AUDIOLIB_floatToQ31_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         natural C and optimized */
   pFxnAUDIOLIB_typeConversion_exec execute;

   /*! @brief Number of vector to be processed for the given input buffer */
   uint32_t nVecs;
   uint32_t samples;   /**< \brief Width of buffer in X dimension in elements. */
   uint32_t channels;  /**< \brief Height of buffer in Y dimension in elements. */
   uint32_t strideIn;  /**< \brief Stride in Y dimension in bytes of input data. */
   uint32_t strideOut; /**< \brief Stride in Y dimension in bytes of output data. */

   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_typeConversion_PrivArgs;

#endif /* AUDIOLIB_TYPECONVERSION_IXX_IXX_OXX_PRIV_H_ */
