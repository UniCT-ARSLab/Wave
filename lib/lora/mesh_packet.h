#pragma once

#define MESH_PACKET_SIZE 33

static uint8_t localAddress = 1;
static uint8_t destination = 0xFF;
static uint8_t msgId = 0;

struct MeshPacket {
  uint32_t deviceId; // 4 bytes
  uint16_t seq;      // 2 bytes
  uint8_t ttl;       // 1 byte
  uint8_t nonce[8];  // 8 bytes

  uint8_t payload[16]; // encrypted lat/lon

  uint16_t crc; // 2 bytes
};
