# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np

class AUDIOLIB_round:
    def __init__(self, inDtype=None, inChannels=1, inSamples=1):
        """Initialize the AUDIOLIB_round class with rounding parameters

        Args:
            inDtype (str): Input data type ('float', 'double')
            inChannels (int): Number of input channels
            inSamples (int): Number of input samples per channel
        """
        self.__inDtype = inDtype
        self.__inChannels = inChannels
        self.__inSamples = inSamples
        

    def exec(self, input_data):
        """Execute the rounding algorithm

        Args:
            input_data (numpy.ndarray): Input data to round, shape (inChannels, inSamples).

        Returns:
            numpy.ndarray: Rounded output data.
        """
        if self.__inDtype == "float":
            outData = np.round(input_data).astype("float")
        elif self.__inDtype == "double":
            outData = np.round(input_data).astype("double")
        else:
            raise ValueError(f"Unsupported data type: {self.__inDtype}")

        return outData
    
   
