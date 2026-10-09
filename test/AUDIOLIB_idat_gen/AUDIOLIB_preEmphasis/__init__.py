# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_preEmphasis kernel module initialization.
This module exposes the class API for the AUDIOLIB_preEmphasis kernel.
"""

# Import the class API
from .preEmphasis import AUDIOLIB_preEmphasis

# Define what should be imported when using "from AUDIOLIB_preEmphasis import *"
__all__ = ["AUDIOLIB_preEmphasis"]
