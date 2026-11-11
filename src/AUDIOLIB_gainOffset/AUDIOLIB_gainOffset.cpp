// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_gainOffset_priv.h"

int32_t AUDIOLIB_gainOffset_getHandleSize(AUDIOLIB_gainOffset_InitArgs *pKerInitArgs)
{
   (void) pKerInitArgs; /* Unused parameter suppression: the size does not depend on the init args */

   /* PrivArgs, then the gain value and the offset value, each region rounded up to
      AUDIOLIB_L2DATA_ALIGNMENT so every region starts aligned (see AUDIOLIB_gainOffset_priv.h) */
   const size_t handleBytes = AUDIOLIB_GAINOFFSET_PRIVARGS_BYTES + (2U * AUDIOLIB_GAINOFFSET_VALUE_REGION_BYTES);

   return (int32_t) handleBytes;
}

AUDIOLIB_STATUS
AUDIOLIB_gainOffset_init_checkParams(AUDIOLIB_kernelHandle               handle,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                     const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                     const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if ((pKerInitArgs->pGain == NULL) || (pKerInitArgs->pOffset == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64) &&
          (bufParamsIn->data_type != AUDIOLIB_INT32)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if ((bufParamsIn->dim_x == 0U) || (bufParamsIn->dim_y == 0U) ||
               (bufParamsIn->dim_x != bufParamsOut->dim_x) || (bufParamsIn->dim_y != bufParamsOut->dim_y)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_gainOffset_init(AUDIOLIB_kernelHandle               handle,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsIn,
                                         const AUDIOLIB_bufParams2D_t       *bufParamsOut,
                                         const AUDIOLIB_gainOffset_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS               status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_init \n");

   pKerPrivArgs->dimX      = bufParamsIn->dim_x;
   pKerPrivArgs->dimY      = bufParamsIn->dim_y;
   const uint32_t strideInElems  = bufParamsIn->stride_y / (uint32_t) AUDIOLIB_sizeof(bufParamsIn->data_type);
   const uint32_t strideOutElems = bufParamsOut->stride_y / (uint32_t) AUDIOLIB_sizeof(bufParamsOut->data_type);
   pKerPrivArgs->strideIn        = (int32_t) strideInElems;
   pKerPrivArgs->strideOut       = (int32_t) strideOutElems;
   pKerPrivArgs->dataType  = bufParamsIn->data_type;

   /* Saturation parameters: only int32 data uses them (set below), 0 for the other types */
   pKerPrivArgs->satLo    = 0;
   pKerPrivArgs->satWidth = 0U;
   pKerPrivArgs->satRail  = 0;

   /* Regions after PrivArgs, each on an AUDIOLIB_L2DATA_ALIGNMENT boundary of the (aligned) handle */
   uint8_t *pRegion      = (uint8_t *) handle + AUDIOLIB_GAINOFFSET_PRIVARGS_BYTES;
   pKerPrivArgs->pGain   = (void *) pRegion;
   pRegion              += AUDIOLIB_GAINOFFSET_VALUE_REGION_BYTES;
   pKerPrivArgs->pOffset = (void *) pRegion;

   /* Copy *pGain and *pOffset once into the handle: exec() never touches the caller's variables.
      float for float and double data, int32_t for int32 data. */
   if ((bufParamsIn->data_type == AUDIOLIB_FLOAT32) || (bufParamsIn->data_type == AUDIOLIB_FLOAT64)) {
      *(float *) pKerPrivArgs->pGain   = *(const float *) pKerInitArgs->pGain;
      *(float *) pKerPrivArgs->pOffset = *(const float *) pKerInitArgs->pOffset;
   }
   else if (bufParamsIn->data_type == AUDIOLIB_INT32) {
      *(int32_t *) pKerPrivArgs->pGain   = *(const int32_t *) pKerInitArgs->pGain;
      *(int32_t *) pKerPrivArgs->pOffset = *(const int32_t *) pKerInitArgs->pOffset;

      const int64_t gain   = (int64_t) *(const int32_t *) pKerInitArgs->pGain;
      const int64_t offset = (int64_t) *(const int32_t *) pKerInitArgs->pOffset;
      int64_t       xLo    = (int64_t) INT32_MIN;
      int64_t       xHi    = (int64_t) INT32_MAX;

      if (gain > 0) {
         xLo = ((int64_t) INT32_MIN - offset) / gain;
         xHi = ((int64_t) INT32_MAX - offset) / gain;
      }
      else if (gain < 0) {
         xLo = ((int64_t) INT32_MAX - offset) / gain;
         xHi = ((int64_t) INT32_MIN - offset) / gain;
      }
      else {
         /* gain 0: every output is the offset, nothing saturates */
      }
      xLo = (xLo < (int64_t) INT32_MIN) ? (int64_t) INT32_MIN : xLo;
      xHi = (xHi > (int64_t) INT32_MAX) ? (int64_t) INT32_MAX : xHi;

      const int64_t width = xHi - xLo; /* 0 to 2^32 - 1 */

      pKerPrivArgs->satLo    = (int32_t) xLo;
      pKerPrivArgs->satWidth = (uint32_t) width;
      pKerPrivArgs->satRail  = (gain < 0) ? INT32_MIN : INT32_MAX;
   }
   else {
      /* Unsupported type: rejected below */
   }

   if (pKerInitArgs->funcStyle == (int8_t) AUDIOLIB_FUNCTION_OPTIMIZED) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         status                = AUDIOLIB_gainOffset_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_ci<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         status                = AUDIOLIB_gainOffset_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_ci<double>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_INT32) {
         status                = AUDIOLIB_gainOffset_init_ci<int32_t>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_ci<int32_t>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else if (pKerInitArgs->funcStyle == (int8_t) AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_cn<double>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_INT32) {
         pKerPrivArgs->execute = AUDIOLIB_gainOffset_exec_cn<int32_t>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_gainOffset_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_gainOffset_exec \n");

   AUDIOLIB_gainOffset_PrivArgs *pKerPrivArgs = (AUDIOLIB_gainOffset_PrivArgs *) handle;
   status                                     = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
