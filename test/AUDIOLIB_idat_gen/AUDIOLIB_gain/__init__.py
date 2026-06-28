# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_gain kernel module initialization.
This module exposes the class API for the AUDIOLIB_gain kernel.
"""

# Import the class API
from .gain import AUDIOLIB_gain

# Define what should be imported when using "from AUDIOLIB_gain import *"
__all__ = ["AUDIOLIB_gain"]
