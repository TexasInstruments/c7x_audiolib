"""
AUDIOLIB_downSample kernel module initialization.
This module exposes the class API for the AUDIOLIB_downSample kernel.
"""

# Import the class API
from .downSample import AUDIOLIB_downSample

# Define what should be imported when using "from AUDIOLIB_downSample import *"
__all__ = ['AUDIOLIB_downSample']
