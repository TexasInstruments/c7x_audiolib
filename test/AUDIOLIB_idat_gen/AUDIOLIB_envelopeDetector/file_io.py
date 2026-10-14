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


def write_header_file(in0, out, derivedParams, testCase, headerFileName):
    """Write static input and reference output for all frames of a test case.

    in0 has shape (numFrames, inChannels, inSamples); out has shape
    (numFrames, inChannels, inSamples).
    """
    testId = testCase["ID"]
    numFrames = testCase["numFrames"]
    inChannels = testCase["inChannels"]
    inSamples = testCase["inSamples"]
    dType = testCase["dType"]

    attributes = '__attribute__((section(".staticData"))) static '

    outputDir = "../../AUDIOLIB_envelopeDetector/test_data/"
    os.makedirs(outputDir, exist_ok=True)

    with open(outputDir + headerFileName, "w") as f:
        write_attributes(f, attributes, dType + " " + "staticRefIn0Case" + str(testId) + "[]=\n")
        for frame in range(0, numFrames):
            for dimY in range(0, inChannels):
                for dimX in range(0, inSamples):
                    print("{:>2}".format(in0[frame, dimY, dimX]), file=f, end=", ")
                print(" ", file=f)
        f.write("\n};")
        f.write("\n\n")

        write_attributes(f, attributes, dType + " " + "staticRefOutCase" + str(testId) + "[]=\n")
        for frame in range(0, numFrames):
            for dimY in range(0, inChannels):
                for dimX in range(0, inSamples):
                    print("{:>2}".format(out[frame, dimY, dimX]), file=f, end=", ")
                print(" ", file=f)
        f.write("\n};")
        f.write("\n\n")

    if platform == "linux" or platform == "linux2":
        call(["indent", "-nut", "-i3", "-l100", outputDir + headerFileName])
        call(["rm", "-f", outputDir + headerFileName + "~"])  # remove backup file
