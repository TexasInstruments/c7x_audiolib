// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_NLMS_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_NLMS_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_nlms.h"

#define UNROLL_FACTOR 4

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_nlms_PrivArgs structure.
 *
 */
#define AUDIOLIB_NLMS_IXX_IXX_OXX_PBLOCK_SIZE (7 * SE_PARAM_SIZE + 4 * SA_PARAM_SIZE)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_nlms_exec_ci
 *         and @ref AUDIOLIB_nlms_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_nlms_exec)(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pInRef,
                                                  void *restrict pStateBuffer,
                                                  void *restrict pScratchBuffer,
                                                  void *restrict pCoefficients,
                                                  void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_nlms_init.
 *
 * @details This function determines the configuration for the streaming engine (SE)
 *          and streaming address generators (SA) based on the function call parameters.
 *          It sets up templates for:
 *          - Updating the circular state buffer with new input.
 *          - Performing FIR filtering (Convolution).
 *          - Calculating Error and Step Size.
 *          - Updating Coefficients.
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_nlms_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_nlms_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_nlms_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 * @param [in]  handle        :  Active handle to the kernel
 * @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 * information of input buffer
 * @param [out] bufParamsOut  :  Pointer to the structure containing dimensional
 * information of output buffer
 * @param [in]  pKerInitArgs  :  Pointer to the structure holding init parameters
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_nlms_init_ci(AUDIOLIB_kernelHandle         handle,
                                      AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                      const AUDIOLIB_nlms_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_nlms_exec for the
 *          operation non interleave data.
 *
 * @details The function uses the C7x Streaming Engine (SE) to fetch data from
 *          the circular state buffer and coefficient buffers, and uses vector
 *          operations to perform the NLMS algorithm.
 *          Key steps:
 *          1. Update State Buffer with new samples.
 *          2. Compute Filter Output (FIR) & Energy.
 *          3. Compute Error & Adaptive Step Size.
 *          4. Update Coefficients based on error.
 *
 * @param [in]  handle          : Active handle to the kernel
 * @param [in]  pIn             : Input signal buffer
 * @param [in]  pInRef          : Desired signal buffer
 * @param [in]  pStateBuffer    : Circular state buffer
 * @param [in]  pScratchBuffer  : Accumulator buffer
 * @param [in]  pCoefficients   : Coefficient buffer
 * @param [out] pOut            : Output signal buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 * @par Performance Considerations:
 *      For best performance,
 *      - the input and output data buffers are expected to be in L2 memory
 *      - the buffer pointers are assumed to be 64-byte aligned
 *
 */

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_nlms_exec_ci(AUDIOLIB_kernelHandle handle,
                                      void *restrict pIn,
                                      void *restrict pInRef,
                                      void *restrict pStateBuffer,   // Circular State Buffer
                                      void *restrict pScratchBuffer, // Accumulator Buffer
                                      void *restrict pCoefficients,
                                      void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel for the non interleave data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_nlms_exec.
 *
 * @details
 *
 * @param [in]  handle          : Active handle to the kernel
 * @param [in]  pIn             : Input signal buffer
 * @param [in]  pInRef          : Desired signal buffer
 * @param [in]  pStateBuffer    : Circular state buffer
 * @param [in]  pScratchBuffer  : Accumulator buffer
 * @param [in]  pCoefficients   : Coefficient buffer
 * @param [out] pOut            : Output signal buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_nlms_exec_cn(AUDIOLIB_kernelHandle handle,
                                      void *restrict pIn,
                                      void *restrict pInRef,
                                      void *restrict pStateBuffer,   // Circular State Buffer
                                      void *restrict pScratchBuffer, // Accumulator Buffer
                                      void *restrict pCoefficients,
                                      void *restrict pOut);
/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel for the interleave data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_nlms_exec.
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
/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    * @ref AUDIOLIB_nlms_exec_cn and
    * @ref AUDIOLIB_nlms_exec_ci.                        */
   pFxnAUDIOLIB_nlms_exec execute;

   uint32_t dim_x;               /**< \brief Width of buffer in X dimension in elements. */
   uint32_t dim_y;               /**< \brief Height of buffer in Y dimension in elements (Channels). */
   uint32_t strideInElements;    /**< \brief Stride in Y dimension in bytes of input data. */
   uint32_t strideOutElements;   /**< \brief Stride in Y dimension in bytes of output data. */
   uint32_t strideStateElements; /**< \brief Stride in Y dimension in bytes of state buffer. */
   uint32_t filterLength;        /**< \brief Length of the adaptive filter. */
   float    stepSize;            /**< \brief Adaptive step size (mu). */
   uint32_t totalSamples;        /**< \brief Number of samples to process per channel. */
   float    regularization;      /**< \brief Regularization constant for NLMS energy division. */

   // Tiling and Blocking parameters for C7x optimization
   uint32_t nTilesSampleLength;
   uint32_t nTilesFilterLength;
   uint32_t nBlocksFilterLength;
   uint32_t nBlocksSampleLength;

   // --- Circular buffer state ---
   uint32_t circBuffSize; /**< \brief Power-of-2 size ≥ filterLength + totalSamples. */
   uint32_t circBuffMask; /**< \brief circBuffSize - 1 (for fast bitwise modulo). */
   int32_t  readIdx;      /**< \brief Current read index in circular buffer. */
   int32_t  writeIdx;     /**< \brief Current write index in circular buffer. */

   /*! @brief bufPblock array to store SE/SA template configurations */
   uint8_t bufPblock[AUDIOLIB_NLMS_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_nlms_PrivArgs;

#endif /* AUDIOLIB_NLMS_IXX_IXX_OXX_PRIV_H_ */
