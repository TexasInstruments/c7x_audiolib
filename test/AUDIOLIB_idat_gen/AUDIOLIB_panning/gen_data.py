from __future__ import print_function
from subprocess import call
from sys import platform

import sys
import numpy as np
import csv
import re
import json
import os
import wave
import matplotlib.pyplot as plt

sys.path.append("../common")
import AUDIOLIB_utils
import file_io
from panning import AUDIOLIB_panning

LAYOUTS_JSON_FILE = "channel_layouts.json"



def load_layouts(json_path):
    if not os.path.exists(json_path):
        raise FileNotFoundError(f"Layouts file not found: {json_path}")

    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    layouts = {}
    for layout_name, config in data.get("layouts", {}).items():
        layouts[layout_name.strip().upper()] = config
    return layouts


LAYOUTS = load_layouts(LAYOUTS_JSON_FILE)


def get_speaker_layout(layout_name):
    normalized_name = layout_name.strip().upper()

    if normalized_name not in LAYOUTS:
        supported = ", ".join(LAYOUTS.keys())
        raise ValueError(f"Unsupported speaker layout: {layout_name}. Supported layouts: {supported}")

    layout_config = LAYOUTS[normalized_name]
    return layout_config.get("speakers", [])


def save_wav_file(filename, signal_data, sampling_rate, num_channels, is_interleaved=True):
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    
    signal_arr = np.array(signal_data, dtype=np.float32)

    if num_channels == 1:
        reshaped_signal = signal_arr.reshape(-1, 1)
    elif is_interleaved:
        reshaped_signal = signal_arr.reshape(-1, num_channels)
    else:
        reshaped_signal = signal_arr.reshape(num_channels, -1).T

    clipped_signal = np.clip(reshaped_signal, -1.0, 1.0)
    int16_data = (clipped_signal * 32767.0).astype(np.int16)


    with wave.open(filename, "wb") as wf:
        wf.setnchannels(num_channels)
        wf.setsampwidth(2)
        wf.setframerate(sampling_rate)
        wf.writeframes(int16_data.tobytes())

    print(f"Saved WAV file to: {filename}")


ABBREV_MAP = {
    "FRONT_LEFT": "FL", "FRONT_LEFT_TREBBLE": "FL_T", "FRONT_LEFT_MID": "FL_M", "FRONT_LEFT_WOOFER": "FL_W",
    "FRONT_RIGHT": "FR", "FRONT_RIGHT_TREBBLE": "FR_T", "FRONT_RIGHT_MID": "FR_M", "FRONT_RIGHT_WOOFER": "FR_W",
    "FRONT_CENTER": "C", "LOW_FREQUENCY_EFFECTS": "LFE", "SUBWOOFER": "LFE", "SIDE_LEFT": "SL", "SIDE_RIGHT": "SR",
    "REAR_LEFT": "RL", "REAR_RIGHT": "RR", "TOP_FRONT_LEFT": "TFL", "TOP_FRONT_RIGHT": "TFR", "TOP_MIDDLE_LEFT": "TML",
    "TOP_MIDDLE_RIGHT": "TMR", "TOP_REAR_LEFT": "TRL", "TOP_REAR_RIGHT": "TRR", "OVERHEAD": "OH",
}

