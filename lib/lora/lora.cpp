#include <Arduino.h>
#include <complex>
#include <crypto.h>
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

#define DEBUG 1

#ifdef DEBUG
struct PacketMetric {
  uint32_t originId;
  uint16_t seq;

  uint32_t firstRxTime;
  uint32_t lastRxTime;

  uint8_t rxCount;
  uint8_t forwardCount;

  int firstRSSI;
  float firstSNR;
};

static PacketMetric metrics[CACHE_SIZE];
static uint8_t metricIndex = 0;

bool updateMetricDuplicate(uint32_t id, uint16_t seq, uint32_t now) {

  for (int i = 0; i < CACHE_SIZE; i++) {

    if (metrics[i].originId == id && metrics[i].seq == seq) {

      metrics[i].lastRxTime = now;
      metrics[i].rxCount++;

      return true;
    }
  }

  return false;
}

void saveFirstReception(uint32_t id, uint16_t seq, uint32_t time, int rssi,
                        float snr) {

  metrics[metricIndex].originId = id;
  metrics[metricIndex].seq = seq;

  metrics[metricIndex].firstRxTime = time;
  metrics[metricIndex].lastRxTime = time;

  metrics[metricIndex].rxCount = 1;

  metrics[metricIndex].firstRSSI = rssi;
  metrics[metricIndex].firstSNR = snr;

  metricIndex++;

  metricIndex %= CACHE_SIZE;
}
#endif

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

#ifdef DEBUG
  uint32_t txStart = micros();
#endif
  lora->beginPacket();

  for (int i = 0; i < MESH_PACKET_SIZE; i++)
    lora->write(raw[i]);

  lora->endPacket();

#ifdef DEBUG
  uint32_t txEnd = micros();
  uint32_t airtime = txEnd - txStart;
  Serial.print("Airtime");
  Serial.println(airtime);
#endif

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

  uint8_t test_payload[8] = {10, 20, 30, 40, 50, 60, 70, 80};

  bool ret = aesGcmEncrypt(test_key, p.originId, p.seq, p.ttl,
                           test_payload,         // Cosa voglio cifrare
                           sizeof(test_payload), // Quanti byte (8)
                           p.payload,            // DOVE scrivere il Ciphertext
                           p.tag                 // DOVE scrivere il Tag
  );

  // memcpy(p.payload, test_payload, sizeof(test_payload));

  if (ret) {
    Serial.println("CRYPTO OK");

  } else {
    Serial.println("ERROR CRYPTO");
  }

  addSeen(p.originId, p.seq);

  sendPacket(p);
}

void onLoRaReceive(int packetSize) {

  digitalWrite(LED_GREEN, LOW);

#ifdef DEBUG
  uint32_t rxTimestamp = micros();
#endif

  if (packetSize != MESH_PACKET_SIZE) {

    digitalWrite(LED_RED, HIGH);

    lora->receive();

    return;
  }

  MeshPacket p;

  uint8_t *raw = (uint8_t *)&p;

  for (int i = 0; i < MESH_PACKET_SIZE; i++)
    raw[i] = lora->read();

#ifdef DEBUG
  int rssi = lora->packetRssi();
  float snr = lora->packetSnr();
#endif

  Serial.println("----- LORA PACKET RECEIVED -----");

  Serial.print("OriginId: ");
  Serial.println(p.originId);

  Serial.print("DeviceId: ");
  Serial.println(p.deviceId);

  Serial.print("Seq: ");
  Serial.println(p.seq);

  Serial.print("TTL: ");
  Serial.println(p.ttl);

#ifdef DEBUG
  Serial.print("RSSI: ");
  Serial.println(rssi);

  Serial.print("SNR: ");
  Serial.println(snr);
#endif

  Serial.print("Hop: ");
  Serial.println(DEFAULT_TTL + 1 - p.ttl);

  // Stampa il payload (assumendo sia lungo 8 byte, o usa sizeof(p.payload))
  Serial.print("Payload (HEX): ");
  for (int i = 0; i < sizeof(p.payload); i++) {
    if (p.payload[i] < 0x10)
      Serial.print("0"); // Aggiunge lo zero iniziale per numeri < 16
    Serial.print(p.payload[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Stampa il tag (assumendo sia lungo 12 byte, o usa sizeof(p.tag))
  Serial.print("Tag (HEX): ");
  for (int i = 0; i < sizeof(p.tag); i++) {
    if (p.tag[i] < 0x10)
      Serial.print("0");
    Serial.print(p.tag[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  uint8_t plain[8];
  int ret = aesGcmDecrypt(test_key,

                          p.originId, p.seq, p.ttl,

                          p.payload, sizeof(p.payload),

                          p.tag,

                          plain);
  if (ret) {
    Serial.print("Decrypted payload: ");
    for (int i = 0; i < sizeof(plain); i++) {
      Serial.print(plain[i]);
      Serial.print(" ");
    }
    Serial.println();
  }

  Serial.println("--------------------------------");

#ifdef DEBUG
  if (updateMetricDuplicate(p.originId, p.seq, rxTimestamp)) {
    cancelForward(p);

    lora->receive();

    digitalWrite(LED_GREEN, HIGH);

    return;
  }
#endif
#ifndef DEBUG
  if (isSeen(p.originId, p.seq)) {
    cancelForward(p);

    lora->receive();

    digitalWrite(LED_GREEN, HIGH);

    return;
  }
#endif

  relayPacket = p;

#ifdef DEBUG
  saveFirstReception(p.originId, p.seq, rxTimestamp, rssi, snr);
#endif
  relayTime = millis() + random(50, 150);
#ifdef DEBUG
  Serial.print("RelayTime");
  Serial.println(relayTime);
#endif
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
