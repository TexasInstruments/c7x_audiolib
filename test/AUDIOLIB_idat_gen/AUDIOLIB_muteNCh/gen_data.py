# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function
import matplotlib.pyplot as plt
import os
import file_io
from subprocess import call
from sys import platform

import sys
import numpy as np
import csv
import re
from muteNCh import AUDIOLIB_muteNCh

sys.path.append("../common")
import AUDIOLIB_utils


def generate_sine(
    num_samples,
    sampling_rate,
    num_frames,
    channel,
    frequency,
    amplitude,
    num_channels=None,
    use_phase_offset=True,
):
    """Generate a continuous sine wave for all numExecReps of a given channel.

    Args:
        num_samples (int): Number of samples per frame.
        sampling_rate (int): Sampling rate in Hz.
        num_frames (int): Number of numExecReps.
        channel (int): Channel index (for phase offset if used).
        frequency (float): Frequency of the sine wave in Hz.
        amplitude (float): Amplitude of the sine wave.
        num_channels (int, optional): Number of channels (for phase offset).
        use_phase_offset (bool, optional): Whether to apply phase offset. Defaults to True.

    Returns:
        numpy.ndarray: Array of sine wave samples, shape (num_samples * num_frames,).
    """
    total_samples = num_samples * num_frames
    t = np.linspace(0, total_samples / sampling_rate, total_samples, endpoint=False)
    phase_offset = (
        (channel * np.pi / num_channels) if use_phase_offset and num_channels else 0.0
    )
    sine_wave = amplitude * np.sin(2 * np.pi * frequency * t + phase_offset)
    return sine_wave


def gen_testCaseParams(testParamsFile, currPrm):
    """Generate and write test case parameters to idat file.

    Args:
        testParamsFile (file object): Input file object to idat.c file.
        currPrm (dict): Dictionary object with test case parameters.
    """
    testId = currPrm["ID"]
    dType = currPrm["dType"]
    numSamples = currPrm["numSamples"]
    numChannels = currPrm["numChannels"]
    isMuted = currPrm["isMuted"]  # Now a list
    samplingRate = currPrm["samplingRate"]
    fadeTime = currPrm["fadeTime"]
    strideInElements = currPrm["strideInElements"]
    strideOutElements = currPrm["strideOutElements"]
    testType = currPrm["testType"]
    demoCase = currPrm["demoCase"]
    outputDataLocation = currPrm["outputDataLocation"]
    numReps = currPrm["numReps"]
    C7100 = currPrm["C7100"]
    numExecReps = currPrm["numExecReps"]
    plotSignals = currPrm["plotSignals"]
    waveFiles = currPrm["waveFiles"]
    amplitude = currPrm["amplitude"]
    frequency = currPrm["frequency"]
    isInterleaved = currPrm["isInterleaved"]
    fadeType = currPrm["fadeType"]
    AUDIOLIB_dType = AUDIOLIB_utils.resolve_dType(dType)

    # Map fadeType string to integer
    fadeType_map = {"LINEAR": 0, "SMOOTH": 1, "HARD": 2}
    fadeType_int = fadeType_map[fadeType]

    if testType in ["STATIC", "SINE"]:
        in_data = "staticRefInCase" + testId
        out_data = "staticRefOutCase" + testId
        isMuted_ref = "staticRefIsMutedCase" + testId
        testType = "STATIC"
    else:
        in_data = "NULL"
        out_data = "NULL"
        isMuted_ref = "NULL"

    if C7100 == "FALSE":
        testParamsFile.write("#if !defined(__C7100__)\n")

    if demoCase == "true":
        testParamsFile.write(
            "#if (defined(ALL_TEST_CASES) || (TEST_CASE == %d) || defined(DEMO_CASE))\n"
            % (int(testId))
        )
    else:
        testParamsFile.write(
            "#if (defined(ALL_TEST_CASES) || (TEST_CASE == %d))\n" % (int(testId))
        )

    idatFile.write("{\n")
    idatFile.write("%s, // Test Pattern \n" % (testType))
    idatFile.write("%s, // staticIn \n" % (in_data))
    idatFile.write("%s, // staticOut \n" % (out_data))
    idatFile.write("%s, // Sample Data Type \n" % (AUDIOLIB_dType))
    idatFile.write("%d, // Samples per channel \n" % (numSamples))
    idatFile.write("%d, // Number of Channels \n" % (numChannels))
    idatFile.write("%s, // isMuted \n" % (isMuted_ref))
    idatFile.write("%s, // Sampling Rate \n" % (samplingRate))
    idatFile.write("%s, // fadeTime \n" % (fadeTime))
    idatFile.write("%d, // isInterleaved \n" % (int(isInterleaved)))
    idatFile.write("%d, // fadeType (0=LINEAR, 1=SMOOTH, 2=HARD)\n" % (fadeType_int))
    idatFile.write(
        "AUDIOLIB_CALC_STRIDE(%s * sizeof(%s), AUDIOLIB_ALIGN_SHIFT_64BYTES), // strideInElements \n"
        % (strideInElements, AUDIOLIB_dType)
    )
    idatFile.write(
        "AUDIOLIB_CALC_STRIDE(%s * sizeof(%s), AUDIOLIB_ALIGN_SHIFT_64BYTES), // strideOutElements \n"
        % (strideOutElements, AUDIOLIB_dType)
    )
    idatFile.write("%s, // Output data location \n" % (outputDataLocation))
    idatFile.write("%d, // number of reps. \n" % (int(numReps)))
    idatFile.write("%d, // test ID\n" % (int(testId)))
    idatFile.write("%d, // numExecReps\n" % (int(numExecReps)))
    idatFile.write("},\n")
    idatFile.write("#endif\n")

    if C7100 == "FALSE":
        testParamsFile.write("#endif // if !defined(__C7100__)\n")

    idatFile.write("\n\n")


