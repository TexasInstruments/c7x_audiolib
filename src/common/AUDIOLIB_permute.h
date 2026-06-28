// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_PERMUTE_H_
#define AUDIOLIB_PERMUTE_H_ 1

#ifdef C7X
#include "c7x_scalable.h"
#include "stdio.h"

// clang-format off

#if defined(_HOST_BUILD)

extern const c7x::uchar_vec AUDIOLIB_zeroVect_uchar_vec;
extern const c7x::ushort_vec AUDIOLIB_zeroVect_ushort_vec;

#if __C7X_VEC_SIZE_BYTES__ == 64
extern const c7x::uchar_vec AUDIOLIB_vperm_data_0_63;
extern const c7x::uchar_vec AUDIOLIB_vperm_data_interweave_0_63; 
extern const c7x::uchar_vec AUDIOLIB_vperm_data_dp_interweave_0_63;
#elif __C7X_VEC_SIZE_BYTES__ == 32
extern const c7x::uchar_vec AUDIOLIB_vperm_data_0_63;
extern const c7x::uchar_vec AUDIOLIB_vperm_data_interweave_0_63;
extern const c7x::uchar_vec AUDIOLIB_vperm_data_dp_interweave_0_63;
#endif // #if __C7X_VEC_SIZE_BYTES__

#else  // not host build..........................................
extern const c7x::uchar_vec AUDIOLIB_zeroVect_uchar_vec;
extern const c7x::ushort_vec AUDIOLIB_zeroVect_ushort_vec;
#if __C7X_VEC_SIZE_BYTES__ == 64
extern const c7x::uchar_vec AUDIOLIB_vperm_data_0_63;
extern const c7x::uchar_vec AUDIOLIB_vperm_data_interweave_0_63;
extern const c7x::uchar_vec AUDIOLIB_vperm_data_dp_interweave_0_63;
#elif __C7X_VEC_SIZE_BYTES__ == 32
extern const c7x::uchar_vec AUDIOLIB_vperm_data_0_63;
extern const c7x::uchar_vec AUDIOLIB_vperm_data_interweave_0_63;
extern const c7x::uchar_vec AUDIOLIB_vperm_data_dp_interweave_0_63;
#endif // #if __C7X_VEC_SIZE_BYTES__
#endif // _HOST_BUILD
#endif
// clang-format on

#endif // AUDIOLIB_PERMUTE_H_
