# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_delayNChannel:
    def __init__(
        self,
        dType=None,
        mode=0,
        interleave=0,
        numChannels=1,
        numSamples=1,
        numExecReps=1,
        delaySize=None,
    ):
        """Initialize the AUDIOLIB_delayNChannel class with parameters

        Args:
            dType (str): Data type ('float', 'double')
            mode (int): Mode of operation (0 or 1)
            interleave (int): Interleave flag (0 or 1)
            numChannels (int): Number of channels
            numSamples (int): Number of samples per channel
            numExecReps (int): Number of execution repetitions
            delaySize (numpy.ndarray): Delay size for each channel. Its maximum value
                                       determines the delay buffer's allocation size.
        """
        self.__dType = dType
        self.__mode = mode
        self.__interleave = interleave
        self.__numChannels = numChannels
        self.__numSamples = numSamples
        self.__numExecReps = numExecReps

        # Define min/max values for random generation
        if dType == "int8_t" or dType == "uint8_t":
            bits = 8
        elif dType == "int16_t" or dType == "uint16_t":
            bits = 16
        elif dType == "int32_t" or dType == "uint32_t":
            bits = 32
        else:
            bits = 32

        # unsigned integer
        if dType and dType[0] == "u":
            self.__minVal = 0
            self.__maxVal = (2**bits) - 1
        else:
            self.__minVal = -(2 ** (bits - 1))
            self.__maxVal = 2 ** (bits - 1)

        self.__delaySize = delaySize
        self.__maxDelay = int(np.max(self.__delaySize))

    def exec(self, inputBuff, delayBuff=None, persistState=False):
        """Execute the delayNChannel algorithm

        Args:
            inputBuff (numpy.ndarray): Input buffer. If interleaved, shape should be (numExecReps * numSamples, numChannels).
                                      If not interleaved, shape should be (numChannels, numExecReps * numSamples).
            delayBuff (numpy.ndarray, optional): Delay buffer. If None, a new buffer will be created.
            persistState (bool, optional): Whether to persist internal state (e.g., read/write indices) across calls. Defaults to False.

        Returns:
            tuple: (output_data, delay_buffer, delay_buffer_size) - The output data, updated delay buffer, and delay buffer size
        """
        # Create delay buffer based on mode if not provided
        if delayBuff is None:
            if self.__mode == 0:
                delayBuffSize = self.__numSamples + self.__maxDelay
                delayBuff = np.zeros(
                    (self.__numChannels, delayBuffSize), dtype=inputBuff.dtype
                )
            elif self.__mode == 1:
                if (self.__numSamples + self.__maxDelay) < 512:
                    delayBuffSize = 512
                else:
                    delayBuffSize = 2 ** int(
                        np.ceil(np.log2(self.__numSamples + self.__maxDelay))
                    )
                delayBuff = np.zeros(
                    (self.__numChannels, delayBuffSize), dtype=inputBuff.dtype
                )
        else:
            if self.__mode == 0:
                delayBuffSize = self.__numSamples + self.__maxDelay
            elif self.__mode == 1:
                if (self.__numSamples + self.__maxDelay) < 512:
                    delayBuffSize = 512
                else:
                    delayBuffSize = 2 ** int(
                        np.ceil(np.log2(self.__numSamples + self.__maxDelay))
                    )

        # Create output buffer
        outBuff = np.zeros(
            (self.__numChannels, self.__numExecReps * self.__numSamples),
            dtype=inputBuff.dtype,
        )

        # Transpose input buffer if interleaved
        if self.__interleave == 1:
            inputBuff = inputBuff.T

        # Process based on mode
        if self.__mode == 0:
            for numExecRep in range(self.__numExecReps):
                for ch in range(self.__numChannels):
                    if self.__delaySize[ch] < self.__numSamples:
                        for i in range(self.__delaySize[ch]):
                            outBuff[ch, numExecRep * self.__numSamples + i] = delayBuff[
                                ch, i
                            ]
                        for i in range(self.__numSamples - self.__delaySize[ch]):
                            outBuff[
                                ch,
                                numExecRep * self.__numSamples
                                + self.__delaySize[ch]
                                + i,
                            ] = inputBuff[ch, numExecRep * self.__numSamples + i]
                        if self.__delaySize[ch] > 0:
                            for i in range(self.__delaySize[ch]):
                                delayBuff[ch, i] = inputBuff[
                                    ch,
                                    numExecRep * self.__numSamples
                                    + (self.__numSamples - self.__delaySize[ch])
                                    + i,
                                ]
                    else:
                        for i in range(self.__numSamples):
                            outBuff[ch, numExecRep * self.__numSamples + i] = delayBuff[
                                ch, i
                            ]
                        for i in range(self.__delaySize[ch] - self.__numSamples):
                            delayBuff[ch, i] = delayBuff[ch, self.__numSamples + i]
                        for i in range(self.__numSamples):
                            delayBuff[
                                ch, (self.__delaySize[ch] - self.__numSamples) + i
                            ] = inputBuff[ch, numExecRep * self.__numSamples + i]
        elif self.__mode == 1:
            if persistState:
                if not hasattr(self, "_AUDIOLIB_delayNChannel__readIdx"):
                    self.__readIdx = [0] * self.__numChannels
                    self.__writeIdx = list(self.__delaySize)
                readIdx = self.__readIdx
                writeIdx = self.__writeIdx
            else:
                if hasattr(self, "_AUDIOLIB_delayNChannel__readIdx"):
                    del self.__readIdx
                    del self.__writeIdx
                readIdx = [0] * self.__numChannels
                writeIdx = [0] * self.__numChannels
                for ch in range(self.__numChannels):
                    writeIdx[ch] = self.__delaySize[ch]

            for numExecRep in range(self.__numExecReps):
                for ch in range(self.__numChannels):
                    # Write Delay Buffer
                    writeSampleCnt1 = min(
                        (delayBuffSize - writeIdx[ch]), self.__numSamples
                    )
                    writeSampleCnt2 = self.__numSamples - writeSampleCnt1
                    for i in range(writeSampleCnt1):
                        delayBuff[ch, writeIdx[ch] + i] = inputBuff[
                            ch, numExecRep * self.__numSamples + i
                        ]
                    if (writeIdx[ch] + self.__numSamples) >= delayBuffSize:
                        writeIdx[ch] = 0
                    else:
                        writeIdx[ch] = writeIdx[ch] + self.__numSamples
                    if writeSampleCnt2 > 0:
                        for i in range(writeSampleCnt2):
                            delayBuff[ch, i] = inputBuff[
                                ch,
                                (numExecRep * self.__numSamples)
                                + (writeSampleCnt1 - writeIdx[ch])
                                + i,
                            ]
                        writeIdx[ch] = writeSampleCnt2

                    # Read Delay Buffer
                    readSampleCnt1 = min(
                        (delayBuffSize - readIdx[ch]), self.__numSamples
                    )
                    readSampleCnt2 = self.__numSamples - readSampleCnt1
                    for i in range(readSampleCnt1):
                        outBuff[ch, numExecRep * self.__numSamples + i] = delayBuff[
                            ch, readIdx[ch] + i
                        ]
                    if (readIdx[ch] + self.__numSamples) >= delayBuffSize:
                        readIdx[ch] = 0
                    else:
                        readIdx[ch] = readIdx[ch] + self.__numSamples

                    if readSampleCnt2 > 0:
                        for i in range(readSampleCnt2):
                            outBuff[
                                ch,
                                (numExecRep * self.__numSamples)
                                + (readSampleCnt1 - readIdx[ch])
                                + i,
                            ] = delayBuff[ch, i]
                        readIdx[ch] = readSampleCnt2

        # Transpose output buffer if interleaved
        if self.__interleave == 1:
            outBuff = outBuff.T

        return outBuff, delayBuff, delayBuffSize
