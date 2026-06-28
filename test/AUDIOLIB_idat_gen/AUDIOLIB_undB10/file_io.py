# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function
from subprocess import call
from sys import platform

import sys
import numpy as np
import csv
import os


def write_attributes(f, attributes, stringToWrite):
    f.write("#ifdef WIN32\n")
    f.write(stringToWrite)
    f.write("#else\n")
    f.write(attributes + stringToWrite)
    f.write("#endif\n")
    f.write("{\n")


def write_header_file(in0, out, testCase, headerFileName):
    testId = testCase["ID"]
    inSamples = testCase["inSamples"]
    inChannels = testCase["inChannels"]
    strideInElements = testCase["strideInElements"]
    strideOutElements = testCase["strideOutElements"]
    dType = testCase["dType"]

    attributesIn0 = '__attribute__((section(".staticData"))) static '
    attributesOut = '__attribute__((section(".staticData"))) static '

    outputDir = "../../AUDIOLIB_undB10/test_data/"

    if not os.path.exists(outputDir):
        os.mkdir(outputDir)

    with open(outputDir + headerFileName, "w") as f:
        write_attributes(
            f,
            attributesIn0,
            dType + " " + "staticRefInCase" + str(testId) + "[]=\n",
        )

        # Check if input is 1D or 2D array
        if isinstance(in0, np.ndarray) and len(in0.shape) == 2:
            # 2D array
            for dimY in range(0, inChannels):
                for dimX in range(0, inSamples):
                    print("{:>2}".format(in0[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)
        else:
            # 1D array - reshape to match expected format
            for dimY in range(0, inChannels):
                for dimX in range(0, inSamples):
                    index = dimY * inSamples + dimX
                    if index < len(in0):
                        print("{:>2}".format(in0[index]), file=f, end=", ")
                    else:
                        print("0", file=f, end=", ")
                print(" ", file=f)

        f.write("\n};")
        f.write("\n\n")
        f.close()

    with open(outputDir + headerFileName, "a") as f:
        write_attributes(
            f,
            attributesOut,
            dType + " " + "staticRefOutCase" + str(testId) + "[]=\n",
        )

        # Check if output is 1D or 2D array
        if isinstance(out, np.ndarray) and len(out.shape) == 2:
            # 2D array
            for dimY in range(0, inChannels):
                for dimX in range(0, inSamples):
                    print("{:>2}".format(out[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)
        else:
            # 1D array - reshape to match expected format
            for dimY in range(0, inChannels):
                for dimX in range(0, inSamples):
                    index = dimY * inSamples + dimX
                    if index < len(out):
                        print("{:>2}".format(out[index]), file=f, end=", ")
                    else:
                        print("0", file=f, end=", ")
                print(" ", file=f)

        f.write("\n};")
        f.write("\n\n")
        f.close()

    if platform == "linux" or platform == "linux2":
        call(
            [
                "indent",
                "-nut",
                "-i3",
                # "-c55",
                "-l100",
                outputDir + headerFileName,
            ]
        )
        call(["rm", outputDir + headerFileName + "~"])  # remove backup file
