// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef COMMON_AUDIOLIB_UTIL_SE1BLOAD_H_
#define COMMON_AUDIOLIB_UTIL_SE1BLOAD_H_ 1

/******************************************************************************/
#ifdef C7X
/******************************************************************************/
/*!
 * \ingroup audiolib_util
 * \brief   load numRows of B into the Bload buffer
 * \details use streaming engine 1 to load numRows of B into the Bload buffer
 * \param  numRows
 * \return  void
 */
/******************************************************************************/
__INLINE_FUNC(AUDIOLIB_UTIL_SE1Bload)
template <typename dataType, uint32_t UNROLL_FACTOR> static inline void AUDIOLIB_UTIL_SE1Bload(int32_t numRows)
{
   _nassert(numRows > 0);

   int32_t r;

   typedef typename c7x::make_full_vector<dataType>::type vec;

   AUDIOLIB_UNROLL(UNROLL_FACTOR)
   /* Load numRows vectors into B panel */
   for (r = 0; r < numRows; r++) {

      /* Fetch B vector from SE1 into C7x register */
      vec valB = c7x::strm_eng<1, vec>::get_adv();

      /* Load B matrix panel with B vector */
      __HWALDB(c7x::reinterpret<__mma_vec>(valB));

      /* Advance HWA State Machine */
      __HWAADV();
   }

   return;
}
#endif /* C7X */

#endif /* COMMON_AUDIOLIB_UTIL_SE1BLOAD_H_ */
