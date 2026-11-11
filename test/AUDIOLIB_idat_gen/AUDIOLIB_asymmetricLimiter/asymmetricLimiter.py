# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np

class AUDIOLIB_asymmetricLimiter:
    def __init__(self, dType=None, inChannels=1, inSamples=1, asymmetricLimiterLower=-1, asymmetricLimiterUpper=-1):
        """Initialize the AUDIOLIB_asymmetricLimiter class with parameters
        
        Args:
            dType (str): Data type ('int', 'float', 'double')
            inChannels (int): Number of input channels
            inSamples (int): Number of input samples per channel
        """
        self.__dType = dType
        self.__inChannels = inChannels
        self.__inSamples = inSamples
        self.__asymmetricLimiterLower = asymmetricLimiterLower
        self.__asymmetricLimiterUpper = asymmetricLimiterUpper
        
        # Define min/max values for random generation
        self.__minVal = -10
        self.__maxVal = 10

    def exec(self, in0, asymmetricLimiterLower, asymmetricLimiterUpper):
        """Execute the asymmetricLimiter algorithm
        
        Args:
            in0 (numpy.ndarray): Input audio data of shape (inChannels, inSamples).
            in1 (float or int): Asymmetriclimiter factor to apply.
            
        Returns:
            numpy.ndarray: Output data with asymmetricLimiter applied.
        """
        # Apply asymmetricLimiter operation
        out = np.clip(in0, asymmetricLimiterLower, asymmetricLimiterUpper)
        
        return out
