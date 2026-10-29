#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H
#include <stdint.h>

/* Clock prescaler (PCB build), unused-pin states, ADC/comparator off. Call first in setup(). */
void pm_init(void);
/* Supply voltage in mV, measured against the internal 1.1 V bandgap (no divider needed).
 * Typical accuracy +-10 % unless the bandgap constant is calibrated (docs/battery-life-calculation.md). */
uint16_t pm_read_vcc_mv(void);
/* Deep sleep (power-down) until the CALL button changes level (pin-change interrupt). */
void pm_sleep_until_button(void);
/* Deep sleep for ~8 s (watchdog interrupt) or until the button changes level. */
void pm_sleep_8s_or_button(void);
#endif
