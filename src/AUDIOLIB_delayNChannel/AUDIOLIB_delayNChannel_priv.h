// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_DELAYNCHANNEL_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_DELAYNCHANNEL_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_delayNChannel.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_delayNChannel_PrivArgs structure.
 *
 */

#define AUDIOLIB_DELAYNCHANNEL_IXX_IXX_OXX_PBLOCK_SIZE (4 * SE_PARAM_SIZE + 3 * SA_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_delayNChannel_exec_ci
 *         and @ref AUDIOLIB_delayNChannel_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_delayNChannel_exec)(AUDIOLIB_kernelHandle handle,
                                                           void *restrict pIn,
                                                           void *restrict pDelay,
                                                           void *restrict pOut,
                                                           void *restrict pScratch);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_delayNChannel_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_delayNChannel_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_delayNChannel_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_delayNChannel_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  bufParamsIn    :  Pointer to the structure containing dimensional
 *                                 information of non-interleaved input buffer
 *  @param [in]  bufParamsDelay :  Pointer to the structure containing dimensional
 *                                 information of delay buffer
 *  @param [out] bufParamsOut   :  Pointer to the structure containing dimensional
 *                                 information of non-interleaved output buffer
 *  @param [in]  pKerInitArgs   :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delayNChannel_init_ci(AUDIOLIB_kernelHandle                  handle,
                                               const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                               const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                               const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                               const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_delayNChannel_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_delayNChannel_interleave_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_delayNChannel_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_delayNChannel_interleave_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle         :  Active handle to the kernel
 *  @param [in]  bufParamsIn    :  Pointer to the structure containing dimensional
 *                                 information of interleaved input buffer
 *  @param [in]  bufParamsDelay :  Pointer to the structure containing dimensional
 *                                 information of delay buffer
 *  @param [out] bufParamsOut   :  Pointer to the structure containing dimensional
 *                                 information of interleaved ouput buffer
 *  @param [in]  pKerInitArgs   :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_delayNChannel_interleave_init_ci(AUDIOLIB_kernelHandle                  handle,
                                                          const AUDIOLIB_bufParams2D_t          *bufParamsIn,
                                                          const AUDIOLIB_bufParams2D_t          *bufParamsDelay,
                                                          const AUDIOLIB_bufParams2D_t          *bufParamsOut,
                                                          const AUDIOLIB_delayNChannel_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_delayNChannel_exec.
 *
 * @details The function uses the streaming engine to read the input samples and
 *          a streaming address generator to write the delayed output. The
 *          per-channel delay-line configuration prepared by
 *          @ref AUDIOLIB_delayNChannel_init is retrieved from the handle.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the non-interleaved input buffer
 *  @param [in]  pDelay      : Pointer to buffer holding the delay
 *  @param [out] pOut        : Pointer to buffer holding the non-interleaved output buffer
 *  @param [in]  pScratch    : Pointer to buffer holding the scratch buffer
 *                             to store intermediate output in de-interleave format
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
AUDIOLIB_STATUS AUDIOLIB_delayNChannel_exec_ci(AUDIOLIB_kernelHandle handle,
                                               void *restrict pIn,
                                               void *restrict pDelay,
                                               void *restrict pOut,
                                               void *restrict pScratch);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel (if data in interleave format).
 *         The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_delayNChannel_exec.
 *
 * @details The function uses the streaming engine to read the input samples and
 *          a streaming address generator to write the delayed output. The
 *          per-channel delay-line configuration prepared by
 *          @ref AUDIOLIB_delayNChannel_init is retrieved from the handle.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the interleaved input buffer
 *  @param [in]  pDelay      : Pointer to buffer holding the delay buffer (non-interleaved)
 *  @param [out] pOut        : Pointer to buffer holding the interleaved output buffer
 *  @param [in]  pScratch    : Pointer to buffer holding the scratch buffer
 *                             to store intermediate output in de-interleave format
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
AUDIOLIB_STATUS AUDIOLIB_delayNChannel_interleave_exec_ci(AUDIOLIB_kernelHandle handle,
                                                          void *restrict pIn,
                                                          void *restrict pDelay,
                                                          void *restrict pOut,
                                                          void *restrict pScratch);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel (if data in interleave format).
 *         The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_delayNChannel_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the interleaved input buffer
 *  @param [in]  pDelay      : Pointer to buffer holding the delay buffer (non-interleaved)
 *  @param [out] pOut        : Pointer to buffer holding the interleaved output buffer
 *  @param [in]  pScratch    : Pointer to buffer holding the scratch buffer
 *                             to store intermediate output in de-interleave format
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_delayNChannel_interleave_exec_cn(AUDIOLIB_kernelHandle handle,
                                                                 void *restrict pIn,
                                                                 void *restrict pDelay,
                                                                 void *restrict pOut,
                                                                 void *restrict pScratch);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_delayNChannel_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input buffer
 *  @param [in]  pDelay      : Pointer to buffer holding the delay buffer
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *  @param [in]  pScratch    : Pointer to buffer holding the scratch buffer
 *                             to store intermediate output in de-interleave format
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_delayNChannel_exec_cn(AUDIOLIB_kernelHandle handle,
                                                      void *restrict pIn,
                                                      void *restrict pDelay,
                                                      void *restrict pOut,
                                                      void *restrict pScratch);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_delayNChannel_exec_cn and
    *         @ref AUDIOLIB_delayNChannel_exec_ci.                        */
   pFxnAUDIOLIB_delayNChannel_exec execute;

   /*! @brief Mode to choose the type of Delay
    *  mode = 0 for Linear Delay
    *  mode = 1 for Circular Delay                                */
   uint32_t mode;

   uint32_t numSamples;  /**< \brief Number of Samples in Input. */
   uint32_t numChannels; /**< \brief Number of Channels. */
   /*! @brief Interleave factor.
    * interleave = 0 for the data in de-interleave format
    * interleave = 1 for the data in interleave format */
   uint32_t interleave;
   uint32_t maxDelay;                    /**< \brief Maximum Delay. */
   uint32_t delaySize[MAX_NUM_CHANNELS]; /**< \brief Delay Size for each channel. */
   uint32_t delayBuffSize;               /**< \brief Total size of Delay buffer (Parameter for Circular Delay). */

   int32_t strideInElements;    /**< \brief Stride in Input dimension in bytes. */
   int32_t strideOutElements;   /**< \brief Stride in Output dimension in bytes. */
   int32_t strideDelayElements; /**< \brief Stride in Delay dimension in bytes. */

   int32_t readIdx[MAX_NUM_CHANNELS];  /**< \brief Read index for Circular Delay buffer. */
   int32_t writeIdx[MAX_NUM_CHANNELS]; /**< \brief Write index for Circular Delay buffer. */

   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_DELAYNCHANNEL_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_delayNChannel_PrivArgs;

#endif /* AUDIOLIB_DELAYNCHANNEL_IXX_IXX_OXX_PRIV_H_ */