def plot_panning_results(testCase, gains,count, sourceAzimuth):
    testId = testCase["ID"]
    speakerNames = testCase["speakerNames"]
    numSpeakers = testCase["numChannels"]
    speakerAngles = testCase["speakerAngles"]

    
    abbrevNames = [ABBREV_MAP.get(name, name) for name in speakerNames]

    gains_arr = np.array(gains).reshape(-1, numSpeakers)
    avg_gains = np.mean(gains_arr, axis=0)

    fig = plt.figure(figsize=(12, 5))
    fig.suptitle(
        f"Test Case {testId}: Panning Verification (Layout: {testCase['speakerLayout']}, Source Angle: {sourceAzimuth}°)",
        fontsize=12,
        fontweight="bold",
    )

    
    ax1 = fig.add_subplot(1, 2, 1, projection="polar")
    valid_mask = ~np.isnan(speakerAngles)
    angles_rad = np.radians(speakerAngles[valid_mask])
    
    active_gains = avg_gains[:len(speakerAngles)]
    valid_gains = active_gains[valid_mask]
    
    aligned_abbrevs = abbrevNames[:len(speakerAngles)]
    valid_abbrevs = np.array(aligned_abbrevs)[valid_mask]

    speaker_radius = np.full_like(angles_rad, 0.8)

    ax1.scatter(
        angles_rad, speaker_radius, color="navy", s=120, zorder=3, label="Speakers"
    )

    for name, angle, gain in zip(valid_abbrevs, angles_rad, valid_gains):
        ax1.annotate( f"{name}\n({gain:.2f})", xy=(angle, 0.8), textcoords="offset points", xytext=(0, 12),
            ha="center", fontsize=8, fontweight="bold")

    src_rad = np.radians(sourceAzimuth)
    ax1.plot( [0, src_rad], [0, 0.8], color="red", linestyle="--", linewidth=2,
        label=f"Source Vector ({sourceAzimuth}°)")

    ax1.set_theta_zero_location("N")
    ax1.set_theta_direction(-1)

    ticks_deg = [0, 45, 90, 135, 180, 225, 270, 315]
    tick_labels = ["0°", "45°", "90°", "135°", "180°", "-135°", "-90°", "-45°"]
    ax1.set_xticks(np.radians(ticks_deg))
    ax1.set_xticklabels(tick_labels)

    ax1.set_yticklabels([])
    ax1.set_ylim(0, 1.1)

    ax1.set_title("Spatial Layout & Gain Distribution", pad=15)
    ax1.legend(loc="lower right", fontsize=8)

    ax2 = fig.add_subplot(1, 2, 2)
    bars = ax2.bar(
        abbrevNames, avg_gains, color="skyblue", edgecolor="steelblue"
    )
    ax2.set_ylabel("Calculated Gain")
    ax2.set_xlabel("Speaker Channel")
    ax2.set_title("Per-Channel Gain Magnitude")
    ax2.set_ylim(0, 1.25)
    plt.xticks(rotation=0, ha="center", fontsize=9)

    for bar in bars:
        height = bar.get_height()
        ax2.text(
            bar.get_x() + bar.get_width() / 2.0,
            height + 0.02,
            f"{height:.3f}",
            ha="center",
            va="bottom",
            fontsize=8,
        )

    plt.tight_layout()
    os.makedirs("panning_plots", exist_ok=True)
    plot_file = os.path.join("panning_plots", f"test_case_{testId}_{count}.png")    
    plt.savefig(plot_file, dpi=150)
    print(f"Saved panning plot to {plot_file}")
    plt.show(block=False)
    plt.pause(1)
    plt.close()


def plot_waveform_results(testCase, in_data, out_data):
    testId = testCase["ID"]
    numSamples = testCase["numSamples"]
    numChannels = testCase["numChannels"]
    numExecReps = testCase["numExecReps"]
    samplingRate = testCase["samplingRate"]
    speakerNames = testCase["speakerNames"]
    isInterleave = testCase["isInterleave"]

    total_samples_per_chan = numSamples * numExecReps
    time_axis = np.arange(total_samples_per_chan) / float(samplingRate)

    in_signal = np.array(in_data, dtype=np.float32).flatten()
    out_arr = np.array(out_data, dtype=np.float32).reshape(numExecReps, -1)
    out_channels = [np.zeros(total_samples_per_chan, dtype=np.float32) for _ in range(numChannels)]

    for frame in range(numExecReps):
        start_sample = frame * numSamples
        end_sample = start_sample + numSamples

        if isInterleave == 1:
            frame_out = out_arr[frame].reshape(numSamples, numChannels)
            for ch in range(numChannels):
                out_channels[ch][start_sample:end_sample] = frame_out[:, ch]
        else:
            frame_out = out_arr[frame].reshape(numChannels, numSamples)
            for ch in range(numChannels):
                out_channels[ch][start_sample:end_sample] = frame_out[ch, :]

    fig, (ax_in, ax_out) = plt.subplots(2, 1, figsize=(12, 7), sharex=True)
    fig.suptitle(
        f"Test Case {testId}: Waveforms (Interleave={isInterleave} | Layout: {testCase['speakerLayout']} | Source: {testCase['sourceAzimuth']}°)",
        fontsize=11,
        fontweight="bold",
    )

    ax_in.plot(time_axis, in_signal, label="In: Mono Source", color="crimson", alpha=0.8, linewidth=1.2)
    ax_in.set_title("Input Waveform (Mono Source)")
    ax_in.set_ylabel("Amplitude")
    ax_in.grid(True, linestyle="--", alpha=0.5)
    ax_in.legend(loc="upper right", fontsize=8)

    for ch in range(numChannels):
        name = speakerNames[ch] if ch < len(speakerNames) else f"Ch {ch}"
        ax_out.plot(time_axis, out_channels[ch], label=f"Out: {name}", alpha=0.8, linewidth=1.2)

    ax_out.set_title("Panned Output Channels Waveform")
    ax_out.set_xlabel("Time (seconds)")
    ax_out.set_ylabel("Amplitude")
    ax_out.grid(True, linestyle="--", alpha=0.5)
    ax_out.legend(loc="upper right", fontsize=8)

    plt.tight_layout()
    os.makedirs("panning_plots", exist_ok=True)
    plot_file = os.path.join("panning_plots", f"test_case_{testId}_waveforms.png")
    plt.savefig(plot_file, dpi=150)
    plt.close(fig)
    print(f"Saved waveform plot to {plot_file}")


