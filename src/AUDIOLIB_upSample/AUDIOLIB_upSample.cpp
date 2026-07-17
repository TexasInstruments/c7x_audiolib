/*******************************************************************************
**+--------------------------------------------------------------------------+**
**|                            ****                                          |**
**|                            ****                                          |**
**|                            ******o***                                    |**
**|                      ********_///_****                                   |**
**|                      ***** /_//_/ ****                                   |**
**|                       ** ** (__/ ****                                    |**
**|                           *********                                      |**
**|                            ****                                          |**
**|                            ***                                           |**
**|                                                                          |**
**|        Copyright (c) 2025 Texas Instruments Incorporated                 |**
**|                                                                          |**
**|              All rights reserved not granted herein.                     |**
**|                                                                          |**
**|                         Limited License.                                 |**
**|                                                                          |**
**|  Texas Instruments Incorporated grants a world-wide, royalty-free,       |**
**|  non-exclusive license under copyrights and patents it now or            |**
**|  hereafter owns or controls to make, have made, use, import, offer to    |**
**|  sell and sell ("Utilize") this software subject to the terms herein.    |**
**|  With respect to the foregoing patent license, such license is granted   |**
**|  solely to the extent that any such patent is necessary to Utilize the   |**
**|  software alone.  The patent license shall not apply to any              |**
**|  combinations which include this software, other than combinations       |**
**|  with devices manufactured by or for TI ("TI Devices").  No hardware     |**
**|  patent is licensed hereunder.                                           |**
**|                                                                          |**
**|  Redistributions must preserve existing copyright notices and            |**
**|  reproduce this license (including the above copyright notice and the    |**
**|  disclaimer and (if applicable) source code license limitations below)   |**
**|  in the documentation and/or other materials provided with the           |**
**|  distribution                                                            |**
**|                                                                          |**
**|  Redistribution and use in binary form, without modification, are        |**
**|  permitted provided that the following conditions are met:               |**
**|                                                                          |**
**|    *  No reverse engineering, decompilation, or disassembly of this      |**
**|  software is permitted with respect to any software provided in binary   |**
**|  form.                                                                   |**
**|                                                                          |**
**|    *  any redistribution and use are licensed by TI for use only with    |**
**|  TI Devices.                                                             |**
**|                                                                          |**
**|    *  Nothing shall obligate TI to provide you with source code for      |**
**|  the software licensed and provided to you in object code.               |**
**|                                                                          |**
**|  If software source code is provided to you, modification and            |**
**|  redistribution of the source code are permitted provided that the       |**
**|  following conditions are met:                                           |**
**|                                                                          |**
**|    *  any redistribution and use of the source code, including any       |**
**|  resulting derivative works, are licensed by TI for use only with TI     |**
**|  Devices.                                                                |**
**|                                                                          |**
**|    *  any redistribution and use of any object code compiled from the    |**
**|  source code and any resulting derivative works, are licensed by TI      |**
**|  for use only with TI Devices.                                           |**
**|                                                                          |**
**|  Neither the name of Texas Instruments Incorporated nor the names of     |**
**|  its suppliers may be used to endorse or promote products derived from   |**
**|  this software without specific prior written permission.                |**
**|                                                                          |**
**|  DISCLAIMER.                                                             |**
**|                                                                          |**
**|  THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY      |**
**|  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE       |**
**|  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR      |**
**|  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TI AND TI'S LICENSORS BE      |**
**|  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR     |**
**|  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF    |**
**|  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR         |**
**|  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,   |**
**|  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE    |**
**|  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,       |**
**|  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                      |**
**+--------------------------------------------------------------------------+**
*******************************************************************************/

/**
 ******************************************************************************
 *  @file     AUDIOLIB_upSample.cpp
 *  @brief    Dispatcher / lifecycle for the AUDIOLIB_upSample kernel.
 *  @version  0.1 - 2025 : Initial Version
 ******************************************************************************
 */

#include "AUDIOLIB_bufParams.h"
#include "AUDIOLIB_upSample_priv.h"

int32_t AUDIOLIB_upSample_getHandleSize(AUDIOLIB_upSample_InitArgs *pKerInitArgs)
{
   (void) pKerInitArgs;
   return (int32_t) sizeof(AUDIOLIB_upSample_PrivArgs);
}

