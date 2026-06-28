# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_gainNCh:
    def __init__(self, dType="float", inChannels=1, inSamples=1, isInterleave=0):
        """Initialize the AUDIOLIB_gainNCh class with private members.

        Args:
            dType (str): Data type, e.g., "float", "double"
            inChannels (int): Number of input channels
            inSamples (int): Number of samples per channel
            isInterleave (int): Whether data is interleaved (1) or planar (0)
        """
        self._dType = dType
        self._inChannels = inChannels
        self._inSamples = inSamples
        self._isInterleave = isInterleave

        # Set min/max values for random generation
        self._minVal = -10
        self._maxVal = 10

    def exec(self, in0, in1):
        """Execute the gainNCh algorithm.

        Args:
            in0 (numpy.ndarray): Input audio data. If planar, shape should be (inChannels, inSamples).
                                If interleaved, shape should be (inSamples, inChannels).
            in1 (numpy.ndarray): Gain values for each channel, shape (inChannels,).

        Returns:
            numpy.ndarray: Output buffer with gain applied.
        """
        # Apply gain based on interleave mode
        if not self._isInterleave:
            # For planar format, multiply each channel by its gain value
            # Using broadcasting: in1[:, np.newaxis] creates a column vector
            # that can be multiplied with each row of in0
            out = in0 * in1[:, np.newaxis]
        else:
            # For interleaved format, transpose input and apply gain
            out = in0 * in1

        return out
