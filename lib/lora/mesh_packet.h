#pragma once

// #define MESH_PACKET_SIZE 37
#include <cstdint>

constexpr uint8_t DEFAULT_TTL = 8;

struct MeshPacket {
  uint32_t originId; // 4 bytes
  uint32_t deviceId; // 4 bytes
  uint16_t seq;      // 2 bytes
  uint8_t ttl;       // 1 byte
  uint8_t nonce[8];  // 8 bytes

  uint8_t payload[16]; // encrypted lat/lon

  uint16_t crc; // 2 bytes
};

constexpr uint8_t MESH_PACKET_SIZE = sizeof(MeshPacket);
