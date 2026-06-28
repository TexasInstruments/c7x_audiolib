# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_delay:
    def __init__(
        self,
        dType="float",
        mode=0,
        delaySize=0,
        interleave=0,
        numChannels=1,
        numSamples=1,
        numExecReps=1,
    ):
        """Initialize the AUDIOLIB_delay class with private members.

        Args:
            dType (str): Data type, e.g., "float", "double", "int16_t", etc.
            mode (int): Delay mode (0 for linear buffer, 1 for circular buffer)
            delaySize (int): Size of the delay in samples
            interleave (int): Whether data is interleaved (1) or planar (0)
            numChannels (int): Number of audio channels
            numSamples (int): Number of samples per channel per execution
            numExecReps (int): Number of execution repetitions
        """
        self._dType = dType
        self._mode = mode
        self._delaySize = delaySize
        self._interleave = interleave
        self._numChannels = numChannels
        self._numSamples = numSamples
        self._numExecReps = numExecReps

        # Determine data type and range
        if dType == "int8_t" or dType == "uint8_t":
            bits = 8
        elif dType == "int16_t" or dType == "uint16_t":
            bits = 16
        elif dType == "int32_t" or dType == "uint32_t":
            bits = 32
        else:
            bits = 32

        # Set min/max values based on data type
        if dType[0] == "u":  # unsigned integer
            self._minVal = 0
            self._maxVal = (2**bits) - 1
        else:
            self._minVal = -(2 ** (bits - 1))
            self._maxVal = 2 ** (bits - 1) - 1

        # Determine delay buffer size based on mode
        if mode == 0:
            self._delayBuffSize = numSamples + delaySize
        elif mode == 1:
            if (numSamples + delaySize) < 512:
                self._delayBuffSize = 512
            else:
                self._delayBuffSize = 2 ** int(np.ceil(np.log2(numSamples + delaySize)))

    def exec(self, inputBuff):
        """Execute the delay algorithm.

        Args:
            inputBuff (numpy.ndarray): Input buffer. If interleaved, shape should be
                                      (numExecReps * numSamples, numChannels).
                                      If planar, shape should be (numChannels, numExecReps * numSamples).

        Returns:
            tuple: (output_buffer, delay_buffer)
        """
        # Initialize delay buffer
        if self._dType == "float":
            delayBuff = np.zeros(
                (self._numChannels, self._delayBuffSize), dtype=np.float32
            )
        elif self._dType == "double":
            delayBuff = np.zeros(
                (self._numChannels, self._delayBuffSize), dtype=np.float64
            )
        else:
            raise ValueError(f"Unsupported data type: {self._dType}")

        # Initialize output buffer
        outBuff = np.zeros(
            (self._numChannels, self._numExecReps * self._numSamples),
            dtype=inputBuff.dtype,
        )

        # Store original input buffer
        inputBuffTemp = inputBuff.copy()

        # Convert interleaved to planar if needed for processing
        if self._interleave == 1:
            inputBuff = inputBuff.T

        # Process based on mode
        if self._mode == 0:  # Linear buffer
            for numExecRep in range(self._numExecReps):
                if self._delaySize < self._numSamples:
                    for ch in range(self._numChannels):
                        # Copy from delay buffer to output
                        for i in range(self._delaySize):
                            outBuff[ch, numExecRep * self._numSamples + i] = delayBuff[
                                ch, i
                            ]
                        # Copy from input to output (after delay)
                        for i in range(self._numSamples - self._delaySize):
                            outBuff[
                                ch, numExecRep * self._numSamples + self._delaySize + i
                            ] = inputBuff[ch, numExecRep * self._numSamples + i]
                        # Update delay buffer with last samples from input
                        if self._delaySize > 0:
                            for i in range(self._delaySize):
                                if (
                                    numExecRep * self._numSamples
                                    + (self._numSamples - self._delaySize)
                                    + i
                                    < inputBuff.shape[1]
                                ):
                                    delayBuff[ch, i] = inputBuff[
                                        ch,
                                        numExecRep * self._numSamples
                                        + (self._numSamples - self._delaySize)
                                        + i,
                                    ]
                else:
                    for ch in range(self._numChannels):
                        # Copy from delay buffer to output
                        for i in range(self._numSamples):
                            outBuff[ch, numExecRep * self._numSamples + i] = delayBuff[
                                ch, i
                            ]
                        # Shift delay buffer
                        for i in range(
                            min(
                                self._delaySize - self._numSamples,
                                self._delayBuffSize - self._numSamples,
                            )
                        ):
                            if (
                                i < self._delayBuffSize
                                and i + self._numSamples < self._delayBuffSize
                            ):
                                delayBuff[ch, i] = delayBuff[ch, self._numSamples + i]
                        # Update delay buffer with new input
                        offset = min(
                            self._delaySize - self._numSamples,
                            self._delayBuffSize - self._numSamples,
                        )
                        for i in range(
                            min(self._numSamples, self._delayBuffSize - offset)
                        ):
                            if (
                                offset + i < self._delayBuffSize
                                and numExecRep * self._numSamples + i
                                < inputBuff.shape[1]
                            ):
                                delayBuff[ch, offset + i] = inputBuff[
                                    ch, numExecRep * self._numSamples + i
                                ]

        elif self._mode == 1:  # Circular buffer
            readIdx = 0
            writeIdx = self._delaySize

            for numExecRep in range(self._numExecReps):
                # Write to delay buffer
                writeSampleCnt1 = min(
                    (self._delayBuffSize - writeIdx), self._numSamples
                )
                writeSampleCnt2 = self._numSamples - writeSampleCnt1

                for i in range(writeSampleCnt1):
                    if numExecRep * self._numSamples + i < inputBuff.shape[1]:
                        delayBuff[:, writeIdx + i] = inputBuff[
                            :, numExecRep * self._numSamples + i
                        ]

                if (writeIdx + self._numSamples) >= self._delayBuffSize:
                    writeIdx = 0
                else:
                    writeIdx = writeIdx + self._numSamples

                if writeSampleCnt2 > 0:
                    for i in range(writeSampleCnt2):
                        if (
                            numExecRep * self._numSamples
                        ) + writeSampleCnt1 + i < inputBuff.shape[1]:
                            delayBuff[:, i] = inputBuff[
                                :, (numExecRep * self._numSamples) + writeSampleCnt1 + i
                            ]
                    writeIdx = writeSampleCnt2

                # Read from delay buffer
                readSampleCnt1 = min((self._delayBuffSize - readIdx), self._numSamples)
                readSampleCnt2 = self._numSamples - readSampleCnt1

                for i in range(readSampleCnt1):
                    outBuff[:, numExecRep * self._numSamples + i] = delayBuff[
                        :, readIdx + i
                    ]

                if (readIdx + self._numSamples) >= self._delayBuffSize:
                    readIdx = 0
                else:
                    readIdx = readIdx + self._numSamples

                if readSampleCnt2 > 0:
                    for i in range(readSampleCnt2):
                        outBuff[
                            :, (numExecRep * self._numSamples) + readSampleCnt1 + i
                        ] = delayBuff[:, i]
                    readIdx = readSampleCnt2

        # Convert back to interleaved if needed
        if self._interleave == 1:
            outBuff = outBuff.T

        return outBuff, delayBuff
