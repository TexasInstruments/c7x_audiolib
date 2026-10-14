# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function
from subprocess import call
from sys import platform

import sys
import numpy as np
import csv

sys.path.append("../common")
import AUDIOLIB_utils
import file_io
from agc import AUDIOLIB_agc

USER_PARAMS_FLOAT = [
    "samplingRate",
    "targetLevel",
    "maxAttenuation",
    "maxGain",
    "ratio",
    "activationThreshold",
    "smoothingTime",
    "recoveryRate",
    "currentGain",
]
USER_PARAMS_INT = ["enableRecovery"]

# Frequency (Hz) of the sine input used when testType is SINE
SINE_FREQUENCY = 1000.0


def generate_sine(numSamples, samplingRate, channel, frequency, amplitude, startSample=0):
    """Generate a sine wave block, phase-continuous across consecutive frames.

    Args:
        param1 (int): Number of samples to generate
        param2 (float): Sampling rate in Hz
        param3 (int): Channel index, used to give each channel a distinct phase
        param4 (float): Frequency of the sine wave in Hz
        param5 (float): Amplitude of the sine wave
        param6 (int): Index of the first sample, keeps phase continuous across frames

    Returns:
        numpy.ndarray: Array of sine wave samples, shape (numSamples,)

    """
    t = np.arange(startSample, startSample + numSamples, dtype=np.float64) / float(samplingRate)
    return amplitude * np.sin(2.0 * np.pi * frequency * t + channel * np.pi / 4.0)


def c_float(value):
    """Format a value as a C single-precision literal, e.g. -20 -> -20.0f"""
    text = "%.9g" % value
    if not any(c in text for c in ".eEn"):
        text += ".0"
    return text + "f"


def gen_testCaseParams(testParamsFile, currPrm):
    """Generate and write test case parameters to idat file

    Args:
        param1 (file object): Input file object to idat.c file
        param2 (dict object): Dict object with test case parameter

    """

    testId = currPrm["ID"]
    dType = currPrm["dType"]
    testType = currPrm["testType"]
    demoCase = currPrm["demoCase"]
    C7100 = currPrm["C7100"]

    if testType in ["STATIC", "SINE"]:
        in0 = "staticRefIn0Case" + testId
        out = "staticRefOutCase" + testId
        testType = "STATIC"
    else:
        in0 = "NULL"
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
    testParamsFile.write("%s, // Test pattern \n" % testType)
    testParamsFile.write("%s, // Input buffer\n" % in0)
    testParamsFile.write("%s, // Output buffer \n" % out)
    testParamsFile.write("%d, // inSamples \n" % currPrm["inSamples"])
    testParamsFile.write("%d, // inChannels \n" % currPrm["inChannels"])
    testParamsFile.write("%d, // numFrames \n" % currPrm["numFrames"])
    testParamsFile.write(
        "AUDIOLIB_CALC_STRIDE(%d * sizeof(%s), AUDIOLIB_ALIGN_SHIFT_64BYTES), // strideIn \n"
        % (currPrm["strideIn"], dType)
    )
    testParamsFile.write(
        "AUDIOLIB_CALC_STRIDE(%d * sizeof(%s), AUDIOLIB_ALIGN_SHIFT_64BYTES), // strideOut \n"
        % (currPrm["strideOut"], dType)
    )
    testParamsFile.write("%d, // samplingRate \n" % int(currPrm["samplingRate"]))
    testParamsFile.write("%s, // targetLevel \n" % c_float(currPrm["targetLevel"]))
    testParamsFile.write("%s, // maxAttenuation \n" % c_float(currPrm["maxAttenuation"]))
    testParamsFile.write("%s, // maxGain \n" % c_float(currPrm["maxGain"]))
    testParamsFile.write("%s, // ratio \n" % c_float(currPrm["ratio"]))
    testParamsFile.write("%s, // activationThreshold \n" % c_float(currPrm["activationThreshold"]))
    testParamsFile.write("%s, // smoothingTime \n" % c_float(currPrm["smoothingTime"]))
    testParamsFile.write("%d, // enableRecovery \n" % currPrm["enableRecovery"])
    testParamsFile.write("%s, // recoveryRate \n" % c_float(currPrm["recoveryRate"]))
    testParamsFile.write("%s, // currentGain \n" % c_float(currPrm["currentGain"]))
    testParamsFile.write("%s, // precision \n" % AUDIOLIB_utils.resolve_dType(dType))
    testParamsFile.write("%s, // Output data location \n" % currPrm["outputDataLocation"])
    testParamsFile.write("%d, // number of reps. \n" % int(currPrm["numReps"]))
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
    idatFile.writelines('#include "AUDIOLIB_agc_idat.h"')
    idatFile.write("\n")
    idatFile.write("/*This file is autogenerated*/\n")
    idatFile.write("/*Please do not edit*/\n")
    idatFile.write("\n\n")
    for testCase in testCases:
        if testCase["testType"] in ["STATIC", "SINE"]:
            AUDIOLIB_utils.gen_ifdefs(idatFile, testCase)

    idatFile.write("static AUDIOLIB_agc_testParams_t testParams[] =  ")
    idatFile.write("{\n")

    for testCase in testCases:
        gen_testCaseParams(idatFile, testCase)

    idatFile.write("};\n\n")
    fIn = open("test_param_function.txt", "r")
    lines = fIn.readlines()
    fIn.close()

    idatFile.writelines(lines)


