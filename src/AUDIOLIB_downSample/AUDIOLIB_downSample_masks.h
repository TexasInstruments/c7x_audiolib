// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0


#ifndef AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_MASKS_H_
#define AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_MASKS_H_

#include <cstdint>

/*!
 * \file
 * \brief Permute/select mask tables and compile-time mask-selection traits used
 *        by the C7x downsample execution variants. Included only by
 *        AUDIOLIB_downSample_ci.cpp.
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

static const uint8_t downSampleRatio3DoublePermute[32] = {
    0,  1,  2,  3,  4,  5,  6,  7,  24, 25, 26, 27, 28, 29, 30, 31,
    16, 17, 18, 19, 20, 21, 22, 23, 8,  9,  10, 11, 12, 13, 14, 15,
};
static const uint8_t downSampleRatio3_6_FloatPermute[32] = {
    0,  1,  2,  3,  12, 13, 14, 15, 24, 25, 26, 27, 4,  5,  6,  7,
    16, 17, 18, 19, 28, 29, 30, 31, 8,  9,  10, 11, 20, 21, 22, 23,
};
static const uint8_t downSampleRatio3_6_SelFloat0[8] = {0, 1, 0, 0, 1, 0, 0, 1};
static const uint8_t downSampleRatio3_6_SelFloat1[8] = {0, 0, 1, 0, 0, 1, 0, 0};
static const uint8_t downSampleRatio3SelDouble0[4]   = {0, 0, 1, 0};
static const uint8_t downSampleRatio3SelDouble1[4]   = {0, 1, 0, 0};

static const uint8_t downSampleRatio4FloatPermute[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                                         11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                                         22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

static const uint8_t downSampleRatio4DoublePermute[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                                          11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                                                          22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

static const uint8_t downSampleRatio5FloatPermute[32] = {0,  1,  2,  3,  20, 21, 22, 23, 8,  9, 10,
                                                         11, 28, 29, 30, 31, 16, 17, 18, 19, 4, 5,
                                                         6,  7,  24, 25, 26, 27, 12, 13, 14, 15};
static const uint8_t downSampleRatio5SelFloat0[8]     = {0, 0, 1, 0, 0, 0, 0, 1};
static const uint8_t downSampleRatio5SelFloat1[8]     = {0, 1, 0, 0, 0, 0, 1, 0};
static const uint8_t downSampleRatio5SelFloat2[8]     = {0, 0, 0, 1, 0, 0, 0, 0};
static const uint8_t downSampleRatio5SelFloatC[8]     = {0, 1, 0, 0, 1, 0, 1, 0};

static const uint8_t downSampleRatio7FloatPermute[32] = {0,  1,  2,  3,  28, 29, 30, 31, 24, 25, 26, 27, 20, 21, 22, 23,
                                                         16, 17, 18, 19, 12, 13, 14, 15, 8,  9,  10, 11, 4,  5,  6,  7};
static const uint8_t downSampleRatio7SelFloat0[8]     = {0, 0, 0, 0, 0, 0, 1, 0};
static const uint8_t downSampleRatio7SelFloat1[8]     = {0, 0, 0, 0, 1, 0, 0, 0};
static const uint8_t downSampleRatio7SelFloat2[8]     = {0, 0, 1, 0, 0, 0, 0, 0};
static const uint8_t downSampleRatio7SelFloat3[8]     = {0, 1, 0, 0, 0, 0, 0, 0};
static const uint8_t downSampleRatio7SelFloatC1[8]    = {0, 0, 1, 1, 0, 0, 0, 0};
static const uint8_t downSampleRatio7SelFloatC2[8]    = {0, 0, 1, 1, 1, 1, 0, 0};

static const uint8_t downSampleRatio8FloatPermute0[32] = {0, 1, 2, 3, 8,  9,  10, 11, 16, 17, 18, 19, 24, 25, 26, 27,
                                                          4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31};

static const uint8_t downSampleRatio8FloatPermute1[32] = {4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31,
                                                          0, 1, 2, 3, 8,  9,  10, 11, 16, 17, 18, 19, 24, 25, 26, 27};

static const uint8_t downSampleRatio8SelFloat0[8] = {0, 0, 0, 0, 1, 1, 1, 1};

static const uint8_t downSampleRatio12FloatPermute0[32] = {0, 1, 2, 3, 12, 13, 14, 15, 24, 25, 26, 27, 0, 0, 0, 0,
                                                           0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0};

static const uint8_t downSampleRatio12FloatPermute1[32] = {0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 4, 5, 6, 7,
                                                           16, 17, 18, 19, 28, 29, 30, 31, 0, 0, 0, 0, 0, 0, 0, 0};

static const uint8_t downSampleRatio12FloatPermute2[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,
                                                           0, 0, 0, 0, 0, 0, 0, 0, 8, 9, 10, 11, 20, 21, 22, 23};

static const uint8_t downSampleRatio12SelFloat0[8] = {0, 0, 0, 1, 1, 1, 0, 0};
static const uint8_t downSampleRatio12SelFloat1[8] = {0, 0, 0, 0, 0, 0, 1, 1};

/*******************************************************************************
 *
 * MASK SELECTION TRAITS
 *
 * Pick the per-element-type permute/select masks at compile time, so the
 * ratio-3/6, ratio-4, ratio-5, ratio-7, ratio-8 and ratio-12 exec variants
 * carry no runtime type branch (the float and double instantiations each
 * resolve to a single mask set).
 *
 ******************************************************************************/

