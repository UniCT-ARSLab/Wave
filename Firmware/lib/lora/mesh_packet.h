#pragma once

// #define MESH_PACKET_SIZE 37
#include <cstdint>

constexpr uint8_t DEFAULT_TTL = 8;

#pragma pack(push, 1)

struct MeshPacket {

  uint32_t originId;

  uint32_t deviceId;

  uint32_t seq;

  uint8_t ttl;

  uint8_t payload[8];

  uint8_t tag[12];
};

#pragma pack(pop)

constexpr uint8_t MESH_PACKET_SIZE = sizeof(MeshPacket);
