// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifdef C7X
#include "AUDIOLIB_permute.h"

// clang-format off

#if defined(_HOST_BUILD)

const c7x::uchar_vec AUDIOLIB_zeroVect_uchar_vec = c7x::uchar_vec(0);
const c7x::ushort_vec AUDIOLIB_zeroVect_ushort_vec = c7x::ushort_vec(0);

#if __C7X_VEC_SIZE_BYTES__ == 64
const c7x::uchar_vec AUDIOLIB_vperm_data_0_63 = c7x::uchar_vec(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63);
const c7x::uchar_vec AUDIOLIB_vperm_data_interweave_0_63 = c7x::uchar_vec(0, 1, 2, 3, 32, 33, 34, 35, 4, 5, 6, 7, 36, 37, 38, 39, 8, 9, 10, 11, 40, 41, 42, 43, 12, 13, 14, 15, 44, 45, 46, 47, 16, 17, 18, 19, 48, 49, 50, 51, 20, 21, 22, 23, 52, 53, 54, 55, 24, 25, 26, 27, 56, 57, 58, 59, 28, 29, 30, 31, 60, 61, 62, 63);
const c7x::uchar_vec AUDIOLIB_vperm_data_dp_interweave_0_63 = c7x::uchar_vec(0, 1, 2, 3, 4, 5, 6, 7, 32, 33, 34, 35, 36, 37, 38, 39, 8, 9, 10, 11, 12, 13, 14, 15, 40, 41, 42, 43, 44, 45, 46, 47, 16, 17, 18, 19, 20, 21, 22, 23, 48, 49, 50, 51, 52, 53, 54, 55, 24, 25, 26, 27, 28, 29, 30, 31, 56, 57, 58, 59, 60, 61, 62, 63);
#elif __C7X_VEC_SIZE_BYTES__ == 32
const c7x::uchar_vec AUDIOLIB_vperm_data_0_63 = c7x::uchar_vec(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
const c7x::uchar_vec AUDIOLIB_vperm_data_interweave_0_63 = c7x::uchar_vec(0, 1, 2, 3, 16, 17, 18, 19, 4, 5, 6, 7, 20, 21, 22, 23, 8, 9, 10, 11, 24, 25, 26, 27, 12, 13, 14, 15, 28, 29, 30, 31);
const c7x::uchar_vec AUDIOLIB_vperm_data_dp_interweave_0_63 = c7x::uchar_vec(0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31);
#endif // #if __C7X_VEC_SIZE_BYTES__

#else  // not host build.........................................................................................................................
const c7x::uchar_vec AUDIOLIB_zeroVect_uchar_vec = (c7x::uchar_vec)(0);
const c7x::ushort_vec AUDIOLIB_zeroVect_ushort_vec = (c7x::ushort_vec)(0);
#if __C7X_VEC_SIZE_BYTES__ == 64
const c7x::uchar_vec AUDIOLIB_vperm_data_0_63 = c7x::uchar_vec(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63);
const c7x::uchar_vec AUDIOLIB_vperm_data_interweave_0_63 = c7x::uchar_vec(0, 1, 2, 3, 32, 33, 34, 35, 4, 5, 6, 7, 36, 37, 38, 39, 8, 9, 10, 11, 40, 41, 42, 43, 12, 13, 14, 15, 44, 45, 46, 47, 16, 17, 18, 19, 48, 49, 50, 51, 20, 21, 22, 23, 52, 53, 54, 55, 24, 25, 26, 27, 56, 57, 58, 59, 28, 29, 30, 31, 60, 61, 62, 63);
const c7x::uchar_vec AUDIOLIB_vperm_data_dp_interweave_0_63 = c7x::uchar_vec(0, 1, 2, 3, 4, 5, 6, 7, 32, 33, 34, 35, 36, 37, 38, 39, 8, 9, 10, 11, 12, 13, 14, 15, 40, 41, 42, 43, 44, 45, 46, 47, 16, 17, 18, 19, 20, 21, 22, 23, 48, 49, 50, 51, 52, 53, 54, 55, 24, 25, 26, 27, 28, 29, 30, 31, 56, 57, 58, 59, 60, 61, 62, 63);
#elif __C7X_VEC_SIZE_BYTES__ == 32
const c7x::uchar_vec AUDIOLIB_vperm_data_0_63 = c7x::uchar_vec(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
const c7x::uchar_vec AUDIOLIB_vperm_data_interweave_0_63 = c7x::uchar_vec(0, 1, 2, 3, 16, 17, 18, 19, 4, 5, 6, 7, 20, 21, 22, 23, 8, 9, 10, 11, 24, 25, 26, 27, 12, 13, 14, 15, 28, 29, 30, 31);
const c7x::uchar_vec AUDIOLIB_vperm_data_dp_interweave_0_63 = c7x::uchar_vec(0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31);
#endif // #if __C7X_VEC_SIZE_BYTES__
#endif // _HOST_BUILD
#endif
// clang-format on
