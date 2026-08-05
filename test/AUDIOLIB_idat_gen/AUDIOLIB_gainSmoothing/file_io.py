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

def write_header_file(in_data, out_data, gain_data, testCase, headerFileName, frames):
    """Write input, output, and gain data to a C header file.

    Args:
        in_data (list): Flattened input data.
        out_data (list): Flattened output data.
        gain_data (list): Flattened gain data.
        testCase (dict): Test case parameters.
        headerFileName (str): Name of the header file to write.
        frames (int): Number of execution frames.
    """
    testId = testCase["ID"]
    inSamples = testCase["inSamples"]
    inChannels = testCase["inChannels"]
    dType = testCase["dType"]
    isInterleaved = testCase["isInterleaved"]
    totalLength = inSamples * inChannels * frames
    gainLength = inSamples * frames
    attributes = '__attribute__((section(".staticData"))) static '
    outputDir = "../../AUDIOLIB_gainSmoothing/test_data/"

    if not os.path.exists(outputDir):
        os.makedirs(outputDir)
        
    with open(os.path.join(outputDir, headerFileName), "w") as f:
        # Define data layout description
        layout_desc = "[ch0_s0, ch1_s0, ch2_s0, ...]" if isInterleaved else "[ch0_s0, ch0_s1, ch0_s2, ..., ch1_s0, ch1_s1, ...]"

        fmt = "{:.6f}f" if np.asarray(in_data).dtype == np.float32 else "{:.12f}"
        # Write input data
        write_attributes(
            f,
            attributes,
            f"{dType} staticRefInCase{testId}[{totalLength}] =\n"
        )
        f.write(f"    /* Frame-based input: {frames} frames, {inChannels} channels, {inSamples} samples per channel per frame, {'interleaved' if isInterleaved else 'non-interleaved'} */\n")
        f.write(f"    /* Data layout: {layout_desc} */\n")

        for frame in range(frames):
            f.write(f"    /* ********Frame {frame} ********* */\n")
            frame_offset = frame * inChannels * inSamples
            
            if isInterleaved:
                # Layout: [S0_C0, S0_C1, ... S0_C(ch-1)], repeated for each sample step
                for s in range(inSamples):
                    start = frame_offset + (s * inChannels)
                    end = start + inChannels
                    line = ", ".join(fmt.format(x) for x in in_data[start:end])
                    f.write(f"    {line},\n")
            else:
                # Layout: [C0_S0, C0_S1, ... C0_S(samp-1)], repeated for each channel
                for c in range(inChannels):
                    start = frame_offset + (c * inSamples)
                    end = start + inSamples
                    line = ", ".join(fmt.format(x) for x in in_data[start:end])
                    f.write(f"    {line},\n")         
            
        f.write("};\n\n")

        # Write output data
        # out_data = out_data.reshape(-1)

        write_attributes(
            f,
            attributes,
            f"{dType} staticRefOutCase{testId}[{totalLength}] =\n"
        )
        f.write(f"    /* Frame-based output: {frames} frames, {inChannels} channels, {inSamples} samples per channel per frame, {'interleaved' if isInterleaved else 'non-interleaved'} */\n")
        f.write(f"    /* Data layout: {layout_desc} */\n")
    
        for frame in range(frames):
            f.write(f"    /* Frame {frame} */\n")
            frame_offset = frame * inChannels * inSamples
            
            if isInterleaved:
                # Layout: [S0_C0, S0_C1, ... S0_C(ch-1)], repeated for each sample step
                for s in range(inSamples):
                    start = frame_offset + (s * inChannels)
                    end = start + inChannels
                    line = ", ".join(fmt.format(x) for x in out_data[start:end])
                    f.write(f"    {line},\n")
            else:
                # Layout: [C0_S0, C0_S1, ... C0_S(samp-1)], repeated for each channel
                for c in range(inChannels):
                    start = frame_offset + (c * inSamples)
                    end = start + inSamples
                    line = ", ".join(fmt.format(x) for x in out_data[start:end])
                    f.write(f"    {line},\n")        
        f.write("};\n\n")

