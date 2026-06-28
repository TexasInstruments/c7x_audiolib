<div align="center">

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://www.ti.com/content/dam/ticom/images/identities/ti-brand/ti-logo-hz-1c-white.svg" width="300">
  <img alt="Texas Instruments Logo" src="https://www.ti.com/content/dam/ticom/images/identities/ti-brand/ti-hz-2c-pos-rgb.svg" width="300">
</picture>

# AUDIOLIB — C7<sup>™</sup> Audio Processing Library

[Summary](#summary) | [Features](#features) | [Operators](#operators) | [Supported Devices](#supported-devices) | [Setup Instructions](#setup-instructions) | [Build Instructions](#build-instructions) | [Related Repos](#related-repos) | [Licensing](#licensing) | [Contributions](#contributions) | [Developer Resources](#developer-resources)

</div>

## Summary

AUDIOLIB is TI's optimized audio processing library for the **C7<sup>™</sup>** DSP architecture.
It provides a suite of kernels targeting the C7<sup>™</sup> vector and streaming engines (SE/SA),
enabling high-throughput audio signal processing on TI SoCs such as AM62D and AM275.
The Sample Rate Converter (`AUDIOLIB_ssrc`) additionally leverages the Matrix Multiply Accelerator (MMA)
for maximum throughput.

Each kernel ships in two variants selected at init time via the `AUDIOLIB_FUNCTION_OPT` /
`AUDIOLIB_FUNCTION_NATC` enum in the kernel's `InitArgs`:

| Suffix | Description |
|--------|-------------|
| `_cn` | C-native reference (`NATC`) — portable, functionally correct, used for validation |
| `_ci` | C7<sup>™</sup> intrinsic (`OPT`) — hardware-optimized, targets SE/SA streaming engines |

Each kernel has a corresponding test driver (`_d.c`). Some kernels additionally include a Python
reference implementation under `test/AUDIOLIB_idat_gen/AUDIOLIB_<kernel>/` for test vector generation.


## Features

- **25+ kernels** spanning gain control, level conversion, audio effects, routing, sample rate conversion, and synthesis
- **C7<sup>™</sup> streaming engine optimized** — leverages SE/SA hardware for peak throughput across all kernels
- **MMA acceleration** — `AUDIOLIB_ssrc` employs the Matrix Multiply Accelerator; all other kernels use C7<sup>™</sup> vector/streaming engines only
- **Portable reference implementations** — `_cn` variants enable host-side validation and porting
- **CMake-based build** with presets for PC simulation and on-target deployment
- **Cortex-A53 support** — `AUDIOLIB_asrc` and `AUDIOLIB_ssrc` include Cortex-A53 optimized implementations (AM62D only); all other kernels target the C7™ DSP exclusively


## Operators

### Gain & Level
| Kernel | Description |
|--------|-------------|
| `AUDIOLIB_gain` | Apply scalar gain to a single audio channel |
| `AUDIOLIB_gainNCh` | Apply gain to multiple audio channels |
| `AUDIOLIB_gainNChTrim` | Multi-channel gain with trim adjustment |
| `AUDIOLIB_balance` | Left/right channel balance control |
| `AUDIOLIB_balanceFader` | Balance control with fader |
| `AUDIOLIB_mute` | Mute a single audio channel |
| `AUDIOLIB_muteNCh` | Mute multiple audio channels |

### Conversion & Measurement
| Kernel | Description |
|--------|-------------|
| `AUDIOLIB_dB10` | Convert linear power ratio to dB (10·log₁₀) |
| `AUDIOLIB_dB20` | Convert linear amplitude ratio to dB (20·log₁₀) |
| `AUDIOLIB_undB10` | Convert dB to linear power ratio |
| `AUDIOLIB_undB20` | Convert dB to linear amplitude ratio |
| `AUDIOLIB_typeConversion` | Convert between audio sample data types |
| `AUDIOLIB_blockStatistics` | Compute statistics (min, max, RMS) over a block |
| `AUDIOLIB_subBlockStatistics` | Compute statistics over sub-blocks |

### Effects & Processing
| Kernel | Description |
|--------|-------------|
| `AUDIOLIB_delay` | Single-channel audio delay line |
| `AUDIOLIB_delayNChannel` | Multi-channel audio delay line |
| `AUDIOLIB_crossfade` | Smooth crossfade between two audio signals |
| `AUDIOLIB_softClip` | Soft clipping / limiting |
| `AUDIOLIB_nlms` | Normalized LMS adaptive filter |

### Routing & Mixing
| Kernel | Description |
|--------|-------------|
| `AUDIOLIB_router` | Route audio channels to configurable outputs |
| `AUDIOLIB_inputAggregator` | Aggregate multiple input streams into one |

### Sample Rate & Synthesis
| Kernel | MMA | Description |
|--------|-----|-------------|
| `AUDIOLIB_asrc` | — | Adaptive Sample Rate Converter — A53 and C7™ optimized |
| `AUDIOLIB_ssrc` | Yes | Sample Rate Converter — MMA accelerated, A53 and C7™ optimized |
| `AUDIOLIB_sinusoidGenerator` | — | Generate a sinusoidal test tone |
| `AUDIOLIB_whiteNoiseGenerator` | — | Generate white noise |

### Lookup & Interpolation
| Kernel | Description |
|--------|-------------|
| `AUDIOLIB_tableInterpolate` | Interpolated table lookup |
| `AUDIOLIB_tableLookup` | Direct table lookup |


## Supported Devices

| Device | C7<sup>™</sup> Core | MMA Version | ARM Core |
|--------|-----|-------------|----------|
| AM62D | C7504 | MMAv2 | Cortex-A53 |
| AM275 | C7524 | MMAv2f (adds 32-bit float support) | — |

> MMA is used only by `AUDIOLIB_ssrc`. All other kernels run on the C7<sup>™</sup> SE/SA engines.


## Setup Instructions

### Prerequisites

- **Linux** host (Ubuntu 20.04 or later recommended)
- **CMake** ≥ 3.21
- **Python 3** with [West](https://docs.zephyrproject.org/latest/develop/west/index.html) — `pip install west`
- **TI C7000 Code Generation Tools** (`ti-cgt-c7000`) — install to `~/ti/`

  Download from [TI CGT C7000](https://www.ti.com/tool/C7000-CGT).
  Default expected path: `~/ti/ti-cgt-c7000_5.0.0.LTS/`

  Or set the environment variable before building:
  ```bash
  export CGT7X_ROOT=/path/to/ti-cgt-c7000_<version>
  ```


## Build Instructions

AUDIOLIB uses **West** (`west build-audiolib`) as the recommended build driver.
West orchestrates fetching dependencies (dsplib) and invoking CMake presets automatically.
Raw CMake preset invocations are also supported for advanced workflows.

### West Workspace Setup (First Time)

```bash
pip install west

# Initialize workspace with AUDIOLIB as the manifest repository
west init -m https://github.com/TexasInstruments/c7x_audiolib --mr main audiolib_workspace
cd audiolib_workspace

# Fetch dependencies (dsplib)
west update
```

### West Build — Common Commands

`west build-audiolib` fetches dsplib, builds it, and builds AUDIOLIB in a single command —
no separate dependency management step needed after `west update`.

```bash
# PC simulation, AM62D (default: buildlib, release, all platforms)
west build-audiolib --soc am62d --platform pc

# PC simulation, AM275
west build-audiolib --soc am275 --platform pc

# On-target build
west build-audiolib --soc am62d --platform target

# Autotest build (includes test binaries)
west build-audiolib --soc am62d --platform pc --type autotest

# Full clean rebuild
west build-audiolib --soc am62d --platform pc --clean

# Debug build
west build-audiolib --soc am62d --platform pc --build-type debug

# Parallel jobs
west build-audiolib --soc am62d --platform pc -j8
```

### Dependency Resolution (Direct CMake Invocation)

When invoking CMake directly (not via `west build-audiolib`), dsplib must be locatable.
CMake checks these locations in order:

| Priority | Location | Notes |
|----------|----------|-------|
| 1 | `<workspace>/dsplib/` | Flat sibling directory |
| 2 | `$DSPLIB_ROOT` env var | Explicit override |

For **library-only builds** (`release-buildlib-*` presets), only `dsplib.h` is required — no
pre-built artifact needed. For **test/example builds** (`release-autotest-*`), the pre-built
dsplib static library must exist under `dsplib/lib/<build_type>/`.

```bash
# Option A: flat sibling (manual clone)
git clone https://github.com/TexasInstruments/c7x_dsplib.git dsplib
cmake -S audiolib -B build --preset=release-buildlib-am62d-pc

# Option B: explicit env var
export DSPLIB_ROOT=/path/to/dsplib
cmake -S . -B build --preset=release-buildlib-am62d-pc
```

### CMake Presets (Advanced)

AUDIOLIB uses **CMake presets** to manage build configurations.

### Available Presets

| Preset | SoC | Platform |
|--------|-----|----------|
| `release-autotest-am62d-pc` | AM62D | PC simulation + tests |
| `release-autotest-am62d-target` | AM62D | On-device + tests |
| `release-autotest-am275-pc` | AM275 | PC simulation + tests |
| `release-autotest-am275-target` | AM275 | On-device + tests |
| `release-buildlib-am62d-pc` | AM62D | PC (library only) |
| `release-buildlib-am62d-target` | AM62D | On-device (library only) |
| `release-buildlib-am275-pc` | AM275 | PC (library only) |
| `release-buildlib-am275-target` | AM275 | On-device (library only) |

### Quick Start — PC simulation (AM62D)

```bash
# Configure
cmake -S . -B build --preset=release-autotest-am62d-pc

# Build
cmake --build build -- -j$(nproc)

# Build test binaries (ctest not supported; run binaries directly on target)
cmake --build build -- -j$(nproc)
```

### On-Target Build (AM62D)

```bash
cmake -S . -B build --preset=release-autotest-am62d-target
cmake --build build -- -j$(nproc)
```

### Library-Only Build

```bash
cmake -S . -B build --preset=release-buildlib-am62d-pc
cmake --build build -- -j$(nproc)
```


## Related Repos

- [MCU+ SDK](https://github.com/TexasInstruments/mcupsdk-core) — SoC drivers and middleware for AM2x/AM6x devices


## Licensing

This repository is licensed under the **Apache License, Version 2.0**.
See [LICENSE](LICENSE) for the full text.

All source files carry an SPDX `Apache-2.0` identifier.


## Contributions

This repository is not currently accepting community contributions.

Bug reports and feature requests are welcome via [TI E2E Community Forums](https://e2e.ti.com).


---

## Developer Resources

[TI E2E™ design support forums](https://e2e.ti.com)
