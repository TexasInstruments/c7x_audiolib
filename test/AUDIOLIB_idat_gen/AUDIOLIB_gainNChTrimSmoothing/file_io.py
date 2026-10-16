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

def write_header_file(in_data, out_data, currentGain_data, targetGain_data, testCase, headerFileName):
    """Write 3D input/output data and 1D per-channel gain data to a C header file.

    Args:
        in_data (np.ndarray): 3D flattened/unflattened input data.
        out_data (np.ndarray): 3D flattened/unflattened output data.
        currentGain_data (list/np.ndarray): per-channel gain the fade starts from.
        targetGain_data (list/np.ndarray): per-channel target gain (before masterGain).
        testCase (dict): Test case parameters.
        headerFileName (str): Name of the header file to write.
    """
    testId = testCase["ID"]
    inSamples = testCase["inSamples"]
    inChannels = testCase["inChannels"]
    dType = testCase["dType"]
    isInterleaved = testCase["isInterleaved"]
    frames = testCase["numExecReps"]
    totalLength = inSamples * inChannels * frames
    
    attributes = '__attribute__((section(".staticData"))) static '
    outputDir = "../../AUDIOLIB_gainNChTrimSmoothing/test_data/"

    # Determine format based on data type
    sample_dtype = np.asarray(in_data).dtype
    # Enough digits to round-trip exactly, so C reads the values the reference was computed from.
    fmt = "{:.9e}f" if sample_dtype == np.float32 else "{:.17e}"

    # Ensure numpy arrays and handle 3D data safely
    in_data = np.asarray(in_data)
    out_data = np.asarray(out_data)
    
    in_flat = in_data.flatten()
    out_flat = out_data.flatten()

    if not os.path.exists(outputDir):
        os.makedirs(outputDir)
        
    filepath = os.path.join(outputDir, headerFileName)
    
    with open(filepath, "w") as f:
        # Define data layout description
        layout_desc = "[ch0_s0, ch1_s0, ch2_s0, ...]" if isInterleaved else "[ch0_s0, ch0_s1, ch0_s2, ..., ch1_s0, ch1_s1, ...]"
        
        # --- 1. Write Input Data ---
        write_attributes(f, attributes, f"{dType} staticRefInCase{testId}[{totalLength}] =\n")
        f.write(f"    /* Frame-based input: {frames} frames, {inChannels} channels, {inSamples} samples per channel per frame, {'interleaved' if isInterleaved else 'non-interleaved'} */\n")
        f.write(f"    /* Data layout: {layout_desc} */\n")

        for frame in range(frames):
            f.write(f"    /* ********Frame {frame} ********* */\n")
            frame_offset = frame * inChannels * inSamples
            
            if isInterleaved:
                for s in range(inSamples):
                    start = frame_offset + (s * inChannels)
                    end = start + inChannels
                    line = ", ".join(fmt.format(x) for x in in_flat[start:end])
                    f.write(f"    {line},\n")
            else:
                for c in range(inChannels):
                    start = frame_offset + (c * inSamples)
                    end = start + inSamples
                    line = ", ".join(fmt.format(x) for x in in_flat[start:end])
                    f.write(f"    {line},\n")         
            
        f.write("};\n\n")

        # --- 2. Write Output Data ---
        write_attributes(f, attributes, f"{dType} staticRefOutCase{testId}[{totalLength}] =\n")
        f.write(f"    /* Frame-based output: {frames} frames, {inChannels} channels, {inSamples} samples per channel per frame, {'interleaved' if isInterleaved else 'non-interleaved'} */\n")
        f.write(f"    /* Data layout: {layout_desc} */\n")
    
        for frame in range(frames):
            f.write(f"    /* Frame {frame} */\n")
            frame_offset = frame * inChannels * inSamples
            
            if isInterleaved:
                for s in range(inSamples):
                    start = frame_offset + (s * inChannels)
                    end = start + inChannels
                    line = ", ".join(fmt.format(x) for x in out_flat[start:end])
                    f.write(f"    {line},\n")
            else:
                for c in range(inChannels):
                    start = frame_offset + (c * inSamples)
                    end = start + inSamples
                    line = ", ".join(fmt.format(x) for x in out_flat[start:end])
                    f.write(f"    {line},\n")        
        f.write("};\n\n")

        # --- Helper for 1D Gain Arrays ---
        def write_gain_array(name, data, description):
            if data is not None:
                gain_arr = np.asarray(data).flatten()
                write_attributes(f, attributes, f"{dType} {name}Case{testId}[{len(gain_arr)}] =\n")
                f.write(f"    /* {description} */\n")
                gain_lines = ", ".join(fmt.format(x) for x in gain_arr)
                f.write(f"    {gain_lines},\n")
                f.write("};\n\n")

        # --- 3. Write Gain Data (1D) ---
        write_gain_array("currentGain", currentGain_data, "currentGain array: one value per channel, gain the fade starts from")
        write_gain_array("targetGain", targetGain_data, "targetGain array: one value per channel, applied as masterGain * targetGain")

    print(f"Successfully generated header file: {headerFileName}")


