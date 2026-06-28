# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_dB20 kernel module initialization.
This module exposes the class API for the AUDIOLIB_dB20 kernel.
"""

# Import the class API
from .dB20 import AUDIOLIB_dB20

# Define what should be imported when using "from AUDIOLIB_dB20 import *"
__all__ = ["AUDIOLIB_dB20"]
