#ifndef LED_STATUS_H
#define LED_STATUS_H
#include <stdint.h>

void led_init(void);
void led_off(void);
/* Non-blocking fast flash while waiting for the acknowledgement; call often. */
void led_waiting_update(uint32_t now_ms);
/* Blocking result patterns (the remote has nothing else to do at this point). */
void led_success(void);     /* one long solid light                */
void led_failure(void);     /* rapid flashes, no acknowledgement   */
void led_served(void);      /* three slow blinks: staff attended   */
void led_low_battery(void); /* two short blips appended to results */
void led_fatal_radio(void); /* radio missing: long-short pattern   */
#endif
