# Build and flashing guide

## Build all firmware (verified on this PC)

```bash
python tools/build_firmware.py            # 22 builds: receiver + remotes 1..10, for Uno and PCB targets
python tools/build_firmware.py --quick    # one of each
python tests/protocol-tests/run_tests.py  # host tests (needs gcc/clang, or: pip install --target tools/.zig ziglang)
```

`.hex` files land in `build/hex/`. Needs `arduino-cli` (shipped inside Arduino IDE 2) with the `arduino:avr` core 1.8.8.

| Target | FQBN | Extra build properties |
|---|---|---|
| Uno prototype (remote or receiver) | `arduino:avr:uno` | `build.extra_flags=-DTABLE_ID=n` (remote) |
| Custom remote PCB | `arduino:avr:pro:cpu=8MHzatmega328` | `build.f_cpu=4000000L`, `build.extra_flags=-DTABLE_ID=n -DBELL_BOARD_PCB=1` |
| Custom receiver PCB | `arduino:avr:pro:cpu=8MHzatmega328` | `build.extra_flags=-DBELL_BOARD_PCB=1` |

Compile-time checks: wrong clock for a PCB build, `TABLE_ID` outside 1..10, channel above 83, volume above 30 stop the build.

## Fuses for the custom boards (not yet tried on a real chip)

| Board | lfuse | hfuse | efuse | Meaning |
|---|---|---|---|---|
| Remote | `0x62` | `0xD9` | `0xFE` | 8 MHz internal RC, **CKDIV8 on** (starts at 1 MHz; firmware changes the prescaler to reach 4 MHz), BOD 1.8 V, SPIEN, EEPROM kept on chip erase is **not** set (see below) |
| Receiver | `0xFF` | `0xD9` | `0xFD` | external full-swing crystal, BOD 2.7 V |

Programming with the ISP header, for example with an Arduino-as-ISP or a USBasp:

```bash
avrdude -c usbasp -p m328p -U lfuse:w:0x62:m -U hfuse:w:0xD9:m -U efuse:w:0xFE:m -U flash:w:build/hex/remote-pcb-table03.hex:i
```

Check the fuse values with a fuse calculator before burning: a wrong clock setting can need a recovery step. `hfuse 0xD9` leaves EEPROM
**erased** on chip erase; set bit 3 (`0xD1`) if you want a reflash to keep the receiver's saved calls and the remote's epoch counter.
Remotes only need the epoch to change per boot, so erasing it is harmless.

## Uno prototype

1. Wire per [`hardware/diagrams/wiring.md`](../hardware/diagrams/wiring.md), radio on its own 3.3 V.
2. `arduino-cli upload -p COMx --fqbn arduino:avr:uno --input-dir build/receiver-uno` (or use the Arduino IDE with the sketch folders and the library folder `firmware/common`).
3. Open the serial monitor at 115200 baud: the receiver prints `CALL table ...` lines; the remote prints with `-DBELL_DEBUG_SERIAL=1`.

## One remote per table

Set `TABLE_ID` per build (the build script does all ten). Label each board. Two boards with the same id will answer each other's replies:
check the label before gluing the case.
