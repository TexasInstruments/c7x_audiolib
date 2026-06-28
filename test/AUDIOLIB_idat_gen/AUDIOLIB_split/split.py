# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_split:
    def __init__(
        self,
        dType="float",
        numSamples=1,
        numChannels=1,
        numOutputs=2,
        numExecReps=1,
        isInputInterleave=1,
    ):
        """Initialize the AUDIOLIB_split class.

        Split Operator Parameters:
        - Input: Single buffer with C_total channels to be split
        - Output: N independent buffers, each with C_out channels
        - Constraint: C_total = N * C_out (equal channels per output)

        Args:
            dType (str): Data type, e.g., "float", "double", "int32"
            numSamples (int): Samples per channel (S)
            numChannels (int): Total input channels (C_total = N * C_out)
                              This is the TOTAL channels to be split, not per output!
            numOutputs (int): Number of output buffers (N). Max 16 per spec.
            numExecReps (int): Number of execution repetitions (frames)
            isInputInterleave (int): 1 for interleaved input, 0 for deinterleaved
        """
        self._dType = dType
        self._numSamples = numSamples
        self._totalChannels = numChannels  # C_total - total input channels to split
        self._numOutputs = numOutputs  # N - number of output ports
        self._numExecReps = numExecReps
        self._isInputInterleave = isInputInterleave

        # Derived values
        self._channelsPerOutput = (
            numChannels // numOutputs
        )  # C_out - channels per output
        self._frameSize = numSamples
        self._samplesPerFrame = numSamples * numChannels  # total elements in one frame

        # Validate numOutputs per spec (max 16)
        self._kMaxOutputs = 16
        if not (1 <= numOutputs <= self._kMaxOutputs):
            raise ValueError(
                f"numOutputs must be between 1 and {self._kMaxOutputs}, got {numOutputs}"
            )

    def exec(self, pInLocal):
        """Execute the split algorithm per specification.

        Split Operator: Inverse of Concat/InputAggregator
        - Input: 1 buffer with C_total = N * C_out channels
        - Output: N independent buffers, each with C_out channels
        - Per-sample channel distribution: each output gets consecutive channels

        Example for INTERLEAVED: C_total=4, N=2, C_out=2
          Input [S0_C0, S0_C1, S0_C2, S0_C3, S1_C0, ...]
          Output[0]: [S0_C0, S0_C1, S1_C0, S1_C1, ...]  (channels 0-1)
          Output[1]: [S0_C2, S0_C3, S1_C2, S1_C3, ...]  (channels 2-3)

        Example for DEINTERLEAVED: C_total=4, N=2, C_out=2
          Input [C0_S0, ..., C0_S255, C1_S0, ..., C1_S255, C2_S0, ..., C3_S255]
          Output[0]: [C0_S0, ..., C0_S255, C1_S0, ..., C1_S255]  (channels 0-1)
          Output[1]: [C2_S0, ..., C2_S255, C3_S0, ..., C3_S255]  (channels 2-3)

        Args:
            pInLocal (numpy.ndarray): Input data of shape
                (numExecReps, S * C_total) where S=samples, C_total=totalChannels
                Layout depends on isInputInterleave:
                - Interleaved: (S, C_total) interleaved per frame
                - Deinterleaved: (C_total, S) deinterleaved per frame

        Returns:
            list: N lists, each a flattened output buffer of length
                  numExecReps * S * C_out
                  where S=samples, C_out=channelsPerOutput
        """
        N = self._numOutputs  # Number of outputs
        S = self._numSamples  # Samples per channel
        C_total = self._totalChannels  # Total input channels to split
        C_out = self._channelsPerOutput  # Output channels per output

        # Map dType string to numpy dtype
        dtype_map = {
            "float": np.float32,
            "double": np.float64,
            "int32": np.int32,
            "uint32": np.uint32,
            "int16": np.int16,
            "uint16": np.uint16,
            "int8": np.int8,
            "uint8": np.uint8,
        }
        np_dtype = dtype_map.get(self._dType, pInLocal.dtype)

        # Allocate N independent output arrays — one per output port
        pOutLocal = [
            np.zeros((self._numExecReps, S * C_out), dtype=np_dtype) for _ in range(N)
        ]

        for frame in range(self._numExecReps):
            if self._isInputInterleave:
                # Reshape aggregated input for this frame to (samples, totalChannels)
                frame_in = pInLocal[frame].reshape((S, C_total))

                # Split per-sample: for each sample, distribute channels to outputs
                # for s in [0,S): out[i][s*Cout : (s+1)*Cout] = in[s, i*Cout : (i+1)*Cout]
                for i in range(N):
                    ch_start = i * C_out
                    ch_end = ch_start + C_out
                    # Extract all samples for this output's channel range and flatten
                    pOutLocal[i][frame] = frame_in[:, ch_start:ch_end].flatten()
            else:
                # Deinterleaved input: reshape to (channels, samples)
                frame_in = pInLocal[frame].reshape((C_total, S))

                # Split per-output: each output gets its consecutive channels
                for i in range(N):
                    ch_start = i * C_out
                    ch_end = ch_start + C_out
                    # Extract channels for this output and flatten to (samples*channels_per_output,)
                    pOutLocal[i][frame] = frame_in[ch_start:ch_end, :].flatten()

        out_lists = [pOutLocal[i].flatten().tolist() for i in range(N)]
        return out_lists
