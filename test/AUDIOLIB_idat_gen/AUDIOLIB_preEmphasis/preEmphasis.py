# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

import numpy as np

class AUDIOLIB_preEmphasis:
    def __init__(self, dType=None, numChannels=1, numSamples=1 , alpha = 0.97, #timeConstant = 1 , 
                 numExecReps = 1, samplingRate = 48000 , isInterleaved = None):
        """Initialize the AUDIOLIB_preEmphasis class with parameters

        Args:
            dType (str): Data type ('int', 'float', 'double')
            numChannels (int): Number of input channels
            numSamples (int): Number of input samples per channel
        """
        self.__dType = dType
        self.__numChannels = numChannels
        self.__numSamples = numSamples
        self.__numExecReps = numExecReps
        self.__samplingRate = samplingRate
        self.__isInterleaved = isInterleaved

        self.__alpha = alpha

    def exec(self, in0):
        """
        Apply pre-emphasis to audio data.
        
        The pre-emphasis equation is:
        y[n] = x[n] - alpha * x[n - 1]
        
        The first sample of each frame uses the final sample from the
        preceding frame or preceding exec() call.
        """
        arr = np.asarray(in0)
        
        if self.__isInterleaved:
            expected_shape = (
                self.__numExecReps,
                self.__numSamples,
                self.__numChannels,
            )
            shape_description = "(num_frames, num_samples, num_channels)"
        else:
            expected_shape = (
                self.__numExecReps,
                self.__numChannels,
                self.__numSamples,
            )
            shape_description = "(num_frames, num_channels, num_samples)"
        
        if arr.shape != expected_shape:
            raise ValueError(
                f"Shape mismatch: received {arr.shape}, "
                f"expected {expected_shape} {shape_description}."
            )
        
        if self.__numSamples < 1:
            raise ValueError(
                "Input must contain at least one sample per frame."
            )
        
        # Use a floating-point output type so fractional results are not
        # truncated when the input has an integer dtype.
        output_dtype = np.result_type(
            arr.dtype,
            np.asarray(self.__alpha).dtype,
            np.float32,
        )
        
        arr = arr.astype(output_dtype, copy=False)
        out = np.empty(arr.shape, dtype=output_dtype)
        
        # Preserve previous samples between separate exec() calls.
        try:
            x_prev = np.asarray(
                self.__x_prev,
                dtype=output_dtype,
            ).copy()
            
            if x_prev.shape != (self.__numChannels,):
                x_prev = np.zeros(
                    self.__numChannels,
                    dtype=output_dtype,
                )
        except AttributeError:
            x_prev = np.zeros(
                self.__numChannels,
                dtype=output_dtype,
            )
        
        for frame_index in range(self.__numExecReps):
            frame = arr[frame_index]
            frame_out = np.empty_like(frame)
            
            if self.__isInterleaved:
                # frame shape: (num_samples, num_channels)
                for channel_index in range(self.__numChannels):
                    x = frame[:, channel_index]
                    y = np.empty_like(x)
                    
                    y[0] = (
                        x[0]
                        - self.__alpha * x_prev[channel_index]
                    )
                    
                    y[1:] = (
                        x[1:]
                        - self.__alpha * x[:-1]
                    )
                    
                    x_prev[channel_index] = x[-1]
                    frame_out[:, channel_index] = y
            else:
                # frame shape: (num_channels, num_samples)
                for channel_index in range(self.__numChannels):
                    x = frame[channel_index]
                    y = np.empty_like(x)
                    
                    y[0] = (
                        x[0]
                        - self.__alpha * x_prev[channel_index]
                    )
                    
                    y[1:] = (
                        x[1:]
                        - self.__alpha * x[:-1]
                    )
                    
                    x_prev[channel_index] = x[-1]
                    frame_out[channel_index] = y
            out[frame_index] = frame_out
        
        self.__x_prev = x_prev
        return out