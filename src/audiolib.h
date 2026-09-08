// Copyright (C) 2026 Texas Instruments Incorporated
//
// SPDX-License-Identifier: Apache-2.0

#ifndef _AUDIOLIB_H_
#define _AUDIOLIB_H_

#include "AUDIOLIB_agc/AUDIOLIB_agc.h"
#include "AUDIOLIB_asrc/AUDIOLIB_asrc.h"
#include "AUDIOLIB_asymmetricLimiter/AUDIOLIB_asymmetricLimiter.h"
#include "AUDIOLIB_balance/AUDIOLIB_balance.h"
#include "AUDIOLIB_balanceFader/AUDIOLIB_balanceFader.h"
#include "AUDIOLIB_blockStatistics/AUDIOLIB_blockStatistics.h"
#include "AUDIOLIB_ceil/AUDIOLIB_ceil.h"
#include "AUDIOLIB_concat/AUDIOLIB_concat.h"
#include "AUDIOLIB_crossfade/AUDIOLIB_crossfade.h"
#include "AUDIOLIB_dB10/AUDIOLIB_dB10.h"
#include "AUDIOLIB_dB20/AUDIOLIB_dB20.h"
#include "AUDIOLIB_delay/AUDIOLIB_delay.h"
#include "AUDIOLIB_preEmphasis/AUDIOLIB_preEmphasis.h"
#include "AUDIOLIB_deEmphasis/AUDIOLIB_deEmphasis.h"
#include "AUDIOLIB_delayNChannel/AUDIOLIB_delayNChannel.h"
#include "AUDIOLIB_envelopeDetector/AUDIOLIB_envelopeDetector.h"
#include "AUDIOLIB_floor/AUDIOLIB_floor.h"
#include "AUDIOLIB_gain/AUDIOLIB_gain.h"
#include "AUDIOLIB_gainSmoothing/AUDIOLIB_gainSmoothing.h"
#include "AUDIOLIB_gainNCh/AUDIOLIB_gainNCh.h"
#include "AUDIOLIB_gainNChTrim/AUDIOLIB_gainNChTrim.h"
#include "AUDIOLIB_gainNChTrimSmoothing/AUDIOLIB_gainNChTrimSmoothing.h"
#include "AUDIOLIB_gainOffset/AUDIOLIB_gainOffset.h"
#include "AUDIOLIB_inputAggregator/AUDIOLIB_inputAggregator.h"
#include "AUDIOLIB_mute/AUDIOLIB_mute.h"
#include "AUDIOLIB_muteNCh/AUDIOLIB_muteNCh.h"
#include "AUDIOLIB_panning/AUDIOLIB_panning.h"
#include "AUDIOLIB_nlms/AUDIOLIB_nlms.h"
#include "AUDIOLIB_rmsSmoothed/AUDIOLIB_rmsSmoothed.h"
#include "AUDIOLIB_round/AUDIOLIB_round.h"
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

/**
 * @brief Position of one speaker relative to the listener, in degrees.
 */
typedef struct {
   float azimuth;   /**< Horizontal angle. 0 is front-center, negative is to the left. */
   float elevation; /**< Vertical angle. 0 is ear-level, positive is above. */
} AUDIOLIB_channelPosition_t;

/**
 * @brief Nominal position of every standard channel, indexed by @ref AUDIOLIB_CHANNEL_TYPE.
 *
 * @details Several channel types deliberately share an angle, e.g. FRONT_LEFT,
 *          FRONT_LEFT_MID and FRONT_LEFT_WOOFER are all drivers of the speaker at
 *          -30 deg. Kernels that pan across the horizontal plane must therefore
 *          cope with co-located channels. The LFE and OVERHEAD entries carry no
 *          meaningful azimuth and are expected to be excluded from panning
 *          altogether.
 */
static const AUDIOLIB_channelPosition_t gChannelPositionMap[] = {
    {-30.0f, 0.0f},   /* FRONT_LEFT            */
    {-30.0f, 0.0f},   /* FRONT_LEFT_TREBBLE    */
    {-30.0f, 0.0f},   /* FRONT_LEFT_MID        */
    {-30.0f, 0.0f},   /* FRONT_LEFT_WOOFER     */
    {30.0f, 0.0f},    /* FRONT_RIGHT           */
    {30.0f, 0.0f},    /* FRONT_RIGHT_TREBBLE   */
    {30.0f, 0.0f},    /* FRONT_RIGHT_MID       */
    {30.0f, 0.0f},    /* FRONT_RIGHT_WOOFER    */
    {0.0f, 0.0f},     /* FRONT_CENTER          */
    {0.0f, 0.0f},     /* LOW_FREQUENCY_EFFECTS */
    {-90.0f, 0.0f},   /* SIDE_LEFT             */
    {90.0f, 0.0f},    /* SIDE_RIGHT            */
    {-135.0f, 0.0f},  /* REAR_LEFT             */
    {135.0f, 0.0f},   /* REAR_RIGHT            */
    {-45.0f, 45.0f},  /* TOP_FRONT_LEFT        */
    {45.0f, 45.0f},   /* TOP_FRONT_RIGHT       */
    {-90.0f, 90.0f},  /* TOP_MIDDLE_LEFT       */
    {90.0f, 90.0f},   /* TOP_MIDDLE_RIGHT      */
    {-135.0f, 45.0f}, /* TOP_REAR_LEFT         */
    {135.0f, 45.0f},  /* TOP_REAR_RIGHT        */
    {0.0f, 90.0f}     /* OVERHEAD              */
};

#endif /*_AUDIOLIB_H_*/
