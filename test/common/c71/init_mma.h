// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef INIT_MMA_H_
#define INIT_MMA_H_

#ifdef C7X
#include <c7x.h>
#else
#include <arm_neon.h>
#endif
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_mma();
#ifdef __cplusplus
}
#endif // extern "C"

#endif
