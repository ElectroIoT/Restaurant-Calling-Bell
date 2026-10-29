/*
 * audio_player.h - non-blocking DFPlayer Mini driver with a small announcement queue.
 *
 * Plays /MP3/NNNN.mp3 via command 0x12 ("play file NNNN in the MP3 folder"), so playback does
 * not depend on the order files were copied to the card. See docs/audio-guide.md.
 * Frame: 7E FF 06 CMD FB P1 P2 CK_H CK_L EF, checksum = 0 - (FF+06+CMD+FB+P1+P2).
 */
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H
#include <stdint.h>
#include <stdbool.h>
#include <Stream.h>

void audio_begin(Stream &port, uint8_t busy_pin /* 255 = not wired */, uint8_t volume);
/* Queue a track number (1..2999). Returns false if the queue is full. Ignored while muted. */
bool audio_enqueue(uint16_t track);
void audio_set_mute(bool muted);
bool audio_is_muted(void);
uint8_t audio_queue_len(void);
bool audio_is_playing(void);
bool audio_is_ready(void);
/* Call every loop iteration; never blocks beyond one 10-byte serial frame. */
void audio_service(uint32_t now_ms);
#endif
