# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_softClip:
    def __init__(
        self,
        dType="float32",
        inChannels=1,
        inSamples=1,
        threshold=0.5,
        endKnee=1.0,
        samplingRate=48000,
        frequency=1000.0,
        amplitude=1.0,
        strideInElements=1,
        strideOutElements=1,
    ):
        """Initialize the AUDIOLIB_softClip class with private members.

        Args:
            dType (str): Data type, e.g., "float32", "float64"
            inChannels (int): Number of input channels
            inSamples (int): Number of samples per channel
            threshold (float): Threshold value for soft clipping
            endKnee (float): End knee value for soft clipping
            samplingRate (int): Sampling rate in Hz
            frequency (float): Frequency for sine wave generation
            amplitude (float): Amplitude for sine wave generation
            strideInElements (int): Stride for input elements
            strideOutElements (int): Stride for output elements
        """
        self._dType = dType
        self._inChannels = inChannels
        self._inSamples = inSamples
        self._threshold = threshold
        self._endKnee = endKnee
        self._samplingRate = samplingRate
        self._frequency = frequency
        self._amplitude = amplitude
        self._strideInElements = strideInElements
        self._strideOutElements = strideOutElements

        # Set min/max values for random generation
        self._minVal = -10
        self._maxVal = 10

    def _generate_sine(self):
        """Generate a continuous sine wave.

        Returns:
            numpy.ndarray: Array of sine wave samples
        """
        t = np.linspace(
            0, self._inSamples / self._samplingRate, self._inSamples, endpoint=False
        )
        sine_wave = self._amplitude * np.sin(2 * np.pi * self._frequency * t)
        return sine_wave

    def exec(self, in0):
        """Execute the soft clip algorithm.

        Args:
            in0 (numpy.ndarray): Input audio data of shape (inChannels, inSamples).

        Returns:
            numpy.ndarray: Output data with soft clipping applied.
        """
        # Apply soft clipping algorithm
        out = np.zeros_like(in0)
        absIN = np.abs(in0)

        # Linear region (below threshold)
        mask_linear = absIN <= self._threshold
        out[mask_linear] = in0[mask_linear]

        # Soft clipping region (above threshold)
        mask_soft = ~mask_linear
        out_soft = (absIN[mask_soft] - self._threshold) / (
            absIN[mask_soft] - 2 * self._threshold + self._endKnee
        )
        out_soft = (self._endKnee - self._threshold) * out_soft + self._threshold
        out[mask_soft] = np.sign(in0[mask_soft]) * out_soft

        return out