def gen_testCaseParams(testParamsFile, currPrm):
    testId = currPrm["ID"]
    dType = currPrm["dType"]
    numSamples = currPrm["numSamples"]
    numChannels = currPrm["numChannels"]
    speakerLayout = currPrm["speakerLayout"]
    sourceAzimuth = float(currPrm["sourceAzimuth"])
    
    get_speaker_layout(speakerLayout)
    channelConfig_data = f"channelConfigCase{testId}"
    samplingRate = currPrm["samplingRate"]
    smoothingTime = currPrm["smoothingTime"]
    strideInElements = currPrm["strideInElements"]
    strideOutElements = currPrm["strideOutElements"]
    testType = currPrm["testType"]
    demoCase = currPrm["demoCase"]
    outputDataLocation = currPrm["outputDataLocation"]
    numReps = currPrm["numReps"]
    numExecReps = currPrm["numExecReps"]
    nextAzimuth = currPrm["nextAzimuth"]
    changeFrame = currPrm["changeFrame"]
    C7100 = currPrm["C7100"]
    alignStride = currPrm["alignStride"]

    AUDIOLIB_dType = AUDIOLIB_utils.resolve_dType(dType)

    if testType in ["STATIC", "SINE", "SWEEP"]:
        in_data = f"staticRefInCase{testId}"
        out_data = f"staticRefOutCase{testId}"
        c_testType = "STATIC"
    else:
        in_data = "NULL"
        out_data = "NULL"
        c_testType = testType

    if C7100 == "FALSE":
        testParamsFile.write("#if !defined(__C7100__)\n")

    if demoCase == "true":
        testParamsFile.write(
            f"#if (defined(ALL_TEST_CASES) || (TEST_CASE == {int(testId)}) || defined(DEMO_CASE))\n"
        )
    else:
        testParamsFile.write(
            f"#if (defined(ALL_TEST_CASES) || (TEST_CASE == {int(testId)}))\n"
        )

    testParamsFile.write("{\n")
    testParamsFile.write(f"{c_testType}, // Test Pattern \n")
    testParamsFile.write(f"{in_data}, // staticIn \n")
    testParamsFile.write(f"{out_data}, // staticOut \n")
    testParamsFile.write(f"{channelConfig_data}, // speaker layout \n")
    testParamsFile.write(f"{AUDIOLIB_dType}, // Sample Data Type \n")
    testParamsFile.write(f"{numSamples}, // Samples per channel \n")
    testParamsFile.write(f"{numChannels}, // Number of Channels \n")
    testParamsFile.write(f"{currPrm['isInterleave']}, // isInterleave \n")
    testParamsFile.write(f"{sourceAzimuth}, // source angle \n")
    testParamsFile.write(f"{nextAzimuth}, // next azimuth angle to test the smoothing\n")
    testParamsFile.write(f"{changeFrame}, // after how many frame the nextAzimuth has to apply \n")
    testParamsFile.write(f"{samplingRate}, // Sampling Rate \n")
    testParamsFile.write(f"{smoothingTime}, // Smoothing Time \n")
    if alignStride == "FALSE":
        testParamsFile.write(f"{strideInElements} * sizeof({AUDIOLIB_dType}), // strideInElements \n")
        testParamsFile.write(f"{strideOutElements} * sizeof({AUDIOLIB_dType}), // strideOutElements \n")
    else:
        testParamsFile.write(f"AUDIOLIB_CALC_STRIDE({strideInElements} * sizeof({AUDIOLIB_dType}), AUDIOLIB_ALIGN_SHIFT_64BYTES), // strideInElements \n")
        testParamsFile.write(f"AUDIOLIB_CALC_STRIDE({strideOutElements} * sizeof({AUDIOLIB_dType}), AUDIOLIB_ALIGN_SHIFT_64BYTES), // strideOutElements \n")
    testParamsFile.write(f"{outputDataLocation}, // Output data location \n")
    testParamsFile.write(f"{int(numReps)}, // number of reps. \n")
    testParamsFile.write(f"{int(numExecReps)}, // numExecReps\n")
    testParamsFile.write(f"{int(testId)}, // test ID\n")
    testParamsFile.write("},\n")
    testParamsFile.write("#endif\n")

    if C7100 == "FALSE":
        testParamsFile.write("#endif // if !defined(__C7100__)\n")

    testParamsFile.write("\n\n")


