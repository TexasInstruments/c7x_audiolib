# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_router kernel module initialization.
This module exposes the class API for the AUDIOLIB_router kernel.
"""

# Import the class API
from .router import AUDIOLIB_router

# Define what should be imported when using "from AUDIOLIB_router import *"
__all__ = ["AUDIOLIB_router"]
