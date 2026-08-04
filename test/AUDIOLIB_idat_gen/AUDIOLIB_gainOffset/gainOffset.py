import numpy as np

class AUDIOLIB_gainOffset:
    def __init__(self, dType=None, inChannels=1, inSamples=1 , gain = 1.0, offset = 0.0):
        """Initialize the AUDIOLIB_gainOffset class with parameters
        
        Args:
            dType (str): Data type ('int', 'float', 'double')
            inChannels (int): Number of input channels
            inSamples (int): Number of input samples per channel
        """
        self.__dType = dType
        self.__inChannels = inChannels
        self.__inSamples = inSamples
        self.__gain = gain
        self.__offset = offset 
        
        # Define min/max values for random generation
        self.__minVal = -10
        self.__maxVal = 10

    def exec(self, in0):
        """Execute the gainOffset algorithm
        
        Args:
            in0 (numpy.ndarray): Input audio data of shape (inChannels, inSamples).
            in1 (float or int): gainOffset factor to apply.
            
        Returns:
            numpy.ndarray: Output data with gainOffset applied.
        """
        # Apply gainOffset operation
        
        out = in0 * np.float32(self.__gain) + np.float32(self.__offset)
       
        return out
