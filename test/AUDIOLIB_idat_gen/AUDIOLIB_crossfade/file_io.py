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
from scipy.io import wavfile
import matplotlib.pyplot as plt


def write_attributes(f, attributes, stringToWrite):
    f.write("#ifdef WIN32\n")
    f.write(stringToWrite)
    f.write("#else\n")
    f.write(attributes + stringToWrite)
    f.write("#endif\n")
    f.write("{\n")


def write_header_file(in0, in1, in2, in3, out, testCase, headerFileName):

    testId = testCase["ID"]
    numSamples = testCase["numSamples"]
    numChannels = testCase["numChannels"]
    isInterleave = testCase["isInterleave"]
    dType = testCase["dType"]

    attributes = '__attribute__((section(".staticData"))) static '
    outputDir = "../../AUDIOLIB_crossfade/test_data/"
    os.makedirs(outputDir, exist_ok=True)
    attributesIn0 = '__attribute__((section(".staticData"))) static '
    attributesIn1 = '__attribute__((section(".staticData"))) static '
    attributesIn2 = '__attribute__((section(".staticData"))) static '
    attributesIn3 = '__attribute__((section(".staticData"))) static '
    attributesout = '__attribute__((section(".staticData"))) static '

    if isInterleave:
        with open(outputDir + headerFileName, "w") as f:
            write_attributes(
                f,
                attributesIn0,
                dType + " " + f"staticRefIn0Case" + str(testId) + "[]=\n",
            )

            for dimY in range(0, numSamples):
                for dimX in range(0, numChannels):
                    print("{:>2}".format(in0[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)

            f.write("\n};")

            f.write("\n\n")
            f.close()

        with open(outputDir + headerFileName, "a") as f:
            write_attributes(
                f,
                attributesIn1,
                dType + " " + f"staticRefIn1Case" + str(testId) + "[]=\n",
            )

            for dimY in range(0, numSamples):
                for dimX in range(0, numChannels):
                    print("{:>2}".format(in1[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)

            f.write("\n};")

            f.write("\n\n")
            f.close()
        with open(outputDir + headerFileName, "a") as f:
            write_attributes(
                f,
                attributesout,
                dType + " " + f"staticRefOutCase" + str(testId) + "[]=\n",
            )

            for dimY in range(0, numSamples):
                for dimX in range(0, numChannels):
                    print("{:>2}".format(out[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)

            f.write("\n};")

            f.write("\n\n")
            f.close()

    else:
        with open(outputDir + headerFileName, "w") as f:
            write_attributes(
                f,
                attributesIn0,
                dType + " " + f"staticRefIn0Case" + str(testId) + "[]=\n",
            )

            for dimY in range(0, numChannels):
                for dimX in range(0, numSamples):
                    print("{:>2}".format(in0[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)

            f.write("\n};")

            f.write("\n\n")
            f.close()

        with open(outputDir + headerFileName, "a") as f:
            write_attributes(
                f,
                attributesIn1,
                dType + " " + f"staticRefIn1Case" + str(testId) + "[]=\n",
            )

            for dimY in range(0, numChannels):
                for dimX in range(0, numSamples):
                    print("{:>2}".format(in1[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)

            f.write("\n};")

            f.write("\n\n")
            f.close()
        with open(outputDir + headerFileName, "a") as f:
            write_attributes(
                f,
                attributesout,
                dType + " " + f"staticRefOutCase" + str(testId) + "[]=\n",
            )

            for dimY in range(0, numChannels):
                for dimX in range(0, numSamples):
                    print("{:>2}".format(out[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)

            f.write("\n};")

            f.write("\n\n")
            f.close()

    with open(outputDir + headerFileName, "a") as f:
        write_attributes(
            f,
            attributesIn2,
            dType + " " + f"staticRefIn2Case" + str(testId) + "[]=\n",
        )

        for dimX in range(0, numSamples):
            print("{:>2}".format(in2[dimX]), file=f, end=", ")
        print(" ", file=f)

        f.write("\n};")

        f.write("\n\n")
        f.close()

    with open(outputDir + headerFileName, "a") as f:
        write_attributes(
            f,
            attributesIn3,
            dType + " " + f"staticRefIn3Case" + str(testId) + "[]=\n",
        )

        for dimX in range(0, numSamples):
            print("{:>2}".format(in3[dimX]), file=f, end=", ")
        print(" ", file=f)

        f.write("\n};")

        f.write("\n\n")
        f.close()
