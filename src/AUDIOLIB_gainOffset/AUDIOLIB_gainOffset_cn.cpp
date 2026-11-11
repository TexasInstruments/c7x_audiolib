// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_gainOffset_priv.h"

/*
 * Per-type arithmetic of the natural-C reference:
 *   paramType : type of *pGain / *pOffset stored in the handle by init
 *   accType   : type gain * x + offset is computed in
 *
 *   | dataType | paramType | accType | output                         |
 *   |----------|-----------|---------|--------------------------------|
 *   | float    | float     | float   | acc                            |
 *   | double   | float     | double  | acc                            |
 *   | int32_t  | int32_t   | int64_t | sat32(acc), exact 64-bit madd  |
 */
template <typename dataType> struct AUDIOLIB_gainOffset_cnTypes;

template <> struct AUDIOLIB_gainOffset_cnTypes<float> {
   typedef float paramType;
   typedef float accType;
};

template <> struct AUDIOLIB_gainOffset_cnTypes<double> {
   typedef float  paramType;
   typedef double accType;
};

template <> struct AUDIOLIB_gainOffset_cnTypes<int32_t> {
   typedef int32_t paramType;
   typedef int64_t accType;
};

/* Accumulator to output sample: identity for float/double, saturation to int32 for int32. */
static inline float AUDIOLIB_gainOffset_cnToOut(float acc) { return acc; }

static inline double AUDIOLIB_gainOffset_cnToOut(double acc) { return acc; }

static inline int32_t AUDIOLIB_gainOffset_cnToOut(int64_t acc)
{
   int64_t sat = acc;

   if (sat > (int64_t) INT32_MAX) {
      sat = (int64_t) INT32_MAX;
   }
   else if (sat < (int64_t) INT32_MIN) {
      sat = (int64_t) INT32_MIN;
   }
   else {
      /* In range: nothing to clamp */
   }

   return (int32_t) sat;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_gainOffset_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   typedef typename AUDIOLIB_gainOffset_cnTypes<dataType>::paramType paramType;
   typedef typename AUDIOLIB_gainOffset_cnTypes<dataType>::accType   accType;

   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_gainOffset_exec_cn");

   const dataType *restrict pInLocal = (const dataType *) pIn;
   dataType *restrict pOutLocal      = (dataType *) pOut;

   const accType  gain      = (accType) *(const paramType *) pKerPrivArgs->pGain;
   const accType  offset    = (accType) *(const paramType *) pKerPrivArgs->pOffset;
   const uint32_t dimX      = pKerPrivArgs->dimX;
   const uint32_t dimY      = pKerPrivArgs->dimY;
   const uint32_t strideIn  = (uint32_t) pKerPrivArgs->strideIn;
   const uint32_t strideOut = (uint32_t) pKerPrivArgs->strideOut;

   for (uint32_t m = 0U; m < dimY; m++) {
      for (uint32_t n = 0U; n < dimX; n++) {
         /* int32: |x * gain| <= 2^62 and |offset| < 2^31, so the sum is exact in int64; clamp once. */
         const accType acc = ((accType) pInLocal[(m * strideIn) + n] * gain) + offset;

         pOutLocal[(m * strideOut) + n] = AUDIOLIB_gainOffset_cnToOut(acc);
      }
   }

   return status;
}

// explicit instantiation for the different data type versions
template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_cn<int32_t>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
