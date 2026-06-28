// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ASRC_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_ASRC_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_asrc.h"

#define ROUND(x) ((x) + 0.5) /* floor( (x) + 0.5) */

/**
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_asrc_PrivArgs structure.
 *
 */
#ifdef C7X
#define AUDIOLIB_ASRC_PBLOCK_SIZE (5 * SE_PARAM_SIZE + 3 * SA_PARAM_SIZE)
#endif

/* Max ratio is dynamically set inside the algorithm based on input and output sample rates. This value is only set to
 * throw an error if the ratio set by the user is outrageous! Algorithm will continue to work with the past set sample
 * rate ratio or expected raton value set at init time.*/
#define AUDIOLIB_ASRC_MAX_IO_SAMPLE_RATE_RATIO ((48.0f / 32.0f) * 1.1f)
/**
 * @brief Number of taps for each sub filter of prototype filter or taps per phase
 */
#define AUDIOLIB_ASRC_NUMBER_OF_SUB_FILTER_TAPS (64U)
/**
 * @brief Number of phases in the prototype filter (This is M = 32 phases in AES paper)
 */
#define AUDIOLIB_ASRC_NUMBER_OF_FILTER_PHASES (32U)
/**
 * @brief ASRC frame length must be a multiple of the frameModuloFactor (=4 for low latency).
 *        AUDIOLIB_ASRC_MAX_MODULO_FACTOR is the maximum value of frameModuloFactor that can be used.
 */
#define AUDIOLIB_ASRC_MAX_MODULO_FACTOR (32U)
/**
 * @brief ASRC input buffer should be twice the size of memory required for input sample count when the sample couunt
 * per frame is less than 64 for the non interleaved input data format
 */
#define AUDIOLIB_ASRC_DOUBLE_BUFFERING_FACTOR (2U)
/**
 * @brief ASRC input buffer should be quadruple the size of memory required for input sample count when the sample
 * couunt per frame is higher than 64 for the non interleaved input data format
 */
#define AUDIOLIB_ASRC_QUADRUPLE_BUFFERING_FACTOR (4U)

/**
 * @brief Mask for the phase index of the prototype filter.
 *        There are 32 phases in the prototype filter.
 */
#define AUDIOLIB_ASRC_PHASE_MASK 0xf800000U

/**
 * @brief Shift value for the phase index of the prototype filter.
 *        There are 32 phases in the prototype filter.
 */
#define AUDIOLIB_ASRC_PHASE_SHIFT 23U
/**
 * @brief Mask for the input increment of the prototype filter.
 *        There are 8 increments in the prototype filter.
 */
#define AUDIOLIB_ASRC_INPUT_INCREMENT_MASK 0x70000000UL
/**
 * @brief Shift value for the input increment of the prototype filter.
 *        There are 8 increments in the prototype filter.
 */
#define AUDIOLIB_ASRC_INPUT_INCREMENT_SHIFT 28U

/**
 * @brief Mask for the fraction of the prototype filter.
 *        The fraction is represented by a Q23.7 fixed point number.
 */
#define AUDIOLIB_ASRC_FRACTION_MASK 0x7fffffU
/**
 * @brief Scaling factor to convert the Q23.7 fraction to a float.
 */
#define AUDIOLIB_ASRC_FRACTION_SCALE (1.f / 8388608.0f)

/**
 * @brief Fixed-point representation Q28.
 */
#define AUDIOLIB_ASRC_FIXEDPOINT_Q28 (1UL << 28)
/**
 *  @brief This is a function pointer type that conforms to the
 *         declaration of @ref AUDIOLIB_asrc_exec_ci_interleaved,
 *         @ref AUDIOLIB_asrc_exec_ci_non_interleaved,
 *         @ref AUDIOLIB_asrc_exec_cn_interleaved,
 *         @ref AUDIOLIB_asrc_exec_cn_non_interleaved.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_asrc_exec)(AUDIOLIB_kernelHandle handle,
                                                  void *restrict pIn,
                                                  void *restrict pNonInterleavedData,
                                                  void *restrict pFiltCoeffs,
                                                  void *restrict pFilterRembuf,
                                                  void *restrict pOut,
                                                  const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                  AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

/**
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /** @brief Structure holding initialization parameters
    */
   AUDIOLIB_asrc_InitArgs initArgs;
   /** @brief Function pointer to point to the right execution variant between
    *         @ref AUDIOLIB_asrc_exec_cn_non_interleaved or @ref AUDIOLIB_asrc_exec_ci_interleaved or
    *         @ref AUDIOLIB_asrc_exec_ci_non_interleaved or @ref AUDIOLIB_asrc_exec_ci_interleaved
    */
   pFxnAUDIOLIB_asrc_exec execute;