template <typename dataType> struct downSampleRatio3_6_Mask;
template <> struct downSampleRatio3_6_Mask<float> {
   static const uint8_t *permute() { return downSampleRatio3_6_FloatPermute; }
   static const uint8_t *sel0() { return downSampleRatio3_6_SelFloat0; }
   static const uint8_t *sel1() { return downSampleRatio3_6_SelFloat1; }
};
template <> struct downSampleRatio3_6_Mask<double> {
   static const uint8_t *permute() { return downSampleRatio3DoublePermute; }
   static const uint8_t *sel0() { return downSampleRatio3SelDouble0; }
   static const uint8_t *sel1() { return downSampleRatio3SelDouble1; }
};

template <typename dataType> struct downSampleRatio4Mask;
template <> struct downSampleRatio4Mask<float> {
   static const uint8_t *permute() { return downSampleRatio4FloatPermute; }
};
template <> struct downSampleRatio4Mask<double> {
   static const uint8_t *permute() { return downSampleRatio4DoublePermute; }
};

template <typename dataType> struct downSampleRatio5Mask;
template <> struct downSampleRatio5Mask<float> {
   static const uint8_t *permute() { return downSampleRatio5FloatPermute; }
   static const uint8_t *sel0() { return downSampleRatio5SelFloat0; }
   static const uint8_t *sel1() { return downSampleRatio5SelFloat1; }
   static const uint8_t *sel2() { return downSampleRatio5SelFloat2; }
   static const uint8_t *selC() { return downSampleRatio5SelFloatC; }
};

template <typename dataType> struct downSampleRatio7Mask;
template <> struct downSampleRatio7Mask<float> {
   static const uint8_t *permute() { return downSampleRatio7FloatPermute; }
   static const uint8_t *sel0() { return downSampleRatio7SelFloat0; }
   static const uint8_t *sel1() { return downSampleRatio7SelFloat1; }
   static const uint8_t *sel2() { return downSampleRatio7SelFloat2; }
   static const uint8_t *sel3() { return downSampleRatio7SelFloat3; }
   static const uint8_t *selC1() { return downSampleRatio7SelFloatC1; }
   static const uint8_t *selC2() { return downSampleRatio7SelFloatC2; }
};

template <typename dataType> struct downSampleRatio8Mask;
template <> struct downSampleRatio8Mask<float> {
   static const uint8_t *permute0() { return downSampleRatio8FloatPermute0; }
   static const uint8_t *permute1() { return downSampleRatio8FloatPermute1; }
   static const uint8_t *sel0() { return downSampleRatio8SelFloat0; }
};

template <typename dataType> struct downSampleRatio12Mask;
template <> struct downSampleRatio12Mask<float> {
   static const uint8_t *permute0() { return downSampleRatio12FloatPermute0; }
   static const uint8_t *permute1() { return downSampleRatio12FloatPermute1; }
   static const uint8_t *permute2() { return downSampleRatio12FloatPermute2; }

   static const uint8_t *sel0() { return downSampleRatio12SelFloat0; }
   static const uint8_t *sel1() { return downSampleRatio12SelFloat1; }
};

#endif /* AUDIOLIB_DOWNSAMPLE_IXX_IXX_OXX_MASKS_H_ */

/* ======================================================================== */
/*  End of file:  AUDIOLIB_downSample_masks.h                               */
/* ======================================================================== */