def gen_idat_file(idatFile, testCases):
    """Generate the idat file.

    Args:
        idatFile (file object): Input file object to idat.c file.
        testCases (list): List of test case dictionaries.
    """
    fIn = open("../common/ti_lice_header_idat.txt", "r")
    lines = fIn.readlines()
    fIn.close()
    idatFile.writelines(lines)
    includeString = '#include "AUDIOLIB_muteNCh_idat.h"'
    idatFile.writelines(includeString)
    idatFile.write("\n")
    idatFile.write("/*This file is autogenerated*/\n")
    idatFile.write("/*Please do not edit*/\n")
    idatFile.write("\n\n")
    for testCase in testCases:
        if testCase["testType"] in ["STATIC", "SINE"]:
            AUDIOLIB_utils.gen_ifdefs(idatFile, testCase)

    idatFile.write("static AUDIOLIB_muteNCh_testParams_t testParams[] =  ")
    idatFile.write("{\n")

    for testCase in testCases:
        gen_testCaseParams(idatFile, testCase)

    idatFile.write("};\n\n")
    fIn = open("test_param_function.txt", "r")
    lines = fIn.readlines()
    idatFile.writelines(lines)


def process_test_case(testCase):
    """Convert str to int/float for necessary parameters and parse isMuted array.

    Args:
        testCase (dict): Dictionary object with test case parameters.

    Returns:
        dict: Dictionary with parameters converted to appropriate types.
    """
    testCase["testId"] = int(testCase["ID"])
    testCase["numSamples"] = int(testCase["numSamples"])
    testCase["numChannels"] = int(testCase["numChannels"])
    isMuted_str = testCase["isMuted"]
    if "," in isMuted_str:
        isMuted = [float(x.strip()) for x in isMuted_str.split(",")]
        if len(isMuted) != testCase["numChannels"]:
            raise ValueError(
                f"isMuted array length {len(isMuted)} does not match numChannels {testCase['numChannels']} for test ID {testCase['ID']}"
            )
    else:
        isMuted = [float(isMuted_str)] * testCase["numChannels"]
    testCase["isMuted"] = isMuted
    testCase["samplingRate"] = int(testCase["samplingRate"])
    testCase["fadeTime"] = float(testCase["fadeTime"])
    testCase["strideInElements"] = int(testCase["strideInElements"])
    testCase["strideOutElements"] = int(testCase["strideOutElements"])
    testCase["numExecReps"] = int(testCase.get("numExecReps", 1))
    testCase["plotSignals"] = testCase["plotSignals"].upper() == "TRUE"
    testCase["waveFiles"] = testCase["waveFiles"].upper() == "TRUE"
    testCase["amplitude"] = float(testCase.get("amplitude", 1.0))
    testCase["frequency"] = float(testCase.get("frequency", 2000.0))
    testCase["isInterleaved"] = testCase["isInterleaved"].upper() == "TRUE"
    testCase["fadeType"] = testCase["fadeType"].upper()
    # Validate isInterleaved
    if testCase["isInterleaved"] not in [True, False]:
        raise ValueError(
            f"Invalid isInterleaved value {testCase['isInterleaved']} for test ID {testCase['ID']}"
        )
    # Validate fadeType
    if testCase["fadeType"] not in ["LINEAR", "SMOOTH", "HARD"]:
        raise ValueError(
            f"Invalid fadeType value {testCase['fadeType']} for test ID {testCase['ID']}"
        )
    # Set initialGain and targetGain based on isMuted array
    testCase["initialGain"] = [1.0 if m == 1 else 0.0 for m in isMuted]
    testCase["targetGain"] = [1.0 - m for m in isMuted]
    return testCase


