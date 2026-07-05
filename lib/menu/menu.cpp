#include <iot_board.h>

#include "menu.h"
#include "state.h"

const char *menuItems[] = {"Idle", "Armed", "Alarm"};

const uint8_t MENU_SIZE = sizeof(menuItems) / sizeof(menuItems[0]);

uint8_t selectedItem = 0;

void initMenu() {
  selectedItem = 0;
  drawMenu();
}

void drawMenu() {
  display->clearDisplay();
  display->setCursor(0, 0);

  // Riga superiore: stato + ID
  display->print("State: ");
  display->println(stateToString(state));
  display->print("Node: ");
  display->println(NODE);

  display->println("----------------");

  // Menu
  for (uint8_t i = 0; i < MENU_SIZE; i++) {
    if (i == selectedItem)
      display->print("> ");
    else
      display->print("  ");

    display->println(menuItems[i]);
  }

  display->display();
}

void menuNext() {
  selectedItem++;

  if (selectedItem >= MENU_SIZE)
    selectedItem = 0;

  drawMenu();
}

void menuSelect() {
  switch (selectedItem) {
  case 0:
    state = BoatState::Idle;
    break;

  case 1:
    state = BoatState::Armed;
    break;

  case 2:
    state = BoatState::Alarm;
    break;
  }

  drawMenu();
}
