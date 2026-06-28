# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_gain:
    def __init__(self, dType=None, inChannels=1, inSamples=1):
        """Initialize the AUDIOLIB_gain class with parameters

        Args:
            dType (str): Data type ('int', 'float', 'double')
            inChannels (int): Number of input channels
            inSamples (int): Number of input samples per channel
        """
        self.__dType = dType
        self.__inChannels = inChannels
        self.__inSamples = inSamples

        # Define min/max values for random generation
        self.__minVal = -10
        self.__maxVal = 10

    def exec(self, in0, in1):
        """Execute the gain algorithm

        Args:
            in0 (numpy.ndarray): Input audio data of shape (inChannels, inSamples).
            in1 (float or int): Gain factor to apply.

        Returns:
            numpy.ndarray: Output data with gain applied.
        """
        # Apply gain operation
        out = in0 * in1

        return out
