#pragma once
#include <cstdint>

const uint8_t NODE_ID = 1;

typedef enum { Idle, Armed, Alarm } BoatState;

void initState();
void updateState();
const char *stateToString(BoatState s);

extern BoatState state;
