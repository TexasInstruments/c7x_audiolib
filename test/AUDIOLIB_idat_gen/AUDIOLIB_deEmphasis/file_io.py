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

def write_header_file(in_data, out_data, testCase, headerFileName):
    """Write header file with flattened input and output, organized by frame.

    Args:
        in_data (list): 1D list of input data
        out_data (list): 1D list of output data
        testCase (dict): Dictionary with test case parameters
        headerFileName (str): Name of the header file to write
    """
    # Ensure inputs and outputs are completely flattened 1D lists of scalars
    in_data = np.ravel(in_data).tolist()
    out_data = np.ravel(out_data).tolist()

    testId = testCase["ID"]
    numSamples = testCase["numSamples"]
    numChannels = testCase["numChannels"]
    dType = testCase["dType"]
    numFrames = testCase["numExecReps"]
    totalLength = numSamples * numChannels * numFrames
    frameSize = numSamples * numChannels

    attributes = '__attribute__((section(".staticData"))) static '
    outputDir = "../../AUDIOLIB_deEmphasis/test_data/"

    if not os.path.exists(outputDir):
        os.makedirs(outputDir)
        
    # fmt = "{:.6f}f" if dType.lower() in ["float", "float32"] else "{:.12f}"

    with open(os.path.join(outputDir, headerFileName), "w") as f:
        # Write input
        write_attributes(
            f,
            attributes,
            f"{dType} staticRefInCase{testId}[{totalLength}] =\n"
        )       
        f.write(f"    /* {numFrames} frames, {numSamples} samples/frame, {numChannels} channels */\n")

        for frame in range(numFrames):
            f.write(f"    /* ******** Frame {frame} ******** */\n")
            frameStart = frame * frameSize
            frameData = in_data[frameStart:frameStart + frameSize]
            # print("********************")
            # fmt = "{:.6f}f" if np.asarray(in_data).dtype == np.float32 else "{:.12f}"
            fmt = "{:.8f}f" 

            if testCase["isInterleaved"]:
                for s in range(numSamples):
                    f.write(f"    /* Sample {s} */\n")
                    start = s * numChannels
                    end = start + numChannels
                    line = ", ".join(fmt.format(x) for x in frameData[start:end])
                   
                    f.write(f"    {line},\n")
            else:
                for ch in range(numChannels):
                    f.write(f"    /* Channel {ch} */\n")
                    start = ch * numSamples
                    end = start + numSamples
                    tmp = frameData[start:end]
                    line = ", ".join(fmt.format((x)) for x in tmp)
                    # print(line)
                    f.write(f"    {line},\n")

        f.write("};\n\n")

        # Write output
        write_attributes(
            f,
            attributes,
            f"{dType} staticRefOutCase{testId}[{totalLength}] =\n"
        )
        f.write(f"    /* {numFrames} frames, {numChannels} channels, {numSamples} samples/channel */\n")

        for frame in range(numFrames):
            # fmt = "{:.6f}f" if np.asarray(in_data).dtype == np.float32 else "{:.12f}"    
            fmt = "{:.8f}f" 
        
            f.write(f"    /* ******** Frame {frame} ******** */\n")
            frameStart = frame * frameSize
            frameData = out_data[frameStart:frameStart + frameSize]
            if testCase["isInterleaved"]:
                for s in range(numSamples):
                    f.write(f"    /* Sample {s} */\n")
                    start = s * numChannels
                    end = start + numChannels
                    line = ", ".join(fmt.format(x) for x in frameData[start:end])
                    f.write(f"    {line},\n")
            else:
                for ch in range(numChannels):
                    f.write(f"    /* Channel {ch} */\n")
                    start = ch * numSamples
                    end = start + numSamples
                    line = ", ".join(fmt.format(x) for x in frameData[start:end])

                    f.write(f"    {line},\n")

        f.write("};\n\n")
        

    if platform == "linux" or platform == "linux2":
        call(
            [
                "indent",
                "-nut",
                "-i3",
                "-l100",
                os.path.join(outputDir, headerFileName),
            ]
        )
        call(["rm", os.path.join(outputDir, headerFileName + "~")])
