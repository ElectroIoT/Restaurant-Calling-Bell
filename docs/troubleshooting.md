# Troubleshooting

| Symptom | Likely cause | What to do |
|---|---|---|
| Remote LED: one long + three short flashes | `radio_begin()` failed (no SPI answer) | check 3V3, SPI wiring, module orientation; the pad order is 3V3 GND CE CSN SCK MOSI MISO IRQ |
| OLED shows `RF FAIL` | receiver radio not answering, or reset by a brown-out | check 3.3 V at the radio; the firmware retries every 5 s |
| 5 rapid flashes on every press | no application reply within about 2 s | receiver off or out of range; wrong `BELL_CHANNEL`/`BELL_RATE` between boards; Wi-Fi on the same spectrum |
| Works at 2 m, fails at 8 m | antenna detuned by metal/battery, weak supply | keep metal 10 mm away; check 22 uF at the radio; raise `BELL_TX_POWER` to `RADIO_PWR_4DBM` (more battery drain) |
| Same table appears twice | two remotes flashed with the same `TABLE_ID` | check labels; ids must be unique 1..10 |
| Calls are accepted but never announced | muted, or DFPlayer not ready/SD missing | un-mute; check `/MP3/0001.mp3` exists; DFPlayer needs about 3.7 s after power-up |
| Announcement plays the wrong table | files not named by number | name files `0001.mp3`...`0010.mp3` exactly in the `MP3` folder |
| OLED blank | wrong address or pin order | I2C address 0x3C (try 0x3D in `config.h`); check GND/VCC order of your module |
| OLED text flickers | not expected: redraw happens only on change | check that the 3.3 V rail is not browning out |
| Remote resets when the battery is old | voltage sag during TX below 1.9 V | test 12; use 2xAAA or fresh cells; lower TX power |
| Remote battery drains in weeks | stuck button, or sleep current high | check the 470 k pull-up is fitted, no LED left lit, radio really in shutdown; measure test 13 |
| Calls lost after a power cut | the save window (reply is sent before the EEPROM write) | enable `SERVED_POLL_ENABLE` (default): the remote re-sends when it gets `UNKNOWN_CALL` |
| Build fails with `#error ... F_CPU` | wrong board/clock for a PCB build | use the FQBN and `build.f_cpu` from the [flashing guide](flashing-guide.md) |
