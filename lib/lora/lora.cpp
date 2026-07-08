#include <Arduino.h>
#include <cstdint>
#include <iot_board.h>
#include <lora.h>
#include <mesh_packet.h>
#include <state.h>

#define CACHE_SIZE 32

extern int counter;

struct SeenPacket {
  uint32_t deviceId;
  uint16_t seq;
};

static SeenPacket cache[CACHE_SIZE];
static uint8_t cacheIndex = 0;

static uint32_t deviceId = NODE;
static uint16_t seqCounter = 0;

MeshPacket relayPacket;
volatile bool shouldRelay = false;
uint32_t relayTime = 0;

void initLoRaNetwork() {

  lora->onReceive(onLoRaReceive);
  lora->receive();

  randomSeed(micros());
}

void sendPacket(const MeshPacket &packet) {

  digitalWrite(LED_YELLOW, LOW);

  const uint8_t *raw = (const uint8_t *)&packet;

  lora->beginPacket();

  for (int i = 0; i < MESH_PACKET_SIZE; i++)
    lora->write(raw[i]);

  lora->endPacket();

  lora->receive();

  digitalWrite(LED_YELLOW, HIGH);
}

bool isSeen(uint32_t id, uint16_t seq) {

  for (int i = 0; i < CACHE_SIZE; i++) {

    if (cache[i].deviceId == id && cache[i].seq == seq) {

      return true;
    }
  }

  return false;
}

void addSeen(uint32_t id, uint16_t seq) {

  cache[cacheIndex].deviceId = id;
  cache[cacheIndex].seq = seq;

  cacheIndex = (cacheIndex + 1) % CACHE_SIZE;
}

void cancelForward(const MeshPacket &p) {

  if (!shouldRelay)
    return;

  if (relayPacket.originId == p.originId && relayPacket.seq == p.seq) {

    shouldRelay = false;

    Serial.println("Forward cancelled");
  }
}

void sendAlert() {

  MeshPacket p = {};

  p.deviceId = deviceId;

  p.originId = deviceId;

  p.seq = seqCounter++;

  p.ttl = DEFAULT_TTL;

  const char *msg = "ALERT";

  memcpy(p.payload, msg, strlen(msg));

  p.crc = 0xFFFF;

  addSeen(p.originId, p.seq);

  sendPacket(p);
}

void onLoRaReceive(int packetSize) {

  digitalWrite(LED_GREEN, LOW);

  if (packetSize != MESH_PACKET_SIZE) {

    digitalWrite(LED_RED, HIGH);

    lora->receive();

    return;
  }

  MeshPacket p;

  uint8_t *raw = (uint8_t *)&p;

  for (int i = 0; i < MESH_PACKET_SIZE; i++)
    raw[i] = lora->read();

  Serial.println("----- LORA PACKET RECEIVED -----");

  Serial.print("OriginId: ");
  Serial.println(p.originId);

  Serial.print("DeviceId: ");
  Serial.println(p.deviceId);

  Serial.print("Seq: ");
  Serial.println(p.seq);

  Serial.print("TTL: ");
  Serial.println(p.ttl);

  Serial.println("--------------------------------");

  if (isSeen(p.originId, p.seq)) {
    cancelForward(p);

    lora->receive();

    digitalWrite(LED_GREEN, HIGH);

    return;
  }

  relayPacket = p;

  relayTime = millis() + random(50, 150);

  shouldRelay = true;

  digitalWrite(LED_GREEN, HIGH);

  counter++;

  lora->receive();
}

void handleLoRaRelay() {

  if (!shouldRelay)
    return;

  if (millis() < relayTime)
    return;

  shouldRelay = false;

  if (relayPacket.ttl == 0)
    return;

  if (isSeen(relayPacket.originId, relayPacket.seq)) {
    return;
  }

  MeshPacket out = relayPacket;

  out.ttl--;

  out.deviceId = deviceId;

  addSeen(out.originId, out.seq);

  sendPacket(out);
}
