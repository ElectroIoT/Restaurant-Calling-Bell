# Test plan

Record every result in [`tests/test-results-template.csv`](../tests/test-results-template.csv) (copy it per board set).
**Status on 2026-10-29: only the software tests (T0) have been run. Every hardware test below is still to do.** No range, battery-life
or reliability figure in this repo has been measured.

Equipment: multimeter, a way to measure current in the 1 uA-30 mA range (uCurrent / Joulescope / scope + shunt), oscilloscope (voltage sag),
2 Uno boards for the prototype stage, a Wi-Fi router, a phone with a Wi-Fi analyser.

| # | Test | Method | Pass criterion |
|---|---|---|---|
| T0 | Host protocol tests and 10-remote simulation | `python tests/protocol-tests/run_tests.py` | 0 failures (**run: 476 checks pass**); all 22 firmware builds succeed (**done**) |
| 1 | MCU programming and clock | burn fuses, flash blink; scope on a toggled pin or `millis()` against a stopwatch | remote 4 MHz, receiver 8 MHz within 1 %; fuse read-back matches |
| 2 | SPI link | `radio_begin()` returns true; read back `RF_CH` and `RF_SETUP` | reads equal what was written, on all 11 boards |
| 3 | Radio init | serial print of registers after init | `RF_SETUP` = `0x20 | power`, `SETUP_RETR` = `0x33`; no `RF FAIL` on the OLED |
| 4 | One-way packet | remote sends, receiver prints `CALL table n seq s` | 100 of 100 calls arrive at 2 m |
| 5 | Application acknowledgement | remote LED solid after press; unplug the receiver and press | solid LED only with the receiver up; 5 flashes with it down; failure shown within 2.5 s |
| 6 | Table id | flash ids 1..10, press each | OLED and voice name the right table on all 10 |
| 7 | Ten remotes | all ten pressed within 1 s, ten times | 10 of 10 accepted each time, 10 entries on the OLED, no duplicates |
| 8 | Duplicate suppression | receiver serial log while pressing once with a weak signal (move away) | one pending entry per press even when the log shows several `res 2` (duplicate) |
| 9 | Simultaneous calls / collisions | press 5 remotes together, 20 times | all accepted within 3 s; count retries |
| 10 | Receiver audio and OLED | play all 14 tracks, mute, queue 5 calls | correct announcement per table; reception continues during playback (a remote pressed mid-announcement is still accepted); OLED does not flicker |
| 11 | Button debounce, stuck button | hold the CALL button 60 s; tap-bounce it; short and long CLEAR | one call per press; stuck button ignored after 30 s and current stays below 10 uA; CLEAR short press clears nothing, 2 s hold clears all |
| 12 | Battery sag under TX | scope across the cell terminals during a call, at 0 dBm, with a fresh cell and a partly used cell (and 2 x AAA) | minimum voltage during the pulse **above 2.2 V**; no brown-out reset (count epoch increments) |
| 13 | Sleep current | measure with the radio shut down, 10 minutes after boot | at most 5 uA assumed in the estimate; record the real value and rerun `tools/battery_calc.py` |
| 14 | Indoor range | receiver at the counter, remote at every table and in the worst corner, 20 presses each | at least 19 of 20 accepted at every table with 0 dBm; record RSSI bit and retries |
| 15 | Wi-Fi interference | run a speed test on a 2.4 GHz AP on channel 11 and 13 next to the receiver while pressing | at least 95 % accepted on first application attempt; if not, change `BELL_CHANNEL` |
| 16 | Receiver restart | 3 calls pending, remove power for 10 s, restore | the same 3 tables are restored in the same order and announced once; remotes that poll are not re-queued twice |
| 17 | Long-duration soak | 72 h, one scripted press per remote every 5 min (relay or servo) | no lost call, no reset except planned, receiver heap/RAM stable (no growth: static allocation only), EEPROM slot counter advances only on changes |
| 18 | Low battery | supply a remote from a bench supply, step 3.0 -> 1.8 V | low-battery flag and LED blips from 2.4 V; calls still accepted down to the voltage where the radio stops; record the cut-off |
| 19 | Regression | repeat 4-9 and 16 after every firmware change | all pass; log the firmware version and git commit |
| 20 | Amplifier thermal | maximum volume, 10 minutes of tones | DFPlayer amplifier does not exceed a temperature you can hold your finger on; 5 V rail droop below 0.3 V |

Stop and fix before ordering boards: any failure in T0-T11, T12 for the chosen battery, and T14 at the farthest table.

## What the software tests do and do not prove

`test_system_sim` runs the real `remote_fsm`, wire codec and `rx_core` over a modelled lossy channel (uplink loss, reply loss, lost radio ACK,
collisions, a restart of the receiver table). It proves the protocol rules (no duplicate entries, recovery after a lost call) under that model.
It does **not** prove RF range, SPI timing, EEPROM timing, DFPlayer behaviour or battery life.
