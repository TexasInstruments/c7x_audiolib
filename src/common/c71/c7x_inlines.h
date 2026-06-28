// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef COMMON_AUDIOLIB_C7X_INLINES_H_
#define COMMON_AUDIOLIB_C7X_INLINES_H_ 1

/******************************************************************************/

/******************************************************************************/
/**
 * \ingroup c7x_inlines
 * \brief   c7x specific inlines
 *
 * \details
 * \param
 * \return  void
 */
/******************************************************************************/

#ifdef C7X
#ifdef __cplusplus
typedef typename c7x::uchar_vec ucharVec;

__INLINE_FUNC(c7x_permute_even_even_xxx)
template <typename dataType>
static inline ucharVec c7x_permute_even_even_xxx(ucharVec permCtrl, ucharVec in1, ucharVec in0)
{
   ucharVec retVal;

   if (sizeof(dataType) == sizeof(int32_t)) {
      retVal = __permute_even_even_int(permCtrl, in1, in0);
   }
   else if (sizeof(dataType) == sizeof(int16_t)) {
      retVal = __permute_even_even_short(permCtrl, in1, in0);
   }
   else {
      retVal = (ucharVec) (0);
   }
   return retVal;
}

__INLINE_FUNC(c7x_permute_odd_odd_xxx)
template <typename dataType>
static inline ucharVec c7x_permute_odd_odd_xxx(ucharVec permCtrl, ucharVec in1, ucharVec in0)
{
   ucharVec retVal;

   if (sizeof(dataType) == sizeof(int32_t)) {
      retVal = __permute_odd_odd_int(permCtrl, in1, in0);
   }
   else if (sizeof(dataType) == sizeof(int16_t)) {
      retVal = __permute_odd_odd_short(permCtrl, in1, in0);
   }
   else {
      retVal = (ucharVec) (0);
   }
   return retVal;
}
#endif /* __cplusplus */
#endif /* C7X */

#endif /* COMMON_AUDIOLIB_C7X_INLINES_H_ */
