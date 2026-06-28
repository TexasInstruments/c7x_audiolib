// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_typeConversion_priv.h"
#include <cstdint>
#include <math.h>

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_q15ToFloat_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_q15ToFloat_exec_cn");

   int16_t *restrict pInLocal   = (int16_t *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples   = pKerPrivArgs->samples;
   uint32_t channels  = pKerPrivArgs->channels;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   for (uint32_t i = 0; i < channels; i++) {
      for (uint32_t j = 0; j < samples; j++) {
         pOutLocal[i * strideOut + j] = (dataType) pInLocal[i * strideIn + j] * Q15_SCALE_FACTOR_INV;
      }
   }

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q15 to FLOAT natural c code");

   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floatToQ15_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_floatToQ15_exec_cn");
   dataType *restrict pInLocal = (dataType *) pIn;
   int16_t *restrict pOutLocal = (int16_t *) pOut;

   uint32_t samples   = pKerPrivArgs->samples;
   uint32_t channels  = pKerPrivArgs->channels;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   dataType mulFactor = Q15_SCALE_FACTOR;
   int32_t  aInt      = 0;
   int16_t  out       = 0;

   for (uint32_t i = 0; i < channels; i++) {
      for (uint32_t j = 0; j < samples; j++) {
         dataType mulVal   = pInLocal[i * strideIn + j] * mulFactor;
         int32_t  intPart  = (int32_t) mulVal;
         dataType fracPart = mulVal - intPart;

         if ((intPart % 2 == 0) && (fabs(fracPart) == 0.5)) {
            aInt = intPart;
         }
         else if (pInLocal[i * strideIn + j] < 0) {
            aInt = (int32_t) (mulVal - 0.5f);
         }
         else {
            aInt = (int32_t) (mulVal + 0.5f);
         }

         /* saturate to 16-bit */
         if (aInt > Q15_MAX) {
            aInt = Q15_MAX;
         }
         if (aInt < Q15_MIN) {
            aInt = Q15_MIN;
         }
         out = (short) aInt;

         pOutLocal[i * strideOut + j] = out;
      }
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for  FLOAT to Q15 natural c code");
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_q23ToFloat_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_q23ToFloat_exec_cn");

   int32_t *restrict pInLocal   = (int32_t *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples   = pKerPrivArgs->samples;
   uint32_t channels  = pKerPrivArgs->channels;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   for (uint32_t i = 0; i < channels; i++) {
      for (uint32_t j = 0; j < samples; j++) {
         pOutLocal[i * strideOut + j] = (dataType) (pInLocal[i * strideIn + j] * Q23_SCALE_FACTOR_INV);
      }
   }

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q23 to FLOAT natural c code");
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_q31ToFloat_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_q23ToFloat_exec_cn");

   int32_t *restrict pInLocal   = (int32_t *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t samples   = pKerPrivArgs->samples;
   uint32_t channels  = pKerPrivArgs->channels;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   for (uint32_t i = 0; i < channels; i++) {
      for (uint32_t j = 0; j < samples; j++) {
         pOutLocal[i * strideOut + j] = (dataType) (pInLocal[i * strideIn + j] * Q31_SCALE_FACTOR_INV);
      }
   }

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for Q23 to FLOAT natural c code");
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floatToQ23_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_floatToQ23_exec_cn");

   dataType *restrict pInLocal = (dataType *) pIn;
   int32_t *restrict pOutLocal = (int32_t *) pOut;

   uint32_t samples   = pKerPrivArgs->samples;
   uint32_t channels  = pKerPrivArgs->channels;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   dataType mulFactor = Q23_SCALE_FACTOR;
   int32_t  aInt      = 0;
   int32_t  out       = 0;

   for (uint32_t i = 0; i < channels; i++) {
      for (uint32_t j = 0; j < samples; j++) {
         dataType mulVal   = pInLocal[i * strideIn + j] * mulFactor;
         int32_t  intPart  = (int32_t) mulVal;
         dataType fracPart = mulVal - intPart;

         if ((intPart % 2 == 0) && (fabs(fracPart) == 0.5)) {
            aInt = intPart;
         }
         else if (pInLocal[i * strideIn + j] < 0) {
            aInt = (int32_t) (mulVal - 0.5f);
         }
         else {
            aInt = (int32_t) (mulVal + 0.5f);
         }

         if (aInt > Q23_MAX) {
            aInt = Q23_MAX;
         }
         if (aInt < Q23_MIN) {
            aInt = Q23_MIN;
         }
         out = (int32_t) aInt;

         pOutLocal[i * strideOut + j] = out;
      }
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT to Q23 natural c code");
   return status;
}

template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_floatToQ31_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS                   status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_typeConversion_PrivArgs *pKerPrivArgs = (AUDIOLIB_typeConversion_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_floatToQ31_exec_cn");

   dataType *restrict pInLocal = (dataType *) pIn;
   int32_t *restrict pOutLocal = (int32_t *) pOut;

   uint32_t samples   = pKerPrivArgs->samples;
   uint32_t channels  = pKerPrivArgs->channels;
   int32_t  strideIn  = pKerPrivArgs->strideIn;
   int32_t  strideOut = pKerPrivArgs->strideOut;

   dataType mulFactor = Q31_SCALE_FACTOR;

   for (uint32_t i = 0; i < channels; i++) {
      for (uint32_t j = 0; j < samples; j++) {
         dataType val    = pInLocal[i * strideIn + j];
         double   mulVal = static_cast<double>(val) * mulFactor; // promote to double

         // --- Handle NaN/Inf manually ---
         if (!(mulVal == mulVal) || (mulVal > 1e308) || (mulVal < -1e308)) {
            pOutLocal[i * strideOut + j] = 0;
            continue;
         }

         // --- Saturate before rounding ---
         if (mulVal >= (double) INT32_MAX)
            mulVal = (double) INT32_MAX;
         else if (mulVal <= (double) INT32_MIN)
            mulVal = (double) INT32_MIN;

         // --- Round to nearest, ties to even ---
         double floorVal = floor(mulVal);
         double diff     = mulVal - floorVal;
         double rounded;
         if (diff > 0.5)
            rounded = floorVal + 1.0;
         else if (diff < 0.5)
            rounded = floorVal;
         else // diff == 0.5
            rounded = (fmod(floorVal, 2.0) == 0.0) ? floorVal : floorVal + 1.0;

         // --- Final integer saturation ---
         int64_t intVal = static_cast<int64_t>(rounded);
         if (intVal > INT32_MAX)
            intVal = INT32_MAX;
         else if (intVal < INT32_MIN)
            intVal = INT32_MIN;

         pOutLocal[i * strideOut + j] = static_cast<int32_t>(intVal);
      }
   }
   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Exiting template function for FLOAT to Q23 natural c code");
   return status;
}

template AUDIOLIB_STATUS
AUDIOLIB_q15ToFloat_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q15ToFloat_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ15_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ15_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q23ToFloat_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q23ToFloat_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q31ToFloat_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_q31ToFloat_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ23_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ23_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ31_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

template AUDIOLIB_STATUS
AUDIOLIB_floatToQ31_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);
