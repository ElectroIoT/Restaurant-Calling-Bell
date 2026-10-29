/*
 * config.h - build-time configuration of the counter receiver.
 *
 * Override with compiler flags, e.g.
 *   --build-property "build.extra_flags=-DAUDIO_VOLUME=25 -DREPEAT_INTERVAL_S=45"
 */
#ifndef RECEIVER_CONFIG_H
#define RECEIVER_CONFIG_H

#include <Arduino.h>
#include "radio_driver.h"
#include "bell_protocol.h"

/* ---- board ------------------------------------------------------------------------- */
/* 0 = Arduino Uno prototype (16 MHz, 5 V). DFPlayer on SoftwareSerial D7/D8, debug on USB serial.
 * 1 = custom receiver PCB: ATmega328P-AU, 3.3 V logic, 8 MHz crystal. DFPlayer on the hardware
 *     UART (D0/D1), programmed through ISP, watchdog enabled. */
#ifndef BELL_BOARD_PCB
#define BELL_BOARD_PCB 0
#endif
#if BELL_BOARD_PCB && (F_CPU != 8000000UL)
#error "Custom receiver PCB build must use F_CPU=8000000 (arduino:avr:pro:cpu=8MHzatmega328)"
#endif

/* ---- pins -------------------------------------------------------------------------- */
#define PIN_RADIO_CE      9
#define PIN_RADIO_CSN     10
#define PIN_RADIO_IRQ     2        /* wired, unused (STATUS polled) */
#define PIN_BUZZER        3        /* active buzzer via NPN, HIGH = on */
#define PIN_BTN_ACK       4        /* active low */
#define PIN_BTN_CLEAR     5        /* active low */
#define PIN_BTN_MUTE      6        /* active low */
#define PIN_AUDIO_SOFT_RX 7        /* Uno prototype: MCU RX <- DFPlayer TX */
#define PIN_AUDIO_SOFT_TX 8        /* Uno prototype: MCU TX -> 1k -> DFPlayer RX */
#define PIN_AUDIO_BUSY    A0       /* DFPlayer BUSY, low while playing; set 255 if not wired */
/* OLED on I2C: SDA = A4, SCL = A5, address below */
#define OLED_I2C_ADDR     0x3C

/* ---- radio (must match the remotes) -------------------------------------------------- */
#ifndef BELL_CHANNEL
#define BELL_CHANNEL 76
#endif
static_assert(BELL_CHANNEL <= 83, "channel above 2483 MHz is outside the ISM band in many countries");
#ifndef BELL_RATE
#define BELL_RATE RADIO_RATE_250K
#endif
#ifndef BELL_TX_POWER
#define BELL_TX_POWER RADIO_PWR_0DBM
#endif
#define BELL_HW_RETRY_DELAY  3
#define BELL_HW_RETRY_COUNT  3
#define RADIO_HEALTH_CHECK_MS  5000

/* ---- audio ------------------------------------------------------------------------- */
/* Files on the microSD card, folder /MP3, names 0001.mp3 ... (see docs/audio-guide.md):
 *   0001..0010 "Table N is calling for service"   0011 generic alert tone
 *   0012 call accepted tone   0013 call served tone   0014 ready/boot tone */
#define AUDIO_TRACK_TABLE_BASE  1      /* track = base + table - 1 */
#define AUDIO_TRACK_ALERT       11
#define AUDIO_TRACK_ACCEPTED    12
#define AUDIO_TRACK_SERVED      13
#define AUDIO_TRACK_READY       14
#ifndef AUDIO_VOLUME
#define AUDIO_VOLUME            20     /* 0..30 */
#endif
static_assert(AUDIO_VOLUME <= 30, "DFPlayer volume is 0..30");
#ifndef AUDIO_ANNOUNCE_MS
#define AUDIO_ANNOUNCE_MS       4500   /* assumed track length when the BUSY pin is not used */
#endif
#ifndef AUDIO_PLAY_ACCEPTED_TONE
#define AUDIO_PLAY_ACCEPTED_TONE 0     /* 1: also play 0012 after each announcement */
#endif
#ifndef AUDIO_PLAY_SERVED_TONE
#define AUDIO_PLAY_SERVED_TONE   1     /* tone 0013 when staff press ACK */
#endif
#ifndef BUZZER_ENABLE
#define BUZZER_ENABLE            1
#endif

/* ---- repeat / queue ---------------------------------------------------------------- */
#ifndef REPEAT_INTERVAL_S
#define REPEAT_INTERVAL_S   30         /* re-announce an unanswered call this often */
#endif
#ifndef REPEAT_MAX
#define REPEAT_MAX          5          /* 0 = repeat forever; after the limit the call stays on the OLED */
#endif
#define AUDIO_QUEUE_LEN     8
static_assert(REPEAT_INTERVAL_S >= 5, "repeat interval shorter than an announcement makes no sense");

/* ---- buttons ----------------------------------------------------------------------- */
#define BTN_DEBOUNCE_MS     30
#define CLEAR_HOLD_MS       2000       /* CLEAR must be held this long to clear all calls */
#define UI_MESSAGE_MS       2000

/* ---- persistence ------------------------------------------------------------------- */
#define PERSIST_EEPROM_BASE 0
#define PERSIST_SLOTS       20         /* 20 * (40+3) = 860 bytes of the 1024-byte EEPROM */

/* ---- watchdog / debug -------------------------------------------------------------- */
#ifndef BELL_ENABLE_WDT
#define BELL_ENABLE_WDT     BELL_BOARD_PCB
#endif
#ifndef BELL_DEBUG_SERIAL
#define BELL_DEBUG_SERIAL   (!BELL_BOARD_PCB)
#endif
#if BELL_DEBUG_SERIAL && BELL_BOARD_PCB
#error "On the PCB the hardware UART belongs to the DFPlayer"
#endif

#endif
