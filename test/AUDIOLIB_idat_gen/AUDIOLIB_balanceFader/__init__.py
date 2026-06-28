# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_balanceFader kernel module initialization.
This module exposes the class API for the AUDIOLIB_balanceFader kernel.
"""

# Import the class API
from .balanceFader import AUDIOLIB_balanceFader

# Define what should be imported when using "from AUDIOLIB_balanceFader import *"
__all__ = ["AUDIOLIB_balanceFader"]
