#include "../include/Button.h"


volatile bool Button::buttonPressed = false;
volatile unsigned long Button::lastPressTime = 0;

Button::Button() {
  // Constructor
}

void Button::initialize() {
  // Set up the button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Attach interrupt
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
}

bool Button::isPressed() {
  return buttonPressed;
}

void Button::clearPressFlag() {
  buttonPressed = false;
}

void IRAM_ATTR Button::buttonISR() {
  unsigned long now = millis();
  
 
  if (now - lastPressTime > 200) {
    buttonPressed = true;
    lastPressTime = now;
  }
}
