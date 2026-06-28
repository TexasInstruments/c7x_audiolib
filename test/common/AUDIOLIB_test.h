// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef TEST_AUDIOLIB_TEST_H_
#define TEST_AUDIOLIB_TEST_H_ 1

#include "TI_memory.h"
#include "TI_profile.h"
#include "TI_test.h"

#if (!LIMIT_WARM_REPS)
#define AUDIOLIB_NUM_WARM_REPS (100)
#else
#define AUDIOLIB_NUM_WARM_REPS (1)
#endif

#if defined(__C7100__) || defined(__C7120__) || defined(__C7502__) || defined(__C7504__) || defined(__C7508__) ||      \
    defined(__C7524__)
#include "c71/AUDIOLIB_test.h"
#else
#ifdef C7X
#error invalid target
#endif
#endif

/* // TBD: How to handle when __C7100__ is defined? */
/* #if defined(__C7120__) */
/* #include "c7120/AUDIOLIB_test.h" */
/* /\* #else */
/*  * #error invalid target *\/ */
/* #endif */

/* #if defined(__C7504__) */
/* #include "c7504/AUDIOLIB_test.h" */
/* /\* #else */
/*  * #error invalid target *\/ */
/* #endif */

#endif /* TEST_AUDIOLIB_TEST_H_ */
