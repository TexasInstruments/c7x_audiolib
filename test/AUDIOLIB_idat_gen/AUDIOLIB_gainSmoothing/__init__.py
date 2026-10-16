"""
AUDIOLIB_gainSmoothing kernel module initialization.
This module exposes the class API for the AUDIOLIB_gainSmoothing kernel.
"""

# Import the class API
from .gainSmoothing import AUDIOLIB_gainSmoothing

# Define what should be imported when using "from AUDIOLIB_gainSmoothing import *"
__all__ = ['AUDIOLIB_gainSmoothing']
