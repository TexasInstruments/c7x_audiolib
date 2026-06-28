# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_gainNCh kernel module initialization.
This module exposes the class API for the AUDIOLIB_gainNCh kernel.
"""

# Import the class API
from .gainNCh import AUDIOLIB_gainNCh

# Define what should be imported when using "from AUDIOLIB_gainNCh import *"
__all__ = ["AUDIOLIB_gainNCh"]
