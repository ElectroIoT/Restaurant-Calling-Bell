// Link test, receiver side. Prints each valid BellCall and answers with an ACK payload.
// Same wiring as tx.cpp. This logic moves to the STM32C031 later; keep the protocol in bell_protocol.h.
#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "bell_protocol.h"

#define PIN_CE   9
#define PIN_CSN  10

RF24 radio(PIN_CE, PIN_CSN);
static uint8_t last_seq[256];
static bool    seen[256];
static uint16_t good = 0, bad_crc = 0, dup = 0;

static void load_ack(uint8_t status) {
  BellAck ack = { status, 0 };
  bell_ack_seal(&ack);
  radio.writeAckPayload(1, &ack, sizeof(ack));   // pipe 1; must be loaded before the next packet arrives
}

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
  radio.openReadingPipe(1, (const uint8_t *)BELL_ADDRESS);
  load_ack(ACK_RECEIVED);
  radio.startListening();
  Serial.println(F("Link test RX ready"));
}

void loop() {
  uint8_t pipe;
  if (!radio.available(&pipe)) return;

  uint8_t len = radio.getDynamicPayloadSize();
  BellCall call;
  if (len != sizeof(call)) {
    radio.flush_rx();
    Serial.print(F("bad length ")); Serial.println(len);
    load_ack(ACK_RECEIVED);
    return;
  }
  radio.read(&call, sizeof(call));

  if (!bell_call_valid(&call)) {
    bad_crc++;
    Serial.println(F("CRC BAD"));
  } else if (seen[call.table_id] && last_seq[call.table_id] == call.seq) {
    dup++;                                   // hardware retry of a packet already counted
  } else {
    seen[call.table_id] = true;
    last_seq[call.table_id] = call.seq;
    good++;
    Serial.print(F("table ")); Serial.print(call.table_id);
    Serial.print(F("  type ")); Serial.print(call.type);
    Serial.print(F("  seq ")); Serial.print(call.seq);
    Serial.print(F("  batt ")); Serial.print(call.battery * 20); Serial.print(F("mV"));
    Serial.print(F("   good ")); Serial.print(good);
    Serial.print(F("  dup ")); Serial.print(dup);
    Serial.print(F("  badcrc ")); Serial.println(bad_crc);
  }
  load_ack(ACK_RECEIVED);                    // reload for the next packet
}
