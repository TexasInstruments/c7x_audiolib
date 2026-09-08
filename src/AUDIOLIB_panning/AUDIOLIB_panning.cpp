// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

/**
******************************************************************************
*  @file     AUDIOLIB_panning.cpp
*******************************************************************************
*/

#include <float.h>
#include <math.h>
#include <string.h>

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_panning_priv.h"
#include "audiolib.h"

/*******************************************************************************
 *
 * PUBLIC API
 *
 ******************************************************************************/

int32_t AUDIOLIB_panning_getHandleSize(AUDIOLIB_panning_InitArgs *pKerInitArgs)
{
   int32_t handleSize = -1;

   if ((pKerInitArgs != NULL) && (pKerInitArgs->numChannels > 0)) {
      /* The handle is [PrivArgs | channelMap | nodes | currentGain], each region
       * rounded up to 64 bytes. The sizes are summed in 64 bits so that a huge
       * channel count cannot overflow; a total that does not fit the int32_t
       * return value is reported as -1. */
      const uint64_t numChannels = (uint64_t) pKerInitArgs->numChannels;
      const uint64_t alignMask   = 63U;

      const uint64_t privBufSize     = ((uint64_t) sizeof(AUDIOLIB_panning_PrivArgs) + alignMask) & ~alignMask;
      const uint64_t channelMapSize  = (((uint64_t) sizeof(AUDIOLIB_channelConfig_t) * numChannels) + alignMask) & ~alignMask;
      const uint64_t vbapNodeSize    = (((uint64_t) sizeof(AUDIOLIB_vbapNode_t) * numChannels) + alignMask) & ~alignMask;
      const uint64_t currentGainSize = (((uint64_t) sizeof(float) * numChannels) + alignMask) & ~alignMask;

      const uint64_t totalSize = privBufSize + channelMapSize + vbapNodeSize + currentGainSize;

      if (totalSize <= (uint64_t) INT32_MAX) {
         handleSize = (int32_t) totalSize;
      }
   }

   return handleSize;
}

