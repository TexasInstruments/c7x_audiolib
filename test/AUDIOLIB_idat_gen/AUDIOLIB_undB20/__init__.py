# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_undB20 kernel module initialization.
This module exposes the class API for the AUDIOLIB_undB20 kernel.
"""

# Import the class API
from .undB20 import AUDIOLIB_undB20

# Define what should be imported when using "from AUDIOLIB_undB20 import *"
__all__ = ["AUDIOLIB_undB20"]
