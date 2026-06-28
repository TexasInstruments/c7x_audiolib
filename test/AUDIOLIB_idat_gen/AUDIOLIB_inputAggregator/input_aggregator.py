# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
Input Aggregator Core Module

This module provides the core input aggregation functionality for combining
multiple input audio buffers into a single unified output buffer. It is used
as a preprocessing stage for downstream kernels like TISP mixer.

The aggregator supports:
- Multiple input buffers with different channel counts
- Interleaved and non-interleaved input/output format combinations
- Float32 and Float64 data types
"""

import numpy as np
from typing import List, Tuple


def input_aggregator(
    individual_inputs: List[np.ndarray],
    is_input_interleave: bool,
    is_output_interleave: bool,
) -> np.ndarray:
    """
    Aggregate multiple input buffers into a single output buffer.

    This function combines N input audio buffers, each potentially having
    different channel counts, into a single unified output buffer. It handles
    format conversion between interleaved and non-interleaved layouts.

    Args:
        individual_inputs: List of numpy arrays, each representing an input buffer.
                          - If is_input_interleave=True: shape is (samples, channels)
                          - If is_input_interleave=False: shape is (channels, samples)
        is_input_interleave: True if inputs are in interleaved format
        is_output_interleave: True if output should be in interleaved format

    Returns:
        np.ndarray: Aggregated output buffer containing all channels.
                   - If is_output_interleave=True: shape is (samples, total_channels)
                   - If is_output_interleave=False: shape is (total_channels, samples)

    Example:
        >>> # Two non-interleaved inputs: 2 channels and 3 channels, 100 samples each
        >>> input1 = np.random.randn(2, 100).astype(np.float32)  # (channels, samples)
        >>> input2 = np.random.randn(3, 100).astype(np.float32)  # (channels, samples)
        >>> output = input_aggregator([input1, input2], is_input_interleave=False, is_output_interleave=False)
        >>> output.shape  # (5, 100) - 5 total channels, 100 samples
    """

    if len(individual_inputs) == 0:
        raise ValueError("At least one input buffer is required")

    # Convert all inputs to planar (channels, samples) format for aggregation
    planar_inputs = []
    for inp in individual_inputs:
        if is_input_interleave:
            # Input shape: (samples, channels) -> transpose to (channels, samples)
            planar_inputs.append(inp.T)
        else:
            # Input shape: (channels, samples) -> already planar
            planar_inputs.append(inp)

    # Aggregate: stack all channels vertically
    # Result shape: (total_channels, samples)
    merged_deinterleaved = np.vstack(planar_inputs)

    # Convert to output format
    if is_output_interleave:
        # Output shape: (samples, total_channels)
        return merged_deinterleaved.T
    else:
        # Output shape: (total_channels, samples)
        return merged_deinterleaved


def generate_inputs(
    in_channels_list: List[int],
    num_samples: int,
    dtype: str,
    is_input_interleave: bool,
    min_val: float = -10.0,
    max_val: float = 10.0,
    seed: int = None,
) -> List[np.ndarray]:
    """
    Generate random input buffers for testing the input aggregator.

    Args:
        in_channels_list: List of channel counts for each input buffer
        num_samples: Number of samples per channel (same for all inputs)
        dtype: Data type - 'float' or 'double'
        is_input_interleave: True to generate interleaved format
        min_val: Minimum value for random data
        max_val: Maximum value for random data
        seed: Optional random seed for reproducibility

    Returns:
        List of numpy arrays representing input buffers
    """
    if seed is not None:
        np.random.seed(seed)

    np_dtype = np.float32 if dtype.startswith("f") else np.float64

    individual_inputs = []
    for num_ch in in_channels_list:
        if is_input_interleave:
            # Interleaved: (samples, channels)
            shape = (num_samples, num_ch)
        else:
            # Non-interleaved (planar): (channels, samples)
            shape = (num_ch, num_samples)

        inp = np.random.uniform(low=min_val, high=max_val, size=shape).astype(np_dtype)
        individual_inputs.append(inp)

    return individual_inputs


def get_output_shape(
    in_channels_list: List[int], num_samples: int, is_output_interleave: bool
) -> Tuple[int, int]:
    """
    Calculate the expected output buffer shape.

    Args:
        in_channels_list: List of channel counts for each input
        num_samples: Number of samples per channel
        is_output_interleave: True if output is interleaved

    Returns:
        Tuple of (dim1, dim2) representing output shape
    """
    total_channels = sum(in_channels_list)

    if is_output_interleave:
        return (num_samples, total_channels)
    else:
        return (total_channels, num_samples)
