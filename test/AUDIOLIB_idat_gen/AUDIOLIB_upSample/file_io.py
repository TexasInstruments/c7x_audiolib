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


def write_array(f, attributes, name, data, rows, cols, dType):
    """Write a 2-D numpy array as a flat C array initializer."""
    write_attributes(f, attributes, dType + " " + name + "[]=\n")
    for dimY in range(0, rows):
        for dimX in range(0, cols):
            print("{:>2}".format(data[dimY, dimX]), file=f, end=", ")
        print(" ", file=f)
    f.write("\n};")
    f.write("\n\n")


def write_header_file(in0, out, testCase, headerFileName):
    """Write the input and (zero-inserted) output reference arrays for one case.

    The arrays are written from their actual numpy shapes, so this works for
    both layouts:
      - planar      : in0 (inChannels, inSamples), out (inChannels, upFactor*inSamples)
      - interleaved : in0 (inSamples, inChannels), out (upFactor*inSamples, inChannels)

    Args:
        in0 (numpy.ndarray): input in memory layout
        out (numpy.ndarray): zero-inserted output in memory layout
        testCase (dict): the test-case parameters
        headerFileName (str): output .h file name (e.g. staticRefCase1.h)
    """
    testId = testCase["ID"]
    dType = testCase["dType"]

    attributes = '__attribute__((section(".staticData"))) static '

    outputDir = "../../AUDIOLIB_upSample/test_data/"

    # Create the directory if it doesn't exist
    os.makedirs(outputDir, exist_ok=True)

    with open(outputDir + headerFileName, "w") as f:
        write_array(
            f, attributes, "staticRefInCase" + str(testId), in0, in0.shape[0], in0.shape[1], dType
        )
        write_array(
            f, attributes, "staticRefOutCase" + str(testId), out, out.shape[0], out.shape[1], dType
        )

    # if platform == "linux" or platform == "linux2":
    #     call(["indent", "-nut", "-i3", "-l100", outputDir + headerFileName])
    #     call(["rm", outputDir + headerFileName + "~"])  # remove backup file
