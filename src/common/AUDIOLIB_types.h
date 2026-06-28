// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef COMMON_AUDIOLIB_TYPES_H_
#define COMMON_AUDIOLIB_TYPES_H_ 1

/* This header is needed to be included in OpenCL programs which link
 * against AUDIOLIB, but OpenCL doesn't need the following headers */
#ifndef __OPENCL_VERSION__
#include <inttypes.h>
#include <stddef.h> // for NULL
#include <stdint.h>
#include <stdio.h> // for printf
#include <stdlib.h>
#endif
#ifdef C7X
#include <c7x.h> // for streaming engine, streaming address gen.
#endif
#include "AUDIOLIB_bufParams.h"
#include "TI_platforms.h"

///* ---------------------------------------------------------------- */
///* Desire C namespace for these defines/includes                    */
///* ---------------------------------------------------------------- */
#if !defined(AUDIOLIB_DEBUGPRINT)
#define AUDIOLIB_DEBUGPRINT 0 //!< Enable debug printf statements
#endif

#if (defined(_HOST_BUILD) && (AUDIOLIB_COMPILER_VERSION < 1003999))
#define AUDIOLIB_const
#else
#define AUDIOLIB_const const
#endif

#define __PRAGMA(x) _Pragma(#x)
#ifdef __cplusplus
#define __INLINE_FUNC(x) __PRAGMA(FUNC_ALWAYS_INLINE)
#else
#define __INLINE_FUNC(x) __PRAGMA(FUNC_ALWAYS_INLINE(x))
#endif

/* ---------------------------------------------------------------- */
/*  MISRAC Rule 4.9(DEFINE.FUNC) Deviation: The following two       */
/*  function-like macros do not have equivalent function            */
/*  implementations.                                                */
/* ---------------------------------------------------------------- */
#if defined(EVM_TEST)
#define AUDIOLIB_PRINTF(fmt, ...) fprintf(stdout, fmt, __VA_ARGS__);

#if AUDIOLIB_DEBUGPRINT > 0
#define AUDIOLIB_DEBUGPRINTFN(N, fmt, ...)                                                                             \
   do {                                                                                                                \
      if (AUDIOLIB_DEBUGPRINT >= (N)) {                                                                                \
         fprintf(stdout, "AUDIOLIB debug %s - %d: " fmt, __FUNCTION__, __LINE__, __VA_ARGS__);                         \
      }                                                                                                                \
   } while (0)

#else // AUDIOLIB_DEBUGPRINT == 0

#define AUDIOLIB_DEBUGPRINTFN(N, fmt, ...)
//#define AUDIOLIB_PRINTF(fmt, ...)
#endif // #if AUDIOLIB_DEBUGPRINT

#else

#if AUDIOLIB_DEBUGPRINT > 0
#define AUDIOLIB_DEBUGPRINTFN(N, fmt, ...)                                                                             \
   if (AUDIOLIB_DEBUGPRINT >= (N)) {                                                                                   \
      fprintf(stdout, "AUDIOLIB debug %s - %d: " fmt, __FUNCTION__, __LINE__, __VA_ARGS__);                            \
   }

#define AUDIOLIB_PRINTF(fmt, ...) fprintf(stdout, fmt, __VA_ARGS__)

#else // AUDIOLIB_DEBUGPRINT == 0

#define AUDIOLIB_DEBUGPRINTFN(N, fmt, ...)
#define AUDIOLIB_PRINTF(fmt, ...)

#endif // #if AUDIOLIB_DEBUGPRINT
#endif // #if defined(EVM_TEST)

#if defined(AUDIOLIB_MESSAGES)
// if enabled, display messages to the library user
// AUDIOLIB_MESSAGES should only be defined when TARGET_BUILD=debug or
// CHECKPARAMS=1
#define AUDIOLIB_MESG(fmt, ...) fprintf(stdout, fmt, __VA_ARGS__)
#else
#define AUDIOLIB_MESG(fmt, ...)
#endif //#if defined(AUDIOLIB_MESSAGES)

/* Original implementation that had the benefit of always being compiled and
thus receiving error checking.  However, the fprintf generates a MISRA-C
violation.
#define AUDIOLIB_DEBUGPRINTFN(N, fmt, ...)                                       \
  do {                                                                         \
    if (AUDIOLIB_DEBUGPRINT >= (N)) {                                            \
      fprintf(stdout, "AUDIOLIB debug %s - %d: " fmt, __FUNCTION__, __LINE__,    \
              __VA_ARGS__);                                                    \
    }                                                                          \
  } while (0)

#define AUDIOLIB_PRINTF(fmt, ...)                                                \
   do {                                                                        \
       fprintf(stdout, fmt, __VA_ARGS__);                                      \
   } while (0)
#endif
 */

