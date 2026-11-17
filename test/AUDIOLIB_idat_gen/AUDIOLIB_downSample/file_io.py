from __future__ import print_function
from subprocess import call
from sys import platform

import math
import sys
import numpy as np
import csv


def write_attributes(f, attributes, stringToWrite):
    f.write("#ifdef WIN32\n")
    f.write(stringToWrite)
    f.write("#else\n")
    f.write(attributes + stringToWrite)
    f.write("#endif\n")
    f.write("{\n")


def write_header_file(in0, out, testCase, headerFileName):
    testId = testCase["ID"]
    inChannels = testCase["inChannels"]
    inSamples = testCase["inSamples"]
    dType = testCase["dType"]
    isInterleave = testCase["isInterleave"]
    downSampleRatio = testCase["downSampleRatio"]
    phase = testCase["phase"]
    totalSize = inChannels * inSamples

    attributesIn0 = '__attribute__((section(".staticData"))) static '
    attributesOut = '__attribute__((section(".staticData"))) static '

    outputDir = "../../AUDIOLIB_downSample/test_data/"

    with open(outputDir + headerFileName, "w") as f:
        write_attributes(
            f,
            attributesIn0,
            dType + " " + "staticRefIn0Case" + str(testId) + "[]=\n",
        )

        if isInterleave == 1:
            for dimY in range(0, inSamples):
                for dimX in range(0, inChannels):
                    print("{:>2}".format(in0[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)
        else:
            for dimY in range(0, inChannels):
                for dimX in range(0, inSamples):
                    print("{:>2}".format(in0[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)

        f.write("\n};")
        f.write("\n\n")

    with open(outputDir + headerFileName, "a") as f:
        write_attributes(
            f,
            attributesOut,
            dType + " " + "staticRefOutCase" + str(testId) + "[]=\n",
        )
        outCount = math.ceil((inSamples - phase) / downSampleRatio);
        print(f"inSample count - {inSamples} , downSampleRatio {downSampleRatio} Output Write Count: {outCount} | Shape: {out.shape}")

        if isInterleave==1:
            for dimY in range(0, outCount):
                for dimX in range(0, inChannels):
                    print("{:>2}".format(out[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)
        else:
            for dimY in range(0, inChannels):
                for dimX in range(0, outCount):
                    print("{:>2}".format(out[dimY, dimX]), file=f, end=", ")
                print(" ", file=f)

        f.write("\n};")
        f.write("\n\n")

    # if platform == "linux" or platform == "linux2":
        # call(
        #     [
        #         "indent",
        #         "-nut",
        #         "-i3",
        #         # "-c55",
        #         "-l100",
        #         outputDir + headerFileName,
        #     ]
        # )
        # call(["rm", outputDir + headerFileName + "~"])  # remove backup file
