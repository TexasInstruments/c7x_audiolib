// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef COMMON_AUDIOLIB_UTIL_DEBUGPRINTVECTOR_H_
#define COMMON_AUDIOLIB_UTIL_DEBUGPRINTFECTOR_H_ 1

#include "../AUDIOLIB_types.h"

//----------------------------------------------------------------------------------
#ifdef __cplusplus
#ifdef C7X
#include <c7x_scalable.h> // for device scalability

#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVector(c7x::uchar_vec vector)
{
#if AUDIOLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::uchar_vec>::value; i++) {
      AUDIOLIB_PRINTF("%04u", *((uchar *) (&vector) + i));
   }

   AUDIOLIB_PRINTF("%s", "\n");
#endif
}

#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVector(c7x::char_vec vector)
{
#if AUDIOLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::char_vec>::value; i++) {
      AUDIOLIB_PRINTF("%04d", *((int8_t *) (&vector) + i));
   }

   AUDIOLIB_PRINTF("%s", "\n");
#endif
}

#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVector(c7x::ushort_vec vector)
{
#if AUDIOLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::ushort_vec>::value; i++) {
      AUDIOLIB_PRINTF("%05u", *((ushort *) (&vector) + i));
   }

   AUDIOLIB_PRINTF("%s", "\n");
#endif
}

#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVector(c7x::short_vec vector)
{
#if AUDIOLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::short_vec>::value; i++) {
      AUDIOLIB_PRINTF("%05d", *((short *) (&vector) + i));
   }

   AUDIOLIB_PRINTF("%s", "\n");
#endif
}

#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVector(c7x::uint_vec vector)
{
#if AUDIOLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::uint_vec>::value; i++) {
      AUDIOLIB_PRINTF("%010u", *((uint *) (&vector) + i));
   }

   AUDIOLIB_PRINTF("%s", "\n");
#endif
}

#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVector(c7x::float_vec vector)
{
#if AUDIOLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::float_vec>::value; i++) {
      AUDIOLIB_PRINTF("%e ", *((float *) (&vector) + i));
   }

   AUDIOLIB_PRINTF("%s", "\n");
#endif
}

#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVector(c7x::double_vec vector)
{
#if MATHLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::double_vec>::value; i++) {
      MATHLIB_PRINTF("%lf", *((double *) (&vector) + i));
   }

   MATHLIB_PRINTF("%s", "\n");
#endif
}
#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVector(c7x::int_vec vector)
{
#if AUDIOLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::int_vec>::value; i++) {
      AUDIOLIB_PRINTF("%010d", *((int *) (&vector) + i));
   }

   AUDIOLIB_PRINTF("%s", "\n");
#endif
}

#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintVectorInHex(c7x::long_vec vector)
{
#if AUDIOLIB_DEBUGPRINT
   for (uint i = 0; i < c7x::element_count_of<c7x::long_vec>::value; i++) {
      AUDIOLIB_PRINTF("%016lx", *((long *) (&vector) + i));
   }

   AUDIOLIB_PRINTF("%s", "\n");
#endif
}
/*
#pragma FUNC_ALWAYS_INLINE
static inline void AUDIOLIB_debugPrintNonScalableVectorInHex(long8 vector)
{
#if AUDIOLIB_DEBUGPRINT
   AUDIOLIB_PRINTF("%010lx ", vector.s[7]);
   AUDIOLIB_PRINTF("%010lx ", vector.s[6]);
   AUDIOLIB_PRINTF("%010lx ", vector.s[5]);
   AUDIOLIB_PRINTF("%010lx ", vector.s[4]);
   AUDIOLIB_PRINTF("%010lx ", vector.s[3]);
   AUDIOLIB_PRINTF("%010lx ", vector.s[2]);
   AUDIOLIB_PRINTF("%010lx ", vector.s[1]);
   AUDIOLIB_PRINTF("%010lx ", vector.s[0]);

   AUDIOLIB_PRINTF("%s", "\n");
#endif
   return;
}
*/
#endif

#endif //#ifdef __cplusplus
#endif /* COMMON_AUDIOLIB_UTIL_DEBUGPRINTVECTOR_H_ */
