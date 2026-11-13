// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ROUND_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_ROUND_IXX_IXX_OXX_PRIV_H_

#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_round.h"

/*!
 * \file
 * \brief Header file for kernel's internal use. For the kernel's interface,
 * please see @ref AUDIOLIB_round
 */

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_round_PrivArgs structure.
 *
 */
#define AUDIOLIB_ROUND_IXX_IXX_OXX_PBLOCK_SIZE ( SE_PARAM_SIZE +  SA_PARAM_SIZE )

/*! @brief Offsets of the SE and SA templates within the bufPblock array. */
#define SE_PARAM_BASE (0x0000)
#define SE_SE0_PARAM_OFFSET (SE_PARAM_BASE)
#define SE_SA0_PARAM_OFFSET (SE_SE0_PARAM_OFFSET + SE_PARAM_SIZE)

/*!
 * @brief Loop cost model shared by @ref AUDIOLIB_round_init_ci, which picks the
 *        exec variant with it, and @ref AUDIOLIB_round_perfEst, which reports it.
 *
 * @details One iteration of @ref AUDIOLIB_round_exec_ci rounds vecsPerIter (1 or
 *          16) vectors, so the loop costs PIPE_UP + nVecs * 2 for vecsPerIter 1
 *          and PIPE_UP + ceil(nVecs / 16) * 29 for vecsPerIter 16. The cycles
 *          per iteration are the software pipeline ii of the compiled .asm, the
 *          same on C7504 and C7524 and for float and double. PIPE_UP (prolog and
 *          epilog) is the compiler's "Total cycles (est.)" constant where LOKI
 *          agrees with it, else the LOKI value. The call overhead is fitted to
 *          LOKI over the 45 test cases: the estimate is within 0 to 1 cycle of
 *          LOKI on C7524 (exact in 42 cases) and 1 to 2 cycles on C7504, whose
 *          calls cost one cycle more; the extra cycle is nVecs == 1.
 */
#define AUDIOLIB_ROUND_CYCLES_PER_VEC (2U)        /* ii, vecsPerIter 1, from the .asm */
#define AUDIOLIB_ROUND_CYCLES_PER_16_VECS (29U)   /* ii, vecsPerIter 16, from the .asm */
#define AUDIOLIB_ROUND_SP_PIPE_UP_CYCLES_1 (8U)   /* float, vecsPerIter 1, compiler and LOKI */
#define AUDIOLIB_ROUND_DP_PIPE_UP_CYCLES_1 (8U)   /* double, vecsPerIter 1, compiler and LOKI */
#define AUDIOLIB_ROUND_SP_PIPE_UP_CYCLES_16 (5U)  /* float, vecsPerIter 16, compiler and LOKI */
#define AUDIOLIB_ROUND_DP_PIPE_UP_CYCLES_16 (20U) /* double, vecsPerIter 16, LOKI; compiler 22 */
#define AUDIOLIB_ROUND_CALL_OVERHEAD_CYCLES (45U) /* cycles outside the loop, fitted to LOKI */

/*!
 * @brief Smallest magnitude at which every representable value of @c dataType
 *        is already an exact integer, i.e. 2^mantissaBits.
 *
 * @details @ref AUDIOLIB_round_exec_ci adds this constant to a non-negative value
 *          and subtracts it again to discard the fractional bits. The sequence
 *          is only valid below the constant: at or above it the sum carries into
 *          the next binade, where the exponent is larger and odd integers are no
 *          longer representable. It therefore doubles as the threshold at which
 *          the input is returned untouched.
 *
 *          The unspecialized template supplies the @c double value; @c float is
 *          provided by the specialization. Both are @c constexpr and so are
 *          implicitly inline, which is what makes them safe to define here.
 *
 * @return  2^mantissaBits for @c dataType
 */
template <typename dataType> constexpr dataType AUDIOLIB_roundNoFractionBound()
{
   return 4503599627370496.0; /* 2^52 */
}

template <> constexpr float AUDIOLIB_roundNoFractionBound<float>()
{
   return 8388608.0f; /* 2^23 */
}

/*!
 *  @brief This is a function pointer type that conforms to the
 *         declaration of the natural and optimized rounding functions.
 */
typedef AUDIOLIB_STATUS (*pFxnAUDIOLIB_round_exec)(AUDIOLIB_kernelHandle handle,
                                                            void *restrict pIn,
                                                            void *restrict pOut);

