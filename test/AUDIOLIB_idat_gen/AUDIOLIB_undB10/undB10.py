# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_undB10:
    def __init__(self, dType="float", inChannels=1, inSamples=1, minX=0.0, maxX=1.0):
        """Initialize the AUDIOLIB_undB10 class with private members.

        Args:
            dType (str): Data type, either "float" or "double"
            inChannels (int): Number of input channels
            inSamples (int): Number of samples per channel
            minX (float): Minimum value for random input generation
            maxX (float): Maximum value for random input generation
        """
        self._dType = dType
        self._inChannels = inChannels
        self._inSamples = inSamples
        self._minX = minX
        self._maxX = maxX

        # Map dType to NumPy dtype
        self._dtype = np.float32 if dType == "float" else np.float64

    def exec(self, in0):
        """Execute the undB10 algorithm.

        Args:
            in0 (numpy.ndarray): Input buffer of shape (inChannels, inSamples) or flattened.

        Returns:
            numpy.ndarray: Output buffer with undB10 conversion applied.
        """
        # Reshape input if needed
        if in0.ndim == 1:
            in0 = in0.reshape(self._inChannels, self._inSamples)

        # Calculate 10^(x/10)
        out = np.power(10, in0 / 10)

        # Handle special cases
        if self._dType == "float":
            out[out > np.finfo(np.float32).max] = np.finfo(np.float32).max
            out[out < np.finfo(np.float32).tiny] = 0.0
        else:
            out[out > np.finfo(np.float64).max] = np.finfo(np.float64).max
            out[out < np.finfo(np.float64).tiny] = 0.0

        return out
