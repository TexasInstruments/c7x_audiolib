# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_muteNCh kernel module initialization.
This module exposes the class API for the AUDIOLIB_muteNCh kernel.
"""

# Import the class API
from .muteNCh import AUDIOLIB_muteNCh

# Define what should be imported when using "from AUDIOLIB_muteNCh import *"
__all__ = ["AUDIOLIB_muteNCh"]
