# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_subBlockStatistics:
    def __init__(
        self,
        sampleDataType="float",
        numChannels=1,
        inputBlockSize=1,
        subBlockSize=1,
        statisticsType=0,
        flagPerChanStats=True,
    ):
        """Initialize the AUDIOLIB_subBlockStatistics class with private members.

        Args:
            sampleDataType (str): Data type, either "float" or "double"
            numChannels (int): Number of input channels
            inputBlockSize (int): Number of samples per channel
            subBlockSize (int): Size of each subblock
            statisticsType (int): Type of statistic to compute:
                0=max, 1=min, 2=max_abs, 3=mean, 4=rms, 5=std,
                6=variance, 7=avg_energy, 8=sum, 9=sum_squares
            flagPerChanStats (bool): Whether to compute statistics per channel (True) or across all channels (False)
        """
        self._sampleDataType = sampleDataType
        self._numChannels = numChannels
        self._inputBlockSize = inputBlockSize
        self._subBlockSize = subBlockSize
        self._statisticsType = statisticsType
        self._flagPerChanStats = flagPerChanStats

        # Map sampleDataType to NumPy dtype
        self._dtype = np.float32 if sampleDataType == "float" else np.float64

    def compute_statistic(self, data):
        """Compute the specified statistic over the data.

        Args:
            data (numpy.ndarray): Flattened block data.

        Returns:
            float: Computed statistic.
        """
        if data.size == 0:
            return 0.0

        if self._statisticsType == 0:  # Maximum
            return np.max(data)
        elif self._statisticsType == 1:  # Minimum
            return np.min(data)
        elif self._statisticsType == 2:  # Maximum Absolute Value
            return np.max(np.abs(data))
        elif self._statisticsType == 3:  # Mean
            return np.mean(data)
        elif self._statisticsType == 4:  # RMS
            return np.sqrt(np.mean(data**2))
        elif self._statisticsType == 5:  # Standard Deviation
            return np.std(data)
        elif self._statisticsType == 6:  # Variance
            return np.var(data)
        elif self._statisticsType == 7:  # Average Energy
            return np.mean(data**2)
        elif self._statisticsType == 8:  # Sum
            return np.sum(data)
        elif self._statisticsType == 9:  # Sum of Squares
            return np.sum(data**2)
        else:
            raise ValueError(f"Invalid statistics_type: {self._statisticsType}")

    def exec(self, inputSignal):
        """Execute the subBlockStatistics algorithm.

        Args:
            inputSignal (numpy.ndarray): Input signal in planar format [ch0_samples, ch1_samples, ...].

        Returns:
            numpy.ndarray: Output buffer containing computed statistics.
        """
        # Calculate output block size and output channels
        outputBlockSize = self._inputBlockSize // self._subBlockSize
        outputNumChannels = self._numChannels if self._flagPerChanStats else 1
        totalOutputSamples = outputBlockSize * outputNumChannels

        # Compute statistics for each subblock
        outputSignal = np.zeros(totalOutputSamples, dtype=self._dtype)

        if self._flagPerChanStats:
            # Per-channel statistics
            for ch in range(self._numChannels):
                for i in range(outputBlockSize):
                    startIdx = ch * self._inputBlockSize + i * self._subBlockSize
                    endIdx = startIdx + self._subBlockSize
                    channel_data = inputSignal[startIdx:endIdx]
                    outputSignal[ch * outputBlockSize + i] = self.compute_statistic(
                        channel_data
                    )
        else:
            # All-channel statistics
            for i in range(outputBlockSize):
                subblockData = []
                for ch in range(self._numChannels):
                    startIdx = ch * self._inputBlockSize + i * self._subBlockSize
                    endIdx = startIdx + self._subBlockSize
                    subblockData.extend(inputSignal[startIdx:endIdx].tolist())
                subblockData = np.array(subblockData, dtype=self._dtype)
                outputSignal[i] = self.compute_statistic(subblockData)

        return outputSignal
