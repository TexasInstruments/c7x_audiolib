# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_ceil kernel module initialization.
This module exposes the class API for the AUDIOLIB_ceil kernel.
"""

# Import the class API
from .ceil import AUDIOLIB_ceil

# Define what should be imported when using "from AUDIOLIB_ceil import *"
__all__ = ["AUDIOLIB_ceil"]
