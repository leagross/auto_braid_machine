#include "Config.h"

// Button init
void buttonSetup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP); // internal pull-up resistor
}

// True if the button is currently pressed
bool buttonPressed() {
  return digitalRead(BUTTON_PIN) == LOW;
}
