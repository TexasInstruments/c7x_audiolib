# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_subBlockStatistics kernel module initialization.
This module exposes the class API for the AUDIOLIB_subBlockStatistics kernel.
"""

# Import the class API
from .subBlockStatistics import AUDIOLIB_subBlockStatistics

# Define what should be imported when using "from AUDIOLIB_subBlockStatistics import *"
__all__ = ["AUDIOLIB_subBlockStatistics"]
