# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np


class AUDIOLIB_typeConversion:
    def __init__(
        self, inDtype=None, outDtype=None, testQ31=0, inChannels=1, inSamples=1
    ):
        """Initialize the TypeConversion class with conversion parameters

        Args:
            inDtype (str): Input data type ('int16_t', 'int32_t', 'float', 'double')
            outDtype (str): Output data type ('int16_t', 'int32_t', 'float', 'double')
            testQ31 (int): Flag for Q31 format (0 for Q23, 1 for Q31)
            inChannels (int): Number of input channels
            inSamples (int): Number of input samples per channel
        """
        self.__inDtype = inDtype
        self.__outDtype = outDtype
        self.__testQ31 = testQ31
        self.__inChannels = inChannels
        self.__inSamples = inSamples

        # Define scaling factors based on data types
        self.__int16_scale = 32768.0
        self.__int32_q23_scale = 8388608.0
        self.__int32_q31_scale = 2147483648.0

        # Define min/max values for saturation
        self.__int16_min = -32768
        self.__int16_max = 32767
        self.__int32_q23_min = -8388608
        self.__int32_q23_max = 8388607
        self.__int32_q31_min = -2147483648
        self.__int32_q31_max = 2147483647

    def exec(self, input_data):
        """Execute the type conversion algorithm

        Args:
            input_data (numpy.ndarray): Input data to convert, shape (inChannels, inSamples).

        Returns:
            numpy.ndarray: Converted output data.
        """
        # Perform the conversion based on input and output types
        if self.__inDtype == "int16_t" and self.__outDtype == "float":
            output_data = input_data.astype(np.float32) / self.__int16_scale

        elif self.__inDtype == "int16_t" and self.__outDtype == "double":
            output_data = input_data.astype(np.float64) / self.__int16_scale

        elif self.__inDtype == "float" and self.__outDtype == "int16_t":
            output_data = self.__float_to_int(
                input_data, self.__int16_scale, self.__int16_min, self.__int16_max
            )

        elif self.__inDtype == "double" and self.__outDtype == "int16_t":
            output_data = self.__float_to_int(
                input_data, self.__int16_scale, self.__int16_min, self.__int16_max
            )

        elif self.__inDtype == "int32_t" and self.__outDtype == "float":
            if self.__testQ31 == 0:
                output_data = input_data.astype(np.float32) / self.__int32_q23_scale
            else:
                output_data = input_data.astype(np.float32) / self.__int32_q31_scale

        elif self.__inDtype == "int32_t" and self.__outDtype == "double":
            if self.__testQ31 == 0:
                output_data = input_data.astype(np.float64) / self.__int32_q23_scale
            else:
                output_data = input_data.astype(np.float64) / self.__int32_q31_scale

        elif self.__inDtype == "float" and self.__outDtype == "int32_t":
            if self.__testQ31 == 0:
                output_data = self.__float_to_int(
                    input_data,
                    self.__int32_q23_scale,
                    self.__int32_q23_min,
                    self.__int32_q23_max,
                )
            else:
                output_data = self.__float_to_int(
                    input_data,
                    self.__int32_q31_scale,
                    self.__int32_q31_min,
                    self.__int32_q31_max,
                )

        elif self.__inDtype == "double" and self.__outDtype == "int32_t":
            if self.__testQ31 == 0:
                output_data = self.__float_to_int(
                    input_data,
                    self.__int32_q23_scale,
                    self.__int32_q23_min,
                    self.__int32_q23_max,
                )
            else:
                output_data = self.__float_to_int(
                    input_data,
                    self.__int32_q31_scale,
                    self.__int32_q31_min,
                    self.__int32_q31_max,
                )
        else:
            raise ValueError(
                f"Unsupported conversion: {self.__inDtype} to {self.__outDtype}"
            )

        return output_data

    def __float_to_int(self, input_data, scale_factor, min_val, max_val):
        """Helper method to convert floating point to integer with proper rounding

        Args:
            input_data (numpy.ndarray): Input floating point data
            scale_factor (float): Scale factor for conversion
            min_val (int): Minimum value for saturation
            max_val (int): Maximum value for saturation

        Returns:
            numpy.ndarray: Converted integer data
        """
        output_data = np.zeros((self.__inChannels, self.__inSamples), dtype=np.int32)

        for i in range(self.__inChannels):
            for j in range(self.__inSamples):
                val = input_data[i][j]
                mulVal = val * scale_factor
                intPart = int(mulVal)
                fracPart = mulVal - intPart

                # Apply rounding rules
                if (intPart % 2 == 0) and (abs(fracPart) == 0.5):
                    # Round to even for exact half values
                    aInt = intPart
                elif val < 0:
                    # Round down for negative values
                    aInt = int(mulVal - 0.5)
                else:
                    # Round up for positive values
                    aInt = int(mulVal + 0.5)

                # Saturate to valid range
                if aInt > max_val:
                    aInt = max_val
                if aInt < min_val:
                    aInt = min_val

                output_data[i][j] = aInt

        return output_data
