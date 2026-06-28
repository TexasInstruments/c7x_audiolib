# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_gainNChTrim kernel module initialization.
This module exposes the class API for the AUDIOLIB_gainNChTrim kernel.
"""

# Import the class API
from .gainNChTrim import AUDIOLIB_gainNChTrim

# Define what should be imported when using "from AUDIOLIB_gainNChTrim import *"
__all__ = ["AUDIOLIB_gainNChTrim"]