#ifdef C7X
   /** @brief Init args for the matTrans kernel **/
   DSPLIB_matTransInitArgs matTransKerInitArgs;
#endif
   /** @brief asrcRatio in terms of double precision.
    *          - asrcRatio = fsout/fsin;
    *          - outputsPerInputRatio = asrcRatio;
    */
   volatile double outputsPerInputRatio;
   /** @brief Number of remaining fractional outputs from one frame to next per channel.
    *         Initialized by @ref AUDIOLIB_asrc_init or @ref AUDIOLIB_asrc_set and
    *         used by @ref AUDIOLIB_asrc_exec
    */
   double fracOutputsRemaining;
   /** @brief Double precision maximum number of outputs
    *         that will be processed in one kernel call
    *         before the input buffer needs to be advanced
    *         to the next set of inputs. Set by @ref AUDIOLIB_asrc_init
    *         Used by @ref AUDIOLIB_asrc_exec
    */
   double maxOutputsPerInputRatio;
   /** @brief Buf param structure containing dimensional information of
    *         the remaining samples buffer. This is used to modulo the output frame length by the user provided
    *         @ref AUDIOLIB_asrc_InitArgs.frameModuloFactor.
    *         Set by @ref AUDIOLIB_asrc_init and
    *         used by @ref AUDIOLIB_asrc_exec
    */
   AUDIOLIB_bufParams2D_t bufParamsFilterRembuf;
   /** @brief Width of the output buffer in elements.
    *         Set by @ref AUDIOLIB_asrc_init and used
    *         by @ref AUDIOLIB_asrc_exec functions.
    */
   uint32_t outBufferDimX;
   /** @brief Total width of the full circular buffer.
    *         - In the interleaved data format, circular buffer is \a pNonInterleavedData
    *         - In the non interleaved data format, circular buffer is \a pIn
    *         Initialized by @ref AUDIOLIB_asrc_init
    */
   uint32_t inBufferTotalDimX;
   /** @brief Total stride of the full circular buffer.
    *         Initialized by @ref AUDIOLIB_asrc_init
    */
   uint32_t inBufferTotalStrideY;
   /** @brief Index of the second sample in history portion of the circular input buffer.
    *         Initialized by @ref AUDIOLIB_asrc_init and updated
    *         by @ref AUDIOLIB_asrc_exec function.
    */
   uint32_t cirBuffStartIndex;
   /** @brief Number of previously computed outputs buffered in the pFilterRembuf.
    *         Set and used by @ref AUDIOLIB_asrc_exec and
    *         used by @ref AUDIOLIB_asrc_exec_ci_non_interleaved, @ref AUDIOLIB_asrc_exec_ci_interleaved,
    *         @ref AUDIOLIB_asrc_exec_cn_non_interleaved and @ref AUDIOLIB_asrc_exec_cn_interleaved
    *         functions.
    */
   int32_t remBufCount;
   /** @brief Size of the channel loop.
    */
   int32_t channelLoopSize;
#ifdef C7X
   /** @brief Buffer to save SE & SA configuration parameters */
   uint8_t bufPblock[AUDIOLIB_ASRC_PBLOCK_SIZE];
#endif
#ifdef ARM_A53
   /** @brief Width of the non-interleaved data
    */
   uint32_t widthInNonInterleavedData;
   /** @brief height of the non-interleaved data
    */
   uint32_t heightInNonInterleavedData;
   /** @brief Stride in non-interleaved data
    */
   int32_t strideInNonInterleavedData;
   /** @brief Stride out non-interleaved data
    */
   int32_t strideOutNonInterleavedData;
#endif
   /** @brief history length
    */
   int32_t history_length;
} AUDIOLIB_asrc_PrivArgs;