def process_test_case(testCase):
    """Convert str to numeric types for necessary parameters

    Args:
        Param1(:obj: dict) : Dictionary object with test case parameters

    Returns:
        Obj: Dictionary object with necessary parameters converted

    """

    testCase["testId"] = int(testCase["ID"])
    testCase["inChannels"] = int(testCase["inChannels"])
    testCase["inSamples"] = int(testCase["inSamples"])
    testCase["numFrames"] = int(testCase["numFrames"])
    testCase["strideIn"] = int(testCase["strideInElements"])
    testCase["strideOut"] = int(testCase["strideOutElements"])
    testCase["amplitude"] = float(testCase["amplitude"])
    testCase["quietAmplitude"] = float(testCase["quietAmplitude"])
    testCase["testType"] = testCase.get("testType", "STATIC").strip().upper()
    for name in USER_PARAMS_FLOAT:
        testCase[name] = float(testCase[name])
    for name in USER_PARAMS_INT:
        testCase[name] = int(testCase[name])

    return testCase


def gen_test_case_header_file(testCase):
    """Generate input frames, run the reference model across all frames and
    write input and expected output into a header file usable in C/C++ code.

    Args:
        param1 (:obj: dict): A dict object with all the test case
        parameters

    Returns:
        [float float]: Minimum and maximum value of the output gain

    """

    testId = testCase["ID"]
    dType = testCase["dType"]
    inChannels = testCase["inChannels"]
    inSamples = testCase["inSamples"]
    numFrames = testCase["numFrames"]

    agc_processor = AUDIOLIB_agc(
        inChannels=inChannels,
        inSamples=inSamples,
        **{name: testCase[name] for name in USER_PARAMS_FLOAT + USER_PARAMS_INT},
    )
    derivedParams = agc_processor.derived_params()

    npType = "float32" if dType[0] == "f" else "double"

    # First half of the frames is "loud" and drives the AGC, the second half
    # is "quiet" so the activation threshold / recovery path is exercised.
    # testType STATIC uses uniform random noise, SINE uses a sine wave.
    numLoudFrames = (numFrames + 1) // 2
    in0 = np.empty((numFrames, inChannels, inSamples), dtype=npType)
    out = np.empty((numFrames, inChannels, inSamples), dtype=np.float64)
    for frame in range(numFrames):
        amplitude = testCase["amplitude"] if frame < numLoudFrames else testCase["quietAmplitude"]
        if testCase["testType"] == "SINE":
            for ch in range(inChannels):
                in0[frame, ch] = generate_sine(
                    inSamples,
                    testCase["samplingRate"],
                    ch,
                    SINE_FREQUENCY,
                    amplitude,
                    startSample=frame * inSamples,
                ).astype(npType)
        else:
            in0[frame] = np.random.uniform(low=-amplitude, high=amplitude, size=(inChannels, inSamples)).astype(npType)
        out[frame] = agc_processor.exec(in0[frame])

    out = out.astype(npType)

    headerFileName = "staticRefCase" + str(testId) + ".h"
    file_io.write_header_file(in0, out, derivedParams, testCase, headerFileName)

    return [np.min(out), np.max(out)]


# CSV file with parameters for all test cases
testCasesCsvFile = "test_cases_list.csv"
testIdList = []  # list of test IDs that we want to generate test data
testIdList += range(0, 60)

myArgs = AUDIOLIB_utils.getCmdLineArgs()


def gen_test_case(testCase):
    if testCase["numFrames"] < 3:
        print(
            "Skipping test case generation for test ID",
            testCase["ID"],
            "as numFrames < 3",
        )
        return
    
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


idatFileName = "../../AUDIOLIB_agc/AUDIOLIB_agc_idat.c"


##########################################################################
# Generate idat.c file by parsing all the test cases in the CSV file     #
##########################################################################


with open(idatFileName, "w") as idatFile:
    gen_idat_file(idatFile, testCases)


if platform == "linux" or platform == "linux2":
    call(["clang-format", "-style=file", "-i", idatFileName])
    call(["indent", "-nut", "-i3", "-c55", "-l200", idatFileName])
    call(["rm", "-f", idatFileName + "~"])  # remove backup file


print("Idat file generation completed")
