#include <iot_board.h>
#include <lora.h>
#include <state.h>

BoatState state;
extern int counter;

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

void initState() { state = BoatState::Idle; }

void updateState() {

  switch (state) {
  case BoatState::Idle:

    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    counter = 0;

    break;

  case BoatState::Armed:
    handleLoRaRelay();
    break;

  case BoatState::Alarm:
    static uint32_t lastSend = 0;

    if (millis() - lastSend > 2500) {
      sendAlert();
      lastSend = millis();
    }

    break;
  }
}