/**
 *  @brief This function is the initialization function for the natural C
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_asrc_init.
 *
 * @details
 *
 *  @param [in]  handle       :  Active handle to the kernel
 *  @param [in]  bufParamsIn  :  Pointer to the structure containing dimensional
 *                               information of input buffer
 *  @param [out] bufParamsOut :  Pointer to the structure containing dimensional
 *                               information of ouput buffer
 *  @param [in]  pKerInitArgs :  Pointer to the structure holding init
 * parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
AUDIOLIB_STATUS AUDIOLIB_asrc_init_cn(AUDIOLIB_kernelHandle   handle,
                                      AUDIOLIB_bufParams2D_t *bufParamsIn,
                                      AUDIOLIB_bufParams2D_t *bufParamsOut,
                                      AUDIOLIB_asrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel when the input data format is non interleaved.
 *          The function declaration conforms to the declaration of @ref AUDIOLIB_asrc_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_asrc_init_ci_non_interleaved would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_asrc_init_ci_non_interleaved runs, it retrieves
 *          the configuration from the bufPBlock and modifies if need be to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_asrc_init_ci_non_interleaved does not lose cycles
 *          to determine the hardware configuration.
 *
 *
 *  @param[in]   handle          : Active handle to the kernel
 *  @param[in]   bufParamsIn     : Pointer to the structure containing
 *                                 dimensional information of input buffer
 *  @param[in]   bufParamsOut    : Pointer to the structure containing
 *                                 dimensional information of output buffer
 *  @param[in]   pKerInitArgs    : Pointer to the structure holding
 *                                 init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_non_interleaved(AUDIOLIB_kernelHandle         handle,
                                                             const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                             const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                             const AUDIOLIB_asrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel for interleaved input data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_asrc_init.
 *
 * @details This function determines the configuration for the streaming engine
 *          and MMA hardware resources based on the function call parameters,
 *          and the configuration is saved in bufPBlock array. In the kernel
 *          call sequence, @ref AUDIOLIB_asrc_exec_ci_interleaved would be
 *          called later independently by the application. When
 *          @ref AUDIOLIB_asrc_exec_ci_interleaved runs, it retrieves
 *          the configuration from the bufPBlock and modifies if need be to set up the
 *          hardware resources. This arrangement is so that
 *          @ref AUDIOLIB_asrc_exec_ci_interleaved does not lose cycles
 *          to determine the hardware configuration.
 *
 *
 *  @param[in]   handle          : Active handle to the kernel
 *  @param[in]   bufParamsIn     : Pointer to the structure containing
 *                                 dimensional information of input buffer (interleaved)
 *  @param[in]   bufParamsOut    : Pointer to the structure containing
 *                                 dimensional information of output buffer
 *  @param[in]   pKerInitArgs    : Pointer to the structure holding
 *                                  init parameters
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */

template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_asrc_init_ci_interleaved(AUDIOLIB_kernelHandle         handle,
                                                         const AUDIOLIB_bufParams2D_t *bufParamsIn,
                                                         const AUDIOLIB_bufParams2D_t *bufParamsOut,
                                                         const AUDIOLIB_asrc_InitArgs *pKerInitArgs);

