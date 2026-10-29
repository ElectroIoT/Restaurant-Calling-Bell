# Audio guide (DFPlayer Mini)

Command frames are built by [`audio_player.cpp`](../firmware/receiver/audio_player.cpp):
`7E FF 06 CMD FB P1 P2 CKH CKL EF`, checksum = 0 - (FF + 06 + CMD + FB + P1 + P2), feedback byte 0.

**Verification status:** the command numbers below (0x0C reset, 0x06 volume, 0x12 play MP3-folder file, 0x16 stop) are from the
public DFPlayer Mini command table as remembered; the module datasheet is **not** in this repo and nothing was tested on a module.
Check against your module's documentation before relying on them. Clone modules (YX5200 / AA1010 variants) differ in BUSY behaviour.

## SD card layout

Format FAT32 (card up to 32 GB). Create a folder named exactly `MP3` and put these files in it (4-digit names; MP3 or WAV decoded by
the module, MP3 recommended):

| File | Content | Used when |
|---|---|---|
| `/MP3/0001.mp3` ... `/MP3/0010.mp3` | "Table 01 is calling for service." ... "Table 10 ..." | a call is accepted, and on each repeat |
| `/MP3/0011.mp3` | generic alert tone | reserved (not played by default) |
| `/MP3/0012.mp3` | call accepted tone | only if `AUDIO_PLAY_ACCEPTED_TONE 1` |
| `/MP3/0013.mp3` | call served tone | when ACK is pressed (`AUDIO_PLAY_SERVED_TONE`) |
| `/MP3/0014.mp3` | ready / boot tone | after power-up |

Track number for table *n* = `AUDIO_TRACK_TABLE_BASE + n - 1` (0001..0010). Using command 0x12 plays by **file name number**, so the
copy order onto the card does not matter (unlike the plain "play track N" command).

**No audio files are included in this repository.** `tools/make_audio_windows.ps1` can synthesise placeholder files with the Windows
speech engine; recorded human voice sounds better and is the recommended final solution.

## Behaviour

* New call: OLED updates first, then the announcement is queued; a 2-beep buzzer pattern starts immediately.
* Queue holds 8 tracks; additional calls are queued, never dropped silently except when the queue is full (the call itself is still
  recorded and shown).
* Unanswered call repeats every `REPEAT_INTERVAL_S` (30 s) up to `REPEAT_MAX` (5) times, then stays on the OLED.
* MUTE button: drops the queue, stops playback, silences the buzzer; calls still appear on the OLED. It is not remembered across power cycles
  (no EEPROM write on a button press).
* Playback end is detected from the BUSY pin (low while playing). If BUSY is not wired, `AUDIO_ANNOUNCE_MS` (4.5 s) is assumed per track.
* Volume: `AUDIO_VOLUME` 0-30 (default 20). Start low; see [power and safety](power-and-safety.md) for the amplifier limits.
