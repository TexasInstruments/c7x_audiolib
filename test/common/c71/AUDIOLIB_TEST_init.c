// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_test.h"

#if !defined(_HOST_BUILD)
extern const uint64_t pte_lvl0[512];
#endif

void AUDIOLIB_TEST_init()
{

#if __C7X_VEC_SIZE_BITS__ == 512
   // Known silicon bug, errata: TBD
   __sa_set_cr(0, __sa_get_cr(1));
#endif

#if !defined(_HOST_BUILD)
   // enable MMU
   enable_cache_mmu((uint64_t) pte_lvl0);

   // invalidate TLB
   invalidate_tlb();
#endif

#ifndef AUDIOLIB_FIXED_SEED
   time_t t;
   time(&t);
   AUDIOLIB_DEBUGPRINTFN(1, "rand seed = %u\n", (unsigned) t);
   srand((unsigned) t);
#else
   AUDIOLIB_DEBUGPRINTFN(1, "fixed seed = %u\n", (unsigned) AUDIOLIB_FIXED_SEED);
   srand((unsigned) AUDIOLIB_FIXED_SEED);
#endif

#if !defined(DEMO)
#if defined(_HOST_BUILD)
#if defined(AUDIOLIB_DEBUGPRINT)
   mma::set_debug_level(AUDIOLIB_DEBUGPRINT);
   streaming_engine::set_debug_level(AUDIOLIB_DEBUGPRINT);
   sa_generator::set_debug_level(AUDIOLIB_DEBUGPRINT);
#endif
#endif
#endif
}