/**
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel when the input data format is non interleaved.
 *         The function declaration conforms to the declaration of @ref AUDIOLIB_asrc_exec.
 *
 * @details The function uses MMA hardware accelerator to perform the
 *          convolution computation. Filter data is loaded into B panel of the
 *          MMA from memory using one streaming engine, while the input data is
 *          loaded into A vectors of the MMA using the other streaming engine.
 *          Result of the compute from MMA C panel is stored into memory using
 *          a stream address generator.
 *
 *  @param [in]  handle       : Active handle to the kernel
 *  @param [in]  pIn          : Pointer to buffer holding the input data
 *  @param [in]  pNonInterleavedData   : Pointer to buffer holding the intermediate non interleaved data
 *  @param [in]  pFiltCoeffs           : Pointer to buffer holding polyphase filter cefficients.
 *  @param [in]  pFilterRembuf: Pointer to buffer holding the filter remaining buffer
 *  @param [out] pOut         : Pointer to buffer holding the output data
 *  @param [in]  pKerInArgs   : Pointer to structure holding input Arguments
 *  @param [out] pKerOutArgs  : Pointer to structure holding output Arguments

 *  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
 *
 *  @par Performance Considerations:
 *    For best performance,
 *    - the input and output data buffers are expected to be in L2 memory
 *    - the buffer pointers are assumed to be 64-byte aligned
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_non_interleaved(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn,
                                                             void *restrict pNonInterleavedData,
                                                             void *restrict pFiltCoeffs,
                                                             void *restrict pFilterRembuf,
                                                             void *restrict pOut,
                                                             const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                             AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

/**
*  @brief This function is the main execution function for the C7x
*         implementation of the kernel when the input data format is channel interleaved.
*         The function declaration conforms to the declaration of @ref AUDIOLIB_asrc_exec.
*
* @details The function uses MMA hardware accelerator to perform the
*          convolution computation. Filter data is loaded into B panel of the
*          MMA from memory using one streaming engine, while the input data is
*          loaded into A vectors of the MMA using the other streaming engine.
*          Result of the compute from MMA C panel is stored into memory using
*          a stream address generator.
*
*  @param [in]  handle       : Active handle to the kernel
*  @param [in]  pIn          : Pointer to buffer holding the input data
*  @param [in]  pNonInterleavedData   : Pointer to buffer holding the intermediate non interleaved data
*  @param [in]  pFiltCoeffs           : Pointer to buffer holding polyphase filter cefficients.
*  @param [in]  pFilterRembuf: Pointer to buffer holding the filter remaining buffer
*  @param [out] pOut         : Pointer to buffer holding the output data
*  @param [in]  pKerInArgs   : Pointer to structure holding input Arguments
*  @param [out] pKerOutArgs  : Pointer to structure holding output Arguments

*  @return      Status value indicating success or failure. Refer to @ref AUDIOLIB_STATUS.
*
*  @par Performance Considerations:
*    For best performance,
*    - the input and output data buffers are expected to be in L2 memory
*    - the buffer pointers are assumed to be 64-byte aligned
*
*/

template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_asrc_exec_ci_interleaved(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pNonInterleavedData,
                                                         void *restrict pFiltCoeffs,
                                                         void *restrict pFilterRembuf,
                                                         void *restrict pOut,
                                                         const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                         AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

/**
 *  @brief This function is the natural C implementation of the main execution function for the
 *         non-interleaved input data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_asrc_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pNonInterleavedData   : Pointer to buffer holding the intermediate non interleaved data
 *  @param [in]  pFiltCoeffs           : Pointer to buffer holding polyphase filter cefficients.
 *  @param [in]  pFilterRembuf: Pointer to buffer holding the filter remaining buffer
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *  @param [in]  pKerInArgs   : Pointer to structure holding input Arguments
 *  @param [out] pKerOutArgs  : Pointer to structure holding output Arguments
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_asrc_exec_cn_non_interleaved(AUDIOLIB_kernelHandle handle,
                                                             void *restrict pIn,
                                                             void *restrict pNonInterleavedData,
                                                             void *restrict pFiltCoeffs,
                                                             void *restrict pFilterRembuf,
                                                             void *restrict pOut,
                                                             const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                             AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

/**
 *  @brief This function is the natural C implementation of the main execution function for the
 *         channel interleaved input data. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_asrc_exec.
 *
 * @details
 *
 *  @param [in]  handle      : Active handle to the kernel
 *  @param [in]  pIn         : Pointer to buffer holding the input data
 *  @param [in]  pNonInterleavedData   : Pointer to buffer holding the intermediate non interleaved data
 *  @param [in]  pFiltCoeffs           : Pointer to buffer holding polyphase filter cefficients.
 *  @param [in]  pFilterRembuf: Pointer to buffer holding the filter remaining buffer
 *  @param [out] pOut        : Pointer to buffer holding the output data
 *  @param [in]  pKerInArgs   : Pointer to structure holding input Arguments
 *  @param [out] pKerOutArgs  : Pointer to structure holding output Arguments
 *
 *  @return      Status value indicating success or failure. Refer to @ref
 * AUDIOLIB_STATUS.
 *
 */
template <typename dataType>
extern AUDIOLIB_STATUS AUDIOLIB_asrc_exec_cn_interleaved(AUDIOLIB_kernelHandle handle,
                                                         void *restrict pIn,
                                                         void *restrict pNonInterleavedData,
                                                         void *restrict pFiltCoeffs,
                                                         void *restrict pFilterRembuf,
                                                         void *restrict pOut,
                                                         const AUDIOLIB_asrc_ExecInArgs *pKerInArgs,
                                                         AUDIOLIB_asrc_ExecOutArgs      *pKerOutArgs);

#endif /* AUDIOLIB_ASRC_IXX_IXX_OXX_PRIV_H_ */
