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

/********************************************************************************
 *  @file     AUDIOLIB_upSample_cn.cpp
 *  @brief    Natural-C reference implementation of AUDIOLIB_upSample.
 *  @version  0.1 - 2025 : Initial Version
 ********************************************************************************/

#include "AUDIOLIB_upSample_priv.h"
#include <cstdint>

/*
 * Natural-C reference upSampler (zero insertion).  Mirrors the Python
 * reference test/AUDIOLIB_idat_gen/AUDIOLIB_upSample/upSample.py exactly:
 * zero the output, then copy each input sample to output sample index
 * phase + k*upFactor, for every channel.
 */
template <typename dataType>
AUDIOLIB_STATUS AUDIOLIB_upSample_exec_cn(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut )
{
   AUDIOLIB_STATUS             status       = AUDIOLIB_SUCCESS;
   AUDIOLIB_upSample_PrivArgs *pKerPrivArgs = (AUDIOLIB_upSample_PrivArgs *) handle;

   AUDIOLIB_DEBUGPRINTFN(0, "%s\n", "Enter AUDIOLIB_upSample_exec_cn");

   dataType *restrict pInLocal  = (dataType *) pIn;
   dataType *restrict pOutLocal = (dataType *) pOut;

   uint32_t upFactor     = pKerPrivArgs->upFactor;
   uint32_t phase        = pKerPrivArgs->phase;
   uint32_t isInterleave = pKerPrivArgs->isInterleave;
   int32_t  strideIn     = pKerPrivArgs->strideIn;  // elements
   int32_t  strideOut    = pKerPrivArgs->strideOut; // elements

   if (!isInterleave) {
      /* planar: rows = channels, columns = samples; insert along columns */
      uint32_t numChannels = pKerPrivArgs->dimYIn;
      uint32_t numSamples  = pKerPrivArgs->dimXIn;
      uint32_t outSamples  = upFactor * numSamples;

      for (uint32_t m = 0; m < numChannels; m++) {
         for (uint32_t n = 0; n < outSamples; n++) {
            pOutLocal[m * strideOut + n] = (dataType) 0;
         }
         for (uint32_t k = 0; k < numSamples; k++) {
            pOutLocal[m * strideOut + phase + k * upFactor] = pInLocal[m * strideIn + k];
         }
      }
   }
   else {
      /* interleaved: rows = samples, columns = channels; insert along rows */
      uint32_t numSamples  = pKerPrivArgs->dimYIn;
      uint32_t numChannels = pKerPrivArgs->dimXIn;
      uint32_t outSamples  = upFactor * numSamples;

      for (uint32_t r = 0; r < outSamples; r++) {
         for (uint32_t c = 0; c < numChannels; c++) {
            pOutLocal[r * strideOut + c] = (dataType) 0;
         }
      }
      for (uint32_t k = 0; k < numSamples; k++) {
         uint32_t outRow = phase + k * upFactor;
         for (uint32_t c = 0; c < numChannels; c++) {
            pOutLocal[outRow * strideOut + c] = pInLocal[k * strideIn + c];
         }
      }
   }

   return status;
}

/* explicit instantiation for the supported data types */
template AUDIOLIB_STATUS
AUDIOLIB_upSample_exec_cn<float>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut );

template AUDIOLIB_STATUS
AUDIOLIB_upSample_exec_cn<double>(AUDIOLIB_kernelHandle handle, void *restrict pIn, void *restrict pOut);

/* ======================================================================== */
/*  End of file:  AUDIOLIB_upSample_cn.cpp                                   */
/* ======================================================================== */