AUDIOLIB_STATUS
AUDIOLIB_panning_init_checkParams(AUDIOLIB_kernelHandle            handle,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsIn,
                                  const AUDIOLIB_bufParams1D_t    *bufParamsGain,
                                  const AUDIOLIB_bufParams2D_t    *bufParamsOut,
                                  const AUDIOLIB_panning_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_init_checkParams \n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsGain == NULL) || (bufParamsOut == NULL) ||
       (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) || (bufParamsOut->data_type != bufParamsIn->data_type) ||
            (bufParamsGain->data_type != bufParamsIn->data_type)) {
      status = AUDIOLIB_ERR_INVALID_TYPE;
   }
   else if (((pKerInitArgs->isInterleave != 0) && (pKerInitArgs->isInterleave != 1)) ||
            (pKerInitArgs->numChannels <= 0)) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Interleaved: output rows are samples and the mono input is a column with
       * row stride stride_y. Planar: output rows are channels and the input is a
       * single row. */
      const bool     isInterleave = (pKerInitArgs->isInterleave == 1);
      const uint32_t numChannels  = (uint32_t) pKerInitArgs->numChannels;
      const uint32_t elemSize     = (uint32_t) sizeof(float);
      const uint32_t outChannels  = isInterleave ? bufParamsOut->dim_x : bufParamsOut->dim_y;
      const uint32_t outSamples   = isInterleave ? bufParamsOut->dim_y : bufParamsOut->dim_x;
      const uint32_t inSamples    = isInterleave ? bufParamsIn->dim_y : bufParamsIn->dim_x;
      /* Strides are signed; compare in 64 bits so a negative stride fails too. */
      const int64_t minOutStride = (int64_t) bufParamsOut->dim_x * (int64_t) elemSize;
      const int64_t minInStride  = isInterleave ? (int64_t) elemSize : 0;

      if ((numChannels != outChannels) || (bufParamsGain->dim_x < numChannels) || (outSamples == 0U) ||
          (inSamples != outSamples) || ((int64_t) bufParamsOut->stride_y < minOutStride) ||
          ((int64_t) bufParamsIn->stride_y < minInStride)) {
         status = AUDIOLIB_ERR_INVALID_DIMENSION;
      }
      else if (pKerInitArgs->channelConfig == NULL) {
         status = AUDIOLIB_ERR_NULL_POINTER;
      }
      else {
         /* Every entry indexes gChannelPositionMap directly, so an out-of-range
          * channel type would read past the end of that table. */
         const uint32_t *channelType = (const uint32_t *) pKerInitArgs->channelConfig;
         const uint32_t  numTypes    = (uint32_t) (sizeof(gChannelPositionMap) / sizeof(gChannelPositionMap[0]));

         for (uint32_t i = 0U; i < numChannels; i++) {
            if (channelType[i] >= numTypes) {
               status = AUDIOLIB_ERR_INVALID_VALUE;
               break;
            }
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_panning_exec_checkParams(AUDIOLIB_kernelHandle handle,
                                                  const void *restrict pIn,
                                                  const void *restrict pGain,
                                                  const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_exec_checkParams \n");

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL) || (pGain == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_panning_init(AUDIOLIB_kernelHandle            handle,
                      AUDIOLIB_bufParams2D_t          *bufParamsIn,
                      AUDIOLIB_bufParams1D_t          *bufParamsGain,
                      AUDIOLIB_bufParams2D_t          *bufParamsOut,
                      const AUDIOLIB_panning_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS            status        = AUDIOLIB_SUCCESS;
   AUDIOLIB_panning_PrivArgs *pKerPrivArgs  = (AUDIOLIB_panning_PrivArgs *) handle;
   const uint32_t            *channelConfig = (const uint32_t *) pKerInitArgs->channelConfig;
   /* init_checkParams only accepts AUDIOLIB_FLOAT32 buffers */
   const int32_t              elemSize      = (int32_t) sizeof(float);

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_init\n");

   /* Forget any execution variant chosen by an earlier set(): it was chosen for
    * the previous geometry, so exec() must fail until set() is called again. */
   pKerPrivArgs->execute = NULL;

   /* Buffer geometry */
   pKerPrivArgs->funcStyle    = pKerInitArgs->funcStyle;
   pKerPrivArgs->isInterleave = (pKerInitArgs->isInterleave == 1) ? 1U : 0U;
   if (pKerPrivArgs->isInterleave == 1U) {
      pKerPrivArgs->samples  = (int32_t) bufParamsOut->dim_y;
      pKerPrivArgs->channels = bufParamsOut->dim_x;
   }
   else {
      pKerPrivArgs->samples  = (int32_t) bufParamsOut->dim_x;
      pKerPrivArgs->channels = bufParamsOut->dim_y;
   }
   pKerPrivArgs->strideInElements  = (int32_t) bufParamsIn->stride_y / elemSize;
   pKerPrivArgs->strideOutElements = (int32_t) bufParamsOut->stride_y / elemSize;

   /* Carve the handle into [PrivArgs | channelMap | nodes | currentGain], each
    * region 64-byte aligned, matching AUDIOLIB_panning_getHandleSize. */
   const size_t channels    = (size_t) pKerPrivArgs->channels;
   const size_t gainBytes   = ((sizeof(float) * channels) + 63U) & ~((size_t) 63U);
   size_t       offset      = (sizeof(AUDIOLIB_panning_PrivArgs) + 63U) & ~((size_t) 63U);
   AUDIOLIB_channelConfig_t *channelMap = (AUDIOLIB_channelConfig_t *) ((uint8_t *) handle + offset);
   offset += ((sizeof(AUDIOLIB_channelConfig_t) * channels) + 63U) & ~((size_t) 63U);
   AUDIOLIB_vbapNode_t *nodes = (AUDIOLIB_vbapNode_t *) ((uint8_t *) handle + offset);
   offset += ((sizeof(AUDIOLIB_vbapNode_t) * channels) + 63U) & ~((size_t) 63U);
   pKerPrivArgs->currentGain = (float *) ((uint8_t *) handle + offset);

   /* The running gains start at zero. The padding lanes are cleared as well,
    * because the interleaved variant loads whole vectors of them. */
   (void) memset(pKerPrivArgs->currentGain, 0, gainBytes);

   /* Directional speaker list. The LFE is non-directional and takes no part in
    * panning, so it is skipped: channelMap entries [0, validChannels) are the
    * directional speakers and nothing else. */
   uint32_t validChannels = 0U;
   pKerPrivArgs->isLFE    = 0U;
   for (uint32_t i = 0U; i < pKerPrivArgs->channels; i++) {
      if (channelConfig[i] == (uint32_t) AUDIOLIB_CHANNEL_LOW_FREQUENCY_EFFECTS) {
         pKerPrivArgs->isLFE = 1U;
      }
      else {
         channelMap[validChannels].azimuth     = gChannelPositionMap[channelConfig[i]].azimuth;
         channelMap[validChannels].configIndex = i;
         validChannels++;
      }
   }
   pKerPrivArgs->validChannels = validChannels;

   /* Insertion sort by ascending azimuth; stable, so co-located speakers keep
    * the caller's order. */
   for (uint32_t i = 1U; i < validChannels; i++) {
      const AUDIOLIB_channelConfig_t key = channelMap[i];
      uint32_t                       j   = i;
      while ((j > 0U) && (channelMap[j - 1U].azimuth > key.azimuth)) {
         channelMap[j] = channelMap[j - 1U];
         j--;
      }
      channelMap[j] = key;
   }

   uint32_t nodeCount = 0U;
   for (uint32_t i = 0U; i < validChannels; i++) {
      if ((nodeCount == 0U) || (channelMap[i].azimuth != nodes[nodeCount - 1U].azimuth)) {
         nodes[nodeCount].azimuth      = channelMap[i].azimuth;
         nodes[nodeCount].firstSpeaker = i;
         nodes[nodeCount].speakerCount = 1U;
         nodeCount++;
      }
      else {
         nodes[nodeCount - 1U].speakerCount++;
      }
   }
   pKerPrivArgs->nodeCount = nodeCount;

   if (nodeCount < 2U) {
      status = AUDIOLIB_ERR_INVALID_DIMENSION;
   }
   else {
      /* Inverse of the 2x2 unit-vector matrix of each adjacent node pair, the last
       * pair wrapping round to node 0. */
      for (uint32_t i = 0U; i < nodeCount; i++) {
         const uint32_t next = (i + 1U) % nodeCount;
         const float    rad1 = nodes[i].azimuth * (AUDIOLIB_PANNING_PI / 180.0f);
         const float    rad2 = nodes[next].azimuth * (AUDIOLIB_PANNING_PI / 180.0f);
         const float    l1X  = cosf(rad1);
         const float    l1Y  = sinf(rad1);
         const float    l2X  = cosf(rad2);
         const float    l2Y  = sinf(rad2);
         const float    det  = (l1X * l2Y) - (l1Y * l2X);

         if (fabsf(det) < AUDIOLIB_PANNING_DET_EPSILON) {
            /* Antipodal directions: the pair spans a straight line through the
             * listener and has no unique solution. Zeroing the inverse makes both
             * pair gains 0, which sends set() to its nearest-node fallback. */
            (void) memset(&nodes[i].invL[0][0], 0, sizeof(nodes[i].invL));
         }
         else {
            const float invDet  = 1.0f / det;
            nodes[i].invL[0][0] = l2Y * invDet;
            nodes[i].invL[0][1] = -l2X * invDet;
            nodes[i].invL[1][0] = -l1Y * invDet;
            nodes[i].invL[1][1] = l1X * invDet;
         }
      }

      pKerPrivArgs->channelMap = channelMap;
      pKerPrivArgs->nodes      = nodes;

      if (pKerInitArgs->funcStyle != AUDIOLIB_FUNCTION_NATC) {
         status = AUDIOLIB_panning_init_ci<float>(handle, bufParamsIn, bufParamsGain, bufParamsOut, pKerInitArgs);
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_panning_set(AUDIOLIB_kernelHandle handle, void *restrict pGain, AUDIOLIB_panning_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_set\n");

   if ((handle == NULL) || (pGain == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if (((AUDIOLIB_panning_PrivArgs *) handle)->nodeCount < 2U) {
      /* init() failed, so there is no speaker geometry to pan across. */
      status = AUDIOLIB_ERR_INVALID_DIMENSION;
   }
   else if ((pKerSetArgs->samplingRate <= 0) || (pKerSetArgs->smoothingTime < 0.0f) ||
            !(fabsf(pKerSetArgs->smoothingTime) <= FLT_MAX) || !(fabsf(pKerSetArgs->sourceAzimuth) <= FLT_MAX)) {
      /* fabsf(x) <= FLT_MAX is false for NaN and +/-inf, so both are rejected. */
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      AUDIOLIB_panning_PrivArgs *pKerPrivArgs = (AUDIOLIB_panning_PrivArgs *) handle;
      float *restrict            pGainLocal   = (float *) pGain;

      AUDIOLIB_channelConfig_t const *const channelMap = pKerPrivArgs->channelMap;
      AUDIOLIB_vbapNode_t const *const      nodes      = pKerPrivArgs->nodes;
      const uint32_t                        lastNode   = pKerPrivArgs->nodeCount - 1U;

      /******* Target gains (VBAP) ********/
      (void) memset(pGainLocal, 0, sizeof(float) * (size_t) pKerPrivArgs->channels);

      /* Wrap the source direction into [-180, 180] degrees */
      float azimuth = fmodf(pKerSetArgs->sourceAzimuth + 180.0f, 360.0f);
      if (azimuth < 0.0f) {
         azimuth += 360.0f;
      }
      azimuth -= 180.0f;

      const float azimuthRad = azimuth * (AUDIOLIB_PANNING_PI / 180.0f);
      const float sourceX    = cosf(azimuthRad);
      const float sourceY    = sinf(azimuthRad);

      /* Pair i spans nodes[i] and nodes[i + 1]; the last pair wraps round to node 0. */
      uint32_t leftNode = lastNode;
      for (uint32_t i = 0U; i < lastNode; i++) {
         if ((nodes[i].azimuth <= azimuth) && (azimuth < nodes[i + 1U].azimuth)) {
            leftNode = i;
            break;
         }
      }
      const uint32_t rightNode = (leftNode == lastNode) ? 0U : (leftNode + 1U);

      /* g = L^-1 * p with the pair's precomputed inverse */
      const AUDIOLIB_vbapNode_t *const pair = &nodes[leftNode];
      const float g1 = (pair->invL[0][0] * sourceX) + (pair->invL[0][1] * sourceY);
      const float g2 = (pair->invL[1][0] * sourceX) + (pair->invL[1][1] * sourceY);

      if ((g1 > 0.0f) && (g2 > 0.0f)) {
         /* Power normalisation. p = g1*l1 + g2*l2 with unit l1, l2 and |p| = 1, so
          * two positive gains give g1^2 + g2^2 >= 1/4: no division by zero. */
         const float invNorm   = 1.0f / sqrtf((g1 * g1) + (g2 * g2));
         const float leftGain  = g1 * invNorm;
         const float rightGain = g2 * invNorm;

         const AUDIOLIB_vbapNode_t *const left    = &nodes[leftNode];
         const uint32_t                   leftEnd = left->firstSpeaker + left->speakerCount;
         for (uint32_t k = left->firstSpeaker; k < leftEnd; k++) {
            pGainLocal[channelMap[k].configIndex] = leftGain;
         }

         const AUDIOLIB_vbapNode_t *const right    = &nodes[rightNode];
         const uint32_t                   rightEnd = right->firstSpeaker + right->speakerCount;
         for (uint32_t k = right->firstSpeaker; k < rightEnd; k++) {
            pGainLocal[channelMap[k].configIndex] = rightGain;
         }
      }
      else {
         /* The source lies outside the pair's arc, or the pair is degenerate: unity
          * gain goes to every speaker of the single nearest direction instead. */
         float    minDistance = FLT_MAX;
         uint32_t nearestNode = 0U;

         for (uint32_t i = 0U; i <= lastNode; i++) {
            float distance = fabsf(azimuth - nodes[i].azimuth);
            distance       = (distance > 180.0f) ? (360.0f - distance) : distance;
            if (distance < minDistance) {
               minDistance = distance;
               nearestNode = i;
            }
         }

         const float                      nearestGain = 1.0f;
         const AUDIOLIB_vbapNode_t *const nearest     = &nodes[nearestNode];
         const uint32_t                   nearestEnd  = nearest->firstSpeaker + nearest->speakerCount;
         for (uint32_t k = nearest->firstSpeaker; k < nearestEnd; k++) {
            pGainLocal[channelMap[k].configIndex] = nearestGain;
         }
      }

      /******* Smoothing ********/
      const float samplesPerTimeConstant =
          (float) pKerSetArgs->samplingRate * (pKerSetArgs->smoothingTime / 1000.0f);

      (void) memset(&pKerPrivArgs->alphaCoeff[0], 0, sizeof(pKerPrivArgs->alphaCoeff));

      if (samplesPerTimeConstant <= 0.0f) {
         pKerPrivArgs->bypassSmoothing      = 1U;
         pKerPrivArgs->smoothingCoefficient = 0.0f;
         pKerPrivArgs->alphaMultiplier      = 0.0f;
         pKerPrivArgs->alphaMultiplierQuad  = 0.0f;
         pKerPrivArgs->alphaMultiplierPair  = 0.0f;
         pKerPrivArgs->alphaN               = 0.0f;

         (void) memcpy(pKerPrivArgs->currentGain, pGainLocal, sizeof(float) * (size_t) pKerPrivArgs->channels);
      }
      else {
         const float alpha = expf(-1.0f / samplesPerTimeConstant);

         pKerPrivArgs->bypassSmoothing      = 0U;
         pKerPrivArgs->smoothingCoefficient = alpha;

         const bool lanesAreSamples = (pKerPrivArgs->isInterleave == 0U);

         pKerPrivArgs->alphaCoeff[0] = alpha;
         for (uint32_t i = 1U; i < (uint32_t) AUDIOLIB_PANNING_LANES_F32; i++) {
            pKerPrivArgs->alphaCoeff[i] = lanesAreSamples ? (pKerPrivArgs->alphaCoeff[i - 1U] * alpha) : alpha;
         }
      
         const float samplesPerVec = lanesAreSamples ? (float) AUDIOLIB_PANNING_LANES_F32 : 1.0f;

         pKerPrivArgs->alphaMultiplier     = powf(alpha, samplesPerVec);
         pKerPrivArgs->alphaMultiplierQuad = powf(alpha, samplesPerVec * 4.0f);
         pKerPrivArgs->alphaMultiplierPair = powf(alpha, samplesPerVec * 2.0f);
         pKerPrivArgs->alphaN              = powf(alpha, (float) pKerPrivArgs->samples);
      }

      /******* Execution variant ********/
      const bool isInterleave = (pKerPrivArgs->isInterleave == 1U);

      if (pKerPrivArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
         pKerPrivArgs->execute = AUDIOLIB_panning_exec_cn<float>;
      }
      else if (pKerPrivArgs->bypassSmoothing == 1U) {
         pKerPrivArgs->execute = isInterleave ? AUDIOLIB_panning_byPassSmoothing_exec_ci<float, true>
                                              : AUDIOLIB_panning_byPassSmoothing_exec_ci<float, false>;
      }
      else {
         pKerPrivArgs->execute = isInterleave ? AUDIOLIB_panning_smooth_interleave_exec_ci<float>
                                              : AUDIOLIB_panning_smooth_nonInterleave_exec_ci<float>;
      }

      pKerPrivArgs->setArgs = *pKerSetArgs;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_panning_get(AUDIOLIB_kernelHandle handle, AUDIOLIB_panning_SetArgs *pKerSetArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_panning_get");

   if ((handle == NULL) || (pKerSetArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else {
      *pKerSetArgs = ((const AUDIOLIB_panning_PrivArgs *) handle)->setArgs;
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_panning_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pGain, void *restrict pOut)
{
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_panning_exec \n");

   const AUDIOLIB_panning_PrivArgs *pKerPrivArgs = (const AUDIOLIB_panning_PrivArgs *) handle;
   AUDIOLIB_STATUS                  status       = AUDIOLIB_ERR_FAILURE;

   /* No execution variant until set() has run after the latest init() */
   if (pKerPrivArgs->execute != NULL) {
      status = pKerPrivArgs->execute(handle, pIn, pGain, pOut);
   }

   return status;
}
