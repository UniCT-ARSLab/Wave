#pragma once

#include <mesh_packet.h>

extern int counter;

void initLoRaNetwork();

void sendPacket(const MeshPacket &packet);

void sendAlert();

void onLoRaReceive(int packetSize);

void handleLoRaRelay();

bool isSeen(uint32_t id, uint16_t seq);

void addSeen(uint32_t id, uint16_t seq);

void cancelForward(const MeshPacket &packet);
