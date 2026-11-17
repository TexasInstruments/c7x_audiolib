// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_MASKS_H_
#define AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_MASKS_H_
/******************************************************************************
 * NOTE:
 *
 * The permute-based implementation is optimized specifically for 256-bit
 * vector hardware. The permute masks and data arrangement assume a fixed
 * 256-bit vector length and are not directly applicable to wider vector
 * hardware.
 *
 * In addition, the float implementations for upsample factors 8, 16, and 24
 * are optimized by leveraging a fixed vector length of 8 float elements
 * (256 bits). These implementations are also specific to 256-bit vector
 * hardware.
 *
 * Float:
 *   - Upsample factors 3, 5, 6, 7, 10, 12, and 14 use the permute-based
 *     implementation.
 *   - Upsample factors 8, 16, and 24 use a vector-length-specific
 *     implementation based on a fixed 256-bit vector length.
 *   - All remaining upsample factors use the generic implementation.
 *
 * Double:
 *   - Upsample factors 3, 4, 5, 6, 7, and 8 use the permute-based
 *     implementation.
 *   - All remaining upsample factors use the generic implementation.
 *
 * For vector hardware wider than 256 bits, the permute masks and
 * vector-length-specific implementations must be redesigned to match the
 * target vector length.
 ******************************************************************************/
#include <cstdint>

/*!
 * \file
 * \brief Permute/select mask tables and compile-time mask-selection traits used
 *        by the C7x upsample execution variants. Included only by
 *        AUDIOLIB_upSample_ci.cpp.
 */

/*******************************************************************************
 *
 * PERMUTE / SELECT MASKS
 *
 * Constant lookup tables used by the ratio-3/6, ratio-4, ratio-5, ratio-7,
 * ratio-8 and ratio-12 execution variants. They are defined once at file scope
 * (instead of being rebuilt on the stack every exec call) and read with
 * stov_ptr inside each variant.
 *
 ******************************************************************************/

/*--------------------------------------------------------------------------*/
/* Double Permute  UPFACTOR 3: {} */
/*--------------------------------------------------------------------------*/
static const uint8_t upSampleRatio3_FloatPermute0[32] = {0,  1,  2, 3, 12, 13, 14, 15, 16, 17, 18,
                                                         19, 4,  5, 6, 7,  12, 13, 14, 15, 16, 17,
                                                         18, 19, 8, 9, 10, 11, 12, 13, 14, 15};

static const uint8_t upSampleRatio3_FloatPermute1[32] = {0,  1,  2,  3,  12, 13, 14, 15, 0, 1, 2, 3, 4,  5,  6,  7,
                                                         16, 17, 18, 19, 0,  1,  2,  3,  4, 5, 6, 7, 20, 21, 22, 23};

static const uint8_t upSampleRatio3_FloatPermute2[32] = {0, 1, 2, 3, 4,  5,  6,  7,  24, 25, 26, 27, 0, 1, 2, 3,
                                                         4, 5, 6, 7, 28, 29, 30, 31, 0,  1,  2,  3,  4, 5, 6, 7};

static const uint8_t upSampleRatio3_SelFloat0[8] = {0, 0, 0, 1, 1, 1, 1, 1};
static const uint8_t upSampleRatio3_SelFloat1[8] = {1, 1, 1, 0, 0, 0, 1, 1};
static const uint8_t upSampleRatio3_SelFloat2[8] = {1, 1, 1, 1, 1, 1, 0, 0};

static const uint8_t upSampleRatio3_DoublePermute0[32] = {0,  1,  2,  3,  4,  5,  6,  7,  16, 17, 18,
                                                          19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                                          30, 31, 8,  9,  10, 11, 12, 13, 14, 15};