#define AUDIOLIB_SOFT_MMA_RESET 0

// switch to enable or disable static inline for AUDIOLIB functions defined in .c
// files (so not many functions)
#define AUDIOLIB_STATIC_INLINE static inline

#ifdef __cplusplus
#ifdef C7X
#include <c7x_scalable.h> // for device scalability

extern const c7x::uchar_vec  AUDIOLIB_zeroVect_uchar_vec;
extern const c7x::ushort_vec AUDIOLIB_zeroVect_ushort_vec;
extern const c7x::uchar_vec  AUDIOLIB_vperm_data_0_63;
#endif
template <int32_t dType> struct AUDIOLIB_element_type {
   using type = uint8_t;
};

template <> struct AUDIOLIB_element_type<AUDIOLIB_UINT8> {
   using type = uint8_t;
};

template <> struct AUDIOLIB_element_type<AUDIOLIB_INT8> {
   using type = int8_t;
};

template <> struct AUDIOLIB_element_type<AUDIOLIB_UINT16> {
   using type = uint16_t;
};

template <> struct AUDIOLIB_element_type<AUDIOLIB_INT16> {
   using type = int16_t;
};

template <> struct AUDIOLIB_element_type<AUDIOLIB_UINT32> {
   using type = uint32_t;
};

template <> struct AUDIOLIB_element_type<AUDIOLIB_INT32> {
   using type = int32_t;
};

template <> struct AUDIOLIB_element_type<AUDIOLIB_FLOAT32> {
   using type = float;
};
#ifdef C7X
template <int32_t dType> struct AUDIOLIB_max_simd {
   static constexpr size_t value = c7x::max_simd<typename AUDIOLIB_element_type<dType>::type>::value;
};
#endif
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup AUDIOLIB_COMMON Common definitions
 * \brief This module consists of definitions (macros, structures, utility
 *        functions) that are commonly applicable to all AUDIOLIB kernels.
 * \details
 */

/**@{*/

typedef double AUDIOLIB_D64; //!< Double precision floating point
typedef float  AUDIOLIB_F32; //!< Single precision floating point

/** \brief The enumeration of all status codes. */
typedef enum {
   AUDIOLIB_SUCCESS                      = 0, /**< 0 => No error */
   AUDIOLIB_ERR_FAILURE                  = 1, /**< 1 => Unspecified error */
   AUDIOLIB_ERR_INVALID_VALUE            = 2, /**< 2 => Invalid parameter value */
   AUDIOLIB_ERR_INVALID_TYPE             = 3, /**< 3 => Invalid parameter type (AUDIOLIB_data_type_e data_type) */
   AUDIOLIB_ERR_INVALID_DIMENSION        = 4, /**< 4 => Dimension parameter (width/height) is too big/small */
   AUDIOLIB_ERR_NULL_POINTER             = 5, /**< 5 => Unsupported null pointer condition */
   AUDIOLIB_ERR_NOT_IMPLEMENTED          = 6, /**< 6 => Parameter configuration is not supported/implemented */
   AUDIOLIB_ERR_NOT_EQUAL_WIDTH_STRIDE   = 7, /**< 7 => Stride should be equal to width * element size */
   AUDIOLIB_ERR_NOT_ALIGNED_PTRS_STRIDES = 8, /**< 8 => Pointers and stride values are not aligned to documented
                                               value  */
   AUDIOLIB_ERR_NOT_ALIGNED_WIDTHS = 9,       /**< 9 => Width values are not aligned to documented value */
   AUDIOLIB_ERR_BUFFER_TOO_SMALL   = 10,      /**< 10 => Buffer size not large enough */
   AUDIOLIB_ERR_INVALID_ACTIVATION = 11,      /**< 11 => Activation selection incompatible with data type */
   AUDIOLIB_ERR_INVALID_SHIFT      = 12,      /**< 12 => Requested shift amount is not valid */
   AUDIOLIB_ERROR_MAX              = 13,
   AUDIOLIB_ERR_INVALID_MMA_FLAG   = 14, /**< 14 => enableMMA flag value in init args not valid */

} AUDIOLIB_STATUS_NAME;

typedef AUDIOLIB_STATUS_NAME AUDIOLIB_STATUS; //!< Return value for AUDIOLIB functions

typedef void *AUDIOLIB_kernelHandle; //!< Handle type for AUDIOLIB operations

/** \brief  Enumeration for the style of function implementation. */
typedef enum {
   AUDIOLIB_FUNCTION_NATC = 0,  /**< Natural C implementation of the function */
   AUDIOLIB_FUNCTION_OPTIMIZED, /**< Optimized C implementation of the function for
                                 the MMA + C7x architecture*/
   AUDIOLIB_FUNCTION_MAX = 128
} AUDIOLIB_FUNCTION_STYLE;

