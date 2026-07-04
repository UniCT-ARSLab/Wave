#include <iot_board.h>
#include <state.h>

BoatState state;

void initState() { BoatState state = BoatState::Idle; }

void updateState() {

  switch (state) {
  case BoatState::Idle:
    // sistema fermo
    break;

  case BoatState::Armed:
    // sistema attivo
    break;

  case BoatState::Alarm:
    // emergenza
    break;
  }
}

const char *stateToString(BoatState s) {
  switch (s) {
  case BoatState::Idle:
    return "Idle";

  case BoatState::Armed:
    return "Armed";

  case BoatState::Alarm:
    return "Alarm";

  default:
    return "Unknown";
  }
}