AUDIOLIB_STATUS AUDIOLIB_upSample_init_checkParams(AUDIOLIB_kernelHandle             handle,
                                                   const AUDIOLIB_bufParams2D_t     *bufParamsIn,
                                                   const AUDIOLIB_bufParams2D_t     *bufParamsOut,
                                                   const AUDIOLIB_upSample_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_init_checkParams\n");

   if ((handle == NULL) || (bufParamsIn == NULL) || (bufParamsOut == NULL) || (pKerInitArgs == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }
   else if ((bufParamsIn->data_type != AUDIOLIB_FLOAT32) && (bufParamsIn->data_type != AUDIOLIB_FLOAT64)) {
      status = AUDIOLIB_ERR_INVALID_TYPE;
   }
   else if (bufParamsIn->data_type != bufParamsOut->data_type) {
      status = AUDIOLIB_ERR_INVALID_TYPE;
   }
   else if (pKerInitArgs->upFactor < 1u) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else if (pKerInitArgs->phase >= pKerInitArgs->upFactor) {
      status = AUDIOLIB_ERR_INVALID_VALUE;
   }
   else {
      /* Output must be upFactor times longer along the sample axis. */
      if (pKerInitArgs->isInterleave) {
         /* interleaved: dim_x = channels, dim_y = samples */
         if ((bufParamsOut->dim_x != bufParamsIn->dim_x) ||
             (bufParamsOut->dim_y != pKerInitArgs->upFactor * bufParamsIn->dim_y)) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
      else {
         /* planar: dim_x = samples, dim_y = channels */
         if ((bufParamsOut->dim_y != bufParamsIn->dim_y) ||
             (bufParamsOut->dim_x != pKerInitArgs->upFactor * bufParamsIn->dim_x)) {
            status = AUDIOLIB_ERR_INVALID_DIMENSION;
         }
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_upSample_exec_checkParams(AUDIOLIB_kernelHandle handle, const void *restrict pIn, const void *restrict pOut)
{
   AUDIOLIB_STATUS status = AUDIOLIB_SUCCESS;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_exec_checkParams\n");

   /* All three pointers are required; scratch is carried inside the handle
    * (bufPblock), so there is no separate scratch argument to validate. */

   if ((handle == NULL) || (pIn == NULL) || (pOut == NULL)) {
      status = AUDIOLIB_ERR_NULL_POINTER;
   }

   return status;
}

AUDIOLIB_STATUS AUDIOLIB_upSample_init(AUDIOLIB_kernelHandle             handle,
                                       AUDIOLIB_bufParams2D_t           *bufParamsIn,
                                       AUDIOLIB_bufParams2D_t           *bufParamsOut,
                                       const AUDIOLIB_upSample_InitArgs *pKerInitArgs)
{
   AUDIOLIB_STATUS             status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_init\n");

   int32_t elemSize = AUDIOLIB_sizeof(bufParamsIn->data_type);

   pKerPrivArgs->dimXIn       = bufParamsIn->dim_x;
   pKerPrivArgs->dimYIn       = bufParamsIn->dim_y;
   pKerPrivArgs->upFactor     = pKerInitArgs->upFactor;
   pKerPrivArgs->phase        = pKerInitArgs->phase;
   pKerPrivArgs->isInterleave = pKerInitArgs->isInterleave;
   pKerPrivArgs->strideIn     = bufParamsIn->stride_y / elemSize;
   pKerPrivArgs->strideOut    = bufParamsOut->stride_y / elemSize;
   pKerPrivArgs->dataType     = bufParamsIn->data_type;

   if (pKerInitArgs->funcStyle == AUDIOLIB_FUNCTION_NATC) {
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {
         pKerPrivArgs->execute = AUDIOLIB_upSample_exec_cn<float>;
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         pKerPrivArgs->execute = AUDIOLIB_upSample_exec_cn<double>;
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }
   else {

      /* Single-channel, unit-stride interleaved float32 can be treated as planar. */
      const bool singleChannelPlanar =
          (pKerPrivArgs->isInterleave == 1u) && (pKerPrivArgs->dimXIn == 1u) && (pKerPrivArgs->strideIn == 1) &&
          (bufParamsIn->data_type == AUDIOLIB_FLOAT32);
      pKerPrivArgs->useNonInterleave = singleChannelPlanar ? 1u : 0u;
      if (bufParamsIn->data_type == AUDIOLIB_FLOAT32) {

         if (pKerPrivArgs->isInterleave == 1 && pKerPrivArgs->useNonInterleave == 0) {
            if (pKerPrivArgs->phase > 0) {
               pKerPrivArgs->execute = AUDIOLIB_upSample_execInterleave_ci<true, float>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_upSample_execInterleave_ci<false, float>;
            }
         }
         else {
            if (pKerPrivArgs->phase > 0) {
               switch (pKerPrivArgs->upFactor) {
               case 2:
               case 4:
                  pKerPrivArgs->execute = AUDIOLIB_upSample2_exec_ci<true, float>;
                  break;
               case 3:
               case 6:
                  pKerPrivArgs->execute = AUDIOLIB_upSample3_exec_ci<true, float>;
                  break;
               case 5:
               case 10:
                  pKerPrivArgs->execute = AUDIOLIB_upSample5_exec_ci<true, float>;
                  break;
               case 7:
               case 14:
                  pKerPrivArgs->execute = AUDIOLIB_upSample7_exec_ci<true, float>;
                  break;
               case 8:
               case 16:
               case 24:
                  pKerPrivArgs->execute = AUDIOLIB_upSample8X_exec_ci<true, float>;
                  break;
               case 12:
                  pKerPrivArgs->execute = AUDIOLIB_upSample6x2_exec_ci<true, float>;
                  break;
               default:
                  pKerPrivArgs->execute = AUDIOLIB_upSampleNonInterleave_exec_ci<true, float>;
                  break;
               }
            }
            else {
               switch (pKerPrivArgs->upFactor) {
               case 2:
               case 4:
                  pKerPrivArgs->execute = AUDIOLIB_upSample2_exec_ci<false, float>;
                  break;
               case 3:
               case 6:
                  pKerPrivArgs->execute = AUDIOLIB_upSample3_exec_ci<false, float>;
                  break;
               case 5:
               case 10:
                  pKerPrivArgs->execute = AUDIOLIB_upSample5_exec_ci<false, float>;
                  break;
               case 7:
               case 14:
                  pKerPrivArgs->execute = AUDIOLIB_upSample7_exec_ci<false, float>;
                  break;
               case 8:
               case 16:
               case 24:
                  pKerPrivArgs->execute = AUDIOLIB_upSample8X_exec_ci<false, float>;
                  break;
               case 12:
                  pKerPrivArgs->execute = AUDIOLIB_upSample6x2_exec_ci<false, float>;
                  break;
               default:
                  pKerPrivArgs->execute = AUDIOLIB_upSampleNonInterleave_exec_ci<false, float>;
                  break;
               }
            }
         }
         status = AUDIOLIB_upSample_init_ci<float>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else if (bufParamsIn->data_type == AUDIOLIB_FLOAT64) {
         if (pKerPrivArgs->isInterleave && pKerPrivArgs->useNonInterleave == 0) {
            if (pKerPrivArgs->phase > 0) {
               pKerPrivArgs->execute = AUDIOLIB_upSample_execInterleave_ci<true, double>;
            }
            else {
               pKerPrivArgs->execute = AUDIOLIB_upSample_execInterleave_ci<false, double>;
            }
         }
         else {
            if (pKerPrivArgs->phase>0 ) {
               switch (pKerPrivArgs->upFactor) {
               case 2:
                  pKerPrivArgs->execute = AUDIOLIB_upSample2_exec_ci<true, double>;
                  break;
               case 3:
                  pKerPrivArgs->execute = AUDIOLIB_upSample3_exec_ci<true, double>;
                  break;
               case 4:
                  pKerPrivArgs->execute = AUDIOLIB_upSample4_exec_ci<true, double>;
                  break;
               case 5:
                  pKerPrivArgs->execute = AUDIOLIB_upSample5_exec_ci<true, double>;
                  break;
               case 6:
                  pKerPrivArgs->execute = AUDIOLIB_upSample6_exec_ci<true, double>;
                  break;
               case 7:
                  pKerPrivArgs->execute = AUDIOLIB_upSample7_exec_ci<true, double>;
                  break;
               case 8:
                  pKerPrivArgs->execute = AUDIOLIB_upSample8_exec_ci<true, double>;
                  break;
               default:
                  pKerPrivArgs->execute = AUDIOLIB_upSampleNonInterleave_exec_ci<true, double>;
                  break;
               }
            }
            else {
               switch (pKerPrivArgs->upFactor) {
               case 2:
                  pKerPrivArgs->execute = AUDIOLIB_upSample2_exec_ci<false, double>;
                  break;
               case 3:
                  pKerPrivArgs->execute = AUDIOLIB_upSample3_exec_ci<false, double>;
                  break;
               case 4:
                  pKerPrivArgs->execute = AUDIOLIB_upSample4_exec_ci<false, double>;
                  break;
               case 5:
                  pKerPrivArgs->execute = AUDIOLIB_upSample5_exec_ci<false, double>;
                  break;
               case 6:
                  pKerPrivArgs->execute = AUDIOLIB_upSample6_exec_ci<false, double>;
                  break;
               case 7:
                  pKerPrivArgs->execute = AUDIOLIB_upSample7_exec_ci<false, double>;
                  break;
               case 8:
                  pKerPrivArgs->execute = AUDIOLIB_upSample8_exec_ci<false, double>;
                  break;
               default:
                  pKerPrivArgs->execute = AUDIOLIB_upSampleNonInterleave_exec_ci<false, double>;
                  break;
               }
            }
         }
         status = AUDIOLIB_upSample_init_ci<double>(handle, bufParamsIn, bufParamsOut, pKerInitArgs);
      }
      else {
         status = AUDIOLIB_ERR_INVALID_TYPE;
      }
   }

   return status;
}

AUDIOLIB_STATUS
AUDIOLIB_upSample_exec(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut)
{
   AUDIOLIB_DEBUGPRINTFN(0, "Enter AUDIOLIB_upSample_exec\n");

   AUDIOLIB_upSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;

   return pKerPrivArgs->execute(handle, pIn, pOut);
}

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample.cpp                                      */
/* ======================================================================== */