def gen_idat_file(idatFile, testCases):
    with open("../common/ti_lice_header_idat.txt", "r", encoding="utf-8") as fIn:
        lines = fIn.readlines()
    
    idatFile.writelines(lines)
    idatFile.writelines('#include "AUDIOLIB_panning_idat.h"\n')
    idatFile.write("/*This file is autogenerated*/\n")
    idatFile.write("/*Please do not edit*/\n\n\n")

    for testCase in testCases:
        if testCase["testType"] in ["STATIC", "SINE", "SWEEP"]:
            AUDIOLIB_utils.gen_ifdefs(idatFile, testCase)

    idatFile.write("static AUDIOLIB_panning_testParams_t testParams[] =  {\n")

    for testCase in testCases:
        gen_testCaseParams(idatFile, testCase)

    idatFile.write("};\n\n")
    
    with open("test_param_function.txt", "r", encoding="utf-8") as fIn:
        lines = fIn.readlines()
    idatFile.writelines(lines)


def process_test_case(testCase):
    testCase["testId"] = int(testCase["ID"])
    testCase["numSamples"] = int(testCase["numSamples"])
    testCase["numChannels"] = int(testCase["numChannels"])
    testCase["sourceAzimuth"] = float(testCase["sourceAzimuth"])
    testCase["changeFrame"] = int(testCase["changeFrame"])
    testCase["nextAzimuth"] = float(testCase["nextAzimuth"] )
    testCase["speakerLayout"] = testCase["speakerLayout"].strip().upper()
    testCase["testType"] = testCase["testType"].strip().upper()
    testCase["numExecReps"] = int(testCase.get("numExecReps", 1))

    testCase["plotGraph"] = testCase.get("plotSignals", "false").lower() == "true"
    testCase["writeWav"] = testCase.get("waveFiles", "false").lower() == "true"
    
    speakerNames = get_speaker_layout(testCase["speakerLayout"])
    testCase["speakerNames"] = speakerNames

    testCase["samplingRate"] = int(testCase["samplingRate"])
    testCase["smoothingTime"] = float(testCase["smoothingTime"])
    testCase["strideInElements"] = int(testCase["strideInElements"])
    testCase["strideOutElements"] = int(testCase["strideOutElements"])
    testCase["isInterleave"] = int(testCase["isInterleave"])
    testCase["frequency"] = float(testCase["frequency"])
    testCase["compare"] = 0
    return testCase


