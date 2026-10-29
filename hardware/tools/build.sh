#!/bin/sh
# Rebuild one board: footprints, schematic, project, PCB, route, ERC, DRC.   sh build.sh remote|receiver [passes]
# Needs KiCad 10 (kicad-cli + its python) and java for Freerouting.
set -e
B="$1"
cd "$(dirname "$0")"
KB="$LOCALAPPDATA/Programs/KiCad/10.0/bin"
python gen_footprints.py
python gen_schematic.py "$B"
python gen_project.py "$B"
"$KB/python.exe" gen_pcb.py "$B"
"$KB/python.exe" route.py "$B" "${2:-40}"
"$KB/python.exe" stitch.py "$B"
case "$B" in remote) P=BellRemote;; receiver) P=BellReceiver;; esac
cd "../$B/kicad"
"$KB/kicad-cli.exe" sch erc --severity-all -o erc.rpt "$P.kicad_sch" || true
"$KB/kicad-cli.exe" pcb drc --refill-zones --save-board --severity-all --format report -o drc.rpt "$P.kicad_pcb" || true
echo "--- ERC"; grep "^\[" erc.rpt | sed 's/\].*/]/' | sort | uniq -c
echo "--- DRC"; grep "^\[" drc.rpt | sed 's/\].*/]/' | sort | uniq -c
