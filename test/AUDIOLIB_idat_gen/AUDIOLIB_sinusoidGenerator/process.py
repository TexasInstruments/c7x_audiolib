# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


def generate_sinusoid(
    numSamples,
    numExecReps,
    startFrequency,
    targetFrequency,
    startPhase,
    smoothingTime,
    samplingRate,
    dType="AUDIOLIB_FLOAT32",
):
    """
    Generate the output sinusoid samples based on frequency smoothing parameters.

    Args:
        numSamples (int): Number of samples per frame.
        numExecReps (int): Number of execution repetitions.
        startFrequency (float): Starting frequency in Hz.
        targetFrequency (float): Target frequency in Hz.
        startPhase (float): Starting phase in degrees.
        smoothingTime (float): Smoothing time in milliseconds.
        samplingRate (int): Sampling rate in Hz.
        dType (str): Data type ("AUDIOLIB_FLOAT32" or "AUDIOLIB_FLOAT64").

    Returns:
        numpy.ndarray: Output array of shape (numSamples * numExecReps,).
    """
    # --- Use numpy float32 consistently ---
    target_dtype = np.float32 if dType == "AUDIOLIB_FLOAT32" else np.float64

    # Calculate total samples across all numExecReps
    totalSamples = numSamples * numExecReps

    # Allocate output array with the target type directly
    out = np.zeros((totalSamples), dtype=target_dtype)

    # --- Define constants with target precision ---
    TWO_PI = target_dtype(2.0 * np.pi)
    PI = target_dtype(np.pi)
    sampling_rate_f = target_dtype(samplingRate)  # Use float version for calculations

    # --- Calculate initial parameters with target precision ---
    # Convert degrees to radians
    phase = target_dtype(startPhase * PI / 180.0)
    phaseIncTarget = target_dtype(TWO_PI * targetFrequency / sampling_rate_f)
    phaseInc = target_dtype(TWO_PI * startFrequency / sampling_rate_f)

    # Calculate smoothing coefficient carefully, handle smoothingTime == 0
    if smoothingTime > 0:
        # Normal smoothing path
        phaseInc = target_dtype(TWO_PI * startFrequency / sampling_rate_f)
        smoothingCoeff = target_dtype(
            1.0 - np.exp(-1.0 / (sampling_rate_f * smoothingTime / 1000.0))
        )
        oneMinusRate = target_dtype(1.0) - smoothingCoeff
    else:
        phaseInc = (
            phaseIncTarget  # Start frequency is ignored, phaseInc is set to target
        )
        smoothingCoeff = target_dtype(1.0)
        oneMinusRate = target_dtype(0.0)

    # --- Initialize loop variables with target precision ---
    localAmp = phaseInc
    localPhase = phase
    MAX_VAL = target_dtype(1048576.0)

    # --- Simulation loop mimicking C's single-precision steps ---
    for i in range(totalSamples):
        # Calculate sine using the current phase
        # np.sin should respect the dtype of localPhase for its calculation and output
        if np.abs(localPhase) > MAX_VAL:
            out[i] = target_dtype(0.0)
        else:
            out[i] = np.sin(localPhase)

        # Update phase for the *next* sample, casting intermediate result
        localPhase = target_dtype(localPhase + localAmp)

        # Wrap phase if necessary, casting intermediate result
        # Using a loop for potential multiple wraps, although unlikely per sample
        if localPhase >= TWO_PI:
            localPhase = target_dtype(localPhase - TWO_PI)

        # Update phase increment (smoothing) for the *next* sample, casting intermediate result
        localAmp = target_dtype(
            localAmp * oneMinusRate + phaseIncTarget * smoothingCoeff
        )

    return out
