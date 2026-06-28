// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_CROSSFADE_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_CROSSFADE_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_crossfade.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_crossfade_PrivArgs structure.
 *
 */
#define AUDIOLIB_CROSSFADE_IXX_IXX_OXX_PBLOCK_SIZE (1 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE)
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)
#define SE_SA1_PARAM_OFFSET (SE_SA0_PARAM_OFFSET + SE_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_crossfade_exec_ci
 *         and @ref AUDIOLIB_crossfade_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_crossfade_exec)(AUDIOLIB_kernelHandle handle,
                                                       void *restrict pIn0,
                                                       void *restrict pIn1,
                                                       void *restrict pIn2,
                                                       void *restrict pIn3,
                                                       void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_crossfade_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_crossfade_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_crossfade_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_crossfade_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn0   :  Pointer to the structure containing dimensional
 *                                information of previous input buffer
 *  @param [in]  bufParamsIn1  :  Pointer to the structure containing dimensional
 *                                information of next input buffer
 *  @param [in]  bufParamsIn2  :  Pointer to the structure containing dimensional
 *                                information of cos gain buffer
 *  @param [in]  bufParamsIn3  :  Pointer to the structure containing dimensional
 *                                information of sine gain buffer
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
AUDIOLIB_STATUS AUDIOLIB_crossfade_init_ci(AUDIOLIB_kernelHandle              handle,
                                           const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                           const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                           const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                           const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                           const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                           const AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_crossfade_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_crossfade_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_crossfade_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_crossfade_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn0   :  Pointer to the structure containing dimensional
 *                                information of previous input buffer
 *  @param [in]  bufParamsIn1  :  Pointer to the structure containing dimensional
 *                                information of next input buffer
 *  @param [in]  bufParamsIn2  :  Pointer to the structure containing dimensional
 *                                information of cos gain buffer
 *  @param [in]  bufParamsIn3  :  Pointer to the structure containing dimensional
 *                                information of sine gain buffer
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
AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_init_ci(AUDIOLIB_kernelHandle              handle,
                                                     const AUDIOLIB_bufParams2D_t      *bufParamsIn0,
                                                     const AUDIOLIB_bufParams2D_t      *bufParamsIn1,
                                                     const AUDIOLIB_bufParams1D_t      *bufParamsIn2,
                                                     const AUDIOLIB_bufParams1D_t      *bufParamsIn3,
                                                     const AUDIOLIB_bufParams2D_t      *bufParamsOut,
                                                     const AUDIOLIB_crossfade_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_crossfade_exec for the non-interleave-data.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          convolution computation. Filter data is loaded into B panel of the
 *          MMA from memory using one streaming engine, while the input data is
 *          loaded into A vectors of the MMA using the other streaming engine.
 *          Result of the compute from MMA C panel is stored into memory using
 *          a stream crossfaderess generator.
 *
 *  @param [in]  handle :  Active handle to the kernel
 *
 *  @param [in]  pIn0   :  Pointer to the buffer containing dimensional
 *                                information of previous input
 *  @param [in]  pIn1   :  Pointer to the buffer containing dimensional
 *                                information of next input
 *  @param [in]  pIn2   :  Pointer to the buffer containing dimensional
 *                                information of cos gain
 *  @param [in]  pIn3   :  Pointer to the buffer containing dimensional
 *                                information of sine gain
 *  @param [out] pOut   :  Pointer to the buffer containing dimensional
 *                                information of output
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
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
AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_ci(AUDIOLIB_kernelHandle handle,
                                           void *restrict pIn0,
                                           void *restrict pIn1,
                                           void *restrict pIn2,
                                           void *restrict pIn3,
                                           void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_crossfade_exec for the interleave data.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          convolution computation. Filter data is loaded into B panel of the
 *          MMA from memory using one streaming engine, while the input data is
 *          loaded into A vectors of the MMA using the other streaming engine.
 *          Result of the compute from MMA C panel is stored into memory using
 *          a stream crossfaderess generator.
 *
 *  @param [in]  handle :  Active handle to the kernel
 *
 *  @param [in]  pIn0   :  Pointer to the buffer containing dimensional
 *                                information of previous input
 *  @param [in]  pIn1   :  Pointer to the buffer containing dimensional
 *                                information of next input
 *  @param [in]  pIn2   :  Pointer to the buffer containing dimensional
 *                                information of cos gain
 *  @param [in]  pIn3   :  Pointer to the buffer containing dimensional
 *                                information of sine gain
 *  @param [out] pOut   :  Pointer to the buffer containing dimensional
 *                                information of output
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
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
AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pIn0,
                                                     void *restrict pIn1,
                                                     void *restrict pIn2,
                                                     void *restrict pIn3,
                                                     void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the NATC implementation of the kernel. The function
 * declaration conforms to the declaration of @ref AUDIOLIB_crossfade_exec for the non inter-leave data.
 *
 *  @param [in]  handle :  Active handle to the kernel
 *
 *  @param [in]  pIn0   :  Pointer to the buffer containing dimensional
 *                                information of previous input
 *  @param [in]  pIn1   :  Pointer to the buffer containing dimensional
 *                                information of next input
 *  @param [in]  pIn2   :  Pointer to the buffer containing dimensional
 *                                information of cos gain
 *  @param [in]  pIn3   :  Pointer to the buffer containing dimensional
 *                                information of sine gain
 *  @param [out] pOut   :  Pointer to the buffer containing dimensional
 *                                information of output
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
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
extern AUDIOLIB_STATUS AUDIOLIB_crossfade_exec_cn(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn0,
                                                  void *restrict pIn1,
                                                  void *restrict pIn2,
                                                  void *restrict pIn3,
                                                  void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the NATC implementation of the kernel. The function
 * declaration conforms to the declaration of @ref AUDIOLIB_crossfade_exec for the inter-leave data.
 *
 *  @param [in]  handle :  Active handle to the kernel
 *
 *  @param [in]  pIn0   :  Pointer to the buffer containing dimensional
 *                                information of previous input
 *  @param [in]  pIn1   :  Pointer to the buffer containing dimensional
 *                                information of next input
 *  @param [in]  pIn2   :  Pointer to the buffer containing dimensional
 *                                information of cos gain
 *  @param [in]  pIn3   :  Pointer to the buffer containing dimensional
 *                                information of sine gain
 *  @param [out] pOut   :  Pointer to the buffer containing dimensional
 *                                information of output
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
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
extern AUDIOLIB_STATUS AUDIOLIB_crossfadeInterleave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                            void *restrict pIn0,
                                                            void *restrict pIn1,
                                                            void *restrict pIn2,
                                                            void *restrict pIn3,
                                                            void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_crossfade_exec_cn and
    *         @ref AUDIOLIB_crossfade_exec_ci.                        */
   pFxnAUDIOLIB_crossfade_exec execute;
   int32_t                     samples;           /**< Number of samples in the input buffer */
   int32_t                     channels;          /**< Number of channels in the input buffer */
   int32_t                     strideInElements;  /**< Stride in Y dimension for input*/
   int32_t                     strideOutElements; /**<Stride in Y dimension for output */
   uint8_t                     isInterleave;      /**Differerntiate between interleave and non-interleave format */
   int32_t                     nVecs;             /*number of vectors processed with in the loop*/
   AUDIOLIB_crossfade_InitArgs initArgs;          /**< Structure holding initialization parameters  */
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_CROSSFADE_IXX_IXX_OXX_PBLOCK_SIZE]; /*< Array to hold SE/SA template */
} AUDIOLIB_crossfade_PrivArgs;

#endif /* AUDIOLIB_GAIN_IXX_IXX_OXX_PRIV_H_ */
