# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_muteNCh:
    def __init__(
        self,
        dType="float32",
        numChannels=1,
        numSamples=1,
        numExecReps=1,
        samplingRate=48000,
        fadeTime=0.0,
        fadeType="HARD",
        initialGain=None,
        targetGain=None,
        isInterleaved=False,
    ):
        """Initialize the AUDIOLIB_muteNCh class with private members.

        Args:
            dType (str): Data type, e.g., "float32", "float64"
            numChannels (int): Number of channels
            numSamples (int): Number of samples per frame
            numExecReps (int): Number of execution repetitions
            samplingRate (int): Sampling rate in Hz
            fadeTime (float): Fade time in milliseconds
            fadeType (str): Type of fade ("HARD", "LINEAR", "SMOOTH", "SYMMETRIC")
            initialGain (list): Initial gain values for each channel
            targetGain (list): Target gain values for each channel
            isInterleaved (bool): Whether the data is interleaved
        """
        self._dType = dType
        self._numChannels = numChannels
        self._numSamples = numSamples
        self._numExecReps = numExecReps
        self._samplingRate = samplingRate
        self._fadeTime = fadeTime
        self._fadeType = fadeType
        self._initialGain = (
            initialGain if initialGain is not None else [1.0] * numChannels
        )
        self._targetGain = targetGain if targetGain is not None else [0.0] * numChannels
        self._isInterleaved = isInterleaved

        # Set min/max values for random generation
        self._minVal = -10
        self._maxVal = 10

    def _calculate_gain(self):
        """Calculate gain values for each channel based on fade type.

        Returns:
            numpy.ndarray: Gain array of shape (numChannels, numSamples * numExecReps).
        """
        totalSamplesPerChannel = self._numSamples * self._numExecReps
        frameSize = self._numSamples
        pCurrentGain = np.zeros(
            (self._numChannels, totalSamplesPerChannel), dtype=np.float32
        )
        gainThreshold = 1e-6

        # Compute gains based on fadeType for each channel
        for ch in range(self._numChannels):
            curr_g = self._initialGain[ch]
            targ_g = self._targetGain[ch]
            if self._fadeType == "HARD" or self._fadeTime == 0:
                # Instantaneous gain change to target
                pCurrentGain[ch, :] = targ_g
            elif self._fadeType == "LINEAR":
                # Linear fade at a constant rate: a full fade, from 1 - target to target, lasts fadeTime
                timeConstantSec = self._fadeTime / 1000.0
                totalFadeSamples = timeConstantSec * self._samplingRate
                gainStep = (targ_g - (1.0 - targ_g)) / totalFadeSamples
                for i in range(totalSamplesPerChannel):
                    if abs(curr_g - targ_g) < gainThreshold:
                        curr_g = targ_g
                    else:
                        curr_g += gainStep
                        if (gainStep > 0 and curr_g > targ_g) or (
                            gainStep < 0 and curr_g < targ_g
                        ):
                            curr_g = targ_g
                    pCurrentGain[ch, i] = curr_g
            elif self._fadeType == "SYMMETRIC":
                # Constant ratio per sample between the floor (-200 dB) and 1, in float32 as the kernel computes it
                floor = np.float32(1e-10)
                totalFadeSamples = (
                    np.float32(self._fadeTime) * np.float32(self._samplingRate)
                ) / np.float32(1000.0)
                ratioUp = np.float32(
                    np.power(np.float32(1.0) / floor, np.float32(1.0) / totalFadeSamples)
                )
                ratio = ratioUp if targ_g == 1.0 else np.float32(np.float32(1.0) / ratioUp)
                # set() starts the ramp from the running gain, no lower than one ratio step below the floor
                curr_g = max(np.float32(curr_g), np.float32(floor / ratio))
                for i in range(totalSamplesPerChannel):
                    curr_g = min(max(np.float32(curr_g * ratio), floor), np.float32(1.0))
                    pCurrentGain[ch, i] = curr_g if curr_g > floor else 0.0
            elif self._fadeType == "SMOOTH":
                # Exponential smoothing on the distance from the target, in float32 as the kernel computes it:
                # the distance shrinks by alpha every sample and the gain is target + distance
                timeConstantSec = np.float32(self._fadeTime) / np.float32(1000.0)
                smoothingCoefficient = np.float32(
                    np.exp(np.float32(-1.0) / (np.float32(self._samplingRate) * timeConstantSec))
                )
                targ_g = np.float32(targ_g)
                distance = np.float32(np.float32(curr_g) - targ_g)
                for i in range(totalSamplesPerChannel):
                    distance = np.float32(distance * smoothingCoefficient)
                    curr_g = np.float32(targ_g + distance)
                    pCurrentGain[ch, i] = curr_g

        return pCurrentGain

    def _apply_gain(self, pInLocal, pCurrentGain):
        """Apply gain values to input data to generate output data.

        Args:
            pInLocal (numpy.ndarray): Input data array of shape (numExecReps, numSamples * numChannels).
            pCurrentGain (numpy.ndarray): Gain array of shape (numChannels, numSamples * numExecReps).

        Returns:
            numpy.ndarray: Output data array of shape (numExecReps, numSamples * numChannels).
        """
        samplesPerFrame = self._numSamples * self._numChannels
        frameSize = self._numSamples
        pOutLocal = np.zeros((self._numExecReps, samplesPerFrame), dtype=np.float32)

        for frame in range(self._numExecReps):
            gainStartIdx = frame * frameSize
            if self._isInterleaved:
                for j in range(self._numSamples):
                    for c in range(self._numChannels):
                        idx = j * self._numChannels + c
                        gainIdx = gainStartIdx + j
                        pOutLocal[frame, idx] = (
                            pInLocal[frame, idx] * pCurrentGain[c, gainIdx]
                        )
            else:
                for i in range(self._numChannels):
                    startIdx = i * self._numSamples
                    endIdx = (i + 1) * self._numSamples
                    for j in range(self._numSamples):
                        gainIdx = gainStartIdx + j
                        pOutLocal[frame, startIdx + j] = (
                            pInLocal[frame, startIdx + j] * pCurrentGain[i, gainIdx]
                        )

        return pOutLocal

    def exec(self, pInLocal):
        """Execute the muteNCh algorithm.

        Args:
            pInLocal (numpy.ndarray): Input data array of shape (numExecReps, numSamples * numChannels).

        Returns:
            numpy.ndarray: Output data array of shape (numExecReps, numSamples * numChannels).
        """
        # Calculate gain values
        pCurrentGain = self._calculate_gain()

        # Apply gain to input data
        pOutLocal = self._apply_gain(pInLocal, pCurrentGain)

        return pOutLocal
