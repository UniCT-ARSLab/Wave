#include <Arduino.h>
#include <autoencoder_data.h>
#include <iot_board.h>
#include <lora.h>
#include <menu.h>
#include <state.h>

void onBtn1Released(uint8_t pinBtn) { menuNext(); }

void onBtn2Released(uint8_t pinBtn) { menuSelect(); }

void setup() {

  IoTBoard::init_display();
  IoTBoard::init_serial(115200);
  IoTBoard::init_buttons();
  IoTBoard::init_leds();
  Wire.begin();
  IoTBoard::init_spi();

  if (!IoTBoard::init_lora()) {
    display->clearDisplay();
    display->println("LoRa FAILED");
    display->display();
    while (1)
      ;
  }
  initLoRaNetwork();
  initState();
  if (state != BoatState::Idle) {
    display->clearDisplay();
    display->println("state FAILED");
    display->display();
    while (1)
      ;
  }

  init_autoencoder();

  initMenu();

  buttons->onBtn1Release(onBtn1Released);
  buttons->onBtn2Release(onBtn2Released);

  drawMenu();
}

void loop() {
  buttons->update();
  updateState();
}
