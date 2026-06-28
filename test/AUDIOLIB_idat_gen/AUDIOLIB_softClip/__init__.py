# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_softClip kernel module initialization.
This module exposes the class API for the AUDIOLIB_softClip kernel.
"""

# Import the class API
from .softClip import AUDIOLIB_softClip

# Define what should be imported when using "from AUDIOLIB_softClip import *"
__all__ = ["AUDIOLIB_softClip"]
