// Link test, button side. Sends a real BellCall every 2 s and reads the receiver's ACK payload.
// Wiring (Nano): SCK D13, MISO D12, MOSI D11, CSN D10, CE D9. Radio VCC to a 3.3V source, 10uF at the radio.
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "bell_protocol.h"

#define PIN_CE   9
#define PIN_CSN  10
#define TABLE_ID 5

RF24 radio(PIN_CE, PIN_CSN);
static uint8_t seq = 0;
static uint16_t sent = 0, acked = 0, payloads = 0;

void setup() {
  Serial.begin(115200);
  if (!radio.begin()) {
    Serial.println(F("RADIO NOT FOUND: check wiring, 3.3V supply and the 10uF capacitor"));
    while (true) delay(1000);
  }
  radio.setChannel(BELL_CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.setCRCLength(RF24_CRC_16);
  radio.setAutoAck(true);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  radio.setRetries(BELL_RETRY_DELAY, BELL_RETRY_COUNT);
  radio.openWritingPipe((const uint8_t *)BELL_ADDRESS);
  radio.stopListening();
  Serial.println(F("Link test TX ready"));
}

void loop() {
  BellCall call = { TABLE_ID, (uint8_t)(seq % 3), seq, 150 /* 3.0V */, 0, 0 };
  bell_call_seal(&call);

  uint32_t t0 = micros();
  bool ok = radio.write(&call, sizeof(call));
  uint32_t rtt = micros() - t0;
  sent++;

  Serial.print(F("seq ")); Serial.print(call.seq);
  Serial.print(F(" type ")); Serial.print(call.type);
  if (ok) {
    acked++;
    Serial.print(F("  ACK  rtt ")); Serial.print(rtt); Serial.print(F("us"));
    if (radio.isAckPayloadAvailable()) {
      BellAck ack;
      radio.read(&ack, sizeof(ack));
      if (bell_ack_valid(&ack)) { payloads++; Serial.print(F("  status ")); Serial.print(ack.status); }
      else Serial.print(F("  ACK payload CRC BAD"));
    } else {
      Serial.print(F("  (no ACK payload)"));
    }
  } else {
    Serial.print(F("  NO ACK"));
  }
  Serial.print(F("   ok ")); Serial.print(acked); Serial.print('/'); Serial.print(sent);
  Serial.print(F("  payload ")); Serial.println(payloads);

  seq++;
  delay(2000);
}
