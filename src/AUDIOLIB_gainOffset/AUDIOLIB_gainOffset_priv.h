// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_GAINOFFSET_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_GAINOFFSET_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_gainOffset.h"

/*!
 * \file
 * \brief Header file for kernel's internal use. For the kernel's interface,
 * please see @ref AUDIOLIB_gainOffset
 */

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_gainOffset_PrivArgs structure.
 */
#define AUDIOLIB_GAINOFFSET_IXX_IXX_OXX_PBLOCK_SIZE (2 * SE_PARAM_SIZE)

/*!
 * @brief Bytes of one gain or offset value in the handle: a float (float and double data) or an
 *        int32_t (int32 data), both 4 bytes.
 */
#define AUDIOLIB_GAINOFFSET_VALUE_BYTES (sizeof(float))

/*!
 * @brief Handle layout, every region starting on an AUDIOLIB_L2DATA_ALIGNMENT boundary
 *        (offsets relative to the handle, which is allocated with that alignment):
 *
 *   | PrivArgs | gain value | offset value |
 */
#define AUDIOLIB_GAINOFFSET_PRIVARGS_BYTES                                                                         \
   AUDIOLIB_CALC_STRIDE(sizeof(AUDIOLIB_gainOffset_PrivArgs), AUDIOLIB_L2DATA_ALIGN_SHIFT)
#define AUDIOLIB_GAINOFFSET_VALUE_REGION_BYTES                                                                     \
   AUDIOLIB_CALC_STRIDE(AUDIOLIB_GAINOFFSET_VALUE_BYTES, AUDIOLIB_L2DATA_ALIGN_SHIFT)

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_gainOffset_exec_ci
 *         and @ref AUDIOLIB_gainOffset_exec_cn.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_gainOffset_exec)(AUDIOLIB_kernelHandle handle,
                                                        void *restrict pIn,
                                                        void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainOffset_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and stream address generator based on the dimensions already stored
 *          in the handle, and saves it in the bufPblock array. When
 *          @ref AUDIOLIB_gainOffset_exec_ci runs, it merely retrieves the
 *          configuration from bufPblock, so it does not lose cycles determining
 *          the hardware configuration.
 *
 *  @param [in]  handle        :  Active handle to the kernel
 *  @param [in]  bufParamsIn   :  Pointer to the structure containing dimensional
 *                                information of input buffer
 *  @param [in]  bufParamsOut  :  Pointer to the structure containing dimensional
 *                                information of output buffer
 *  @param [in]  pKerInitArgs  :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainOffset_init_ci(AUDIOLIB_kernelHandle               handle,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                            const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs);

/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainOffset_exec.
 *
 * @details The input is read through streaming engine SE0 and the output is
 *          written through stream address generator SA0. Each iteration computes
 *          one vector of \f$ g \cdot x + c \f$ and stores it with the SA0
 *          predicate, which masks the tail of a partial vector.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainOffset_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_gainOffset_exec.
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainOffset_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_gainOffset_exec_cn and
    *         @ref AUDIOLIB_gainOffset_exec_ci.                        */
   pFxnAUDIOLIB_gainOffset_exec execute;

   uint32_t nVecs;     /**< \brief Number of output vectors stored by the optimized loop (for int32
                            data: vectors of 64-bit lanes, 4 samples on C7504). */
   /*! @brief Gain value inside the handle (region after PrivArgs, aligned), copied from *pGain:
    *         a float for float/double data, an int32_t for int32 data. */
   void *pGain;
   /*! @brief Offset value inside the handle (aligned region), copied from *pOffset. */
   void *pOffset;
   uint32_t dataType;  /**< \brief Data type of the buffers, @ref AUDIOLIB_data_type_e (for perfEst). */
   /*! @brief int32 data: the inputs x for which gain * x + offset fits in int32 are
    *         [satLo, satLo + satWidth] (computed exactly by init). Inside, the wrapped 32-bit
    *         result is exact; outside, the output is a rail chosen by the sign of x. */
   int32_t  satLo;
   uint32_t satWidth; /**< \brief int32 data: width of the linear input range (unsigned). */
   int32_t  satRail;  /**< \brief int32 data: rail for x > 0 out of range: INT32_MAX if gain >= 0,
                           else INT32_MIN; x < 0 takes the other rail (satRail ^ -1). */
   uint32_t dimX;      /**< \brief Width of buffer in X dimension in elements. */
   uint32_t dimY;      /**< \brief Height of buffer in Y dimension in elements. */
   int32_t  strideIn;  /**< \brief Input stride in Y dimension in elements. */
   int32_t  strideOut; /**< \brief Output stride in Y dimension in elements. */

   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_GAINOFFSET_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_gainOffset_PrivArgs;

#endif /* AUDIOLIB_GAINOFFSET_IXX_IXX_OXX_PRIV_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_gainOffset_priv.h                                */
/* ======================================================================== */
