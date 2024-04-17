#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

// allow for non blocking error beeps
// this class is currently unused

class Buzzer {
public:
  // Constructor
  Buzzer(uint8_t ctlPin): m_ctlPin(ctlPin) {}

  // Member functions
  void buzz(const String &pattern);
  void update();

private:
  // Member variables
  uint8_t m_ctlPin;

  // bool m_buzzing = false;
  uint32_t m_beginActionTimestamp = 0;
  uint32_t m_nextActionTimestamp = 0;
  size_t m_position = 0;

  String m_currentBuzzPattern = "";
};

#endif