#define AUDIOLIB_PARAM_SIZE 128 //!< Parameter structure size in bytes

#define AUDIOLIB_ALIGN_SHIFT_64BYTES 6  //!< Number of bits to shift for 64-byte memory alignment
#define AUDIOLIB_ALIGN_SHIFT_128BYTES 7 //!< Number of bits to shift for 128-byte memory alignment
#define AUDIOLIB_ALIGN_SHIFT_256BYTES 8 //!< Number of bits to shift for 256-byte memory alignment

#define AUDIOLIB_ALIGN_64BYTES (1 << AUDIOLIB_ALIGN_SHIFT_64BYTES)   //!< Align by 64-byte memory alignment
#define AUDIOLIB_ALIGN_128BYTES (1 << AUDIOLIB_ALIGN_SHIFT_128BYTES) //!< Align by 128-byte memory alignment
#define AUDIOLIB_ALIGN_256BYTES (1 << AUDIOLIB_ALIGN_SHIFT_256BYTES) //!< Align by 256-byte memory alignment

#define AUDIOLIB_L2DATA_ALIGN_SHIFT AUDIOLIB_ALIGN_SHIFT_64BYTES //!< Set the default L2 data alignment

/** @brief Macro that specifies the alignment of data buffers in L2 memory for
 * optimal performance */
#define AUDIOLIB_L2DATA_ALIGNMENT (((uint32_t) 1) << ((uint32_t) AUDIOLIB_L2DATA_ALIGN_SHIFT))

/* ---------------------------------------------------------------- */
/*  MISRAC Rule 4.9(DEFINE.FUNC) Deviation: The advisory is not     */
/*  being addressed                                                 */
/* ---------------------------------------------------------------- */

/* @} */

/******************************************************************************
 *
 *  Do not document these in the User Guide
 ******************************************************************************/

/**
 * @cond
 */
/**@{*/

/* ---------------------------------------------------------------- */
/*  MISRAC Rule 4.9(DEFINE.FUNC) Deviation: The advisory is not     */
/*  being addressed                                                 */
/* ---------------------------------------------------------------- */
// remove asm comments for Loki testing as they may add cycles
#if defined(QT_TEST) || defined(RTL_TEST) || defined(EVM_TEST)
#if defined(_HOST_BUILD)
#define AUDIOLIB_asm(string) ;
#else
#define AUDIOLIB_asm(string)                                                                                           \
   ;                                                                                                                   \
   asm(string);
#endif // _HOST_BUILD
#else
#define AUDIOLIB_asm(string) ;
#endif

/* ---------------------------------------------------------------- */
/*  MISRAC Rule 4.9(DEFINE.FUNC) Deviation: The advisory is not     */
/*  being addressed  so as not to lose portability across different */
/*  platforms.                                                      */
/* ---------------------------------------------------------------- */
// cl7x unroll attributes not recognized by gcc/gpp compiler and generate
// warnings. Remove them with macro
#if defined(_HOST_BUILD)
#define str(x) #x
#define xstr(x) str(x)
#ifdef WIN32
#define __attribute__()
#define AUDIOLIB_UNROLL(count)
#else
#define AUDIOLIB_UNROLL(count) _Pragma(str(UNROLL(count)))
#endif
#else
#define AUDIOLIB_UNROLL(COUNT) [[TI::unroll(COUNT)]]
#endif

#if defined(_HOST_BUILD)
#ifdef WIN32
#define __attribute__()
#define AUDIOLIB_MUST_ITERATE(initial, max, multiple)
#else
#define AUDIOLIB_MUST_ITERATE(initial, max, multiple) _Pragma(str(MUST_ITERATE(initial, max, multiple)))
#endif
#else
#define AUDIOLIB_MUST_ITERATE(INTIAL, MAX, MULTIPLE) [[TI::must_iterate(INTIAL, MAX, MULTIPLE)]]
#endif

#ifndef AUDIOLIB_NUMBER_RANDOM_DIM_TESTS
#define AUDIOLIB_NUMBER_RANDOM_DIM_TESTS 25
#endif

#if defined(PERFORMANCE_TEST)
#define AUDIOLIB_PERFORMANCE_TEST_PATTERN SEQUENTIAL
#else
#define AUDIOLIB_PERFORMANCE_TEST_PATTERN RANDOM_SIGNED
#endif

#define AUDIOLIB_TEST_OUTPUT_HEAP 0
#if defined(_HOST_BUILD)
// Valgrind works better when output is in the heap (it can't track statically
// allocated memory), so in host emulation mode, place test outputs in the heap
// rather than statically allocated MSMC
#define AUDIOLIB_TEST_OUTPUT_MSMC AUDIOLIB_TEST_OUTPUT_HEAP
#else
#define AUDIOLIB_TEST_OUTPUT_MSMC 1
#endif

