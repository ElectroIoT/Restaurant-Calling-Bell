#include <Arduino.h>
#include "config.h"
#include "audio_player.h"

#define DF_CMD_VOLUME      0x06
#define DF_CMD_RESET       0x0C
#define DF_CMD_STOP        0x16
#define DF_CMD_PLAY_MP3DIR 0x12   /* /MP3/NNNN.mp3 */

enum State : uint8_t { ST_BOOT_WAIT, ST_RESET_WAIT, ST_READY };

static Stream  *g_port;
static uint8_t  g_busy_pin;
static uint8_t  g_volume;
static State    g_state = ST_BOOT_WAIT;
static uint32_t g_state_since;
static bool     g_muted;
static bool     g_playing;
static uint32_t g_play_started;
static uint16_t g_queue[AUDIO_QUEUE_LEN];
static uint8_t  g_head, g_count;

static void send_frame(uint8_t cmd, uint16_t param)
{
    uint8_t f[10] = {0x7E, 0xFF, 0x06, cmd, 0x00, (uint8_t)(param >> 8), (uint8_t)param, 0, 0, 0xEF};
    uint16_t sum = 0;
    for (uint8_t i = 1; i <= 6; i++)
        sum += f[i];
    uint16_t ck = (uint16_t)(0 - sum);
    f[7] = (uint8_t)(ck >> 8);
    f[8] = (uint8_t)ck;
    g_port->write(f, sizeof(f));
}

void audio_begin(Stream &port, uint8_t busy_pin, uint8_t volume)
{
    g_port = &port;
    g_busy_pin = busy_pin;
    g_volume = volume;
    if (g_busy_pin != 255)
        pinMode(g_busy_pin, INPUT_PULLUP);
    g_state = ST_BOOT_WAIT;
    g_state_since = millis();
}

bool audio_enqueue(uint16_t track)
{
    if (g_muted)
        return false;
    if (g_count >= AUDIO_QUEUE_LEN)
        return false;
    g_queue[(uint8_t)((g_head + g_count) % AUDIO_QUEUE_LEN)] = track;
    g_count++;
    return true;
}

void audio_set_mute(bool muted)
{
    g_muted = muted;
    if (muted) {
        g_count = 0;                      /* drop queued announcements */
        if (g_state == ST_READY && g_playing) {
            send_frame(DF_CMD_STOP, 0);
            g_playing = false;
        }
    }
}

bool audio_is_muted(void)    { return g_muted; }
uint8_t audio_queue_len(void) { return g_count; }
bool audio_is_playing(void)  { return g_playing; }
bool audio_is_ready(void)    { return g_state == ST_READY; }

static bool track_finished(uint32_t now)
{
    uint32_t elapsed = now - g_play_started;
    if (g_busy_pin != 255) {
        if (elapsed < 400)                /* BUSY needs a moment to go low after the command */
            return false;
        if (digitalRead(g_busy_pin) == HIGH)
            return true;                  /* BUSY high = idle */
        return elapsed > 30000;           /* safety net: a stuck-low BUSY line cannot block forever */
    }
    return elapsed >= AUDIO_ANNOUNCE_MS;
}

void audio_service(uint32_t now)
{
    switch (g_state) {
    case ST_BOOT_WAIT:                    /* DFPlayer needs ~1 s after power-up before it listens */
        if ((uint32_t)(now - g_state_since) >= 1200) {
            send_frame(DF_CMD_RESET, 0);
            g_state = ST_RESET_WAIT;
            g_state_since = now;
        }
        return;
    case ST_RESET_WAIT:                   /* reset + SD card initialisation ~1.5-3 s */
        if ((uint32_t)(now - g_state_since) >= 2500) {
            send_frame(DF_CMD_VOLUME, g_volume);
            g_state = ST_READY;
            g_state_since = now;
        }
        return;
    case ST_READY:
        break;
    }

    if (g_playing && track_finished(now))
        g_playing = false;
    if (g_playing || g_count == 0)
        return;
    if ((uint32_t)(now - g_state_since) < 100)   /* let the volume command settle */
        return;

    uint16_t track = g_queue[g_head];
    g_head = (uint8_t)((g_head + 1) % AUDIO_QUEUE_LEN);
    g_count--;
    send_frame(DF_CMD_PLAY_MP3DIR, track);
    g_playing = true;
    g_play_started = now;
}