def gen_test_case_header_file(testCase):
    """Generate test case and write out input, output, and gain tensors into a header file usable in C/C++ code.

    Args:
        testCase (dict): Dictionary with test case parameters.

    Returns:
        list: [min_output, max_output] across output channels.
    """
    testId = testCase["ID"]
    numSamples = testCase["numSamples"]
    numChannels = testCase["numChannels"]
    samplingRate = testCase["samplingRate"]
    numExecReps = testCase["numExecReps"]
    plotSignals = testCase["plotSignals"]
    waveFiles = testCase["waveFiles"]
    testType = testCase["testType"]
    amplitude = testCase["amplitude"]
    frequency = testCase["frequency"]
    isInterleaved = testCase["isInterleaved"]
    frameSize = numSamples
    samplesPerFrame = numSamples * numChannels

    [minVal, maxVal] = [-10, 10]

    # Create an instance of AUDIOLIB_muteNCh
    mute_processor = AUDIOLIB_muteNCh(
        dType=testCase["dType"],
        numChannels=numChannels,
        numSamples=numSamples,
        numExecReps=numExecReps,
        samplingRate=samplingRate,
        fadeTime=testCase["fadeTime"],
        fadeType=testCase["fadeType"],
        initialGain=testCase["initialGain"],
        targetGain=testCase["targetGain"],
        isInterleaved=isInterleaved,
    )

    # Generate input samples for all channels
    pInLocal = np.zeros((numExecReps, samplesPerFrame), dtype=np.float32)
    if testType == "STATIC":
        if isInterleaved:
            for frame in range(numExecReps):
                for j in range(numSamples):
                    for c in range(numChannels):
                        idx = j * numChannels + c
                        pInLocal[frame, idx] = np.random.uniform(
                            low=minVal, high=maxVal
                        )
        else:
            for frame in range(numExecReps):
                for i in range(numChannels):
                    startIdx = i * numSamples
                    endIdx = (i + 1) * numSamples
                    pInLocal[frame, startIdx:endIdx] = np.random.uniform(
                        low=minVal, high=maxVal, size=numSamples
                    ).astype(np.float32)
    elif testType == "SINE":
        if isInterleaved:
            for frame in range(numExecReps):
                for j in range(numSamples):
                    for c in range(numChannels):
                        idx = j * numChannels + c
                        frame_start = frame * numSamples
                        sine_wave = generate_sine(
                            numSamples,
                            samplingRate,
                            numExecReps,
                            c,
                            frequency,
                            amplitude,
                            numChannels,
                            use_phase_offset=True,
                        )
                        pInLocal[frame, idx] = sine_wave[frame_start + j]
        else:
            for i in range(numChannels):
                startIdx = i * numSamples
                endIdx = (i + 1) * numSamples
                sine_wave = generate_sine(
                    numSamples,
                    samplingRate,
                    numExecReps,
                    channel=i,
                    frequency=frequency,
                    amplitude=amplitude,
                    num_channels=numChannels,
                    use_phase_offset=True,
                )
                for frame in range(numExecReps):
                    frame_start = frame * numSamples
                    frame_end = (frame + 1) * numSamples
                    pInLocal[frame, startIdx:endIdx] = sine_wave[frame_start:frame_end]

    # Apply gains to input data using the AUDIOLIB_muteNCh class
    pOutLocal = mute_processor.exec(pInLocal)

    # For compatibility with the rest of the code, we need to calculate the gain values
    # We can use the _calculate_gain method from the class
    pCurrentGain = mute_processor._calculate_gain()

    # Generate plots for all channels if plotSignals is TRUE
    if plotSignals:
        # Plot input/output for each channel
        for ch in range(numChannels):
            file_io.plot_signal_pair(
                pInLocal,
                pOutLocal,
                numSamples,
                numChannels,
                numExecReps,
                testId,
                "input_output",
                channel=ch,
                colors=["blue", "green"],
                amplitude=amplitude,
                frequency=frequency,
                test_type=testType,
                is_interleaved=isInterleaved,
            )
        # Plot gain for each channel
        for ch in range(numChannels):
            file_io.plot_signal_pair(
                pCurrentGain,
                None,
                numSamples,
                numChannels,
                numExecReps,
                testId,
                "gain",
                channel=ch,
                colors=["brown"],
                amplitude=amplitude,
                frequency=frequency,
                test_type=testType,
                is_interleaved=isInterleaved,
            )

    # Generate WAV files if waveFiles is TRUE
    if waveFiles:
        file_io.write_wav_files(
            pInLocal, pOutLocal, pCurrentGain, testCase, numExecReps
        )

    # Prepare flattened 1D lists
    in_list = pInLocal.flatten().tolist()
    out_list = pOutLocal.flatten().tolist()
    gain_list = pCurrentGain.flatten().tolist()

    # Write header file
    headerFileName = "staticRefCase" + str(testId) + ".h"
    file_io.write_header_file(
        in_data=in_list,
        out_data=out_list,
        gain_data=gain_list,
        testCase=testCase,
        headerFileName=headerFileName,
        frames=numExecReps,
    )

    min_output = np.min(out_list)
    max_output = np.max(out_list)
    return [min_output, max_output]


