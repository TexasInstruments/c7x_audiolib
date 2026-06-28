// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_PRIV_H_
#define AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_PRIV_H_

#include "../AUDIOLIB_gainNCh/AUDIOLIB_gainNCh_priv.h"
#include "../common/AUDIOLIB_utility.h"
#include "AUDIOLIB_balanceFader.h"

/*!
 * @brief Macro to define the size of bufPblock array of
 *        @ref AUDIOLIB_balanceFader_PrivArgs structure.
 *
 */

/*!
 * @brief Structure that is reserved for internal use by the kernel
 */
typedef struct {
   int32_t                   samples;           // Number of samples per channel
   int32_t                   numChannels;       // Total number of channels
   uint8_t                   isInterleave;      // Interleave flag
   int32_t                   strideInElements;  // Stride Y of input buffer (elements)
   int32_t                   strideOutElements; // Stride Y of output buffer (elements)
   AUDIOLIB_gainNCh_PrivArgs gainNChArgs;       /**
                                                 * @brief Handle for the underlying N-Channel Gain kernel.
                                                 * @details The balanceFader kernel delegates the actual audio processing
                                                 * (multiplying audio by gains) to this kernel.
                                                 */
   AUDIOLIB_balanceFader_SetArgs setArgs;       /**< A copy of the last-set runtime parameters. */
} AUDIOLIB_balanceFader_PrivArgs;
#endif /* AUDIOLIB_BALANCEFADER_IXX_IXX_OXX_PRIV_H_ */
