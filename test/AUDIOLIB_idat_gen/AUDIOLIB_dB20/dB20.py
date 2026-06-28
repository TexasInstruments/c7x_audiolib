# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_dB20:
    def __init__(self, dType="float", inChannels=1, inSamples=1, minX=0.0, maxX=1.0):
        """Initialize the AUDIOLIB_dB20 class with private members.

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
        """Execute the dB20 algorithm.

        Args:
            in0 (numpy.ndarray): Input buffer of shape (inChannels, inSamples) or flattened.

        Returns:
            numpy.ndarray: Output buffer with dB20 conversion applied.
        """
        # Reshape input if needed
        if in0.ndim == 1:
            in0 = in0.reshape(self._inChannels, self._inSamples)

        # Calculate log10 of input
        out = np.log10(in0)

        # Handle special cases
        if self._dType == "float":
            out[in0 > np.finfo(np.float32).max] = 308.2547
            out[in0 <= 0.0] = np.uint32(0xFF800000)  # NaN/-Inf bit pattern for float32
        else:
            out[in0 > np.finfo(np.float64).max] = 308.2547
            out[in0 <= 0.0] = np.uint64(
                0xFFF0000000000000
            )  # NaN/-Inf bit pattern for float64

        # Multiply by 20 to get dB20
        out *= 20

        return out