# CSV file with parameters for all test cases
testCasesCsvFile = "../AUDIOLIB_muteNCh/test_cases_list.csv"
testIdList = list(range(0, 100))

myArgs = AUDIOLIB_utils.getCmdLineArgs()


def gen_test_case(testCase):
    if testCase["testType"] in ["STATIC", "SINE"]:
        [minY, maxY] = gen_test_case_header_file(testCase)
        print(
            "Test case generation completed for test ID",
            testCase["ID"],
            "w/ min:",
            minY,
            "and max",
            maxY,
        )
    else:
        print(
            "Skipping test case generation for test ID",
            testCase["ID"],
            "as testType is RANDOM",
        )


with open(testCasesCsvFile, encoding="utf-8-sig") as csv_file:
    testCaseReader = csv.DictReader(csv_file)
    testCases = list(testCaseReader)

    for testCase in testCases:
        testCase = process_test_case(testCase)

    for testCase in testCases:
        if myArgs.allCases == True:
            gen_test_case(testCase)
        elif int(testCase["ID"]) in testIdList:
            gen_test_case(testCase)

idatFileName = "../../AUDIOLIB_muteNCh/AUDIOLIB_muteNCh_idat.c"

with open(idatFileName, "w") as idatFile:
    gen_idat_file(idatFile, testCases)

if platform == "linux" or platform == "linux2":
    call(["clang-format", "-style=file", "-i", idatFileName])
    call(["indent", "-nut", "-i3", "-c55", "-l200", idatFileName])
    call(["rm", idatFileName + "~"])

print("Idat file generation completed")
