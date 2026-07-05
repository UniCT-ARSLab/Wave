#pragma once

#include <mesh_packet.h>

void initLoRaNetwork();

void handleLoRaRelay();

void onLoRaReceive(int packetSize);

void sendAlert();

static void processPacket(const MeshPacket &packet);

void sendPacket(const MeshPacket &packet);
