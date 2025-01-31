#include "Buzzer.h"

void Buzzer::buzz(const String &pattern) {
  if (pattern.length() == 0) return;
  m_currentBuzzPattern = pattern;
  m_beginActionTimestamp = millis();
  m_nextActionTimestamp = m_beginActionTimestamp;
  m_position = 0;
  digitalWrite(m_ctlPin, LOW);
}

void Buzzer::update() {
  if (m_currentBuzzPattern.length() == 0) return;
  uint32_t now = millis();
  if (now > m_nextActionTimestamp) {
    if (m_position == m_currentBuzzPattern.length()) {
      m_currentBuzzPattern = "";
      digitalWrite(m_ctlPin, LOW);
      return;
    }
    m_beginActionTimestamp = millis();
    char action = m_currentBuzzPattern.charAt(m_position);
    switch (action) {
      case 'b':
        digitalWrite(m_ctlPin, HIGH);
        break;
      case 'e':
        m_nextActionTimestamp = m_beginActionTimestamp + 100;
        break;
      case 'E':
        m_nextActionTimestamp = m_beginActionTimestamp + 1000;
        break;
      case 'p':
        digitalWrite(m_ctlPin, LOW);
        break;
      case ' ':
        m_nextActionTimestamp = m_beginActionTimestamp + 100;
        break;
      case '-':
        m_nextActionTimestamp = m_beginActionTimestamp + 1000;
        break;
      case '=':
        m_nextActionTimestamp = m_beginActionTimestamp + 10000;
        break;
    }
    ++ m_position;
  }
}

void Buzzer::init() {
  pinMode(m_ctlPin, OUTPUT);
  digitalWrite(m_ctlPin, LOW);
}
