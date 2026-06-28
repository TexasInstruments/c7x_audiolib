# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_router:
    def __init__(
        self,
        dType="float",
        inChannels=1,
        inSamples=1,
        isInterleave=0,
        routerConfig=None,
        strideInElements=1,
        strideOutElements=1,
    ):
        """Initialize the AUDIOLIB_router class with private members.

        Args:
            dType (str): Data type, e.g., "float", "double"
            inChannels (int): Number of input channels
            inSamples (int): Number of samples per channel
            isInterleave (int): Whether data is interleaved (1) or planar (0)
            routerConfig (list): List of output channel indices
            strideInElements (int): Stride for input elements
            strideOutElements (int): Stride for output elements
        """
        self._dType = dType
        self._inChannels = inChannels
        self._inSamples = inSamples
        self._isInterleave = isInterleave
        self._routerConfig = routerConfig if routerConfig is not None else [0]
        self._strideInElements = strideInElements
        self._strideOutElements = strideOutElements

        # Set min/max values for random generation
        self._minVal = -10
        self._maxVal = 10

    def exec(self, in0):
        """Execute the router algorithm.

        Args:
            in0 (numpy.ndarray): Input audio data. If interleaved, shape should be (inSamples, inChannels).
                                If planar, shape should be (inChannels, inSamples).

        Returns:
            numpy.ndarray: Output data with channels routed according to routerConfig.
        """
        # Route channels to output based on interleaving mode
        if self._isInterleave == 1:
            # Interleaved format: samples x channels
            # Route channels to output
            output = []
            for ch in self._routerConfig:
                if 0 <= ch < self._inChannels:  # Valid channel index
                    output.append(in0[:, ch])
                else:  # Invalid → pad with zeros
                    output.append(np.zeros(in0.shape[0], dtype=in0.dtype))

            output = np.stack(output, axis=1)

        else:
            # Planar format: channels x samples
            # Route channels to output
            output = []
            for ch in self._routerConfig:
                if 0 <= ch < self._inChannels:  # Valid channel index
                    output.append(in0[ch, :])
                else:  # Invalid → pad with zeros
                    output.append(np.zeros(self._inSamples, dtype=in0.dtype))

            output = np.vstack(output)

        return output
