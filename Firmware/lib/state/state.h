#pragma once
#include <cstdint>

#define NODE 2
// #define DATA 1

typedef enum { Idle, Armed, Alarm } BoatState;

void initState();
void updateState();
const char *stateToString(BoatState s);

extern BoatState state;
