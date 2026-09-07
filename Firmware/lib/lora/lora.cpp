#include <Arduino.h>
#include <complex>
#include <crypto.h>
#include <cstdint>
#include <iot_board.h>
#include <lora.h>
#include <mesh_packet.h>
#include <state.h>
#include <synctms.h>
#define CACHE_SIZE 32

#define DEBUG 1

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
MeshPacket out;
MeshPacket p;
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

  p = {};

  p.deviceId = deviceId;

  p.originId = deviceId;

  p.seq = seqCounter++;

  p.ttl = DEFAULT_TTL;

  uint8_t test_payload[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  uint64_t txTimestamp = gettime();

  memcpy(test_payload, &txTimestamp, sizeof(txTimestamp));

  Serial.print("TX timestamp BEFORE GCM: ");
  Serial.println(txTimestamp);

  Serial.print("Payload plaintext: ");

  for (int i = 0; i < 8; i++) {
    Serial.printf("%02X ", test_payload[i]);
  }
  bool ret = aesGcmEncrypt(test_key, p.originId, p.seq, p.ttl,
                           test_payload,         // Cosa voglio cifrare
                           sizeof(test_payload), // Quanti byte (8)
                           p.payload,            // DOVE scrivere il Ciphertext
                           p.tag                 // DOVE scrivere il Tag
  );

  if (ret) {
    // Serial.println("CRYPTO OK");
  } else {
    Serial.println("ERROR CRYPTO");
  }

  addSeen(p.originId, p.seq);

  ret = aesCtrEncryptPacket(p);

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

  uint8_t *raw = (uint8_t *)&p;

  for (int i = 0; i < MESH_PACKET_SIZE; i++)
    raw[i] = lora->read();

  if (!aesCtrDecryptPacket(p)) {

    // Serial.println("CTR DECRYPT FAILED");

    digitalWrite(LED_RED, HIGH);

    lora->receive();

    return;
  }

  // Serial.println("CTR DECRYPT OK");

#ifdef DEBUG
  int rssi = lora->packetRssi();
  float snr = lora->packetSnr();

  Serial.println("----- LORA PACKET RECEIVED -----");

  Serial.print("OriginId: ");
  Serial.println(p.originId);

  Serial.print("DeviceId: ");
  Serial.println(p.deviceId);

  Serial.print("Seq: ");
  Serial.println(p.seq);

  Serial.print("TTL: ");
  Serial.println(p.ttl);

  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.print("SNR: ");
  Serial.print(snr);
  Serial.println(" dB");

  Serial.print("Hop: ");
  Serial.println(DEFAULT_TTL + 1 - p.ttl);

  Serial.print("Payload (HEX): ");
  for (int i = 0; i < sizeof(p.payload); i++) {
    if (p.payload[i] < 0x10)
      Serial.print("0"); // Aggiunge lo zero iniziale per numeri < 16
    Serial.print(p.payload[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

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
    uint64_t txTimestamp;
    memcpy(&txTimestamp, plain, sizeof(txTimestamp));
    uint64_t rxTimestamp = gettime();
    uint64_t latency = (uint64_t)rxTimestamp - (uint64_t)txTimestamp;

    // Serial.print("TX timestamp: ");
    // Serial.println(txTimestamp);
    //
    // Serial.print("RX timestamp: ");
    // Serial.println(rxTimestamp);
    //
    Serial.print("Latency: ");
    Serial.print((double)latency / 1000.0);
    Serial.println("ms");

    Serial.print("Decrypted payload: ");
    for (int i = 0; i < sizeof(plain); i++) {
      Serial.print(plain[i]);
      Serial.print(" ");
    }
    Serial.println();
  }

  // Serial.println("--------------------------------");
#endif

  if (isSeen(p.originId, p.seq)) {
    cancelForward(p);

    lora->receive();

    digitalWrite(LED_GREEN, HIGH);

    return;
  }

  relayPacket = p;

  relayTime = millis() + random(100, 500);

#ifdef DEBUG
  Serial.print("RelayTime: ");
  Serial.print(relayTime - millis());
  Serial.println(" ms");
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

  out = relayPacket;
  out.ttl--;

  out.deviceId = deviceId;

  addSeen(out.originId, out.seq);

  if (!aesCtrEncryptPacket(out)) {
    // Serial.println("Relay CTR encrypt failed");
    return;
  }

  sendPacket(out);
}
