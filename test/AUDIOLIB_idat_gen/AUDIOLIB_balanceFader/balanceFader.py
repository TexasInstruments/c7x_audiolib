# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function
import numpy as np
import sys
import os

# Add the common directory to sys.path
sys.path.append(
    os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "common")
)
import AUDIOLIB_channelMappings


class AUDIOLIB_balanceFader:
    def __init__(
        self,
        dType="float",
        numSamples=256,
        numChannels=6,
        balance=0.0,
        fader=0.0,
        sideGainFactor=0.7,
        lfeBalanceGainFactor=0.7,
        lfeFaderGainFactor=0.7,
        lfeBalanceMode=0,
        lfeFaderMode=0,
        isInterleave=False,
        channelLayout=None,
    ):
        """Initialize the AUDIOLIB_balanceFader class with private members.

        Args:
            dType (str): Data type, either "float" or "double"
            numSamples (int): Number of samples per channel
            numChannels (int): Number of audio channels
            balance (float): Balance value between -1.0 (left) and 1.0 (right)
            fader (float): Fader value between -1.0 (front) and 1.0 (back)
            sideGainFactor (float): Gain factor for side channels
            lfeBalanceGainFactor (float): Gain factor for LFE balance
            lfeFaderGainFactor (float): Gain factor for LFE fader
            lfeBalanceMode (int): LFE balance mode (0=unaffected, 1=partial, 2=full)
            lfeFaderMode (int): LFE fader mode (0=unaffected, 1=average, 2=position_based, 3=full)
            isInterleave (bool): Whether data is interleaved
            channelLayout (list): List of channel names
        """
        self._dType = dType
        self._numSamples = numSamples
        self._numChannels = numChannels
        self._balance = balance
        self._fader = fader
        self._sideGainFactor = sideGainFactor
        self._lfeBalanceGainFactor = lfeBalanceGainFactor
        self._lfeFaderGainFactor = lfeFaderGainFactor
        self._lfeBalanceMode = lfeBalanceMode
        self._lfeFaderMode = lfeFaderMode
        self._isInterleave = isInterleave

        # Set default channel layout if not provided
        if channelLayout is None:
            self._channelLayout = self._get_channel_layout(numChannels)
        else:
            self._channelLayout = channelLayout

        # Map dType to NumPy dtype
        self._dtype = np.float32 if dType == "float" else np.float64

        # Constants
        self._CMAP = AUDIOLIB_channelMappings.AUDIOLIB_CHANNEL_MAP
        self._LFE_BALANCE_MAP = AUDIOLIB_channelMappings.AUDIOLIB_LFE_BALANCE_MODE_MAP
        self._LFE_FADER_MAP = AUDIOLIB_channelMappings.AUDIOLIB_LFE_FADER_MODE_MAP

    def _get_channel_layout(self, numChannels):
        """Define the speaker configuration for a given number of channels."""
        base_layout = [
            "FRONT_LEFT_TREBBLE",
            "FRONT_RIGHT_TREBBLE",
            "FRONT_CENTER",
            "LOW_FREQUENCY_EFFECTS",
            "SIDE_LEFT",
            "SIDE_RIGHT",
            "REAR_LEFT",
            "REAR_RIGHT",
            "FRONT_LEFT_MID",
            "FRONT_RIGHT_MID",
            "TOP_FRONT_LEFT",
            "TOP_FRONT_RIGHT",
            "FRONT_LEFT_WOOFER",
            "FRONT_RIGHT_WOOFER",
            "TOP_REAR_LEFT",
            "TOP_REAR_RIGHT",
        ]
        full_layout = (base_layout * (numChannels // len(base_layout) + 1))[
            :numChannels
        ]
        return full_layout

    def exec(self, in0):
        """Execute the balanceFader algorithm.

        Args:
            in0 (numpy.ndarray): Input audio data. If not interleaved, shape should be (numChannels, numSamples).
                                If interleaved, shape should be (numSamples, numChannels).

        Returns:
            tuple: (output_buffer, channel_config_integers) - The output data and channel configuration integers.
        """
        # --- 1. Get the custom channel layout and convert to integer array ---
        channel_config_integers = [
            self._CMAP[f"AUDIOLIB_CHANNEL_{name}"] for name in self._channelLayout
        ]

        # --- 2. Calculate Fundamental Gain Components ---
        balance_angle = (np.pi / 4.0) * (self._balance + 1)
        targetGainL = np.cos(balance_angle)
        targetGainR = np.sin(balance_angle)

        fader_angle = (np.pi / 4.0) * (self._fader + 1)
        frontGain = np.sin(fader_angle)
        backGain = np.cos(fader_angle)
        sideGain = (
            self._sideGainFactor * backGain + (1 - self._sideGainFactor) * frontGain
            if self._fader < 0
            else backGain
        )
        topMiddleGain = 0.5 * (frontGain + backGain)

        # --- 3. Calculate Final Gain for Each Channel in the Layout ---
        final_channel_gains = np.zeros(self._numChannels, dtype=self._dtype)
        for i, channel_name in enumerate(self._channelLayout):
            channel_enum = self._CMAP[f"AUDIOLIB_CHANNEL_{channel_name}"]
            gain = 0.0

            front_left_channels = [
                self._CMAP["AUDIOLIB_CHANNEL_FRONT_LEFT"],
                self._CMAP["AUDIOLIB_CHANNEL_FRONT_LEFT_TREBBLE"],
                self._CMAP["AUDIOLIB_CHANNEL_FRONT_LEFT_MID"],
                self._CMAP["AUDIOLIB_CHANNEL_FRONT_LEFT_WOOFER"],
            ]
            front_right_channels = [
                self._CMAP["AUDIOLIB_CHANNEL_FRONT_RIGHT"],
                self._CMAP["AUDIOLIB_CHANNEL_FRONT_RIGHT_TREBBLE"],
                self._CMAP["AUDIOLIB_CHANNEL_FRONT_RIGHT_MID"],
                self._CMAP["AUDIOLIB_CHANNEL_FRONT_RIGHT_WOOFER"],
            ]

            if channel_enum in front_left_channels:
                gain = targetGainL * frontGain
            elif channel_enum in front_right_channels:
                gain = targetGainR * frontGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_FRONT_CENTER"]:
                gain = 1.0 * frontGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_LOW_FREQUENCY_EFFECTS"]:
                lfeBalanceGain = 1.0
                if (
                    self._lfeBalanceMode
                    == self._LFE_BALANCE_MAP["AUDIOLIB_LFE_BALANCE_PARTIAL"]
                ):
                    lfeBalanceGain = self._lfeBalanceGainFactor + (
                        1 - self._lfeBalanceGainFactor
                    ) * np.cos(2 * balance_angle)
                elif (
                    self._lfeBalanceMode
                    == self._LFE_BALANCE_MAP["AUDIOLIB_LFE_BALANCE_FULL"]
                ):
                    lfeBalanceGain = 0.5 * (targetGainL + targetGainR)

                lfeFaderGain = 1.0
                if (
                    self._lfeFaderMode
                    == self._LFE_FADER_MAP["AUDIOLIB_LFE_FADER_AVERAGE"]
                ):
                    lfeFaderGain = 0.5 * (frontGain + backGain)
                elif (
                    self._lfeFaderMode
                    == self._LFE_FADER_MAP["AUDIOLIB_LFE_FADER_POSITION_BASED"]
                ):
                    lfeFaderGain = (
                        1 - self._lfeFaderGainFactor
                    ) * frontGain + self._lfeFaderGainFactor * backGain
                elif (
                    self._lfeFaderMode == self._LFE_FADER_MAP["AUDIOLIB_LFE_FADER_FULL"]
                ):
                    lfeFaderGain = backGain
                gain = lfeBalanceGain * lfeFaderGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_SIDE_LEFT"]:
                gain = targetGainL * sideGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_SIDE_RIGHT"]:
                gain = targetGainR * sideGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_REAR_LEFT"]:
                gain = targetGainL * backGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_REAR_RIGHT"]:
                gain = targetGainR * backGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_TOP_FRONT_LEFT"]:
                gain = targetGainL * frontGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_TOP_FRONT_RIGHT"]:
                gain = targetGainR * frontGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_TOP_MIDDLE_LEFT"]:
                gain = targetGainL * topMiddleGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_TOP_MIDDLE_RIGHT"]:
                gain = targetGainR * topMiddleGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_TOP_REAR_LEFT"]:
                gain = targetGainL * backGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_TOP_REAR_RIGHT"]:
                gain = targetGainR * backGain
            elif channel_enum == self._CMAP["AUDIOLIB_CHANNEL_OVERHEAD"]:
                gain = 1.0 * topMiddleGain

            final_channel_gains[i] = gain

        # Apply gains to generate output
        if not self._isInterleave:
            # Non-interleaved: shape (numChannels, numSamples)
            # Reshape final_channel_gains to allow broadcasting with in0
            out = in0 * final_channel_gains[:, np.newaxis]
            out_list = out.flatten()
        else:
            # Interleaved: shape (numSamples, numChannels)
            # Reshape final_channel_gains to allow broadcasting with in0
            in0_transposed = in0.T  # Shape (numSamples, numChannels)
            out = in0_transposed * final_channel_gains
            out_list = out.flatten()

        return out_list, channel_config_integers
