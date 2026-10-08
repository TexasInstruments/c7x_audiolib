"""
AUDIOLIB_gainNChTrimSmoothing kernel module initialization.
This module exposes the class API for the AUDIOLIB_gainNChTrimSmoothing kernel.
"""

# Import the class API
from .gainNChTrimSmoothing import AUDIOLIB_gainNChTrimSmoothing

# Define what should be imported when using "from AUDIOLIB_gainNChTrimSmoothing import *"
__all__ = ['AUDIOLIB_gainNChTrimSmoothing']
