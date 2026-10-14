# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""
AUDIOLIB_envelopeDetector kernel module initialization.
This module exposes the class API for the AUDIOLIB_envelopeDetector kernel.
"""

# Import the class API
from .envelopeDetector import AUDIOLIB_envelopeDetector

# Define what should be imported when using "from AUDIOLIB_envelopeDetector import *"
__all__ = ["AUDIOLIB_envelopeDetector"]