static const uint8_t upSampleRatio3_DoublePermute1[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                                          11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

static const uint8_t upSampleRatio3_DoublePermute2[32] = {0, 1, 2,  3,  4,  5,  6,  7,  24, 25, 26, 27, 28, 29, 30, 31,
                                                          8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};

static const uint8_t upSampleRatio3_SelDouble0[4] = {0, 0, 1, 1};
static const uint8_t upSampleRatio3_SelDouble1[4] = {1, 1, 0, 1};
static const uint8_t upSampleRatio3_SelDouble2[4] = {1, 1, 1, 0};

/*--------------------------------------------------------------------------*/
/* Double Permute  UPFACTOR 4: {} */
/*--------------------------------------------------------------------------*/
static const uint8_t upSampleRatio4_DoublePermute0[32] = {0,  1,  2,  3,  4,  5,  6,  7,  16, 17, 18,
                                                          19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 16, 17, 18, 19, 20, 21, 22, 23};

static const uint8_t upSampleRatio4_DoublePermute1[32] = {8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
                                                          19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 16, 17, 18, 19, 20, 21, 22, 23};

/*--------------------------------------------------------------------------*/
/* Double Permute  UPFACTOR 5: {} */
/*--------------------------------------------------------------------------*/
static const uint8_t upSampleRatio5_FloatPermute0[32] = {0,  1,  2,  3,  16, 17, 18, 19, 16, 17, 18,
                                                         19, 16, 17, 18, 19, 16, 17, 18, 19, 4,  5,
                                                         6,  7,  16, 17, 18, 19, 16, 17, 18, 19};

static const uint8_t upSampleRatio5_FloatPermute1[32] = {16, 17, 18, 19, 16, 17, 18, 19, 8,  9,  10,
                                                         11, 16, 17, 18, 19, 16, 17, 18, 19, 16, 17,
                                                         18, 19, 16, 17, 18, 19, 12, 13, 14, 15};

static const uint8_t upSampleRatio5_FloatPermute3[32] = {16, 17, 18, 19, 4,  5,  6,  7,  16, 17, 18,
                                                         19, 16, 17, 18, 19, 16, 17, 18, 19, 16, 17,
                                                         18, 19, 8,  9,  10, 11, 16, 17, 18, 19};

static const uint8_t upSampleRatio5_SelFloat2[8] = {1, 1, 1, 1, 0, 1, 1, 1};

static const uint8_t upSampleRatio5_FloatPermute4[32] = {16, 17, 18, 19, 16, 17, 18, 19, 16, 17, 18,
                                                         19, 12, 13, 14, 15, 16, 17, 18, 19, 16, 17,
                                                         18, 19, 16, 17, 18, 19, 16, 17, 18, 19};

static const uint8_t upSampleRatio5_DoublePermute0[32] = {0,  1,  2,  3,  4,  5,  6,  7,  16, 17, 18,
                                                          19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 16, 17, 18, 19, 20, 21, 22, 23};

static const uint8_t upSampleRatio5_SelDouble0[8] = {0, 1, 1, 1};
static const uint8_t upSampleRatio5_SelDouble1[8] = {1, 0, 1, 1};
static const uint8_t upSampleRatio5_SelDouble2[8] = {1, 1, 0, 1};
static const uint8_t upSampleRatio5_SelDouble3[8] = {1, 1, 1, 0};

/*--------------------------------------------------------------------------*/
/* Double Permute  UPFACTOR 6: {} */
/*--------------------------------------------------------------------------*/
static const uint8_t upSampleRatio6_FloatPermute0[32] = {0,  1,  2,  3,  16, 17, 18, 19, 16, 17, 18,
                                                         19, 16, 17, 18, 19, 16, 17, 18, 19, 16, 17,
                                                         18, 19, 4,  5,  6,  7,  16, 17, 18, 19};
static const uint8_t upSampleRatio6_FloatPermute1[32] = {16, 17, 18, 19, 16, 17, 18, 19, 16, 17, 18,
                                                         19, 16, 17, 18, 19, 8,  9,  10, 11, 16, 17,
                                                         18, 19, 16, 17, 18, 19, 16, 17, 18, 19};
static const uint8_t upSampleRatio6_FloatPermute2[32] = {16, 17, 18, 19, 16, 17, 18, 19, 12, 13, 14,
                                                         15, 16, 17, 18, 19, 16, 17, 18, 19, 16, 17,
                                                         18, 19, 16, 17, 18, 19, 16, 17, 18, 19};

static const uint8_t upSampleRatio6_DoublePermute0[32] = {0,  1,  2,  3,  4,  5,  6,  7,  16, 17, 18,
                                                          19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 16, 17, 18, 19, 20, 21, 22, 23};
static const uint8_t upSampleRatio6_DoublePermute1[32] = {16, 17, 18, 19, 20, 21, 22, 23, 16, 17, 18,
                                                          19, 20, 21, 22, 23, 8,  9,  10, 11, 12, 13,
                                                          14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
/*--------------------------------------------------------------------------*/
/* MASK  UPFACTOR 7: {} */
/*--------------------------------------------------------------------------*/
static const uint8_t upSampleRatio7_FloatPermute0[32] = {0,  1,  2,  3,  16, 17, 18, 19, 16, 17, 18, 19, 16, 17, 18, 19,
                                                         16, 17, 18, 19, 16, 17, 18, 19, 16, 17, 18, 19, 4,  5,  6,  7};

static const uint8_t upSampleRatio7_FloatPermute1[32] = {16, 17, 18, 19, 16, 17, 18, 19, 16, 17, 18,
                                                         19, 16, 17, 18, 19, 16, 17, 18, 19, 16, 17,
                                                         18, 19, 8,  9,  10, 11, 16, 17, 18, 19};

static const uint8_t upSampleRatio7_SelFloat2[8] = {1, 1, 1, 0, 1, 1, 1, 1};

static const uint8_t upSampleRatio7_SelFloat3[8] = {1, 1, 1, 1, 0, 1, 1, 1};

static const uint8_t upSampleRatio7_SelFloat4[8] = {1, 1, 1, 1, 1, 0, 1, 1};

static const uint8_t upSampleRatio7_FloatPermute5[32] = {16, 17, 18, 19, 24, 25, 26, 27, 8,  9,  10,
                                                         11, 16, 17, 18, 19, 16, 17, 18, 19, 16, 17,
                                                         18, 19, 16, 17, 18, 19, 16, 17, 18, 19};

static const uint8_t upSampleRatio7_FloatPermute6[32] = {28, 29, 30, 31, 12, 13, 14, 15, 16, 17, 18,
                                                         19, 16, 17, 18, 19, 16, 17, 18, 19, 16, 17,
                                                         18, 19, 16, 17, 18, 19, 16, 17, 18, 19};

static const uint8_t upSampleRatio7_DoublePermute0[32] = {0,  1,  2,  3,  4,  5,  6,  7,  16, 17, 18,
                                                          19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 16, 17, 18, 19, 20, 21, 22, 23};

static const uint8_t upSampleRatio7_DoublePermute1[32] = {16, 17, 18, 19, 20, 21, 22, 23, 16, 17, 18,
                                                          19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 8,  9,  10, 11, 12, 13, 14, 15};

static const uint8_t upSampleRatio7_DoublePermute2[32] = {16, 17, 18, 19, 20, 21, 22, 23, 16, 17, 18,
                                                          19, 20, 21, 22, 23, 0,  1,  2,  3,  4,  5,
                                                          6,  7,  16, 17, 18, 19, 20, 21, 22, 23};
static const uint8_t upSampleRatio7_DoublePermute3[32] = {16, 17, 18, 19, 20, 21, 22, 23, 8,  9,  10,
                                                          11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 16, 17, 18, 19, 20, 21, 22, 23};

/*--------------------------------------------------------------------------*/
/* Double Permute  UPFACTOR 8 {} */
/*--------------------------------------------------------------------------*/
static const uint8_t upSampleRatio8_DoublePermute0[32] = {0,  1,  2,  3,  4,  5,  6,  7,  16, 17, 18,
                                                          19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 16, 17, 18, 19, 20, 21, 22, 23};

static const uint8_t upSampleRatio8_DoublePermute1[32] = {8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
                                                          19, 20, 21, 22, 23, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 16, 17, 18, 19, 20, 21, 22, 23};
/*--------------------------------------------------------------------------*/
/* ASSIGN MASK  */
/*--------------------------------------------------------------------------*/
template <typename dataType> struct upSampleRatio3_Mask;
template <> struct upSampleRatio3_Mask<float> {
   static const uint8_t *sel0() { return upSampleRatio3_SelFloat0; }
   static const uint8_t *sel1() { return upSampleRatio3_SelFloat1; }
   static const uint8_t *sel2() { return upSampleRatio3_SelFloat2; }

   static const uint8_t *permute0() { return upSampleRatio3_FloatPermute0; }
   static const uint8_t *permute1() { return upSampleRatio3_FloatPermute1; }
   static const uint8_t *permute2() { return upSampleRatio3_FloatPermute2; }
};
template <> struct upSampleRatio3_Mask<double> {
   static const uint8_t *sel0() { return upSampleRatio3_SelDouble0; }
   static const uint8_t *sel1() { return upSampleRatio3_SelDouble1; }
   static const uint8_t *sel2() { return upSampleRatio3_SelDouble2; }
   static const uint8_t *permute0() { return upSampleRatio3_DoublePermute0; }
   static const uint8_t *permute1() { return upSampleRatio3_DoublePermute1; }
   static const uint8_t *permute2() { return upSampleRatio3_DoublePermute2; }
};

template <typename dataType> struct upSampleRatio5_Mask;
template <> struct upSampleRatio5_Mask<float> {
   static const uint8_t *permute0() { return upSampleRatio5_FloatPermute0; }
   static const uint8_t *permute1() { return upSampleRatio5_FloatPermute1; }
   static const uint8_t *sel1() { return upSampleRatio5_SelFloat2; }
   static const uint8_t *permute3() { return upSampleRatio5_FloatPermute3; }
   static const uint8_t *permute4() { return upSampleRatio5_FloatPermute4; }
};
template <> struct upSampleRatio5_Mask<double> {
   static const uint8_t *sel0() { return upSampleRatio5_SelDouble0; }
   static const uint8_t *sel1() { return upSampleRatio5_SelDouble1; }
   static const uint8_t *sel2() { return upSampleRatio5_SelDouble2; }
   static const uint8_t *sel3() { return upSampleRatio5_SelDouble3; }
};

template <typename dataType> struct upSampleRatio4_Mask;

template <> struct upSampleRatio4_Mask<double> {
   static const uint8_t *permute0() { return upSampleRatio4_DoublePermute0; }
   static const uint8_t *permute1() { return upSampleRatio4_DoublePermute1; }
};

template <typename dataType> struct upSampleRatio6_Mask;
template <> struct upSampleRatio6_Mask<float> {
   static const uint8_t *permute0() { return upSampleRatio6_FloatPermute0; }
   static const uint8_t *permute1() { return upSampleRatio6_FloatPermute1; }
   static const uint8_t *permute2() { return upSampleRatio6_FloatPermute2; }
};

template <> struct upSampleRatio6_Mask<double> {
   static const uint8_t *permute0() { return upSampleRatio6_DoublePermute0; }
   static const uint8_t *permute1() { return upSampleRatio6_DoublePermute1; }
};

template <typename dataType> struct upSampleRatio7_Mask;
template <> struct upSampleRatio7_Mask<float> {
   static const uint8_t *permute0() { return upSampleRatio7_FloatPermute0; }
   static const uint8_t *permute1() { return upSampleRatio7_FloatPermute1; }
   static const uint8_t *sel2() { return upSampleRatio7_SelFloat2; }
   static const uint8_t *sel3() { return upSampleRatio7_SelFloat3; }
   static const uint8_t *sel4() { return upSampleRatio7_SelFloat4; }
   static const uint8_t *permute5() { return upSampleRatio7_FloatPermute5; }
   static const uint8_t *permute6() { return upSampleRatio7_FloatPermute6; }
};

template <> struct upSampleRatio7_Mask<double> {
   static const uint8_t *permute0() { return upSampleRatio7_DoublePermute0; }
   static const uint8_t *permute1() { return upSampleRatio7_DoublePermute1; }
   static const uint8_t *permute2() { return upSampleRatio7_DoublePermute2; }
   static const uint8_t *permute3() { return upSampleRatio7_DoublePermute3; }
};

template <typename dataType> struct upSampleRatio8_Mask;
template <> struct upSampleRatio8_Mask<double> {
   static const uint8_t *permute0() { return upSampleRatio8_DoublePermute0; }
   static const uint8_t *permute1() { return upSampleRatio8_DoublePermute1; }
};

#endif /* AUDIOLIB_UPSAMPLE_IXX_IXX_OXX_MASKS_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample_masks.h                               */
/* ======================================================================== */
