#include <Arduino.h>
#include <cstdint>
#include <iot_board.h>
#include <lora.h>
#include <state.h>

#define CACHE_SIZE 10

struct SeenPacket {
  uint32_t deviceId;
  uint16_t seq;
};

static SeenPacket cache[CACHE_SIZE];
static uint8_t cacheIndex = 0;

static uint32_t deviceId = 1;
static uint16_t seqCounter = 0;

MeshPacket relayPacket;
volatile bool shouldRelay = false;

void initLoRaNetwork() {
  lora->onReceive(onLoRaReceive);
  lora->receive();
}

void sendPacket(const MeshPacket &packet) {

  digitalWrite(LED_YELLOW, LOW);
  delay(250);
  const uint8_t *raw = (const uint8_t *)&packet;

  lora->beginPacket();

  for (int i = 0; i < MESH_PACKET_SIZE; i++)
    lora->write(raw[i]);

  lora->endPacket();
  lora->receive();

  digitalWrite(LED_YELLOW, HIGH);
}

bool alreadySeen(uint32_t id, uint16_t seq) {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (cache[i].deviceId == id && cache[i].seq == seq)
      return true;
  }

  cache[cacheIndex] = {id, seq};
  cacheIndex = (cacheIndex + 1) % CACHE_SIZE;

  return false;
}

void sendAlert() {
  MeshPacket p = {};

  p.deviceId = deviceId;
  p.seq = seqCounter++;
  p.ttl = 8; // hop iniziale

  // payload dummy (poi AES-CTR)
  const char *msg = "ALERT";
  memcpy(p.payload, msg, strlen(msg));

  // CRC placeholder (lo implementerai dopo)
  p.crc = 0xFFFF;

  sendPacket(p);
}

static void processPacket(const MeshPacket &p) {
  if (p.deviceId == deviceId)
    return;

  if (alreadySeen(p.deviceId, p.seq))
    return;

  if (p.ttl == 0)
    return;

  switch (state) {
  case BoatState::Idle:
  case BoatState::Armed: {
    MeshPacket out = p;
    out.ttl--;

    sendPacket(out);
    break;
  }

  case BoatState::Alarm:
    break;
  }
}

void onLoRaReceive(int packetSize) {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  if (packetSize != MESH_PACKET_SIZE) {
    digitalWrite(LED_RED, HIGH);
    lora->receive();
    return;
  }
  digitalWrite(LED_GREEN, HIGH);

  MeshPacket p;

  uint8_t *raw = (uint8_t *)&p;

  for (int i = 0; i < MESH_PACKET_SIZE; i++)
    raw[i] = lora->read();

  Serial.println("----- LORA PACKET RECEIVED -----");

  Serial.print("DeviceId: ");
  Serial.println(p.deviceId);

  Serial.print("Seq: ");
  Serial.println(p.seq);

  Serial.print("TTL: ");
  Serial.println(p.ttl);

  Serial.print("Payload: ");
  for (int i = 0; i < 16; i++) {
    if (p.payload[i] == 0)
      break;
    Serial.print((char)p.payload[i]);
  }

  Serial.println();
  Serial.println("--------------------------------");

  relayPacket = p;
  shouldRelay = true;

  lora->receive();
}

void handleLoRaRelay() {
  if (!shouldRelay)
    return;

  shouldRelay = false;

  if (state != BoatState::Alarm) {
    if (relayPacket.ttl > 0) {
      MeshPacket out = relayPacket;
      out.ttl--;

      sendPacket(out);
    }
  }
}
