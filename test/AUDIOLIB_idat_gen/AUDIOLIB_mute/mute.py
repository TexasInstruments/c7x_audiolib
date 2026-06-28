# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_mute:
    def __init__(
        self,
        dType="float",
        numSamples=1,
        numChannels=1,
        isMuted=0.0,
        samplingRate=48000,
        fadeTime=0.0,
        isInterleaved=False,
        fadeType="LINEAR",
        numExecReps=1,
        testType="STATIC",
        amplitude=1.0,
        frequency=2000.0,
    ):
        """Initialize the AUDIOLIB_mute class with private members.

        Args:
            dType (str): Data type, e.g., "float", "double"
            numSamples (int): Number of samples per channel per frame
            numChannels (int): Number of audio channels
            isMuted (float): Mute state (0.0 = unmuted, 1.0 = muted)
            samplingRate (int): Sampling rate in Hz
            fadeTime (float): Fade time in milliseconds
            isInterleaved (bool): Whether data is interleaved (True) or planar (False)
            fadeType (str): Type of fade ("LINEAR", "SMOOTH", or "HARD")
            numExecReps (int): Number of execution repetitions
            testType (str): Type of test ("STATIC" or "SINE")
            amplitude (float): Amplitude for sine wave generation
            frequency (float): Frequency for sine wave generation in Hz
        """
        self._dType = dType
        self._numSamples = numSamples
        self._numChannels = numChannels
        self._isMuted = isMuted
        self._samplingRate = samplingRate
        self._fadeTime = fadeTime
        self._isInterleaved = isInterleaved
        self._fadeType = fadeType
        self._numExecReps = numExecReps
        self._testType = testType
        self._amplitude = amplitude
        self._frequency = frequency

        # Set min/max values for random generation
        self._minVal = -10
        self._maxVal = 10

        # Derived values
        self._frameSize = numSamples  # Samples per channel per frame
        self._samplesPerFrame = numSamples * numChannels  # Total samples per frame
        self._totalSamplesPerChannel = (
            numSamples * numExecReps
        )  # Total samples per channel across all frames

        # Set initial gain based on isMuted
        self._initialGain = 1.0 if isMuted == 1.0 else 0.0

    def _generate_sine(self, channel=0, use_phase_offset=True):
        """Generate a continuous sine wave for all frames of a given channel.

        Args:
            channel (int): Channel index (for phase offset if used)
            use_phase_offset (bool): Whether to apply phase offset

        Returns:
            numpy.ndarray: Array of sine wave samples
        """
        total_samples = self._numSamples * self._numExecReps
        t = np.linspace(
            0, total_samples / self._samplingRate, total_samples, endpoint=False
        )
        phase_offset = (
            (channel * np.pi / self._numChannels)
            if use_phase_offset and self._numChannels
            else 0.0
        )
        sine_wave = self._amplitude * np.sin(
            2 * np.pi * self._frequency * t + phase_offset
        )
        return sine_wave

    def exec(self, pInLocal):
        """Execute the mute algorithm.

        Args:
            pInLocal (numpy.ndarray): Input data array of shape (numExecReps, samplesPerFrame).

        Returns:
            tuple: (output_data, gain_data)
        """
        # Compute target gain
        targetGain = 1.0 - self._isMuted
        current_gain = self._initialGain
        pCurrentGain = np.zeros(self._totalSamplesPerChannel, dtype=np.float32)
        gainThreshold = 1e-6

        # Compute gains based on fadeType
        if self._fadeType == "HARD":
            # Instantaneous gain change to target
            pCurrentGain[:] = targetGain
        elif self._fadeType == "LINEAR":
            # Linear fade
            timeConstantSec = self._fadeTime / 1000.0
            totalFadeSamples = timeConstantSec * self._samplingRate
            gainStep = (
                (targetGain - current_gain) / totalFadeSamples
                if totalFadeSamples > 0 and self._fadeTime > 0
                else 0
            )

            for i in range(self._totalSamplesPerChannel):
                if (
                    abs(current_gain - targetGain) < gainThreshold
                    or self._fadeTime == 0
                ):
                    current_gain = targetGain
                else:
                    current_gain += gainStep
                    if (gainStep > 0 and current_gain > targetGain) or (
                        gainStep < 0 and current_gain < targetGain
                    ):
                        current_gain = targetGain
                pCurrentGain[i] = current_gain
        elif self._fadeType == "SMOOTH":
            # Exponential smoothing
            for frame in range(self._numExecReps):
                startIdx = frame * self._frameSize
                endIdx = (frame + 1) * self._frameSize

                bypassSmoothing = self._fadeTime == 0.0
                if frame > 0:
                    prev_idx = startIdx - 1
                    bypassSmoothing |= (
                        abs(pCurrentGain[prev_idx] - targetGain) < gainThreshold
                    )

                if bypassSmoothing:
                    pCurrentGain[startIdx:endIdx] = targetGain
                else:
                    timeConstantSec = self._fadeTime / 1000.0
                    smoothingCoefficient = np.exp(
                        -1.0 / (self._samplingRate * timeConstantSec)
                    )

                    for j in range(startIdx, endIdx):
                        if frame == 0 and j == startIdx:
                            prevGain = current_gain
                        else:
                            prevGain = pCurrentGain[j - 1]
                        pCurrentGain[j] = (smoothingCoefficient * prevGain) + (
                            (1.0 - smoothingCoefficient) * targetGain
                        )

        # Apply gains to input data
        pOutLocal = np.zeros(
            (self._numExecReps, self._samplesPerFrame), dtype=np.float32
        )

        for frame in range(self._numExecReps):
            gainStartIdx = frame * self._frameSize
            if self._isInterleaved:
                for j in range(self._numSamples):
                    for c in range(self._numChannels):
                        idx = j * self._numChannels + c
                        gainIdx = gainStartIdx + j
                        pOutLocal[frame, idx] = (
                            pInLocal[frame, idx] * pCurrentGain[gainIdx]
                        )
            else:
                for i in range(self._numChannels):
                    startIdx = i * self._numSamples
                    endIdx = (i + 1) * self._numSamples
                    for j in range(self._numSamples):
                        gainIdx = gainStartIdx + j
                        pOutLocal[frame, startIdx + j] = (
                            pInLocal[frame, startIdx + j] * pCurrentGain[gainIdx]
                        )

        # Flatten arrays for return
        out_list = pOutLocal.flatten().tolist()
        gain_list = pCurrentGain.flatten().tolist()

        return out_list, gain_list
