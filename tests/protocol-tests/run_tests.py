#!/usr/bin/env python3
"""Build and run the host-side protocol tests.

Compiler search order: $CC, gcc, clang, cc, then `python -m ziglang cc` (pip install ziglang;
this repo's tools/.zig folder is used automatically if present). If no host compiler exists the
tests are only syntax-checked with avr-gcc (Arduino toolchain) and reported as NOT RUN.
"""
import glob
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
COMMON = os.path.join(ROOT, "firmware", "common", "src")
BUILD = os.path.join(HERE, "build")
LIB_SRC = ["bell_protocol.c", "call_table.c", "persist_log.c", "remote_fsm.c", "rx_core.c"]
TESTS = ["test_protocol", "test_call_table", "test_persist_log", "test_remote_fsm", "test_system_sim"]


def find_compiler():
    if os.environ.get("CC"):
        return os.environ["CC"].split()
    for name in ("gcc", "clang", "cc"):
        if shutil.which(name):
            return [name]
    zig_dir = os.path.join(ROOT, "tools", ".zig")
    env_py = os.environ.copy()
    if os.path.isdir(zig_dir):
        env_py["PYTHONPATH"] = zig_dir + os.pathsep + env_py.get("PYTHONPATH", "")
    probe = subprocess.run([sys.executable, "-m", "ziglang", "version"], env=env_py,
                           capture_output=True, text=True)
    if probe.returncode == 0:
        os.environ["PYTHONPATH"] = env_py["PYTHONPATH"]
        return [sys.executable, "-m", "ziglang", "cc"]
    return None


def find_avr_gcc():
    found = glob.glob(os.path.expanduser(
        "~/AppData/Local/Arduino15/packages/arduino/tools/avr-gcc/*/bin/avr-gcc*"))
    return found[0] if found else shutil.which("avr-gcc")


def main():
    os.makedirs(BUILD, exist_ok=True)
    cc = find_compiler()
    srcs = [os.path.join(COMMON, s) for s in LIB_SRC]
    flags = ["-std=c99", "-Wall", "-Wextra", "-O1", "-g", "-I" + COMMON, "-I" + HERE]

    if cc is None:
        avr = find_avr_gcc()
        print("No host C compiler found: syntax-checking only (tests NOT RUN).")
        if not avr:
            print("avr-gcc not found either.")
            return 2
        rc = 0
        for t in TESTS:
            r = subprocess.run([avr, "-mmcu=atmega328p", "-fsyntax-only"] + flags +
                               [os.path.join(HERE, t + ".c")])
            rc |= r.returncode
        return rc

    failed = 0
    for t in TESTS:
        exe = os.path.join(BUILD, t + (".exe" if os.name == "nt" else ""))
        build = subprocess.run(cc + flags + [os.path.join(HERE, t + ".c")] + srcs + ["-o", exe],
                               capture_output=True, text=True)
        if build.returncode != 0:
            print("BUILD FAILED:", t)
            print(build.stdout, build.stderr)
            failed += 1
            continue
        run = subprocess.run([exe], capture_output=True, text=True)
        print(run.stdout, end="")
        if run.returncode != 0:
            failed += 1
    print("ALL PASSED" if failed == 0 else "%d test program(s) FAILED" % failed)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
