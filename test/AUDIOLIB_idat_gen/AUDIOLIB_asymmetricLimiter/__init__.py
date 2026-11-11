# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_asymmetricLimiter kernel module initialization.
This module exposes the class API for the AUDIOLIB_asymmetricLimiter kernel.
"""

# Import the class API
from .asymmetricLimiter import AUDIOLIB_asymmetricLimiter

# Define what should be imported when using "from AUDIOLIB_asymmetricLimiter import *"
__all__ = ['AUDIOLIB_asymmetricLimiter']
