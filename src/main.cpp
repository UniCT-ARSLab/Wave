#include <Arduino.h>
#include <ios>
#include <iot_board.h>
#include <state.h>

BoatState state;

void setup() {

  IoTBoard::init_display();
  IoTBoard::init_serial(115200);
  IoTBoard::init_buttons();
  Wire.begin();
  IoTBoard::init_spi();

  display->clearDisplay();
  display->println("Setup OK");
  display->display();
  state = BoatState::Idle;
}

void loop() {

  switch (state) {

  case BoatState::Idle:
    display->setCursor(0, 0);
    display->clearDisplay();
    display->println("State:Idle");
    display->display();
    delay(1000);
    state = BoatState::Armed;
    break;

  case BoatState::Armed:

    display->clearDisplay();
    display->println("State:Armed");
    display->display();
    delay(1000);
    state = BoatState::Alarm;

  case BoatState::Alarm:
    display->clearDisplay();
    display->println("State:Alarm");
    display->display();
    delay(1000);
    state = BoatState::Idle;
    break;

  default:
    Serial.println("no valid state");

    break;
  }
}
