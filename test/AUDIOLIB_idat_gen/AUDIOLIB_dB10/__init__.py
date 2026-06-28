# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_dB10 kernel module initialization.
This module exposes the class API for the AUDIOLIB_dB10 kernel.
"""

# Import the class API
from .dB10 import AUDIOLIB_dB10

# Define what should be imported when using "from AUDIOLIB_dB10 import *"
__all__ = ["AUDIOLIB_dB10"]
