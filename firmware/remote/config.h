/*
 * config.h - build-time configuration of the table remote.
 *
 * Override any value with a compiler flag, e.g.
 *   arduino-cli compile ... --build-property "build.extra_flags=-DTABLE_ID=7"
 * (see docs/flashing-guide.md and tools/build_remotes.py, which builds all ten).
 */
#ifndef REMOTE_CONFIG_H
#define REMOTE_CONFIG_H

#include <Arduino.h>
#include "radio_driver.h"

/* ---- identity ---------------------------------------------------------------------- */
#ifndef TABLE_ID
#define TABLE_ID 1                 /* 1..10 */
#endif
static_assert(TABLE_ID >= 1 && TABLE_ID <= 10, "TABLE_ID must be 1..10");

/* ---- board ------------------------------------------------------------------------- */
/* 0 = Arduino Uno prototype (16 MHz, 5 V logic: use a 3.3 V radio supply and 4.7k series
 *     resistors on SCK/MOSI/CSN/CE, see docs/wiring.md)
 * 1 = custom remote PCB: ATmega328P-AU, 3 V coin cell, 4 MHz (8 MHz RC with CKDIV8 fuse,
 *     firmware switches the prescaler to /2). ATmega328P is only rated for 4 MHz below 2.7 V. */
#ifndef BELL_BOARD_PCB
#define BELL_BOARD_PCB 0
#endif
#if BELL_BOARD_PCB && (F_CPU != 4000000UL)
#error "Custom remote PCB build must use F_CPU=4000000 (build.f_cpu=4000000L), see docs/flashing-guide.md"
#endif

/* ---- pins (same on Uno prototype and PCB) ------------------------------------------- */
#define PIN_RADIO_CE    9          /* PB1 */
#define PIN_RADIO_CSN   10         /* PB2 */
#define PIN_BUTTON      4          /* PD4 = PCINT20, active low */
#define PIN_LED         5          /* PD5, active high through series resistor */
#define PIN_RADIO_IRQ   2          /* PD2, wired but unused: STATUS is polled */

/* Custom PCB has a 470k external pull-up so a stuck button draws ~6 uA instead of ~100 uA
 * through the internal pull-up. Uno prototype uses the internal pull-up. */
#ifndef BUTTON_EXTERNAL_PULLUP
#define BUTTON_EXTERNAL_PULLUP BELL_BOARD_PCB
#endif

/* ---- radio ------------------------------------------------------------------------- */
#ifndef BELL_CHANNEL
#define BELL_CHANNEL 76            /* 2476 MHz: above Wi-Fi channels 1/6/11, inside 2400-2483.5 */
#endif
static_assert(BELL_CHANNEL <= 83, "channel above 2483 MHz is outside the ISM band in many countries");
#ifndef BELL_RATE
#define BELL_RATE RADIO_RATE_250K  /* best sensitivity (-96 dBm); airtime ~0.55 ms per frame */
#endif
#ifndef BELL_TX_POWER
#define BELL_TX_POWER RADIO_PWR_0DBM  /* 0 dBm = 12 mA TX; coin-cell friendly. +4 dBm = 16 mA. */
#endif
#define BELL_HW_RETRY_DELAY  3     /* ARD code: (3+1)*250 = 1000 us, must exceed the ACK airtime */
#define BELL_HW_RETRY_COUNT  3     /* hardware retransmissions per transmit */
static_assert(BELL_RATE != RADIO_RATE_250K || BELL_HW_RETRY_DELAY >= 1, "ARD too short for 250 kbps");

/* ---- application retry ------------------------------------------------------------- */
#define CALL_MAX_ATTEMPTS        8
#define CALL_REPLY_TIMEOUT_MS    40      /* wait for CALL_ACCEPTED after the radio ACK */
#define CALL_BACKOFF_BASE_MS     60
#define CALL_BACKOFF_JITTER_MS   120     /* + 0..119 ms pseudo-random, spreads simultaneous remotes */
static_assert(CALL_MAX_ATTEMPTS >= 1 && CALL_MAX_ATTEMPTS <= 30, "attempts out of range");

/* ---- served notification (poll) ---------------------------------------------------- */
/* After a call is accepted the remote cannot listen continuously (RX = 14 mA). It instead
 * wakes every 8 s and asks "is my call served?" for a limited window, then sleeps for good. */
#ifndef SERVED_POLL_ENABLE
#define SERVED_POLL_ENABLE 1
#endif
#define SERVED_POLL_WINDOW_S     300     /* give up after 5 minutes */
#define SERVED_POLL_PERIOD_S     8       /* fixed by the watchdog timeout */

/* ---- user interface timing ---------------------------------------------------------- */
#define BUTTON_DEBOUNCE_MS       30
#define BUTTON_LOCKOUT_MS        1500    /* ignore further presses after a call */
#define BUTTON_STUCK_MS          30000   /* a button held this long is treated as stuck */
#define LED_WAIT_PERIOD_MS       100     /* fast flash while waiting for acknowledgement */
#define LED_SUCCESS_MS           1000    /* solid */
#define LED_ERROR_FLASHES        5       /* rapid flashes on failure */

/* ---- battery ----------------------------------------------------------------------- */
#ifndef LOW_BATT_MV
#define LOW_BATT_MV 2400           /* below this the low-battery flag is sent and LED blips */
#endif
#ifndef BELL_DEBUG_SERIAL
#define BELL_DEBUG_SERIAL 0        /* Uno prototype only; leave 0 on the PCB */
#endif
#if BELL_DEBUG_SERIAL && BELL_BOARD_PCB
#error "Serial debug is for the Uno prototype only"
#endif

#endif
