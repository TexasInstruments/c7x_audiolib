# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_floor kernel module initialization.
This module exposes the class API for the AUDIOLIB_floor kernel.
"""

# Import the class API
from .floor import AUDIOLIB_floor

# Define what should be imported when using "from AUDIOLIB_floor import *"
__all__ = ["AUDIOLIB_floor"]
