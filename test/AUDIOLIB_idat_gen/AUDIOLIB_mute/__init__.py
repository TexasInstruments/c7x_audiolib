# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_mute kernel module initialization.
This module exposes the class API for the AUDIOLIB_mute kernel.
"""

# Import the class API
from .mute import AUDIOLIB_mute

# Define what should be imported when using "from AUDIOLIB_mute import *"
__all__ = ["AUDIOLIB_mute"]
