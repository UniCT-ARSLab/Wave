#include <autoencoder_data.h>
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
#ifndef DATA
    handleLoRaRelay();
    if (is_there_anomaly()) {
      state = BoatState::Alarm;
    }
#endif
#ifdef DATA
    print_data();
#endif
    break;

  case BoatState::Alarm:

#ifndef DATA
    static uint32_t lastSend = 0;

    if (millis() - lastSend > 2500) {
      sendAlert();
      lastSend = millis();
    }
#endif

#ifdef DATA
    print_data();
#endif

    break;
  }
}
