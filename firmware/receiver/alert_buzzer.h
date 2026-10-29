#ifndef ALERT_BUZZER_H
#define ALERT_BUZZER_H
#include <stdint.h>
void buzzer_begin(uint8_t pin);
/* Queue `count` short beeps (non-blocking). Replaces a pattern in progress. */
void buzzer_beeps(uint8_t count, bool muted);
void buzzer_service(uint32_t now_ms);
#endif
