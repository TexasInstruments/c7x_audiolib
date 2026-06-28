// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef _AUDIOLIB_H_
#define _AUDIOLIB_H_

#include "AUDIOLIB_asrc/AUDIOLIB_asrc.h"
#include "AUDIOLIB_balance/AUDIOLIB_balance.h"
#include "AUDIOLIB_balanceFader/AUDIOLIB_balanceFader.h"
#include "AUDIOLIB_blockStatistics/AUDIOLIB_blockStatistics.h"
#include "AUDIOLIB_concat/AUDIOLIB_concat.h"
#include "AUDIOLIB_crossfade/AUDIOLIB_crossfade.h"
#include "AUDIOLIB_dB10/AUDIOLIB_dB10.h"
#include "AUDIOLIB_dB20/AUDIOLIB_dB20.h"
#include "AUDIOLIB_delay/AUDIOLIB_delay.h"
#include "AUDIOLIB_delayNChannel/AUDIOLIB_delayNChannel.h"
#include "AUDIOLIB_gain/AUDIOLIB_gain.h"
#include "AUDIOLIB_gainNCh/AUDIOLIB_gainNCh.h"
#include "AUDIOLIB_gainNChTrim/AUDIOLIB_gainNChTrim.h"
#include "AUDIOLIB_inputAggregator/AUDIOLIB_inputAggregator.h"
#include "AUDIOLIB_mute/AUDIOLIB_mute.h"
#include "AUDIOLIB_muteNCh/AUDIOLIB_muteNCh.h"
#include "AUDIOLIB_nlms/AUDIOLIB_nlms.h"
#include "AUDIOLIB_router/AUDIOLIB_router.h"
#include "AUDIOLIB_sinusoidGenerator/AUDIOLIB_sinusoidGenerator.h"
#include "AUDIOLIB_softClip/AUDIOLIB_softClip.h"
#include "AUDIOLIB_split/AUDIOLIB_split.h"
#include "AUDIOLIB_ssrc/AUDIOLIB_ssrc.h"
#include "AUDIOLIB_subBlockStatistics/AUDIOLIB_subBlockStatistics.h"
#include "AUDIOLIB_tableInterpolate/AUDIOLIB_tableInterpolate.h"
#include "AUDIOLIB_tableLookup/AUDIOLIB_tableLookup.h"
#include "AUDIOLIB_typeConversion/AUDIOLIB_typeConversion.h"
#include "AUDIOLIB_undB10/AUDIOLIB_undB10.h"
#include "AUDIOLIB_undB20/AUDIOLIB_undB20.h"
#include "AUDIOLIB_whiteNoiseGenerator/AUDIOLIB_whiteNoiseGenerator.h"
#include "common/AUDIOLIB_bufParams.h"
#include "common/AUDIOLIB_types.h"

/**
 * @brief Defines standard channel locations in a 3D audio space.
 *
 * Angles are provided relative to the listener:
 * - Azimuth: Horizontal angle. 0° is front-center, negative angles are to the left, positive to the right.
 * - Elevation: Vertical angle. 0° is ear-level, positive angles are above.
 */
typedef enum {
   AUDIOLIB_CHANNEL_FRONT_LEFT,            /**< Azimuth: -30°, Elevation: 0° */
   AUDIOLIB_CHANNEL_FRONT_LEFT_TREBBLE,    /**< Azimuth: -30°, Elevation: 0° */
   AUDIOLIB_CHANNEL_FRONT_LEFT_MID,        /**< Azimuth: -30°, Elevation: 0° */
   AUDIOLIB_CHANNEL_FRONT_LEFT_WOOFER,     /**< Azimuth: -30°, Elevation: 0° */
   AUDIOLIB_CHANNEL_FRONT_RIGHT,           /**< Azimuth: +30°, Elevation: 0° */
   AUDIOLIB_CHANNEL_FRONT_RIGHT_TREBBLE,   /**< Azimuth: +30°, Elevation: 0° */
   AUDIOLIB_CHANNEL_FRONT_RIGHT_MID,       /**< Azimuth: +30°, Elevation: 0° */
   AUDIOLIB_CHANNEL_FRONT_RIGHT_WOOFER,    /**< Azimuth: +30°, Elevation: 0° */
   AUDIOLIB_CHANNEL_FRONT_CENTER,          /**< Azimuth: 0°,   Elevation: 0° */
   AUDIOLIB_CHANNEL_LOW_FREQUENCY_EFFECTS, /**< Non-directional subwoofer channel */
   AUDIOLIB_CHANNEL_SIDE_LEFT,             /**< Azimuth: -90°, Elevation: 0° */
   AUDIOLIB_CHANNEL_SIDE_RIGHT,            /**< Azimuth: +90°, Elevation: 0° */
   AUDIOLIB_CHANNEL_REAR_LEFT,             /**< Azimuth: -135°,Elevation: 0° */
   AUDIOLIB_CHANNEL_REAR_RIGHT,            /**< Azimuth: +135°,Elevation: 0° */
   AUDIOLIB_CHANNEL_TOP_FRONT_LEFT,        /**< Azimuth: -45°, Elevation: +45° */
   AUDIOLIB_CHANNEL_TOP_FRONT_RIGHT,       /**< Azimuth: +45°, Elevation: +45° */
   AUDIOLIB_CHANNEL_TOP_MIDDLE_LEFT,       /**< Azimuth: -90°, Elevation: +90° */
   AUDIOLIB_CHANNEL_TOP_MIDDLE_RIGHT,      /**< Azimuth: +90°, Elevation: +90° */
   AUDIOLIB_CHANNEL_TOP_REAR_LEFT,         /**< Azimuth: -135°,Elevation: +45° */
   AUDIOLIB_CHANNEL_TOP_REAR_RIGHT,        /**< Azimuth: +135°,Elevation: +45° */
   AUDIOLIB_CHANNEL_OVERHEAD               /**< Azimuth: N/A,  Elevation: +90° (Voice of God) */
} AUDIOLIB_CHANNEL_TYPE;

#endif /*_AUDIOLIB_H_*/
