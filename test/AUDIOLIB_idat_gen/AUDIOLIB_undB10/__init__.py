# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_undB10 kernel module initialization.
This module exposes the class API for the AUDIOLIB_undB10 kernel.
"""

# Import the class API
from .undB10 import AUDIOLIB_undB10

# Define what should be imported when using "from AUDIOLIB_undB10 import *"
__all__ = ["AUDIOLIB_undB10"]
