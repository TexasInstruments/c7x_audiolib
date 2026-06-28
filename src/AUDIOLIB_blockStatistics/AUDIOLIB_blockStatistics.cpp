// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#include "AUDIOLIB_blockStatistics_priv.h"
#include "AUDIOLIB_bufParams.h"

int32_t AUDIOLIB_blockStatistics_getHandleSize(AUDIOLIB_blockStatistics_InitArgs *pKerInitArgs)
{
   int32_t privBufSize = sizeof(AUDIOLIB_blockStatistics_PrivArgs);
   return privBufSize;
}

AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_init_checkParams(AUDIOLIB_kernelHandle                    handle,
                                          const AUDIOLIB_bufParams2D_t            *bufParamsIn,
                                          const AUDIOLIB_bufParams2D_t            *bufParamsOut,
                                          const AUDIOLIB_blockStatistics_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   if (status == AUDIOLIB_SUCCESS) {
      if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else if (bufParamsIn->data_type != bufParamsOut->data_type) {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
      else {
         /* Nothing to do here */
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_blockStatistics_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                          const void *restrict pIn,
                                                          const void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_exec_checkParams \n");

   if ((pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      status = AUDIOLIB_SUCCESS;
   }

   return status;
}

/**
 * @brief API to set the kernel
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_blockStatistics_set(AUDIOLIB_kernelHandle             handle,
                                             AUDIOLIB_blockStatistics_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;
   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   if (status == AUDIOLIB_SUCCESS) {
      AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
      memcpy(&pKerPrivArgs->setArgs, pKerSetArgs, sizeof(AUDIOLIB_blockStatistics_SetArgs));
      pKerPrivArgs->statisticsType = pKerSetArgs->statisticsType;

      // Set execute function based on statistics type and funcStyle
      if (pKerSetArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         switch (pKerSetArgs->statisticsType) {
         case AUDIOLIB_STAT_BLOCK_MAX:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_max_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_MIN:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_min_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_MAX_ABS:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_maxAbs_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_MEAN:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_mean_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_RMS:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_rms_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_STDDEV:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_stdDev_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_VARIANCE:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_variance_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_AVG_ENERGY:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_avgEnergy_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_SUM:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_add_exec_cn<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_SUM_SQUARES:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_sqrAdd_exec_cn<dataType>;
            break;
         }
      }
      else {
         switch (pKerSetArgs->statisticsType) {
         case AUDIOLIB_STAT_BLOCK_MAX:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_max_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_MIN:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_min_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_MAX_ABS:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_maxAbs_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_MEAN:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_mean_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_RMS:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_rms_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_STDDEV:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_stdDev_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_VARIANCE:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_variance_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_AVG_ENERGY:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_avgEnergy_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_SUM:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_add_exec_ci<dataType>;
            break;
         case AUDIOLIB_STAT_BLOCK_SUM_SQUARES:
            pKerPrivArgs->execute = AUDIOLIB_blockStatistics_sqrAdd_exec_ci<dataType>;
            break;
         }
      }
   }

   return status;
}

// Non-template wrapper function with C linkage
extern "C" AUDIOLIB_STATUS AUDIOLIB_blockStatistics_set(AUDIOLIB_kernelHandle             handle,
                                                        AUDIOLIB_blockStatistics_SetArgs *pKerSetArgs)
{
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
   if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT32) {
      return AUDIOLIB_blockStatistics_set<float>(handle, pKerSetArgs);
   }
   else if (pKerPrivArgs->dataType == AUDIOLIB_FLOAT64) {
      return AUDIOLIB_blockStatistics_set<double>(handle, pKerSetArgs);
   }
   return AUDIOLIB_ERR_INVALID_TYPE;
}

AUDIOLIB_STATUS AUDIOLIB_blockStatistics_get(AUDIOLIB_kernelHandle             handle,
                                             AUDIOLIB_blockStatistics_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   if (handle == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (pKerSetArgs == NULL) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;
      memcpy(pKerSetArgs, &pKerPrivArgs->setArgs, sizeof(AUDIOLIB_blockStatistics_SetArgs));
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_blockStatistics_init(AUDIOLIB_kernelHandle                    handle,
                                              AUDIOLIB_bufParams2D_t                  *bufParamsIn,
                                              AUDIOLIB_bufParams2D_t                  *bufParamsOut,
                                              const AUDIOLIB_blockStatistics_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS                    status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_init \n");

   pKerPrivArgs->inputBlockSize  = bufParamsIn->dim_x;
   pKerPrivArgs->outputBlockSize = bufParamsOut->dim_x;
   pKerPrivArgs->numChannels     = bufParamsIn->dim_y;
   pKerPrivArgs->dataType        = bufParamsIn->data_type;

   if (pKerPrivArgs->numChannels == 1) {
      pKerPrivArgs->strideInElements  = 0; // For single channel, no stride is needed
      pKerPrivArgs->strideOutElements = 1;
   }
   else {
      pKerPrivArgs->strideInElements  = bufParamsIn->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
      pKerPrivArgs->strideOutElements = bufParamsOut->stride_y / AUDIOLIB_sizeof(bufParamsIn->data_type);
   }

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_blockStatistics_max_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_blockStatistics_max_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_blockStatistics_max_exec_ci<float>;
         status                = AUDIOLIB_blockStatistics_init_ci<float>(handle, bufParamsIn, bufParamsOut);
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_blockStatistics_max_exec_ci<double>;
         status                = AUDIOLIB_blockStatistics_init_ci<double>(handle, bufParamsIn, bufParamsOut);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_blockStatistics_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_STATUS status;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_blockStatistics_exec \n");

   AUDIOLIB_blockStatistics_PrivArgs *pKerPrivArgs = (AUDIOLIB_blockStatistics_PrivArgs *) handle;

   status = pKerPrivArgs->execute(handle, pIn, pOut);

   return status;
}
