# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_delay kernel module initialization.
This module exposes the class API for the AUDIOLIB_delay kernel.
"""

# Import the class API
from .delay import AUDIOLIB_delay

# Define what should be imported when using "from AUDIOLIB_delay import *"
__all__ = ["AUDIOLIB_delay"]
