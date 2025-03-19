#ifndef BUTTON_H
#define BUTTON_H

#include "Config.h"

class Button {
public:
  Button();
  void initialize();
  bool isPressed();
  void clearPressFlag();
  

  static void IRAM_ATTR buttonISR();
  
private:
  static volatile bool buttonPressed;
  static volatile unsigned long lastPressTime;
};

#endif // BUTTON_H