/*!
 *  @brief This function is the initialization function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_round_init.
 *
 * @details This function determines the streaming engine and streaming address
 *          generator configuration from the function call parameters, and saves
 *          it in the bufPblock array. Buffers whose row pitch equals their row
 *          length are streamed as a single 1D stream; otherwise a 2D stream
 *          walks the frame row by row. In the kernel call sequence,
 *          @ref AUDIOLIB_round_exec_ci would be called later independently by
 *          the application. When that function runs, it merely retrieves the
 *          configuration from the bufPblock and uses it to set up the hardware
 *          resources. This arrangement ensures that it does not lose cycles to
 *          determine the hardware configuration.
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
AUDIOLIB_STATUS AUDIOLIB_round_init_ci(AUDIOLIB_kernelHandle                   handle,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                            const AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                            const AUDIOLIB_round_InitArgs *pKerInitArgs);


/*!
 *  @brief This function is the main execution function for the C7x
 *         implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_round_exec.
 *
 * @details Streaming engine 0 reads the input frame for the rounding and
 *          streaming engine 1 reads it again for the sign; one streaming
 *          address generator writes the output frame. Each loop iteration
 *          rounds @p vecsPerIter vectors and is branch free: every lane runs
 *          the same seven vector operations and lane dependent choices are
 *          predicated selects. No float to integer conversion is involved, so
 *          the full exponent range of the type is preserved.
 *
 *          The magnitude is rounded to nearest with ties to even by adding and
 *          then subtracting @c 2^mantissaBits, which forces the fractional bits
 *          out. The add is only valid below that bound, because at or above it
 *          the sum carries into the next binade where the exponent is larger
 *          and odd integers are no longer representable; a predicated select
 *          therefore keeps the magnitude untouched once it reaches the bound,
 *          which also carries infinities and NaNs through. Finally the sign bit
 *          of the input is ORed back in, so the result has the sign of the
 *          input, as @c std::rint gives, including -0.0 for inputs in (-0.5, -0].
 *
 *          @ref AUDIOLIB_round_init_ci selects @p vecsPerIter 1 or 16 per frame
 *          with the cost model next to @ref AUDIOLIB_ROUND_CYCLES_PER_VEC.
 *          With 16, surplus vectors of the last group read zeros past the end
 *          of the streams and the streaming address generator masks their
 *          stores.
 *
 *  @tparam dataType     float or double
 *  @tparam vecsPerIter  Vectors rounded per loop iteration, 1 or 16
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
template <typename dataType, uint32_t vecsPerIter>
AUDIOLIB_STATUS AUDIOLIB_round_exec_ci(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 *  @brief This function is the main execution function for the natural
 *         C implementation of the kernel. The function declaration conforms
 *         to the declaration of @ref AUDIOLIB_round_exec.
 *
 * @details This is the bit exact reference for @ref AUDIOLIB_round_exec_ci. It
 *          walks the frame with a scalar row and column loop and applies
 *          @c std::rint to each sample, which rounds to nearest and resolves
 *          halfway cases to even under the reset FPCR rounding mode. This
 *          agrees with @c np.round in the test data generator; @c std::round
 *          does not, because it rounds halfway cases away from zero.
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
AUDIOLIB_round_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   /*! @brief Function pointer to point to the right execution variant between
    *         natural C and optimized */
   pFxnAUDIOLIB_round_exec execute;

   /*! @brief Number of vector to be processed for the given input buffer */
   uint32_t nVecs;
   /*! @brief Modelled loop cycles of the exec variant chosen by
    *         @ref AUDIOLIB_round_init_ci, reported by @ref AUDIOLIB_round_perfEst */
   uint64_t loopCycles;
   uint32_t samples;   /**< \brief Width of buffer in X dimension in elements. */
   uint32_t channels;  /**< \brief Height of buffer in Y dimension in elements. */
   uint32_t strideIn;  /**< \brief Stride in Y dimension in bytes of input data. */
   uint32_t strideOut; /**< \brief Stride in Y dimension in bytes of output data. */

   /*! @brief bufPblock array to store SE/SA template */
   uint8_t bufPblock[AUDIOLIB_ROUND_IXX_IXX_OXX_PBLOCK_SIZE];

} AUDIOLIB_round_PrivArgs;

#endif /* AUDIOLIB_ROUND_IXX_IXX_OXX_PRIV_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_round_priv.h          */
/* ======================================================================== */
