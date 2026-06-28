# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_balance kernel module initialization.
This module exposes the class API for the AUDIOLIB_balance kernel.
"""

# Import the class API
from .balance import AUDIOLIB_balance

# Define what should be imported when using "from AUDIOLIB_balance import *"
__all__ = ["AUDIOLIB_balance"]
