// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_BALANCE_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_BALANCE_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_balance.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_balance_PrivArgs structure.
 *
 */
#define AUDIOLIB_BALANCE_IXX_IXX_OXX_PBLOCK_SIZE (1 * SE_PARAM_SIZE + 1 * SA_PARAM_SIZE)

#define SE_PARAM_BASE (0x0000)              ///< Base offset for hardware accelerator parameter blocks.
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE) ///< Offset for the first Streaming Engine (SE) parameter set.
#define SE_SA0_PARAM_OFFSET                                                                                            \
   (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE) ///< Offset for the first Stream Address Generator (SA) parameter set.

#define AUDIOLIB_BALANCE__GAIN_THRESHOLD 1e-6f // Threshold for gain comparison to bypass smoothing

#if __C7X_VEC_SIZE_BITS__ == 256
#define ALPHA_COEFF_INDEX 8
#else
#define ALPHA_COEFF_INDEX 16
#endif

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_balance_exec_ci
 *         and @ref AUDIOLIB_balance_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_balance_exec)(AUDIOLIB_kernelHandle handle,
                                                     void *restrict pInL,
                                                     void *restrict pInR,
                                                     void *restrict pOutL,
                                                     void *restrict pOutR);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_balance_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_balance_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_balance_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_balance_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
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
AUDIOLIB_STATUS AUDIOLIB_balance_init_ci(AUDIOLIB_kernelHandle            handle,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                         const AUDIOLIB_balance_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_balance_exec.
 *
 * @details This function executes the C7x implementation of the balance kernel
 *          using the configuration stored in the bufPBlock array. The
 *          bufPBlock array is populated by the @ref AUDIOLIB_balance_init_ci
 *          function.
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  pInL         :  Pointer to the input left channel audio data
 *  @param [in]  pInR         :  Pointer to the input right channel audio data
 *  @param [out] pOutL        :  Pointer to the output left channel audio data
 *  @param [out] pOutR        :  Pointer to the output right channel audio data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_balance_exec_ci(AUDIOLIB_kernelHandle handle,
                                         void *restrict pInL,
                                         void *restrict pInR,
                                         void *restrict pOutL,
                                         void *restrict pOutR);

/*!
 *  @brief This function is the execution function for the Natural C
 *         implementation of the balance kernel. The function declaration
 *         conforms to the declaration of @ref AUDIOLIB_balance_exec.
 *
 * @details This function executes the Natural C implementation of the balance kernel
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  pInL         :  Pointer to the input left channel audio data
 *  @param [in]  pInR         :  Pointer to the input right channel audio data
 *  @param [out] pOutL        :  Pointer to the output left channel audio data
 *  @param [out] pOutR        :  Pointer to the output right channel audio data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_balance_exec_cn(AUDIOLIB_kernelHandle handle,
                                                void *restrict pInL,
                                                void *restrict pInR,
                                                void *restrict pOutL,
                                                void *restrict pOutR);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_balance_exec_cn and
    *         @ref AUDIOLIB_balance_exec_ci.                        */
   pFxnAUDIOLIB_balance_exec execute;
   int32_t                   samples;           /**< Number of samples in the input buffer */
   int32_t                   channels;          /**< Number of channels in the input buffer */
   int32_t                   strideInElements;  /**< Stride in Y dimension for input*/
   int32_t                   strideOutElements; /**< Stride in Y dimension for output */
   int32_t                   nVecs;             /**< ceiling division of samples and eleCount of a vector*/
   float                     currentGainL;      /**< Current gain for the left channel, initialized to zero */
   float                     currentGainR;      /**< Current gain for the right channel, initialized to zero */
   float                     targetGainL; /**< Target gain for the left channel, computed from the balance parameter */
   float                     targetGainR; /**< Target gain for the right channel, computed from the balance parameter */
   float smoothingCoefficient; /**< Smoothing coefficient (alpha) between 0 and 1, derived from smoothing Time and
                                  sampling Rate */
   AUDIOLIB_balance_InitArgs initArgs; /**< Structure holding initialization parameters  */
   AUDIOLIB_balance_SetArgs  setArgs;  /**< Structure to hold the set parameters*/
   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_BALANCE_IXX_IXX_OXX_PBLOCK_SIZE]; /*< Array to hold SE/SA template */
   float   alphaCoeff[ALPHA_COEFF_INDEX]; /**< Array to hold alpha coefficients for the gain smoothing */
   int32_t bypassSmoothing;               /**< Flag to indicate if smoothing is bypassed */
   float   alphaMultiplier; /**< Pre-calculated multiplier to efficiently update a full vector of gain values in the C7x
                             implementation. */
} AUDIOLIB_balance_PrivArgs;

#endif /* AUDIOLIB_GAIN_IXX_IXX_OXX_PRIV_H_ */
