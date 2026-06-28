# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


def calculate_gain(
    numChannels,
    numExecReps,
    numSamples,
    samplingRate,
    fadeTime,
    fadeType,
    initialGain,
    targetGain,
):
    """Calculate gain values for each channel based on fade type.

    Args:
        numChannels (int): Number of channels.
        numExecReps (int): Number of execution repetitions.
        numSamples (int): Number of samples per frame.
        samplingRate (int): Sampling rate in Hz.
        fadeTime (float): Fade time in milliseconds.
        fadeType (str): Type of fade ("HARD", "LINEAR", "SMOOTH").
        initialGain (list): Initial gain values for each channel.
        targetGain (list): Target gain values for each channel.

    Returns:
        numpy.ndarray: Gain array of shape (numChannels, numSamples * numExecReps).
    """
    totalSamplesPerChannel = numSamples * numExecReps
    frameSize = numSamples
    pCurrentGain = np.zeros((numChannels, totalSamplesPerChannel), dtype=np.float32)
    gainThreshold = 1e-6

    # Compute gains based on fadeType for each channel
    for ch in range(numChannels):
        curr_g = initialGain[ch]
        targ_g = targetGain[ch]
        if fadeType == "HARD":
            # Instantaneous gain change to target
            pCurrentGain[ch, :] = targ_g
        elif fadeType == "LINEAR":
            # Linear fade
            timeConstantSec = fadeTime / 1000.0
            totalFadeSamples = timeConstantSec * samplingRate
            gainStep = (
                (targ_g - curr_g) / totalFadeSamples
                if totalFadeSamples > 0 and fadeTime > 0
                else 0
            )
            for i in range(totalSamplesPerChannel):
                if abs(curr_g - targ_g) < gainThreshold or fadeTime == 0:
                    curr_g = targ_g
                else:
                    curr_g += gainStep
                    if (gainStep > 0 and curr_g > targ_g) or (
                        gainStep < 0 and curr_g < targ_g
                    ):
                        curr_g = targ_g
                pCurrentGain[ch, i] = curr_g
        elif fadeType == "SMOOTH":
            # Exponential smoothing
            bypassSmoothing = fadeTime == 0.0
            prevGain = curr_g
            for frame in range(numExecReps):
                startIdx = frame * frameSize
                endIdx = (frame + 1) * frameSize
                if frame > 0:
                    prev_idx = startIdx - 1
                    bypassSmoothing = (
                        abs(pCurrentGain[ch, prev_idx] - targ_g) < gainThreshold
                    )
                if bypassSmoothing:
                    pCurrentGain[ch, startIdx:endIdx] = targ_g
                else:
                    timeConstantSec = fadeTime / 1000.0
                    smoothingCoefficient = np.exp(
                        -1.0 / (samplingRate * timeConstantSec)
                    )
                    for j in range(startIdx, endIdx):
                        if frame == 0 and j == startIdx:
                            prevGain = curr_g
                        else:
                            prevGain = pCurrentGain[ch, j - 1]
                        pCurrentGain[ch, j] = (smoothingCoefficient * prevGain) + (
                            (1.0 - smoothingCoefficient) * targ_g
                        )

    return pCurrentGain


def apply_gain(
    pInLocal, pCurrentGain, numExecReps, numSamples, numChannels, isInterleaved
):
    """Apply gain values to input data to generate output data.

    Args:
        pInLocal (numpy.ndarray): Input data array of shape (numExecReps, numSamples * numChannels).
        pCurrentGain (numpy.ndarray): Gain array of shape (numChannels, numSamples * numExecReps).
        numExecReps (int): Number of execution repetitions.
        numSamples (int): Number of samples per frame.
        numChannels (int): Number of channels.
        isInterleaved (bool): Whether the data is interleaved.

    Returns:
        numpy.ndarray: Output data array of shape (numExecReps, numSamples * numChannels).
    """
    samplesPerFrame = numSamples * numChannels
    frameSize = numSamples
    pOutLocal = np.zeros((numExecReps, samplesPerFrame), dtype=np.float32)

    for frame in range(numExecReps):
        gainStartIdx = frame * frameSize
        if isInterleaved:
            for j in range(numSamples):
                for c in range(numChannels):
                    idx = j * numChannels + c
                    gainIdx = gainStartIdx + j
                    pOutLocal[frame, idx] = (
                        pInLocal[frame, idx] * pCurrentGain[c, gainIdx]
                    )
        else:
            for i in range(numChannels):
                startIdx = i * numSamples
                endIdx = (i + 1) * numSamples
                for j in range(numSamples):
                    gainIdx = gainStartIdx + j
                    pOutLocal[frame, startIdx + j] = (
                        pInLocal[frame, startIdx + j] * pCurrentGain[i, gainIdx]
                    )

    return pOutLocal
