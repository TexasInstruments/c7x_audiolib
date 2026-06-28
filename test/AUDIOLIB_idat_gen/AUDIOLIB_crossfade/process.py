# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0


import numpy as np


def fader(in0, in1, fadeIn, fadeOut):
    out = in0 * fadeOut + in1 * fadeIn
    return out
