// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_softClip.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_softClip_PrivArgs structure.
 *
 */

#define AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_softClip_exec_ci
 *         and @ref AUDIOLIB_softClip_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_softClip_exec)(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_softClip_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_softClip_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_softClip_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_softClip_exec_ci does not lose cycles
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
AUDIOLIB_STATUS AUDIOLIB_softClip_init_ci(AUDIOLIB_kernelHandle             handle,
                                          const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                          const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                          const AUDIOLIB_softClip_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_softClip_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          convolution computation. Filter data is loaded into B panel of the
 *          MMA from memory using one streaming engine, while the input data is
 *          loaded into A vectors of the MMA using the other streaming engine.
 *          Result of the compute from MMA C panel is stored into memory using
 *          a stream softClipress generator.
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
AUDIOLIB_STATUS AUDIOLIB_softClip_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_softClip_exec.
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
extern AUDIOLIB_STATUS AUDIOLIB_softClip_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/**
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_softClip_exec_cn and
    *         @ref AUDIOLIB_softClip_exec_ci.                        */
   pFxnAUDIOLIB_softClip_exec execute;

   uint32_t                   nVecs;     /**< \brief Number of vector to be processed for the given input buffer */
   uint32_t                   dim_x;     /**< \brief Width of buffer in X dimension in elements */
   uint32_t                   dim_y;     /**< \brief Height of buffer in Y dimension in elements */
   int32_t                    inStride;  /**< \brief Stride of Input buffer */
   int32_t                    outStride; /**< \brief Stride of Output buffer */
   AUDIOLIB_softClip_InitArgs initArgs;  /**< \brief Structure holding initialization parameters */
   AUDIOLIB_softClip_SetArgs  setArgs;   /**< \brief Structure to hold the set parameters */
   float   threshold; /**< \brief The amplitude where the soft clipping curve fully transitions to its maximum limit */
   float   endKnee;   /**< \brief Stride of Output buffer */
   uint8_t bufPblock[AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_PBLOCK_SIZE]; /**< \brief bufPblock array to store SE/SA template */

} AUDIOLIB_softClip_PrivArgs;

#endif /* AUDIOLIB_SOFTCLIP_IXX_IXX_OXX_PRIV_H_ */
