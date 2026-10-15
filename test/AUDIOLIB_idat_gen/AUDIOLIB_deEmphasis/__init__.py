# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_deEmphasis kernel module initialization.
This module exposes the class API for the AUDIOLIB_deEmphasis kernel.
"""

# Import the class API
from .deEmphasis import AUDIOLIB_deEmphasis

# Define what should be imported when using "from AUDIOLIB_deEmphasis import *"
__all__ = ["AUDIOLIB_deEmphasis"]
