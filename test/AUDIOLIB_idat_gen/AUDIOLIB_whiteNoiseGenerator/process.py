# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np

# Constants
a = 1664525
c = 1013904223
m = 2**32

# LCG steps between the start states of neighbouring channels (AUDIOLIB_WHITENOISEGENERATOR_CHANNEL_SPACING)
CHANNEL_SPACING = 51274189


def lcg_jump(x, steps):
    """Advance LCG state x by `steps` steps, using powers of the 2x2 matrix [[a, c], [0, 1]]."""
    result = [[1, 0], [0, 1]]
    base = [[a, c], [0, 1]]
    while steps > 0:
        if steps & 1:
            result = [
                [(result[0][0] * base[0][0]) % m, (result[0][0] * base[0][1] + result[0][1]) % m],
                [0, 1],
            ]
        base = [[(base[0][0] * base[0][0]) % m, (base[0][0] * base[0][1] + base[0][1]) % m], [0, 1]]
        steps >>= 1
    return (result[0][0] * x + result[0][1]) % m


def generate_white_noise(
    numSamples, numChannels, numExecReps, seed, range_val, dType="AUDIOLIB_FLOAT32"
):
    """
    Generate multichannel white noise across multiple frames, maintaining PRNG state continuity.

    Channel ch runs its own LCG sequence, starting ch * CHANNEL_SPACING steps after the seed.

    Args:
        numSamples (int): Number of samples per channel per frame.
        numChannels (int): Number of channels.
        numExecReps (int): Number of frames to generate.
        seed (int): Initial LCG seed of channel 0.
        range_val (float): Output range [-range_val, range_val].
        dType (str): "AUDIOLIB_FLOAT32" or "AUDIOLIB_FLOAT64".

    Returns:
        numpy.ndarray: Array of (numExecReps * numChannels * numSamples) samples, frame-major,
        then channel, then sample.
    """
    # Choose dtype
    target_dtype = np.float32 if dType == "AUDIOLIB_FLOAT32" else np.float64

    frameLength = numSamples * numChannels
    out = np.zeros(frameLength * numExecReps, dtype=target_dtype)
    x = [lcg_jump(seed, ch * CHANNEL_SPACING) for ch in range(numChannels)]
    scale = target_dtype(target_dtype(range_val) * target_dtype(2.0**-31))

    for frame in range(numExecReps):
        for ch in range(numChannels):
            for i in range(numSamples):
                x[ch] = (a * x[ch] + c) % m
                # (x - 2^31) / 2^31 * range, rounded as the kernel does: int32 -> float, then one multiply
                out[frame * frameLength + ch * numSamples + i] = target_dtype(x[ch] - 2**31) * scale

    return out
