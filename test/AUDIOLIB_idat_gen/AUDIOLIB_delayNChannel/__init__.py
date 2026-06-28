# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_delayNChannel kernel module initialization.
This module exposes the class API for the AUDIOLIB_delayNChannel kernel.
"""

# Import the class API
from .delayNChannel import AUDIOLIB_delayNChannel

# Define what should be imported when using "from AUDIOLIB_delayNChannel import *"
__all__ = ["AUDIOLIB_delayNChannel"]
