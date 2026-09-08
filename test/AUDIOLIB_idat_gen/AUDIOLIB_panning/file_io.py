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

def write_speaker_angles(testCase, headerFileName):
    testID = testCase["ID"]
    speakerLayout = testCase["speakerLayoutInt"]

    outputDir = "../../AUDIOLIB_panning/test_data/"
    if not os.path.exists(outputDir):
            os.makedirs(outputDir)

    with open(os.path.join(outputDir, headerFileName), "a") as f:
        
        f.write("\n")
        f.write("// Speaker enum values\n")
        f.write(
        f"int32_t channelConfigCase{testID}"
        f"[{len(speakerLayout)}] =\n"
            )

        f.write("{\n")

        for spr in speakerLayout:
            f.write(f"    {spr},\n")

        f.write("};\n")

def write_header_file(in_data, out_data, gains, testCase, headerFileName, frames):
    """Write header file with flattened 1D left and right channel inputs, outputs, and gains, organized by frame.

    Args:
        in_data (list): 1D list of left channel input data [frames * numSamples * numChannels]
        out_left (list): 1D list of left channel output data [frames * numSamples * numChannels]
        out_right (list): 1D list of right channel output data [frames * numSamples * numChannels]
        gain_left (list): 1D list of left channel gain data [numSamples * frames]
        gain_right (list): 1D list of right channel gain data [numSamples * frames]
        testCase (dict): Dictionary with test case parameters
        headerFileName (str): Name of the header file to write
        frames (int): Number of frames
    """
    testId = testCase["ID"]
    numSamples = testCase["numSamples"]
    numChannels = testCase["numChannels"]
    dType = testCase["dType"]
    numFrames = testCase["numExecReps"]
    totalLength = numSamples * numChannels * numFrames

    attributes = '__attribute__((section(".staticData"))) static '
    outputDir = "../../AUDIOLIB_panning/test_data/"

    if not os.path.exists(outputDir):
        os.makedirs(outputDir)
    fmt = "{:.6f}f" if np.asarray(in_data).dtype == np.float32 else "{:.12f}"

    with open(os.path.join(outputDir, headerFileName), "w") as f:
        # Write input
        write_attributes(
            f,
            attributes,
            f"{dType} staticRefInCase{testId}[{numSamples * numFrames}] =\n"
        )       
        f.write( f"    /* {numFrames} frames, {numSamples} samples/frame, Mono input */\n")
        # f.write("{\n")

        for frame in range(numFrames):
            f.write(f"    /* ******** Frame {frame} ******** */\n")
            start = frame * numSamples
            end = start + numSamples
            if testCase["isInterleave"]:
                for sample in in_data[start:end]:
                    f.write(f"    {fmt.format(sample)},\n")
            else:
                line = ", ".join(fmt.format(x) for x in in_data[start:end])
                f.write(f"    {line},\n")

        f.write("};\n\n")


        # Write left channel output
        write_attributes(
        f,
        attributes,
        f"{dType} staticRefOutCase{testId}[{totalLength}] =\n")
        f.write(f"    /* {numChannels} channels, {numSamples} samples per channel per frame */\n")

        f.write( f"    /* {numFrames} frames, {numChannels} channels, {numSamples} samples/channel */\n")

        frameSize = numSamples * numChannels

        for frame in range(numFrames):
            f.write(f"    /* ******** Frame {frame} ******** */\n")
            frameStart = frame * frameSize
            frameData = out_data[frameStart:frameStart + frameSize]
            if testCase["isInterleave"]:
                # [S0_C0, S0_C1, ..., S1_C0 ...]
                for s in range(numSamples):
                    f.write(f"    /* Sample {s} */\n")
                    start = s * numChannels
                    end = start + numChannels
                    line = ", ".join(fmt.format(x) for x in frameData[start:end])
                    f.write(f"    {line},\n")
            else:
                # [C0_S0...][C1_S0...]
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
