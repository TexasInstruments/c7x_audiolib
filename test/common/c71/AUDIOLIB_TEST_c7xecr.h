// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_TEST_C7XECR_H_
#define AUDIOLIB_TEST_C7XECR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void AUDIOLIB_TEST_c7xSetTCR0(uint64_t param);

void AUDIOLIB_TEST_c7xSetTBR0(uint64_t param);

void AUDIOLIB_TEST_c7xSetMAR(uint64_t param);

void AUDIOLIB_TEST_c7xSetSCR(uint64_t param);

// added during J721E bring-up to perform TLB invalidate ALL after the MMU setup
// code
void AUDIOLIB_TEST_c7xMmuTlbInvAll(void);

void AUDIOLIB_TEST_c7xSetL2CFG(uint64_t param);

uint64_t AUDIOLIB_TEST_c7xGetL2CFG(void);

void AUDIOLIB_TEST_c7xSetL1DCFG(uint64_t param);

uint64_t AUDIOLIB_TEST_c7xGetL1DCFG(void);

#ifdef __cplusplus
}
#endif

#endif /* end of AUDIOLIB_TEST_C7XECR_H_ definition */
