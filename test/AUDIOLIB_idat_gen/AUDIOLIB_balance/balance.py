# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_balance:
    def __init__(
        self,
        dType="float",
        numSamples=1,
        numChannels=1,
        balance=0.0,
        samplingRate=48000,
        smoothingTime=0.0,
        numExecReps=1,
        testType="STATIC",
        amplitude=1.0,
        frequency=2000.0,
    ):
        """Initialize the AUDIOLIB_balance class with private members.

        Args:
            dType (str): Data type, e.g., "float", "double"
            numSamples (int): Number of samples per channel per frame
            numChannels (int): Number of audio channels
            balance (float): Balance value between -1.0 (full left) and 1.0 (full right)
            samplingRate (int): Sampling rate in Hz
            smoothingTime (float): Smoothing time in milliseconds
            numExecReps (int): Number of execution repetitions
            testType (str): Type of test ("STATIC" or "SINE")
            amplitude (float): Amplitude for sine wave generation
            frequency (float): Frequency for sine wave generation in Hz
        """
        self._dType = dType
        self._numSamples = numSamples
        self._numChannels = numChannels
        self._balance = balance
        self._samplingRate = samplingRate
        self._smoothingTime = smoothingTime
        self._numExecReps = numExecReps
        self._testType = testType
        self._amplitude = amplitude
        self._frequency = frequency

        # Set min/max values for random generation
        self._minVal = -10
        self._maxVal = 10

        # Derived values
        self._samplesPerFrame = numSamples * numChannels  # Total samples per frame
        self._totalSamplesPerChannel = (
            numSamples * numExecReps
        )  # Total samples per channel across all frames
        self._frameSize = numSamples  # Samples per channel per frame

    def _generate_sine(self, channel=0):
        """Generate a continuous sine wave for all frames of a given channel.

        Args:
            channel (int): Channel index (for phase offset if needed)

        Returns:
            numpy.ndarray: Array of sine wave samples
        """
        total_samples = self._numSamples * self._numExecReps
        t = np.linspace(
            0, total_samples / self._samplingRate, total_samples, endpoint=False
        )
        sine_wave = self._amplitude * np.sin(2 * np.pi * self._frequency * t)
        return sine_wave

    def exec(self, pInLocalLeft, pInLocalRight):
        """Execute the balance algorithm.

        Args:
            pInLocalLeft (numpy.ndarray): Input data array for left channel of shape (numExecReps, samplesPerFrame).
            pInLocalRight (numpy.ndarray): Input data array for right channel of shape (numExecReps, samplesPerFrame).

        Returns:
            tuple: (output_left, output_right, gain_left, gain_right)
        """
        # Compute target gains and smoothing coefficient
        angle = (1.0 + self._balance) * (np.pi / 4.0)
        targetGainL = np.cos(angle)
        targetGainR = np.sin(angle)

        # Set targetGainL to 0.0 if it is negligibly small
        if abs(targetGainL) < 1e-10:
            targetGainL = 0.0
        # Set targetGainR to 0.0 if it is negligibly small
        if abs(targetGainR) < 1e-10:
            targetGainR = 0.0

        # Initialize gain arrays
        pCurrentGainL = np.zeros(self._totalSamplesPerChannel, dtype=np.float32)
        pCurrentGainR = np.zeros(self._totalSamplesPerChannel, dtype=np.float32)

        # Compute gains frame by frame
        gainThreshold = 1e-6  # Threshold for considering gains equal to targets
        for frame in range(self._numExecReps):
            startIdx = frame * self._frameSize
            endIdx = (frame + 1) * self._frameSize

            # Check if smoothing should be bypassed for this frame
            bypassSmoothing = self._smoothingTime == 0.0
            if frame > 0:  # Check gain convergence for frames after the first
                prev_idx = startIdx - 1
                bypassSmoothing |= (
                    abs(pCurrentGainL[prev_idx] - targetGainL) < gainThreshold
                    and abs(pCurrentGainR[prev_idx] - targetGainR) < gainThreshold
                )

            if bypassSmoothing:
                # No smoothing: Set all gains in this frame to target values
                pCurrentGainL[startIdx:endIdx] = targetGainL
                pCurrentGainR[startIdx:endIdx] = targetGainR
            else:
                # Compute smoothing coefficient
                smoothingTimeSec = (
                    self._smoothingTime / 1000.0
                )  # Convert msec to seconds
                smoothingCoefficient = np.exp(
                    -1.0 / (self._samplingRate * smoothingTimeSec)
                )

                # Compute gains for the frame, starting from index 0
                for j in range(startIdx, endIdx):
                    if frame == 0 and j == startIdx:
                        # Use initial gain of 0.0 for index 0 of frame 0
                        prevGainL = 0.0
                        prevGainR = 0.0
                    else:
                        # Use previous frame's last gain or previous sample's gain
                        prevGainL = pCurrentGainL[j - 1]
                        prevGainR = pCurrentGainR[j - 1]

                    pCurrentGainL[j] = (smoothingCoefficient * prevGainL) + (
                        (1.0 - smoothingCoefficient) * targetGainL
                    )
                    pCurrentGainR[j] = (smoothingCoefficient * prevGainR) + (
                        (1.0 - smoothingCoefficient) * targetGainR
                    )

        # Apply gains to input data frame by frame
        pOutLocalLeft = np.zeros(
            (self._numExecReps, self._samplesPerFrame), dtype=np.float32
        )
        pOutLocalRight = np.zeros(
            (self._numExecReps, self._samplesPerFrame), dtype=np.float32
        )

        for frame in range(self._numExecReps):
            gainStartIdx = frame * self._frameSize
            for i in range(self._numChannels):
                startIdx = i * self._numSamples
                endIdx = (i + 1) * self._numSamples
                for j in range(self._numSamples):
                    gainIdx = gainStartIdx + j
                    pOutLocalLeft[frame, startIdx + j] = (
                        pInLocalLeft[frame, startIdx + j] * pCurrentGainL[gainIdx]
                    )
                    pOutLocalRight[frame, startIdx + j] = (
                        pInLocalRight[frame, startIdx + j] * pCurrentGainR[gainIdx]
                    )

        # Flatten arrays for return
        out_left_list = pOutLocalLeft.flatten().tolist()
        out_right_list = pOutLocalRight.flatten().tolist()
        gain_left_list = pCurrentGainL.flatten().tolist()
        gain_right_list = pCurrentGainR.flatten().tolist()

        return out_left_list, out_right_list, gain_left_list, gain_right_list
