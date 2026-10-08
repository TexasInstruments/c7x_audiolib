# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""Generate AUDIOLIB_gainNChTrimSmoothing test data from test_cases_list.csv.

Per-channel gain columns. Each cell holds one value for every channel, or a quoted
comma list with exactly inChannels values (same convention as AUDIOLIB_muteNCh):

    trimGain     The per-channel gain setting the user chooses. The kernel fades each
                 channel to masterGain * trimGain[ch]. In SMOOTH rows masterGain is 1, so
                 trimGain is the per-channel control value the smoother chases.
    initialGain  The gain actually applied to each channel when the case starts, i.e. the
                 kernel's running gain. It already includes whatever masterGain and trim
                 were in effect before, so it is NOT multiplied by masterGain. A channel is
                 left unchanged only when initialGain[ch] == masterGain * trimGain[ch].
    masterGain   One linear gain applied on top of every channel's trimGain.

All gains are linear. fadeTime is the ramp duration (SYMMETRIC) or the time constant
(SMOOTH), in ms.
"""

from __future__ import print_function
from subprocess import call
from sys import platform

import sys
import numpy as np
import csv
import wave

sys.path.append("../common")
import AUDIOLIB_utils
import file_io
import os
from gainNChTrimSmoothing import AUDIOLIB_gainNChTrimSmoothing

# fadeType values written to the idat; must match the C enum
FADE_TYPE_ID = {"SYMMETRIC": 0, "SMOOTH": 1}


def gen_testCaseParams(testParamsFile, currPrm):
    """Generate and write test case parameters to idat file

    Args:
        param1 (file object): Input file object to idat.c file
        param2 (dict object): Dict object with test case parameter

    """

    testId = currPrm["ID"]
    dType = currPrm["dType"]
    inChannels = currPrm["inChannels"]
    inSamples = currPrm["inSamples"]
    isInterleaved = currPrm["isInterleaved"]
    strideIn0 = currPrm["strideInElements"]
    strideOut = currPrm["strideOutElements"]
    testType = currPrm["testType"]
    demoCase = currPrm["demoCase"]
    outputDataLocation = currPrm["outputDataLocation"]
    numReps = currPrm["numReps"]
    C7100 = currPrm["C7100"]
    masterGain = currPrm["masterGain"]
    fadeTime = currPrm["fadeTime"]
    samplingRate = currPrm["samplingRate"]
    numExecReps = currPrm["numExecReps"]
    fadeType = currPrm["fadeType"]

    if testType == "STATIC" or testType == "SINE":
        in0 = "staticRefInCase" + testId
        currentGain = "currentGainCase" + testId
        targetGain = "targetGainCase" + testId
        out = "staticRefOutCase" + testId
    else:
        in0 = "NULL"
        currentGain = "NULL"
        targetGain = "NULL"
        out = "NULL"

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

    testParamsFile.write("{\n")
    # SINE only selects how Python synthesises the input; C reads both as static arrays.
    testPattern = "STATIC" if testType in ("STATIC", "SINE") else testType
    testParamsFile.write("%s, // Test pattern \n" % ((testPattern)))
    testParamsFile.write("%s, // Input buffer\n" % ((in0)))
    testParamsFile.write("%s, // Input currentGain (per channel) \n" % ((currentGain)))
    testParamsFile.write("%s, // Input targetGain (per channel) \n" % ((targetGain)))
    testParamsFile.write("%.9ef, // masterGain \n" % (np.float32(masterGain)))
    testParamsFile.write("%s, // Output buffer \n" % ((out)))
    testParamsFile.write("%s, // inSamples \n" % ((inSamples)))
    testParamsFile.write("%s, // inChannels \n" % ((inChannels)))
    testParamsFile.write("%s, // isInterleaved \n" % ((isInterleaved)))
    testParamsFile.write(
        "AUDIOLIB_CALC_STRIDE(%s * sizeof(%s), AUDIOLIB_ALIGN_SHIFT_64BYTES), // strideIn0 \n"
        % ((strideIn0), (dType))
    )
    testParamsFile.write(
        "AUDIOLIB_CALC_STRIDE(%s * sizeof(%s), AUDIOLIB_ALIGN_SHIFT_64BYTES), // strideOut \n"
        % ((strideOut), (dType))
    )
    testParamsFile.write("%s, // fadeTime (ms) \n" % (repr(float(fadeTime))))
    testParamsFile.write("%d, // samplingRate \n" % (int(samplingRate)))
    testParamsFile.write("%d, // numExecReps (frames) \n" % (int(numExecReps)))
    testParamsFile.write("%d, // fadeType (0=SYMMETRIC, 1=SMOOTH) \n" % (FADE_TYPE_ID[fadeType]))
    testParamsFile.write("%s, // precision \n" % ((AUDIOLIB_utils.resolve_dType(dType))))
    testParamsFile.write("%s, // Output data location \n" % (outputDataLocation))
    testParamsFile.write("%d, // number of reps. \n" % (int(numReps)))
    testParamsFile.write("%d, // test ID\n" % int(testId))
    testParamsFile.write("},\n")
    testParamsFile.write("#endif\n")

    if C7100 == "FALSE":
        testParamsFile.write("#endif // if !defined(__C7100__)\n")

    testParamsFile.write("\n\n")


def gen_idat_file(idatFile, testCases):
    """Generate the idat file

    Args:
        param1 (file object): Input file object to idat.c file
        param2 (dict object): Dict object with test case parameter

    """

    fIn = open("../common/ti_lice_header_idat.txt", "r")
    lines = fIn.readlines()
    fIn.close()
    idatFile.writelines(lines)
    includeString = '#include "AUDIOLIB_gainNChTrimSmoothing_idat.h"'
    idatFile.writelines(includeString)
    idatFile.write("\n")
    idatFile.write("/*This file is autogenerated*/\n")
    idatFile.write("/*Please do not edit*/\n")
    idatFile.write("\n\n")
    # generate ifdefs for header files
    for testCase in testCases:
        if testCase["testType"] == "STATIC" or testCase["testType"] == "SINE":
            AUDIOLIB_utils.gen_ifdefs(idatFile, testCase)

    # generate parameters for test cases
    idatFile.write("static AUDIOLIB_gainNChTrimSmoothing_testParams_t testParams[] =  ")
    idatFile.write("{\n")

    for testCase in testCases:
        gen_testCaseParams(idatFile, testCase)

    idatFile.write("};\n\n")
    fIn = open("test_param_function.txt", "r")
    lines = fIn.readlines()

    idatFile.writelines(lines)


def save_wav_file(filename, signal_data, testCase, is_interleaved=True, peak=None):
    os.makedirs(os.path.dirname(filename), exist_ok=True)

    num_channels = testCase["inChannels"]
    num_samples = testCase["inSamples"]
    num_frames = testCase["numExecReps"]
    sampling_rate = testCase["samplingRate"]

    signal_arr = np.asarray(signal_data, dtype=np.float32)

    bit_depth = 32

    with wave.open(filename, "wb") as wf:

        wf.setnchannels(num_channels)
        wf.setframerate(sampling_rate)

        if bit_depth == 32:
            wf.setsampwidth(4)
            scale = 2147483647.0
            dtype = np.int32
        else:
            wf.setsampwidth(2)
            scale = 32767.0
            dtype = np.int16

        # Pass the same peak for input and output so both files share one scale.
        if peak is None:
            peak = np.max(np.abs(signal_arr))
        gain = 1.0

        if peak > 1.0:
            gain = 1.0 / peak
            signal_arr = signal_arr * gain

        # Convert in float64 and clip: in float32, 1.0 * 2147483647 rounds to 2^31 and
        # wraps to -2^31, turning the loudest sample into a full-scale click.
        def to_pcm(x):
            return np.clip(np.round(np.asarray(x, dtype=np.float64) * scale), -scale - 1.0, scale).astype(dtype)

        if is_interleaved:
            audio = to_pcm(signal_arr)
            wf.writeframes(
                np.ascontiguousarray(audio).tobytes())
        else:
            frame_size = num_channels * num_samples
            for f in range(num_frames):
                start = f * frame_size
                end = start + frame_size
                frame = signal_arr[start:end]
                frame = frame.reshape(
                    num_channels,
                    num_samples
                ).T
                frame = to_pcm(frame)
                wf.writeframes(
                    np.ascontiguousarray(frame).tobytes()
                )


def parse_gain_list(value, numChannels, name, testId):
    """Parse a per-channel gain cell: one value for every channel, or a comma list
    with exactly numChannels values (same convention as AUDIOLIB_muteNCh)."""
    values = [float(x.strip()) for x in str(value).split(",")]
    if len(values) == 1:
        return values * numChannels
    if len(values) != numChannels:
        raise ValueError(
            f"{name} has {len(values)} values but inChannels is {numChannels} for test ID {testId}"
        )
    return values


def process_test_case(testCase):
    """Convert str to int/float for necessary parameters and parse the per-channel gains

    Args:
        Param1(:obj: dict) : Dictionary object with test case parameters

    Returns:
        Obj: Dictionary object with necessary parameters converted

    """
    testCase["testId"] = int(testCase["ID"])
    testCase["inChannels"] = int(testCase["inChannels"])
    testCase["inSamples"] = int(testCase["inSamples"])
    testCase["isInterleaved"] = int(testCase["isInterleaved"])
    testCase["strideInElements"] = int(testCase["strideInElements"])
    testCase["strideOutElements"] = int(testCase["strideOutElements"])
    testCase["samplingRate"] = int(testCase["samplingRate"])
    testCase["numExecReps"] = int(testCase["numExecReps"])
    testCase["fadeTime"] = float(testCase["fadeTime"])
    testCase["fadeType"] = testCase["fadeType"].strip().upper()
    testCase["masterGain"] = float(testCase["masterGain"])
    testCase["plotSignals"] = testCase["plotSignals"].strip().upper() == "TRUE"
    testCase["waveFiles"] = testCase["waveFiles"].strip().upper() == "TRUE"
    testCase["amplitude"] = float(testCase["amplitude"])
    testCase["frequency"] = float(testCase["frequency"])
    testCase["initialGain"] = parse_gain_list(testCase["initialGain"], testCase["inChannels"], "initialGain", testCase["ID"])
    testCase["trimGain"] = parse_gain_list(testCase["trimGain"], testCase["inChannels"], "trimGain", testCase["ID"])

    if testCase["fadeType"] not in FADE_TYPE_ID:
        raise ValueError(f"Invalid fadeType {testCase['fadeType']} for test ID {testCase['ID']}")
    if testCase["fadeTime"] < 0:
        raise ValueError(f"fadeTime must be >= 0 for test ID {testCase['ID']}")
    if testCase["fadeType"] == "SYMMETRIC" and (
        min(testCase["initialGain"] + testCase["trimGain"]) < 0 or testCase["masterGain"] < 0
    ):
        raise ValueError(f"SYMMETRIC fades need gains >= 0 for test ID {testCase['ID']}")

    return testCase


def gen_test_case_header_file(testCase):
    """Generate test case and write out input, gains, and output
    into a header file usable in C/C++ code.

    Args:
        param1 (:obj: dict): A dict object with all the test case
        parameters

    Returns:
        [int int]: Minimum and maximum value in output

    """

    ##############################
    # Get test parameter of case #
    ##############################

    testId = testCase["ID"]
    dType = testCase["dType"]
    inChannels = testCase["inChannels"]
    inSamples = testCase["inSamples"]
    isInterleaved = testCase["isInterleaved"]
    numExecReps = testCase["numExecReps"]
    frequency = testCase["frequency"]
    amplitude = testCase["amplitude"]
    samplingRate = testCase["samplingRate"]
    testType = testCase["testType"]

    # Gains are rounded to float32 here, so the reference and the C test use identical values.
    initialGain = np.array(testCase["initialGain"], dtype=np.float32)
    trimGain = np.array(testCase["trimGain"], dtype=np.float32)
    masterGain = np.float32(testCase["masterGain"])

    gainNChTrimSmoothing_processor = AUDIOLIB_gainNChTrimSmoothing(
        dType=dType,
        inChannels=inChannels,
        inSamples=inSamples,
        samplingRate=samplingRate,
        fadeTime=testCase["fadeTime"],
        fadeType=testCase["fadeType"],
        isInterleaved=isInterleaved,
        numExecReps=numExecReps,
    )

    [minVal, maxVal] = [-10, 10]
    # Seeded per test ID so regenerating the data gives the same files.
    rng = np.random.default_rng(testCase["testId"])

    if isInterleaved:
        # Shape: [numFrames, inSamples, inChannels]
        in0 = np.empty((numExecReps, inSamples, inChannels), dtype=np.float32)
    else:
        # Shape: [numFrames, inChannels, inSamples]
        in0 = np.empty((numExecReps, inChannels, inSamples), dtype=np.float32)

    if dType[0] == "f":
        for frame in range(numExecReps):
            sample_idx = np.arange(inSamples) + frame * inSamples
            t = sample_idx / float(samplingRate)

            for ch in range(inChannels):
                if testType.strip().upper() == "SINE":
                    mono_signal = amplitude * np.sin(2.0 * np.pi * frequency * t)
                else:
                    mono_signal = rng.uniform(low=minVal, high=maxVal, size=inSamples)

                if isInterleaved:
                    # Interleaved shape per frame: (inSamples, inChannels)
                    in0[frame, :, ch] = mono_signal
                else:
                    # Non-interleaved shape per frame: (inChannels, inSamples)
                    in0[frame, ch, :] = mono_signal

    # Execute the gainNChTrimSmoothing algorithm: one set(), then exec() over all frames
    gainNChTrimSmoothing_processor.set(initialGain, trimGain, masterGain)
    out, gains = gainNChTrimSmoothing_processor.exec(in0)

    if testCase["waveFiles"]:
        wav_dir = os.path.join("wav_outputs", f"case_{testId}")
        wavPeak = max(np.max(np.abs(in0)), np.max(np.abs(out)))
        save_wav_file(os.path.join(wav_dir, f"input_case_{testId}.wav"), in0.flatten().tolist(), testCase, is_interleaved=(isInterleaved == 1), peak=wavPeak)
        save_wav_file(os.path.join(wav_dir, f"output_case_{testId}.wav"), out.flatten().tolist(), testCase, is_interleaved=(isInterleaved == 1), peak=wavPeak)

    if testCase["plotSignals"]:
        file_io.plot_gain_curves(gains, testCase, outputDir="plots")

    headerFileName = "staticRefCase" + str(testId) + ".h"
    file_io.write_header_file(in0, out, currentGain_data=initialGain, targetGain_data=trimGain, testCase=testCase, headerFileName=headerFileName)
    return [np.min(out), np.max(out)]


# CSV file with paratmeters for all test cases
testCasesCsvFile = "../AUDIOLIB_gainNChTrimSmoothing/test_cases_list.csv"
testIdList = []  # list of test IDs that we want to generate test data
testIdList += range(0, 100)

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
            "as tesType is RANDOM",
        )


#############################################################################
# Open CSV file with test cases and generate header file with test data for #
# test cases of interest; based on testIdList                               #
#############################################################################

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


idatFileName = "../../AUDIOLIB_gainNChTrimSmoothing/AUDIOLIB_gainNChTrimSmoothing_idat.c"


##########################################################################
# Generate idat.cpp file by parsing all the test cases in the CSV file # #
##########################################################################


with open(idatFileName, "w") as idatFile:
    gen_idat_file(idatFile, testCases)


if platform == "linux" or platform == "linux2":
    call(["clang-format", "-style=file", "-i", idatFileName])
    call(["indent", "-nut", "-i3", "-c55", "-l200", idatFileName])
    call(["rm", idatFileName + "~"])  # remove backup file


print("Idat file generation completed")