def gen_test_case_header_file(testCase):
    testId = testCase["ID"]
    numSamples = testCase["numSamples"]
    numChannels = testCase["numChannels"]
    sourceAzimuth = testCase["sourceAzimuth"]
    samplingRate = testCase["samplingRate"]
    smoothingTime = testCase["smoothingTime"]
    testType = testCase["testType"]
    isInterleave = testCase["isInterleave"]
    numExecReps = testCase["numExecReps"]

    minVal, maxVal = -10, 10
    target_freq = testCase.get("frequency", 1000.0)

    if isInterleave:
        pInLocal = np.zeros((numExecReps, numSamples, 1), dtype=np.float32)
    else:
        pInLocal = np.zeros((numExecReps, 1, numSamples), dtype=np.float32)
        
    for frame in range(numExecReps):
        sample_idx = np.arange(numSamples) + frame * numSamples
        t = sample_idx / float(samplingRate)
        if testType == "SINE":
            mono_signal = np.sin(2.0 * np.pi * target_freq * t).astype(np.float32)
        elif testType == "SWEEP":
            total_duration = (numSamples * numExecReps) / float(samplingRate)
            f_start = 20.0
            f_end = target_freq if target_freq > 20.0 else 20000.0

            phase = 2.0 * np.pi * (
                f_start * t +
                ((f_end - f_start) / (2.0 * total_duration)) * (t ** 2)
            )
            mono_signal = np.sin(phase).astype(np.float32)
        else:
            mono_signal = np.random.uniform(
                low=minVal,
                high=maxVal,
                size=numSamples
            ).astype(np.float32)

        if isInterleave:
            pInLocal[frame, :, :] = mono_signal.reshape(numSamples, 1)
        else:
            pInLocal[frame, :, :] = mono_signal.reshape(1, numSamples)

    panning_processor = AUDIOLIB_panning(
        dType=testCase["dType"],
        numSamples=numSamples,
        numChannels=numChannels,
        sourceAzimuth=sourceAzimuth,
        nextAzimuth=testCase["nextAzimuth"],
        changeFrame = testCase["changeFrame"],
        samplingRate=samplingRate,
        smoothingTime=smoothingTime,
        speakerNames=testCase["speakerNames"],
        numExecReps=testCase["numExecReps"],
        isInterleave=isInterleave,
    )

    out_channels_list, gain_channels_list, speaker_layout, angle = panning_processor.exec(pInLocal)

    in_list = pInLocal.flatten().tolist()
    out_channels_list = np.array(out_channels_list).flatten().tolist()
    # gain_channels_list = np.array(gain_channels_list).flatten().tolist()
    testCase["speakerLayoutInt"] = speaker_layout
    testCase["speakerAngles"] = angle

    if testCase["plotGraph"]:
        azimuthList = [ testCase["sourceAzimuth"] , testCase["nextAzimuth"]]
        for i in range(gain_channels_list.shape[0]):
            plot_panning_results(testCase, gain_channels_list[i], i + 1 , azimuthList[i])
        plot_waveform_results(testCase, in_list, out_channels_list)

    if testCase["writeWav"]:
        wav_dir = os.path.join("wav_outputs", f"case_{testId}")
        input_signal = np.array(in_list, dtype=np.float32)
        output_signal = np.array(out_channels_list, dtype=np.float32)

        save_wav_file(os.path.join(wav_dir, f"input_case_{testId}.wav"), input_signal, samplingRate, 1, is_interleaved=(isInterleave == 1))
        save_wav_file(os.path.join(wav_dir, f"output_case_{testId}.wav"), output_signal, samplingRate, numChannels, is_interleaved=(isInterleave == 1))

    headerFileName = f"staticRefCase{testId}.h"
    file_io.write_header_file(
        in_data=in_list,
        out_data=out_channels_list,
        gains=gain_channels_list,
        testCase=testCase,
        headerFileName=headerFileName,
        frames=0,
    )

    return [np.min(out_channels_list), np.max(out_channels_list)]


def gen_test_case(testCase):
    if testCase["testType"] in ["STATIC", "SINE", "SWEEP"]:
        minY, maxY = gen_test_case_header_file(testCase)
        print(f"Test case generation completed for test ID {testCase['ID']} w/ min: {minY:f} and max: {maxY:f}")
    else:
        print(f"Skipping test case generation for test ID {testCase['ID']} as testType is {testCase['testType']}")
    
    headerFileName = f"staticRefCase{testCase['ID']}.h"
    file_io.write_speaker_angles(testCase, headerFileName)


testCasesCsvFile = "../AUDIOLIB_panning/test_cases_list.csv"
testIdList = list(range(0, 100))
myArgs = AUDIOLIB_utils.getCmdLineArgs()

with open(testCasesCsvFile, encoding="utf-8-sig") as csv_file:
    testCaseReader = csv.DictReader(csv_file)
    testCases = [process_test_case(row) for row in testCaseReader]

    for testCase in testCases:
        if myArgs.allCases or int(testCase["ID"]) in testIdList:
            gen_test_case(testCase)

idatFileName = "../../AUDIOLIB_panning/AUDIOLIB_panning_idat.c"

with open(idatFileName, "w", encoding="utf-8") as idatFile:
    gen_idat_file(idatFile, testCases)

if platform in ["linux", "linux2"]:
    call(["clang-format", "-style=file", "-i", idatFileName])
    call(["indent", "-nut", "-i3", "-c55", "-l200", idatFileName])
    if os.path.exists(idatFileName + "~"):
        os.remove(idatFileName + "~")

print("Idat file generation completed")
