#!/usr/bin/env python3
"""Compile all firmware targets with arduino-cli and collect .hex files in build/hex/.

  python tools/build_firmware.py            # Uno prototypes + PCB targets (remote table 1..10)
  python tools/build_firmware.py --quick    # one remote (table 1) and one receiver per board type

Requires arduino-cli with the arduino:avr core (Arduino IDE 2 ships arduino-cli; set
ARDUINO_CLI to override the auto-detected path).
"""
import argparse
import glob
import os
import shutil
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
HEX = os.path.join(ROOT, "build", "hex")
PRO8 = "arduino:avr:pro:cpu=8MHzatmega328"   # 3.3 V / 8 MHz ATmega328P board definition


def find_cli():
    if os.environ.get("ARDUINO_CLI"):
        return os.environ["ARDUINO_CLI"]
    p = shutil.which("arduino-cli")
    if p:
        return p
    for cand in glob.glob(r"C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"):
        return cand
    sys.exit("arduino-cli not found (set ARDUINO_CLI)")


def build(cli, name, sketch, fqbn, props):
    out = os.path.join(ROOT, "build", name)
    cmd = [cli, "compile", "--fqbn", fqbn, "--library", os.path.join(ROOT, "firmware", "common"),
           "--build-path", out, "--warnings", "all"]
    for p in props:
        cmd += ["--build-property", p]
    cmd.append(os.path.join(ROOT, "firmware", sketch))
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("FAIL", name)
        print(r.stdout, r.stderr)
        return False
    summary = [l for l in r.stdout.splitlines() if l.startswith(("Sketch uses", "Global variables"))]
    print("OK  ", name.ljust(26), " | ".join(summary))
    os.makedirs(HEX, exist_ok=True)
    for h in glob.glob(os.path.join(out, "*.ino.hex")):
        if not h.endswith("with_bootloader.hex"):
            shutil.copy(h, os.path.join(HEX, name + ".hex"))
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    a = ap.parse_args()
    cli = find_cli()
    ok = True
    tables = [1] if a.quick else range(1, 11)

    ok &= build(cli, "receiver-uno", "receiver", "arduino:avr:uno", [])
    ok &= build(cli, "receiver-pcb", "receiver", PRO8, ["build.extra_flags=-DBELL_BOARD_PCB=1"])
    for t in tables:
        ok &= build(cli, "remote-uno-table%02d" % t, "remote", "arduino:avr:uno",
                    ["build.extra_flags=-DTABLE_ID=%d" % t])
        ok &= build(cli, "remote-pcb-table%02d" % t, "remote", PRO8,
                    ["build.f_cpu=4000000L", "build.extra_flags=-DTABLE_ID=%d -DBELL_BOARD_PCB=1" % t])
    print("hex files:", HEX)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