#define STRIDE_OPT_4CYCLE 1

/* Indicates when the data format is interleaved */
#define AUDIOLIB_DATA_FORMAT_INTERLEAVED 1
/* Indicates when the data format is non-interleaved */
#define AUDIOLIB_DATA_FORMAT_NON_INTERLEAVED 0

/* ---------------------------------------------------------------- */
/*  MISRAC Rule 4.9(DEFINE.FUNC) Deviation: The advisory is not     */
/*  being addressed                                                 */
/* ---------------------------------------------------------------- */

#if defined(__C7100__)
#define AUDIOLIB_BYTE_WIDTH 64 //!< Byte width of C7x
#define AUDIOLIB_CALC_STRIDE(BYTES, ALIGN_SHIFT) (((((BYTES) -1) >> (ALIGN_SHIFT)) + 1) << (ALIGN_SHIFT))
#endif

#if defined(__C7120__)
#define AUDIOLIB_BYTE_WIDTH 64 //!< Byte width of C7x
#define AUDIOLIB_CALC_STRIDE(BYTES, ALIGN_SHIFT) (((((BYTES) -1) >> (ALIGN_SHIFT)) + 1) << (ALIGN_SHIFT))
#endif
#if defined(__C7504__) || defined(__C7524__)
#define AUDIOLIB_BYTE_WIDTH 32 //!< Byte width of C7x
#define AUDIOLIB_CALC_STRIDE(BYTES, ALIGN_SHIFT) (((((BYTES) -1) >> (ALIGN_SHIFT)) + 1) << (ALIGN_SHIFT))
#endif

/* ---------------------------------------------------------------- */
/*  MISRAC Rule 4.9(DEFINE.FUNC) Deviation: The following           */
/*  function-like macros are intended to be used across different   */
/*  data types.                                                     */
/* ---------------------------------------------------------------- */
#define AUDIOLIB_min(x, y) (((x) < (y)) ? (x) : (y)) /**< A macro to return the minimum of 2 values. */
#define AUDIOLIB_max(x, y) (((x) < (y)) ? (y) : (x)) /**< A macro to return the maximum of 2 values. */
#define AUDIOLIB_ceilingDiv(x, y)                                                                                      \
   (((x) + (y) -1) / (y)) /**< A macro to return the ceiling of the division of two integers. */

/* ---------------------------------------------------------------- */
/*  MISRAC Rule 4.9(DEFINE.FUNC) Deviation: The advisory is not     */
/*  being addressed                                                 */
/* ---------------------------------------------------------------- */

/*! @brief MMA size as a function of precision */

#define AUDIOLIB_MMA_SIZE_8_BIT __MMA_A_COLS(sizeof(int8_t))
#define AUDIOLIB_MMA_SIZE_16_BIT __MMA_A_COLS(sizeof(int16_t))
#define AUDIOLIB_MMA_SIZE_32_BIT __MMA_A_COLS(sizeof(int32_t))

#define AUDIOLIB_MMA_BIAS_SIZE_32_BIT __MMA_A_COLS(sizeof(int32_t))
#define AUDIOLIB_MMA_BIAS_SIZE_64_BIT ((AUDIOLIB_MMA_BIAS_SIZE_32_BIT) >> 1)

/******************************************************************************
 *
 * COMMON MMA CONIFGURATIONS
 *
 ******************************************************************************/
#if defined(__C7X_MMA_2_256F__)
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i32f_i32f_o32f;
#endif // if defined(__C7X_MMA_2_256F__)

#ifdef C7X
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i32s_i32s_o32s;

extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i16s_i16s_o16s;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i16s_i16s_o16u;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i16u_i16s_o16s;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i16u_i16s_o16u;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i16s_i16u_o16u;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i16s_i16u_o16s;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i16u_i16u_o16s;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i16u_i16u_o16u;

extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i8s_i8s_o8s;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i8s_i8s_o8u;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i8u_i8s_o8s;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i8u_i8s_o8u;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i8s_i8u_o8s;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i8s_i8u_o8u;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i8u_i8u_o8s;
extern const __HWA_CONFIG_REG_v1 configRegisterStruct_i8u_i8u_o8u;

extern const __HWA_OFFSET_REG offsetRegStruct_zeros;
extern const __HWA_OFFSET_REG offsetRegStruct_diagonal_32bit;
extern const __HWA_OFFSET_REG offsetRegStruct_diagonal_16bit;
extern const __HWA_OFFSET_REG offsetRegStruct_diagonal_8bit;
#endif
/**@}*/
/** @endcond  */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* COMMON_AUDIOLIB_TYPES_H_ */