# Light chart theme (dataviz reference palette): surface, text, muted, grid, categorical slots 1-4.
PLOT_STYLE = {
    "surface": "#fcfcfb", "text": "#0b0b0b", "textSecondary": "#52514e",
    "muted": "#898781", "grid": "#e1e0d9",
    "series": ["#2a78d6", "#eb6834", "#1baf7a", "#eda100"],
}
# Lowest level shown on dB axes; a gain of 0 is drawn here instead of -inf.
PLOT_DB_FLOOR = -120.0


def apply_plot_style(ax):
    """Recessive grid and axes on the light surface."""
    ax.set_facecolor(PLOT_STYLE["surface"])
    ax.grid(True, color=PLOT_STYLE["grid"], linewidth=0.8)
    ax.tick_params(colors=PLOT_STYLE["textSecondary"], labelsize=9)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(PLOT_STYLE["muted"])


def gain_to_db(gain):
    return np.maximum(20.0 * np.log10(np.maximum(np.asarray(gain, dtype=np.float64), 1e-30)), PLOT_DB_FLOOR)


def plot_gain_curves(gains, testCase, outputDir="plots", maxChannels=8):
    """Plot the per-sample gain of each channel (one small panel per channel, in dB).

    Args:
        gains (np.ndarray): applied gain as [numFrames, numChannels, numSamples]
        testCase (dict): Test case parameters
        outputDir (str): Directory the PNG is written to
        maxChannels (int): At most this many channels are drawn (4 per row)
    """
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.lines import Line2D

    S = PLOT_STYLE
    testId = testCase["ID"]
    fs = float(testCase["samplingRate"])
    fadeType = testCase["fadeType"]
    fadeTime = float(testCase["fadeTime"])
    numFrames, numChannels, numSamples = gains.shape
    channels = list(range(min(numChannels, maxChannels)))
    start = np.array(testCase["initialGain"], dtype=np.float32)
    target = np.float32(testCase["masterGain"]) * np.array(testCase["trimGain"], dtype=np.float32)
    timeMs = np.arange(numFrames * numSamples) / fs * 1000.0
    frameMs = numSamples / fs * 1000.0
    tauSamples = fs * fadeTime / 1000.0
    color = S["series"][0] if fadeType == "SYMMETRIC" else S["series"][1]

    # A SMOOTH fade is essentially over after a few time constants; zoom so its shape is readable.
    xMax = timeMs[-1]
    if fadeType == "SMOOTH" and fadeTime > 0:
        xMax = min(xMax, 8.0 * fadeTime)
    numShown = int(np.searchsorted(timeMs, xMax, side="right"))

    def fmt_gain(g):
        return "0" if g == 0 else f"{g:.3g}"

    def fmt_db(g):
        return "-inf dB" if g == 0 else f"{20 * np.log10(g):+.1f} dB"

    ncols = min(4, len(channels))
    nrows = -(-len(channels) // ncols)
    fig, axes = plt.subplots(nrows, ncols, figsize=(3.3 * ncols + 0.4, 2.6 * nrows + 1.7), sharex=True,
                             squeeze=False, facecolor=S["surface"])
    for i, ax in enumerate(axes.flat):
        if i >= len(channels):
            ax.set_visible(False)
            continue
        ch = channels[i]
        full = gains[:, ch, :].reshape(-1)
        y = gain_to_db(full[:numShown])
        changed = not np.isclose(target[ch], start[ch], rtol=1e-6)
        apply_plot_style(ax)
        ax.grid(True, axis="y", color=S["grid"], linewidth=0.8)
        ax.grid(False, axis="x")

        # Frame boundaries: the fade carries on across them.
        for f in range(1, numFrames):
            if f * frameMs < xMax:
                ax.axvline(f * frameMs, color=S["grid"], linewidth=1.2, zorder=0)
        if target[ch] > 0:
            ax.axhline(gain_to_db(target[ch]), color=S["muted"], linewidth=1.0, linestyle="--", zorder=1)
        ax.plot(timeMs[:numShown], y, color=color, linewidth=2.0, zorder=3)
        ax.plot(timeMs[0], y[0], marker="o", markersize=6, color=color, markeredgecolor=S["surface"],
                markeredgewidth=1.5, zorder=4)

        if changed and fadeType == "SYMMETRIC":
            notOnTarget = np.nonzero(full != target[ch])[0]
            land = int(notOnTarget[-1]) + 1 if notOnTarget.size else 0
            if land < full.size:
                ax.axvline(timeMs[land], color=S["textSecondary"], linewidth=1.0, linestyle=":", zorder=2)
                ax.annotate(f"lands at {timeMs[land]:.4g} ms", (timeMs[land], 1.0), xycoords=("data", "axes fraction"),
                            xytext=(-4, -2), textcoords="offset points", ha="right", va="top",
                            fontsize=8.5, color=S["textSecondary"])
        if changed and fadeType == "SMOOTH" and fadeTime > 0:
            for multiple, text in ((1, "63 %"), (5, "99 %")):
                k = int(round(multiple * tauSamples)) - 1
                if 0 <= k < numShown:
                    ax.plot(timeMs[k], y[k], marker="o", markersize=5, color=S["text"], zorder=5)
                    rising = target[ch] > start[ch]
                    ax.annotate(text, (timeMs[k], y[k]), xytext=(4, -10 if rising else 6), textcoords="offset points",
                                fontsize=8.5, color=S["text"])

        # Keep flat or nearly flat channels from looking noisy.
        lo, hi = float(np.min(y)), float(np.max(y))
        if target[ch] > 0:
            lo, hi = min(lo, float(gain_to_db(target[ch]))), max(hi, float(gain_to_db(target[ch])))
        pad = max(0.12 * (hi - lo), 1.0)
        ax.set_ylim(lo - pad, hi + pad)
        ax.set_xlim(0, xMax)

        if changed:
            title = f"ch{ch}:  {fmt_gain(start[ch])} → {fmt_gain(target[ch])}"
            subtitle = f"{fmt_db(start[ch])} → {fmt_db(target[ch])}"
        else:
            title = f"ch{ch}:  {fmt_gain(start[ch])}  (unchanged)"
            subtitle = fmt_db(start[ch])
        ax.set_title(title, loc="left", fontsize=10, color=S["text"], pad=16)
        ax.text(0, 1.02, subtitle, transform=ax.transAxes, fontsize=8.5, color=S["textSecondary"], va="bottom")
        if i % ncols == 0:
            ax.set_ylabel("gain (dB)", color=S["textSecondary"], fontsize=9)
        if i // ncols == nrows - 1 or i + ncols >= len(channels):
            ax.set_xlabel("time (ms)", color=S["textSecondary"], fontsize=9)
            ax.tick_params(labelbottom=True)

    if fadeType == "SYMMETRIC":
        meaning = "fadeTime is the ramp duration"
        caption = ("Every channel that changes lands on its target exactly fadeTime after set(), on a straight line in dB. "
                   "Light vertical lines are frame boundaries: the fade carries on across frames.")
        marker = Line2D([], [], color=S["textSecondary"], linewidth=1.0, linestyle=":", label="fade lands")
    else:
        meaning = "fadeTime is the time constant"
        caption = ("Each channel covers 63 % of its change after 1 x fadeTime and 99 % after 5 x fadeTime, and never "
                   "lands exactly. Light vertical lines are frame boundaries: the smoothing carries on across frames.")
        marker = Line2D([], [], color=S["text"], marker="o", markersize=5, linestyle="none", label="63 % / 99 % points")
    shown = []
    if numChannels > len(channels):
        shown.append(f"first {len(channels)} of {numChannels} channels")
    if xMax < timeMs[-1]:
        shown.append(f"first {xMax:g} of {timeMs[-1]:.0f} ms")

    fig.text(0.012, 0.985, f"Case {testId}: {fadeType} fade, {meaning} ({fadeTime:g} ms)", fontsize=13,
             color=S["text"], va="top", fontweight="semibold")
    fig.text(0.012, 0.985 - 0.32 / fig.get_figheight(),
             f"fs {fs:g} Hz  ·  {numFrames} frames x {numSamples} samples ({frameMs:.3g} ms each)  ·  "
             f"masterGain {testCase['masterGain']:g}" + (f"  ·  {', '.join(shown)}" if shown else ""),
             fontsize=9.5, color=S["textSecondary"], va="top")
    handles = [Line2D([], [], color=color, linewidth=2.0, label="applied gain"),
               Line2D([], [], color=S["muted"], linewidth=1.0, linestyle="--", label="target = masterGain x trimGain"),
               Line2D([], [], color=color, marker="o", markersize=6, linestyle="none", label="start (initialGain)"),
               marker]
    fig.legend(handles=handles, loc="upper right", bbox_to_anchor=(0.99, 0.995), ncol=2, frameon=False, fontsize=8.5,
               labelcolor=S["text"])
    fig.text(0.012, 0.012, caption, fontsize=8.5, color=S["textSecondary"], va="bottom", wrap=True)
    top = 1.0 - 0.8 / fig.get_figheight()
    bottom = 0.55 / fig.get_figheight()
    fig.tight_layout(rect=(0, bottom, 1, top), h_pad=2.2, w_pad=1.6)
    os.makedirs(outputDir, exist_ok=True)
    path = os.path.join(outputDir, f"case_{testId}_gain_{fadeType.lower()}.png")
    fig.savefig(path, dpi=140, facecolor=S["surface"])
    plt.close(fig)
    print(f"Saved gain plot: {path}")
