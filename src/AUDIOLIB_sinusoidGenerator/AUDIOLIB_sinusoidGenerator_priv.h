// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_sinusoidGenerator.h"

#define ELEMENT_COUNT(x) c7x::element_count_of<x>::value
#define SE_PARAM_BASE (0x0000)
#define SE_SA0_PARAM_OFFSET (SE_PARAM_SIZE)

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_sinusoidGenerator_PrivArgs structure.
 *
 */
#define AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE + 2 * SA_PARAM_SIZE)

#if __C7X_VEC_SIZE_BITS__ == 256
#define ALPHA_COEFF_INDEX 8
#else
#define ALPHA_COEFF_INDEX 16
#endif

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_sinusoidGenerator_exec_ci
 *         and @ref AUDIOLIB_sinusoidGenerator_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_sinusoidGenerator_exec)(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_sinusoidGenerator_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_sinusoidGenerator_exec_ci would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_sinusoidGenerator_exec_ci runs, it merely retrieves
 *          the configuration from the bufPBlock and uses it to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_sinusoidGenerator_exec_ci does not lose cycles
 *          to determine the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
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
AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_init_ci(AUDIOLIB_kernelHandle                      handle,
                                                   const AUDIOLIB_bufParams1D_t              *bufParamsOut,
                                                   const AUDIOLIB_sinusoidGenerator_InitArgs *pKerInitArgs);

/*!
 * @brief This function is the main execution function for the C7x
 * vector implementation of the kernel. The function declaration conforms
 * to the declaration of @ref AUDIOLIB_sinusoidGenerator_exec.
 *
 * @details The function uses C7x vector instructions to compute the
 * sinusoid output.
 * Result of the compute is stored into memory using
 * a stream address generator (SA).
 *
 * @param [in]  handle      : Active handle to the kernel
 * @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 * @par Performance Considerations:
 * For best performance,
 * - the output data buffer is expected to be in L2 memory
 * - the buffer pointers are assumed to be 64-byte aligned
 *
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_vector_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/*!
 * @brief This function is the main execution function for the C7x
 * scalar implementation of the kernel. The function declaration conforms
 * to the declaration of @ref AUDIOLIB_sinusoidGenerator_exec.
 *
 * @details The function uses scalar C7x instructions to compute the
 * sinusoid output.
 *
 * @param [in]  handle      : Active handle to the kernel
 * @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 * @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_scalar_ci(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_sinusoidGenerator_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [out] pOut        : Pointer to buffer holding the output buffer
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_sinusoidGenerator_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pOut);

/**
 * @brief Structure that is reserved for internal use by the kernel
 */
/**
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    * @ref AUDIOLIB_sinusoidGenerator_exec_cn and
    * @ref AUDIOLIB_sinusoidGenerator_exec_ci variants.              */
   pFxnAUDIOLIB_sinusoidGenerator_exec execute;

   uint32_t                            nVecs;      /**< \brief Number of vector to be processed for the given buffer */
   uint32_t                            numSamples; /**< \brief Width of buffer in X dimension in elements */
   AUDIOLIB_sinusoidGenerator_InitArgs initArgs;   /**< \brief Structure holding initialization parameters */
   AUDIOLIB_sinusoidGenerator_SetArgs  setArgs;    /**< \brief Structure to hold the set parameters */
   float                               phase;      /**< \brief The current phase of the sine wave */
   float                               phaseIncTarget;       /**< \brief The target phase increment value */
   float                               phaseInc;             /**< \brief The current phase increment value */
   float                               smoothingCoefficient; /**< \brief The (1 - alpha) coefficient for smoothing */
   float                               oneMinusSmoothingCoefficient; /**< \brief The alpha coefficient for smoothing */
   uint8_t bufPblock[AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_PBLOCK_SIZE]; /**< \brief bufPblock array to store SE/SA
                                                                            template */
   float alphaMultiplier; /**< \brief Pre-calculated multiplier to efficiently update a full vector of alpha
                                      values in the C7x vector implementation. */
   float alphaCoeff[ALPHA_COEFF_INDEX]; /**< \brief Array to hold pre-calculated alpha coefficients for vector
                                           processing */
   float ramp[ALPHA_COEFF_INDEX]; /**< \brief Array to hold ramp coefficients (0, 1, 2, ...) for vector processing */
   /** @brief A flag (0 or 1) to indicate if frequency smoothing should be bypassed. */
   int32_t bypassSmoothing;
   /** @brief The index of the last sample in the last vector, used for state saving. */
   uint32_t lastSampleIdx;
} AUDIOLIB_sinusoidGenerator_PrivArgs;

#endif /* AUDIOLIB_SINUSOIDGENERATOR_IXX_IXX_OXX_PRIV_H_ */
