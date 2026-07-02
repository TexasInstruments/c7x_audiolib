# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_split:
    def __init__(
        self,
        dType="float",
        numSamples=1,
        outChannelsList=None,
        numExecReps=1,
        isInputInterleave=1,
    ):
        """Initialize the AUDIOLIB_split class.

        Split Operator Parameters:
        - Input: Single buffer with C_total channels to be split
        - Output: N independent buffers, where output i has outChannelsList[i] channels
        - Constraint: C_total = sum(outChannelsList)

        Args:
            dType (str): Data type, e.g., "float", "double", "int32"
            numSamples (int): Samples per channel (S)
            outChannelsList (list[int]): Per-output channel counts (each output may differ)
            numExecReps (int): Number of execution repetitions (frames)
            isInputInterleave (int): 1 for interleaved input, 0 for deinterleaved
        """
        if outChannelsList is None:
            outChannelsList = [1]
        self._dType = dType
        self._numSamples = numSamples
        self._outChannelsList = list(outChannelsList)
        self._numOutputs = len(self._outChannelsList)  # N - number of output ports
        self._totalChannels = sum(self._outChannelsList)  # C_total
        self._numExecReps = numExecReps
        self._isInputInterleave = isInputInterleave

        # Validate numOutputs per spec (max 64)
        self._kMaxOutputs = 64
        if not (1 <= self._numOutputs <= self._kMaxOutputs):
            raise ValueError(
                f"numOutputs must be between 1 and {self._kMaxOutputs}, got {self._numOutputs}"
            )

    def exec(self, pInLocal):
        """Execute the split algorithm per specification.

        Split Operator: Inverse of Concat/InputAggregator
        - Input: 1 buffer with C_total = sum(outChannelsList) channels
        - Output: N independent buffers, output i with outChannelsList[i] channels
        - Each output gets a contiguous, cumulative slice of channels

        Args:
            pInLocal (numpy.ndarray): Input data of shape (numExecReps, S * C_total).
                Layout depends on isInputInterleave:
                - Interleaved: (S, C_total) interleaved per frame
                - Deinterleaved: (C_total, S) deinterleaved per frame

        Returns:
            list: N lists, each a flattened output buffer of length
                  numExecReps * S * outChannelsList[i]
        """
        N = self._numOutputs
        S = self._numSamples
        C_total = self._totalChannels

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
            np.zeros((self._numExecReps, S * self._outChannelsList[i]), dtype=np_dtype)
            for i in range(N)
        ]

        for frame in range(self._numExecReps):
            if self._isInputInterleave:
                # (samples, totalChannels) — per-sample interleaved
                frame_in = pInLocal[frame].reshape((S, C_total))
                ch_start = 0
                for i in range(N):
                    ch_end = ch_start + self._outChannelsList[i]
                    pOutLocal[i][frame] = frame_in[:, ch_start:ch_end].flatten()
                    ch_start = ch_end
            else:
                # (channels, samples) — planar / deinterleaved
                frame_in = pInLocal[frame].reshape((C_total, S))
                ch_start = 0
                for i in range(N):
                    ch_end = ch_start + self._outChannelsList[i]
                    pOutLocal[i][frame] = frame_in[ch_start:ch_end, :].flatten()
                    ch_start = ch_end

        out_lists = [pOutLocal[i].flatten().tolist() for i in range(N)]
        return out_lists
