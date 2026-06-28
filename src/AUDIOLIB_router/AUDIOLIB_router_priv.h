// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ROUTER_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_ROUTER_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_router.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_router_PrivArgs structure.
 *
 *
 */
#define MAX_SE_PARAMS (16)
#define AUDIOLIB_ROUTER_IXX_IXX_OXX_PBLOCK_SIZE (MAX_SE_PARAMS * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE + SE_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_router_exec_ci
 *         and @ref AUDIOLIB_router_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_router_exec)(AUDIOLIB_kernelHandle handle,
                                                    void *restrict pIn,
                                                    void *restrict pOut,
                                                    void *restrict pOutScratch);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel.This function have not any impact in the
 *          template setting.
 *         @ref AUDIOLIB_router_init.
 *
 *  @param [in]  handle        :  Active handle to the kernel

 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType> AUDIOLIB_STATUS AUDIOLIB_router_init_ci(AUDIOLIB_kernelHandle handle);

/*!
*  @brief This function is the initialization function for the C7x
*         implementation of the kernel. The function declaration conforms
*         to the declaration of @ref AUDIOLIB_router_init for the interleave data.
*
* @details This function determines the configuration for the streaming engine
*          and MMA hardware resources based on the function call parameters,
*          and the configuration is saved in bufPBlock array. In the kernel
*          call sequence, @ref AUDIOLIB_router_exec_ci would be
*          called later independently by the application. When
*          @ref AUDIOLIB_router_exec_ci runs, it merely retrieves
*          the configuration from the bufPBlock and uses it to set up the
*          hardware resources. This arrangement is so that
*          @ref AUDIOLIB_router_exec_ci does not lose cycles
*          to determine the hardware configuration.
*
*  @param [in]  handle        :  Active handle to the kernel

* parameters
*
*  @return      Status value indicating success or failure. Refer to @ref
* AUDIOLIB_STATUS.
*
*/

template <typename dataType> AUDIOLIB_STATUS AUDIOLIB_router_init_interLeave_set_ci(AUDIOLIB_kernelHandle handle);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_router_init for the non interleaeve data.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_router_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_router_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_router_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel

 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType> AUDIOLIB_STATUS AUDIOLIB_router_init_set_ci(AUDIOLIB_kernelHandle handle);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_router_exec for the
 *          operation non interleave data.
 *
 * @details The function uses the streaming engine to read the selected input
 *          channels and a streaming address generator to write them, in the
 *          requested order, into the output buffer. The per-channel offset and
 *          run-length configuration prepared by @ref AUDIOLIB_router_set is
 *          retrieved from the handle.
 *
 *  @param [in]  handle             : Active handle to the kernel
 *  @param [in]  pIn                : Pointer to buffer holding the input buffer
 *  @param [out] pOut               : Pointer to buffer holding the output buffer
 *  @param [out] pOutScratch        : Pointer to buffer holding the internediate output buffer
 *
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
AUDIOLIB_STATUS AUDIOLIB_router_exec_ci(AUDIOLIB_kernelHandle handle,
                                        void *restrict pIn,
                                        void *restrict pOut,
                                        void *restrict pOutScratch);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_router_exec for the
 *          operation  interleave data.
 *
 * @details The function uses the streaming engine to read the selected input
 *          channels and a streaming address generator to write them, in the
 *          requested order, into the output buffer. The per-channel offset and
 *          run-length configuration prepared by @ref AUDIOLIB_router_set is
 *          retrieved from the handle.
 *
 *  @param [in]  handle             : Active handle to the kernel
 *  @param [in]  pIn                : Pointer to buffer holding the input buffer
 *  @param [out] pOut               : Pointer to buffer holding the output buffer
 *  @param [out] pOutScratch        : Pointer to buffer holding the internediate output buffer
 *
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
AUDIOLIB_STATUS AUDIOLIB_router_interLeave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                   void *restrict pIn,
                                                   void *restrict pOut,
                                                   void *restrict pOutScratch);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel for the non interleave data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_router_exec.
 *
 * @details
 *
 *  @param [in]  handle             : Active handle to the kernel
 *  @param [in]  pIn                : Pointer to buffer holding the input buffer
 *  @param [out] pOut               : Pointer to buffer holding the output buffer
 *  @param [out] pOutScratch        : Pointer to buffer holding the internediate output buffer
 *
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_router_exec_cn(AUDIOLIB_kernelHandle handle,
                                               void *restrict pIn,
                                               void *restrict pOut,
                                               void *restrict pOutScratch);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel for the interleave data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_router_exec.
 *
 * @details
 *
 *  @param [in]  handle             : Active handle to the kernel
 *  @param [in]  pIn                : Pointer to buffer holding the input buffer
 *  @param [out] pOut               : Pointer to buffer holding the output buffer
 *  @param [out] pOutScratch        : Pointer to buffer holding the internediate output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_routerInterLeave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pOut,
                                                         void *restrict pOutScratch);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_router_exec_cn and
    *         @ref AUDIOLIB_router_exec_ci.                        */
   pFxnAUDIOLIB_router_exec execute;

   /*! @brief Number of vector to be processed for the given input buffer */
   uint8_t   isInterleave;      /**< Checking the input data is in the interleave format or not */
   uint32_t  samples;           /**< \brief Width of buffer in X dimension in elements. */
   uint32_t  inputChannels;     /**< \brief Height of buffer in Y dimension in elements. */
   uint32_t  outputChannels;    /**< \brief Height of buffer in Y dimension in elements. */
   uint32_t  strideIn;          /**< \brief Stride in Y dimension in bytes of input data. */
   uint32_t  strideOut;         /**< \brief Stride in Y dimension in bytes of output data. */
   uint32_t  data_type;         /**< \brief Data type of the input buffer. */
   uint32_t  runCount;          /**< \brief Number of runs to be processed. */
   uint32_t  seOpenOffset1;     /**< \brief offset of the se1 for the first loop*/
   uint32_t  seOpenOffset2;     /**< \brief offset of the se1 for the second loop*/
   uint32_t  sVecs;             /**< \brief Number of vector in the second transpose. */
   uint32_t *nVecs;             /**< \brief Number of vector in the first transpose. */
   uint32_t *outputOffsetArray; /**< \brief Output offset array */
   int32_t  *muteArray;         /**< \brief Mute array to store the gain values */
   uint32_t *runOffsetArray;    /**< \brief Run offset array to store the consecutive run offset values */
   uint32_t *runLengthArray;    /**< \brief Run length array to store the consecutive run length values */
   uint32_t *runMuteArray;      /**< \brief Run mute array to store the consecutive run mute values */

   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_ROUTER_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_router_PrivArgs;

#endif /* AUDIOLIB_ROUTER_IXX_IXX_OXX_PRIV_H_ */
