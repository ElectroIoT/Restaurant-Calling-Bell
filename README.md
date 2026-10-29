# Restaurant Calling Bell (SI24R1)

Open-source wireless table-calling system: **10 table remotes** (ATmega328P + SI24R1, coin cell or 2xAAA) and **one counter receiver**
(ATmega328P + SI24R1 + SSD1306 OLED + DFPlayer Mini voice). Application-level acknowledgement, bounded retries, duplicate suppression,
pending calls that survive a power cut.

**Status (2026-10-29): firmware compiles, protocol logic tested in software, no hardware tested yet.** See
[docs/hardware-verification.md](docs/hardware-verification.md) for exactly what is verified. Full plan with advice: open
[docs/plan.html](docs/plan.html) in a browser.

The earlier ATtiny85/STM32 concept is kept in `docs/archive-v0-attiny-stm32/`. Vendor datasheets: `docs/datasheets/`.

## Layout

```
docs/        plan.html, architecture, protocol, radio driver choice, audio, power/safety, battery, assembly, flashing,
             troubleshooting, test plan, EasyEDA guide, hardware verification, datasheets
firmware/    common/ (library: protocol, call table, retry FSM, EEPROM log, radio driver), remote/, receiver/
hardware/    remote/ receiver/ (BOM CSVs, EasyEDA data pack, KiCad REFERENCE boards), diagrams/ (Mermaid + wiring), bom/, tools/
tests/       protocol-tests/ (host C tests + 10-remote simulation), test-results-template.csv
tools/       build_firmware.py, battery_calc.py, make_audio_windows.ps1
```

## Quick start

```bash
python tools/build_firmware.py --quick        # compile one remote + receiver for Uno and PCB targets
python tools/build_firmware.py                # all 22 builds, hex files in build/hex/
python tests/protocol-tests/run_tests.py      # host tests (gcc/clang, or: pip install --target tools/.zig ziglang)
python tools/battery_calc.py                  # battery estimate (assumptions, not measurements)
python hardware/tools/gen_bom.py              # regenerate BOM CSVs from hardware/tools/*_design.py
```

Toolchain used: arduino-cli 1.5.1 (inside Arduino IDE 2), `arduino:avr` 1.8.8 (avr-gcc 7.3). Per-board build properties are in
[docs/flashing-guide.md](docs/flashing-guide.md). Table id: `-DTABLE_ID=n` (1..10, checked at compile time).

## Where to start

1. Read [docs/plan.html](docs/plan.html), then [docs/test-plan.md](docs/test-plan.md).
2. Build the two-Uno prototype ([wiring](hardware/diagrams/wiring.md)) and pass tests 2-9.
3. Lay out the PCBs in EasyEDA Pro from [hardware/*/easyeda](hardware) and [docs/easyeda-guide.md](docs/easyeda-guide.md).
4. Order parts from [hardware/bom/BOM-SUMMARY.md](hardware/bom/BOM-SUMMARY.md) (check the "needs checking" list first).

## Known limits

No range, battery-life or reliability number here is measured. The coin-cell option depends on a sag test you still have to run. DFPlayer pin
order/spacing and the coin-holder polarity are assumptions. No audio files are included. KiCad boards in `hardware/*/kicad` are an
unreviewed auto-routed reference, not a release.

## Licence

MIT, see [LICENSE](LICENSE) (firmware, tools, docs and design data).
