#include <Arduino.h>
#include "PowerManagement.h"
#include <LiquidCrystal.h>

LiquidCrystal LCD(LCD_RS_PIN, LCD_EN_PIN, LCD_D0_PIN, LCD_D1_PIN, LCD_D2_PIN, LCD_D3_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("PMS starting...");


  //pin setup
  pinMode(LC_ENABLE, OUTPUT);
  pinMode(M2_ENABLE, OUTPUT);
  pinMode(Motor_ENABLE, OUTPUT);
  pinMode(M9_ENABLE, OUTPUT);
  pinMode(NS_ENABLE, OUTPUT);
  pinMode(AUX_ENABLE, OUTPUT);
  //initialization code

  //start up rovecomm
  RoveComm.begin(RC_PMSBOARD_IPADDRESS);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  
}

float PMSmap(uint16_t measured, uint16_t toADC, uint16_t fromADC, float toAnalog, float fromAnalog) {
  float m = (toAnalog - fromAnalog) / (toADC - fromADC);
  return (measured - fromADC) * m + fromAnalog;
}

//Not right
void readCellVoltages() {
  for (int i = 0; i < NUM_CELLS; i++) {
    float readVal = analogRead(cellPins[i]);
    float newVal = PMSmap(readVal, 0, 3.3, 0, 1023);
    cellVoltages[i] = newVal;

  }
}

void readPackVotlage() {
  packVoltage = 0;
  for (int i = 0; i < NUM_CELLS; i++) {
    packVoltage += cellVoltages[i];
  }
}

/**
 * @param data bitmask of system states (1 or 0) in order of MOTOR, LC, AUX, NS
 */
void enableBusses(uint8_t data) {
  if (data & MOTOR_ENABLE_BIT) {
    digitalWrite(Motor_ENABLE, HIGH);
    motorEnabled = true;
    delay(500);
  }
  if (data & LC_ENABLE_BIT) {
    digitalWrite(LC_ENABLE, HIGH);
    lowCurrentEnabled = true;
    delay(500);
  }
  if (data & AUX_ENABLE_BIT) {
    digitalWrite(AUX_ENABLE, HIGH);
    auxEnabled = true;
    delay(500);
  }
  if (data & NETWORK_ENABLE_BIT) {
    digitalWrite(NS_ENABLE, HIGH);
    nsEnabled = true;
    delay(500);
  }
}
void disableBusses(uint8_t data) {
  if (data & MOTOR_ENABLE_BIT) {
    digitalWrite(Motor_ENABLE, LOW);
    motorEnabled = false;
    delay(500);
  }
  if (data & LC_ENABLE_BIT) {
    digitalWrite(LC_ENABLE, LOW);
    lowCurrentEnabled = false;
    delay(500);
  }
  if (data & AUX_ENABLE_BIT) {
    digitalWrite(AUX_ENABLE, LOW);
    auxEnabled = false;
    delay(500);
  }
  if (data & NETWORK_ENABLE_BIT) {
    digitalWrite(NS_ENABLE, LOW);
    nsEnabled = false;
    delay(500);
  }
}

void restart() {
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT);
  //buzz on
  delay(1000);
  //buzz off
  enableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT);

}
void eStop() {
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT);
  //ring buzzer here
}
void suicide() {
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT);
  //buzz infinitely
}

void readPackCurrent() {
  float measure = analogRead(PACK_CURRENT_SENSE_PIN);
  packCurrent = PMSmap(measure, 0, 1023, 0, 3.3);
}

void telemetry() {
  RoveComm.write(RC_PMSBOARD_CELLVOLTAGE_DATA_ID,RC_PMSBOARD_CELLVOLTAGE_DATA_COUNT,cellVoltages);
  RoveComm.write(RC_PMSBOARD_PACKVOLTAGE_DATA_ID, packVoltage);
  RoveComm.write(RC_PMSBOARD_PACKCURRENT_DATA_ID, packCurrent);
  uint8_t busStatuses =
    (motorEnabled ? MOTOR_ENABLE_BIT : 0) |
    (lowCurrentEnabled ? LC_ENABLE_BIT : 0) |
    (auxEnabled ? AUX_ENABLE_BIT : 0) |
    (nsEnabled ? NETWORK_ENABLE_BIT : 0);
  RoveComm.write(RC_PMSBOARD_BUSSTATUS_DATA_ID,busStatuses);
}


