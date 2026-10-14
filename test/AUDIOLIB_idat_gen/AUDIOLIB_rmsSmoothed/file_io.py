# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function
from subprocess import call
from sys import platform

import os


def write_attributes(f, attributes, stringToWrite):
    f.write("#ifdef WIN32\n")
    f.write(stringToWrite)
    f.write("#else\n")
    f.write(attributes + stringToWrite)
    f.write("#endif\n")
    f.write("{\n")


def write_array(f, values, formatSpec):
    for i, val in enumerate(values):
        f.write(formatSpec % val)
        f.write(", " if i < len(values) - 1 else "")
        f.write("\n" if (i + 1) % 8 == 0 else "")
    f.write("\n};\n\n")


def write_header_file(in0, out, testCase, headerFileName):
    """Write the input and reference output of one test case to a C header.

    Args:
        in0 (ndarray): Input samples, [frames, numChannels, blockSize], planar per frame.
        out (ndarray): Reference output, one smoothed RMS value per frame.
        testCase (dict): Test case parameters.
        headerFileName (str): Name of the header file, e.g. 'staticRefCase1.h'.
    """
    testId = testCase["ID"]
    dType = testCase["dType"]

    attributes = '__attribute__((section(".staticData"))) static '
    # Use higher precision for double to avoid truncation
    formatSpec = "%.17g" if dType == "double" else "%.9f"
    outputDir = "../../AUDIOLIB_rmsSmoothed/test_data/"
    os.makedirs(outputDir, exist_ok=True)

    with open(outputDir + headerFileName, "w") as f:
        write_attributes(f, attributes, dType + " staticRefIn0Case" + str(testId) + "[]=\n")
        write_array(f, in0.reshape(-1), formatSpec)

        write_attributes(f, attributes, dType + " staticRefOutCase" + str(testId) + "[]=\n")
        write_array(f, out.reshape(-1), formatSpec)

    if platform == "linux" or platform == "linux2":
        call(["indent", "-nut", "-i3", "-l100", outputDir + headerFileName])
        call(["rm", outputDir + headerFileName + "~"])  # remove backup file
