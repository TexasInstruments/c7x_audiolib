# Copyright (C) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

"""west_ext.py — audiolib West extension commands.

Adds:
  west build-audiolib   build dsplib dependency then audiolib

Build philosophy
----------------
dsplib is built explicitly by this script before invoking audiolib cmake.
audiolib cmake is a pure consumer — it expects pre-built dsplib artifacts at
workspace/dsplib/lib/{build_type}/ and uses if(EXISTS) to locate them.

Supports both workspace layouts via _dep_root() probing:
  Standalone : workspace/audiolib/,  workspace/dsplib/
  Embedded   : workspace/xlib/audiolib/, workspace/xlib/dsplib/

Parallelisation (per-combo threads)
-------------------------------------
All combos run in parallel via ThreadPoolExecutor.  Within each thread,
dsplib → audiolib sequentially.  At peak, up to 16 cmake processes live.

  Thread 0  [am62d/pc/release]     dsplib ──► audiolib
  ...
  Thread 7  [am275/target/debug]   dsplib ──► audiolib

subprocess.run() releases the GIL; all threads run in true parallel.

SOC discovery is fully dynamic — parses CMakePresets.json.

Terminal output
---------------
Set NERD_FONTS=1 to enable nerd font icons.
Colours are auto-disabled when stdout is not a TTY.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from textwrap import dedent

from west.commands import WestCommand

# ── header generation lock (autotest only) ───────────────────────────────────
# gen_all_test_case_headers writes to the source tree — only needs to run once
# across all parallel combos.  First combo to arrive generates; others skip.
_headers_lock = threading.Lock()
_headers_done = False

# ── terminal aesthetics ───────────────────────────────────────────────────────

_TTY = sys.stdout.isatty()


def _detect_nerd():
    """Auto-detect nerd font availability.

    Priority:
      1. NERD_FONTS=1/0  — explicit user override always wins
      2. Terminal env vars — kitty, wezterm, ghostty, windows terminal
      3. fc-list          — checks if a nerd font is installed on the system
    """
    val = os.environ.get("NERD_FONTS", "").lower()
    if val in ("1", "true"):
        return True
    if val in ("0", "false"):
        return False

    # Known nerd-font-friendly terminals
    if os.environ.get("KITTY_WINDOW_ID"):
        return True
    if os.environ.get("TERM_PROGRAM") == "WezTerm":
        return True
    if os.environ.get("WT_SESSION"):
        return True  # Windows Terminal
    if os.environ.get("TERM") in ("xterm-kitty", "xterm-ghostty"):
        return True

    # fc-list: nerd fonts installed on this system?
    try:
        out = subprocess.run(
            ["fc-list", "--format=%{family}\n"],
            capture_output=True,
            text=True,
            timeout=2,
        ).stdout
        if any("nerd" in ln.lower() for ln in out.splitlines()):
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass

    return False


_NERD = _detect_nerd()


def _c(code):
    return f"\033[{code}m" if _TTY else ""


C_RESET = _c(0)
C_BOLD = _c(1)
C_DIM = _c(2)
C_GREEN = _c(32)
C_RED = _c(31)
C_CYAN = _c(36)
C_YELLOW = _c(33)

I_BUILD = "\uf0ad " if _NERD else "⟳ "
I_DONE = "\uf00c " if _NERD else "✓ "
I_SKIP = "\uf05e " if _NERD else "⊘ "
I_FAIL = "\uf00d " if _NERD else "✗ "
I_PKG = "\uf487 " if _NERD else "◈ "


class _Printer:
    """Thread-safe status printer with icons, colours, and elapsed times."""

    def __init__(self):
        self._lock = threading.Lock()
        self._timers = {}

    def _ts(self, label):
        return time.monotonic() - self._timers.get(label, time.monotonic())

    def building(self, label):
        with self._lock:
            self._timers[label] = time.monotonic()
            print(f"  {C_CYAN}{I_BUILD}{C_RESET}{label}", flush=True)

    def done(self, label):
        with self._lock:
            print(
                f"  {C_GREEN}{I_DONE}{C_RESET}{label}  "
                f"{C_DIM}({self._ts(label):.1f}s){C_RESET}",
                flush=True,
            )

    def skipped(self, label):
        with self._lock:
            print(f"  {C_DIM}{I_SKIP}{label} — cached{C_RESET}", flush=True)

    def failed(self, label, log=None):
        with self._lock:
            log_hint = f"  {C_DIM}→ {log}{C_RESET}" if log else ""
            print(
                f"  {C_RED}{I_FAIL}{C_RESET}{label}  "
                f"{C_RED}FAILED{C_RESET}{log_hint}",
                flush=True,
            )

    def section(self, title):
        with self._lock:
            bar = "─" * (len(title) + 4)
            print(f"\n  ┌{bar}┐")
            print(f"  │  {C_BOLD}{title}{C_RESET}  │")
            print(f"  └{bar}┘", flush=True)


def _summary(title, combos, failed_labels, total_elapsed):
    n_ok = len(combos) - len(failed_labels)
    n_total = len(combos)
    overall = (
        f"{C_GREEN}{I_DONE.strip()}{C_RESET}"
        if not failed_labels
        else f"{C_RED}{I_FAIL.strip()}{C_RESET}"
    )

    c_lib = len(title)
    c_soc = max(len(s) for s, _, _ in combos)
    c_plat = max(len(p) for _, p, _ in combos)
    c_type = max(len(t) for _, _, t in combos)

    div = (
        f'  ├{"─"*(c_lib+2)}┼{"─"*(c_soc+2)}┼{"─"*(c_plat+2)}┼{"─"*(c_type+2)}┼{"─"*5}┤'
    )
    top = (
        f'  ┌{"─"*(c_lib+2)}┬{"─"*(c_soc+2)}┬{"─"*(c_plat+2)}┬{"─"*(c_type+2)}┬{"─"*5}┐'
    )
    bot = (
        f'  └{"─"*(c_lib+2)}┴{"─"*(c_soc+2)}┴{"─"*(c_plat+2)}┴{"─"*(c_type+2)}┴{"─"*5}┘'
    )

    print(
        f"\n  {C_BOLD}{title}{C_RESET}  {overall}  "
        f"{C_GREEN}{n_ok}{C_RESET}/{n_total} combos  "
        f"{C_DIM}({total_elapsed:.1f}s){C_RESET}"
    )
    print(top)
    print(
        f'  │ {"Library":<{c_lib}} │ {"SOC":<{c_soc}} │ {"Platform":<{c_plat}} │ {"Type":<{c_type}} │  {C_BOLD}  {C_RESET} │'
    )
    print(div)
    for soc, plat, btype in combos:
        combo_label = f"{soc}/{plat}/{btype}"
        is_failed = any(combo_label in f for f in failed_labels)
        icon = (
            f"{C_RED}{I_FAIL.strip()}{C_RESET}"
            if is_failed
            else f"{C_GREEN}{I_DONE.strip()}{C_RESET}"
        )
        print(
            f"  │ {title:<{c_lib}} │ {soc:<{c_soc}} │ {plat:<{c_plat}} │ {btype:<{c_type}} │  {icon}  │"
        )
    print(bot, flush=True)


# ── soc discovery ─────────────────────────────────────────────────────────────


def _soc_map(audiolib_dir: Path) -> dict:
    """Parse CMakePresets.json — returns {soc: {device, platforms[]}}."""
    presets_file = audiolib_dir / "CMakePresets.json"
    with open(presets_file) as f:
        presets_data = json.load(f)

    all_presets = {p["name"]: p for p in presets_data.get("configurePresets", [])}

    def resolve(preset_name, var):
        visited, queue = set(), [preset_name]
        while queue:
            name = queue.pop(0)
            if name in visited:
                continue
            visited.add(name)
            p = all_presets.get(name, {})
            if var in p.get("cacheVariables", {}):
                return p["cacheVariables"][var]
            inherits = p.get("inherits", [])
            queue.extend([inherits] if isinstance(inherits, str) else inherits)
        return None

    pattern = re.compile(r"^(?:release|debug)-buildlib-(\w+)-(pc|target)$")
    soc_map = {}
    for p in presets_data.get("configurePresets", []):
        if p.get("hidden", False):
            continue
        m = pattern.match(p["name"])
        if not m:
            continue
        soc, platform = m.group(1), m.group(2)
        device = resolve(p["name"], "DEVICE")
        if device is None:
            continue
        if soc not in soc_map:
            soc_map[soc] = {"device": device, "platforms": []}
        if platform not in soc_map[soc]["platforms"]:
            soc_map[soc]["platforms"].append(platform)
    return soc_map


# ── west command ──────────────────────────────────────────────────────────────


class BuildAudiolib(WestCommand):
    """west build-audiolib: build dsplib then audiolib, all combos in parallel."""

    def __init__(self):
        super().__init__(
            name="build-audiolib",
            help="build dsplib then audiolib for all combos in parallel",
            description=dedent("""\
                Parallel per-combo build — dsplib → audiolib per thread.
                SOCs and platforms discovered automatically from CMakePresets.json.
                Set NERD_FONTS=1 for enhanced terminal icons.
            """),
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description,
            formatter_class=argparse.RawDescriptionHelpFormatter,
        )
        parser.add_argument("--soc", nargs="+", default=None)
        parser.add_argument(
            "--platform", choices=["pc", "target", "all"], default="all"
        )
        parser.add_argument(
            "--build-type",
            choices=["release", "debug", "all"],
            default="all",
            dest="build_type",
        )
        parser.add_argument(
            "--type",
            choices=["buildlib", "autotest"],
            default="buildlib",
            dest="preset_type",
        )
        parser.add_argument("--preset", default=None)
        parser.add_argument("--build-dir", default=None, dest="build_dir")
        parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
        parser.add_argument("--fresh", action="store_true", default=False)
        parser.add_argument(
            "--clean",
            action="store_true",
            default=False,
            help="delete build directories before building (full rebuild)",
        )
        parser.add_argument(
            "--no-throttle",
            action="store_false",
            default=True,
            dest="throttle",
            help="disable load-average throttling (-l{ncpus})",
        )
        return parser

    # ── helpers ───────────────────────────────────────────────────────────────

    def _dep_root(self, name):
        for candidate in [Path(self.topdir) / name, Path(self.topdir) / "xlib" / name]:
            if candidate.is_dir():
                return candidate
        return Path(self.topdir) / name

    def _dep_artifact(self, name, device, platform, build_type):
        dep = self._dep_root(name)
        if platform == "pc":
            suffix = ".lib" if sys.platform == "win32" else ".a"
            return dep / "lib" / build_type / f"{name.upper()}_{device}_x86_64{suffix}"
        else:
            return dep / "lib" / build_type / f"{name.upper()}_{device}.lib"

    def _run_chain(self, *cmds, log=None):
        """Run commands sequentially, capturing output to log file if given."""
        fh = open(log, "w") if log else None
        try:
            for cmd in cmds:
                if fh:
                    fh.write(f'$ {" ".join(str(c) for c in cmd)}\n')
                    fh.flush()
                rc = subprocess.run(cmd, stdout=fh, stderr=fh).returncode
                if rc != 0:
                    return rc
            return 0
        finally:
            if fh:
                fh.close()

    # ── per-combo worker ──────────────────────────────────────────────────────

    def _build_combo(
        self, soc, platform, device, build_type, args, audiolib_dir, printer
    ):
        label = f"{soc}/{platform}/{build_type}"

        # ── dsplib ────────────────────────────────────────────────────────────
        artifact = self._dep_artifact("dsplib", device, platform, build_type)
        dsp_label = f"{I_PKG.strip()} dsplib    {label}"
        log_dir = audiolib_dir / "build" / "logs" / soc / platform / build_type
        log_dir.mkdir(parents=True, exist_ok=True)

        if artifact.exists():
            printer.skipped(dsp_label)
        else:
            dep_src = self._dep_root("dsplib")
            dep_build = (
                audiolib_dir / "build" / "deps" / "dsplib" / soc / platform / build_type
            )
            preset = f"{build_type}-buildlib-{soc}-{platform}"
            log = log_dir / "dsplib.log"
            printer.building(dsp_label)
            ncpus = os.cpu_count() or 4
            build_cmd = ["cmake", "--build", str(dep_build), "--parallel"]
            if args.throttle:
                build_cmd += ["--", f"-l{ncpus}"]
            rc = self._run_chain(
                ["cmake", "-S", str(dep_src), "-B", str(dep_build), "--preset", preset],
                build_cmd,
                log=log,
            )
            if rc != 0:
                printer.failed(dsp_label, log=log)
                raise RuntimeError(f"dsplib build failed for {label}")
            printer.done(dsp_label)

        # ── gen test case headers (autotest only, once shared across combos) ──
        if args.preset_type == "autotest":
            global _headers_done
            with _headers_lock:
                if not _headers_done:
                    hlabel = f"{I_PKG.strip()} gen_headers  (once, shared)"
                    printer.building(hlabel)
                    gen_preset = f"{build_type}-autotest-{soc}-{platform}"
                    gen_dir = audiolib_dir / "build" / soc / platform / build_type
                    gen_log = log_dir / "gen_headers.log"
                    _ncpus = os.cpu_count() or 4
                    _build_cmd = [
                        "cmake",
                        "--build",
                        str(gen_dir),
                        "--target",
                        "gen_all_test_case_headers",
                        "--parallel",
                    ]
                    if args.throttle:
                        _build_cmd += ["--", f"-l{_ncpus}"]
                    rc = self._run_chain(
                        [
                            "cmake",
                            "-S",
                            str(audiolib_dir),
                            "-B",
                            str(gen_dir),
                            "--preset",
                            gen_preset,
                        ],
                        _build_cmd,
                        log=str(gen_log),
                    )
                    if rc != 0:
                        printer.failed(hlabel, log=gen_log)
                        raise RuntimeError(
                            f"gen_all_test_case_headers failed — see {gen_log}"
                        )
                    _headers_done = True
                    printer.done(hlabel)

        # ── audiolib ──────────────────────────────────────────────────────────
        al_label = f"{I_PKG.strip()} audiolib  {label}"
        preset = args.preset or f"{build_type}-{args.preset_type}-{soc}-{platform}"
        build_dir = (
            Path(args.build_dir) / soc / platform / build_type
            if args.build_dir
            else audiolib_dir / "build" / soc / platform / build_type
        )
        log = log_dir / "audiolib.log"
        if args.clean and build_dir.exists():
            shutil.rmtree(build_dir)
        configure = [
            "cmake",
            "-S",
            str(audiolib_dir),
            "-B",
            str(build_dir),
            "--preset",
            preset,
        ]
        if args.fresh:
            configure.append("--fresh")
        printer.building(al_label)
        ncpus = os.cpu_count() or 4
        build_cmd = ["cmake", "--build", str(build_dir), "--parallel"]
        if args.throttle:
            build_cmd += ["--", f"-l{ncpus}"]
        rc = self._run_chain(configure, build_cmd, log=log)
        if rc != 0:
            printer.failed(al_label, log=log)
            raise RuntimeError(f"audiolib build failed for {label}")
        printer.done(al_label)

    # ── main entry point ──────────────────────────────────────────────────────

    def do_run(self, args, unknown):
        audiolib_dir = next(
            (
                p
                for p in [
                    Path(self.topdir) / "audiolib",
                    Path(self.topdir) / "xlib" / "audiolib",
                ]
                if p.is_dir()
            ),
            None,
        )
        if audiolib_dir is None:
            self.die(
                "audiolib not found at workspace/audiolib/ or workspace/xlib/audiolib/"
            )

        dsplib_dir = next(
            (
                p
                for p in [
                    Path(self.topdir) / "dsplib",
                    Path(self.topdir) / "xlib" / "dsplib",
                ]
                if p.is_dir()
            ),
            None,
        )
        if dsplib_dir is None:
            self.die('dsplib/ not found — run "west update" first')

        soc_map = _soc_map(audiolib_dir)
        if not soc_map:
            self.die("No buildlib presets found in CMakePresets.json")

        requested_socs = args.soc or list(soc_map.keys())
        unknown_socs = [s for s in requested_socs if s not in soc_map]
        if unknown_socs:
            self.die(
                f"Unknown SOC(s): {unknown_socs}  Available: {list(soc_map.keys())}"
            )

        build_types = (
            ["release", "debug"] if args.build_type == "all" else [args.build_type]
        )
        combos = []
        for soc in requested_socs:
            device = soc_map[soc]["device"]
            platforms = soc_map[soc]["platforms"]
            if args.platform != "all":
                platforms = [p for p in platforms if p == args.platform]
                if not platforms:
                    self.die(f'Platform "{args.platform}" not available for {soc}')
            for platform in platforms:
                for build_type in build_types:
                    combos.append((soc, platform, device, build_type))

        printer = _Printer()
        printer.section(f"west build-audiolib  —  {len(combos)} combos")
        print(f"  {C_DIM}workspace : {self.topdir}{C_RESET}")
        print(
            f"  {C_DIM}combos    : "
            + "  ".join(f"{s}/{p}/{bt}" for s, p, _, bt in combos)
            + C_RESET,
            flush=True,
        )

        t0 = time.monotonic()
        failed = []
        with ThreadPoolExecutor(max_workers=len(combos)) as executor:
            futures = {
                executor.submit(
                    self._build_combo,
                    soc,
                    plat,
                    device,
                    bt,
                    args,
                    audiolib_dir,
                    printer,
                ): f"{soc}/{plat}/{bt}"
                for soc, plat, device, bt in combos
            }
            for future in as_completed(futures):
                if future.exception():
                    failed.append(f"{futures[future]}: {future.exception()}")

        # combos without device for summary table
        combos_nodv = [(s, p, bt) for s, p, _, bt in combos]
        _summary("AUDIOLIB", combos_nodv, failed, time.monotonic() - t0)

        if failed:
            self.die("\nFailed combos:\n  " + "\n  ".join(failed))
