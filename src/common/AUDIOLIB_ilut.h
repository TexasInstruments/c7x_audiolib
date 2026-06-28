// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_ILUT_H_
#define AUDIOLIB_ILUT_H_ 1

#if defined(__C7524__)
#include "AUDIOLIB_types.h"
#include "c7x.h"
#include "c7x_scalable.h"

/**
 * \ingroup AUDIOLIB_COMMON
 */

/**
 * \defgroup AUDIOLIB_ILUT Internal Lookup table utilities
 * \brief This module consists of utility functions tailored towards LUT that are commonly applicable to various
 * AUDIOLIB kernels.
 */
/**@{*/

#define AUDIOLIB_UPPER_WORDS_OFFSET (0)
#define AUDIOLIB_LOWER_WORDS_OFFSET (128)

#define AUDIOLIB_KTABLE_OFFSET 0
#define AUDIOLIB_JTABLE_OFFSET 4
#define AUDIOLIB_LOGTABLE_OFFSET 8
#define AUDIOLIB_VTABLE_OFFSET 16

#if AUDIOLIB_DEBUGPRINT
static void print_uint_vec(c7x::uint_vec vec)
{
   const unsigned n = c7x::element_count_of<c7x::uint_vec>::value;

   for (unsigned i = 0; i < n; i++) {
      printf("[%u] = %u", i, vec.s[i]);

      if (i + 1 < n)
         printf(", ");
   }

   puts("");
}

static void print_uint_vec_hex(c7x::uint_vec vec)
{
   const unsigned n = c7x::element_count_of<c7x::uint_vec>::value;

   for (unsigned i = 0; i < n; i++) {
      printf("[%u] = 0x%X", i, vec.s[i]);

      if (i + 1 < n)
         printf(", ");
   }

   puts("");
}
#endif

/**
 * @brief This method reads bits 63-32 of LUT value at vecOffset.
 *
 * @tparam      vecType       : datatype of implementation
 * @param [in]  vecOffset   : input vector of LUT offsets
 *
 * @return  vector with LUT upper bit values.
 *
 * @remarks None
 */

template <typename vecType> static inline c7x::uint_vec AUDIOLIB_ILUTReadUpperBits(vecType indices)
{
   auto elements = __ilut_read_uint(indices + AUDIOLIB_UPPER_WORDS_OFFSET);

   return c7x::reinterpret<c7x::uint_vec>(elements);
}

/**
 * @brief This method reads bits 31-0 of LUT value at vecOffset.
 *
 * @tparam      vecType       : datatype of implementation
 * @param [in]  vecOffset   : input vector of LUT offsets
 *
 * @return  vector with LUT lower bit values.
 *
 * @remarks None
 */

template <typename vecType> static inline c7x::uint_vec AUDIOLIB_ILUTReadLowerBits(vecType indices)
{
   auto elements = __ilut_read_uint(indices + AUDIOLIB_LOWER_WORDS_OFFSET);

   return c7x::reinterpret<c7x::uint_vec>(elements);
}

/**
 * @brief This method intializes the upper and lower bit lookup tables.
 *
 * @remarks None
 */
static inline void AUDIOLIB_ILUTInit()
{
   const uint32_t tableSize = 20;

   // Upper Bits ILUT - Contains bits 63-32 for all lookup table values

   uint32_t AUDIOLIB_ILUTHigh[tableSize] = {
       0x3FF00000u, // 2^(0/4) 1.000000000 ti_math_kTable
       0x3FF306FEu, // 2^(1/4) 1.189207115
       0x3FF6A09Eu, // 2^(2/4) 1.414213562
       0x3FFAE89Fu, // 2^(3/4) 1.681792831
       0x3FF00000u, // 2^(0/16) 1.000000000 ti_math_jTable
       0x3FF0B558u, // 2^(1/16) 1.044273782
       0x3FF172B8u, // 2^(2/16) 1.090507733
       0x3FF2387Au, // 2^(3/16) 1.138788635
       0x00000000u, // 0.0000000000 ti_math_logtable
       0xBFBE2707u, // -0.1177830356
       0xBFCC8FF7u, // -0.2231435513
       0xBFD4618Bu, // -0.3184537311
       0xBFD9F323u, // -0.4054651081
       0xBFDF128Fu, // -0.4855078157
       0xBFE1E85Fu, // -0.5596157879
       0xBFE41D8Fu, // -0.6286086594
       0x00000000u, // 0.00000000000000000000 ti_math_vTable
       0x3FE0C152u, //  0.52359877559829887308
       0x3FF921FBu, // 1.57079632679489661923
       0x3FF0C152u  // 1.04719755119659774615
   };

   // Lower Bits ILUT - Contains bits 31-0 for all lookup table values

   uint32_t AUDIOLIB_ILUTLow[tableSize] = {
       0x00000000u, // 2^(0/4) 1.000000000 ti_math_kTable
       0x0A318737u, // 2^(1/4) 1.189207115
       0x6665983Eu, // 2^(2/4) 1.414213562
       0x997CAD14u, // 2^(3/4) 1.681792831
       0x00000000u, // 2^(0/16) 1.000000000 ti_math_jTable
       0x6CDC29EBu, // 2^(1/16) 1.044273782
       0x3C945254u, // 2^(2/16) 1.090507733
       0x6E861A8Cu, // 2^(3/16) 1.138788635
       0x00000000u, // 0.0000000000 ti_math_logtable
       0x6DECF45Eu, // -0.1177830356
       0xC792CA49u, // -0.2231435513
       0xC2174681u, // -0.3184537311
       0xECBD59C8u, // -0.4054651081
       0x5F9891BEu, // -0.4855078157
       0x5E6B627Du, // -0.5596157879
       0xE8435F76u, // -0.6286086594
       0x00000000u, // 0.00000000000000000000 ti_math_vTable
       0x382D7366u, //  0.52359877559829887308
       0x54442D18u, // 1.57079632679489661923
       0x382D7366u  // 1.04719755119659774615
   };

   for (uint32_t i = 0; i < tableSize; i++) {
      __ilut_init(i + AUDIOLIB_UPPER_WORDS_OFFSET, AUDIOLIB_ILUTHigh[i]);
   }

   for (uint32_t i = 0; i < tableSize; i++) {
      __ilut_init(i + AUDIOLIB_LOWER_WORDS_OFFSET, AUDIOLIB_ILUTLow[i]);
   }

#if AUDIOLIB_DEBUGPRINT
   // Print all LUT Values

   const auto n = c7x::element_count_of<c7x::uint_vec>::value;

   printf("Upper Bits\n");
   for (int i = 0; i < tableSize;) {
      c7x::uint_vec indices = c7x::uint_vec(0);

      for (unsigned int j = 0; i < tableSize && j < n; i++, j++) {
         indices.s[j] = i;
      }

      puts("values:");
      print_uint_vec_hex(AUDIOLIB_ILUTReadUpperBits(indices));
   }
   puts("");

   printf("Lower Bits\n");
   for (int i = 0; i < tableSize;) {
      c7x::uint_vec indices = c7x::uint_vec(0);

      for (unsigned int j = 0; i < tableSize && j < n; i++, j++) {
         indices.s[j] = i;
      }

      puts("values:");
      print_uint_vec_hex(AUDIOLIB_ILUTReadLowerBits(indices));
   }
   puts("");
#endif
}

/** @}*/

#endif

#endif
