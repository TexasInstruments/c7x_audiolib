# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_round kernel module initialization.
This module exposes the class API for the AUDIOLIB_round kernel.
"""

# Import the class API
from .round import AUDIOLIB_round

# Define what should be imported when using "from AUDIOLIB_round import *"
__all__ = ['AUDIOLIB_round']
