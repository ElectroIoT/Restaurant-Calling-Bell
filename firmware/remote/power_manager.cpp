#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include "config.h"
#include "power_manager.h"

/* PD4 = PCINT20 is in pin-change group 2 (PCINT16..23). */
#if PIN_BUTTON != 4
#error "pm_*: pin-change setup below assumes the button is on D4 (PD4/PCINT20)"
#endif

ISR(PCINT2_vect) { }
ISR(WDT_vect)    { }   /* interrupt-only watchdog: wakes the CPU, does not reset it */

void pm_init(void)
{
#if BELL_BOARD_PCB
    /* Fuses select 8 MHz RC with CKDIV8 (1 MHz at reset, safe down to 1.8 V). Move to 4 MHz,
     * the highest speed the ATmega328P is rated for below 2.7 V. Timed sequence. */
    cli();
    CLKPR = 0x80;
    CLKPR = 0x01;          /* divide by 2 */
    sei();
#endif
    ADCSRA = 0;                         /* ADC off */
    ACSR |= (1 << ACD);                 /* analog comparator off */
    DIDR0 = 0x3F;                       /* digital input buffers off on A0..A5 */
    power_twi_disable();
    power_usart0_disable();
    power_timer1_disable();
    power_timer2_disable();
    MCUSR = 0;
    wdt_disable();

    /* Unused pins: input with pull-up, so nothing floats (floating inputs waste current).
     * Used pins are configured by their own modules afterwards. */
    for (uint8_t p = 0; p < 20; p++) {
        if (p == PIN_RADIO_CE || p == PIN_RADIO_CSN || p == PIN_BUTTON || p == PIN_LED ||
            p == 11 || p == 12 || p == 13)       /* SPI MOSI/MISO/SCK */
            continue;
        pinMode(p, INPUT_PULLUP);
    }
}

uint16_t pm_read_vcc_mv(void)
{
    power_adc_enable();
    ADMUX = (1 << REFS0) | (1 << MUX3) | (1 << MUX2) | (1 << MUX1);   /* AVcc ref, measure 1.1 V bandgap */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);                /* prescaler /64 */
    delay(2);                                                          /* bandgap settling */
    ADCSRA |= (1 << ADSC);                                             /* first conversion is discarded */
    while (ADCSRA & (1 << ADSC)) { }
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) { }
    uint16_t adc = ADC;
    ADCSRA = 0;
    power_adc_disable();
    if (adc == 0)
        return 0;
    return (uint16_t)(1125300UL / adc);                               /* 1.1 V * 1023 * 1000 / ADC */
}

static void deep_sleep(void)
{
    /* Arm the button pin-change interrupt, then sleep. Interrupts are disabled while arming and
     * re-enabled atomically with sleep so an edge between the check and sleep is not lost. */
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    power_spi_disable();
    power_timer0_disable();
    cli();
    PCIFR  |= (1 << PCIF2);
    PCMSK2 |= (1 << PCINT20);
    PCICR  |= (1 << PCIE2);
    sleep_enable();
    sleep_bod_disable();
    sei();
    sleep_cpu();
    sleep_disable();
    PCICR &= ~(1 << PCIE2);
    power_timer0_enable();
    power_spi_enable();
}

void pm_sleep_until_button(void)
{
    wdt_reset();
    deep_sleep();
}

void pm_sleep_8s_or_button(void)
{
    cli();
    wdt_reset();
    MCUSR &= ~(1 << WDRF);
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);   /* interrupt mode, 8 s */
    sei();
    deep_sleep();
    wdt_disable();                                       /* stop the watchdog: it draws current */
}
