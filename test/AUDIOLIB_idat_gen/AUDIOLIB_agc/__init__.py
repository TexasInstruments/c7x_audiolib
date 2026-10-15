# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_agc kernel module initialization.
This module exposes the class API for the AUDIOLIB_agc kernel.
"""

# Import the class API
from .agc import AUDIOLIB_agc

# Define what should be imported when using "from AUDIOLIB_agc import *"
__all__ = ["AUDIOLIB_agc"]
