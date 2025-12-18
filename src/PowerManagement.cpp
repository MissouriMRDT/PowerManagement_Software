#include <Arduino.h>
#include "PowerManagement.h"
#include <LiquidCrystal.h>

LiquidCrystal LCD(LCD_RS_PIN, LCD_EN_PIN, LCD_D0_PIN, LCD_D1_PIN, LCD_D2_PIN, LCD_D3_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

//dummy variable
uint8_t dummy = 0;


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

  //initialize LCD
  LCD.begin(20,4);

  //start up rovecomm
  RoveComm.begin(RC_PMSBOARD_IPADDRESS);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  readCellVoltages();
  readPackCurrent();
  readPackVotlage();
  readAuxCurrent();

  for (int i = 0; i < 6; i++) {
    if (cellVoltages[i] < 2.7) {
      errorCellCritical();
    } 
  }
  readPackVotlage();
  if (packVoltage < 18) {
    suicide();
  }
  uint32_t currTime = millis();
  if (packCurrent >= 75) {
    if ((currTime - lastAcceptableTimePackCurrent) >= 5) {
      errorPackOvercurrent();
    }
    else {
      lastAcceptableTimePackCurrent = currTime;
    }
  }
  
  if (auxCurrent >= 15) {
    if (currTime - lastAcceptableTimePackCurrent >= 5) {
      errorAuxOvercurrent();
    }
    else {
      lastAcceptableTimePackCurrent = currTime;
    }
  }

  //handle rovecom data
  RoveComm.read(packet);
  switch (packet.dataId) {
    case RC_PMSBOARD_ESTOP_DATA_ID: 
      eStop();
      break;
    case RC_PMSBOARD_REBOOT_DATA_ID:
      restart();
      break;
    case RC_PMSBOARD_SUICIDE_DATA_ID:
      suicide();
      break;
    case RC_PMSBOARD_ENABLEBUS_DATA_ID:
      enableBusses(((uint8_t*)packet.data)[0]);
      break;
    case RC_PMSBOARD_DISABLEBUS_DATA_ID:
      uint8_t data = ((uint8_t*)packet.data)[0] & ~NETWORK_ENABLE_BIT;
      disableBusses(data);
      break;
    case RC_PMSBOARD_SETBUS_DATA_ID:
      uint8_t data = ((uint8_t*)packet.data)[0] | NETWORK_ENABLE_BIT; // make sure network switch is also enabled
      enableBusses(data);
      disableBusses(~data);
      break;
  }

}

float PMSmap(uint16_t measured, uint16_t toADC, uint16_t fromADC, float toAnalog, float fromAnalog) {
  float m = (toAnalog - fromAnalog) / (toADC - fromADC);
  return (measured - fromADC) * m + fromAnalog;
}

void readCellVoltages() {
  for (int i = 0; i < NUM_CELLS; i++) {
    for (int j = 0; j < 1000; j++) {
      float newVoltage = PMSmap((uint16_t) analogRead(cellPins[i]), 0, 1023, 0, 4.2); //how do I get full range of voltage
      if (j == 0 || cellVoltages[i] < newVoltage) {
        cellVoltages[i] = newVoltage;
      }
    }
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
/**
 * @param data bitmask of system states (1 or 0) in order of MOTOR, LC, AUX, NS
 */
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

/**
 * @note Turns off all systems for 1 second
 */
void restart() {
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT);
  //buzz on
  delay(1000);
  //buzz off
  enableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT);

}

/**
 * @note Turns off all systems except network
 */
void eStop() {
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT);
  //ring buzzer here
}

/**
 * @note Turns off all systems
 */
void suicide() {
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT);
  //buzz infinitely
}

void readPackCurrent() {
  float measure = analogRead(PACK_CURRENT_SENSE_PIN);
  packCurrent = PMSmap(measure, 0, 1023, 0, 100);
}

void readAuxCurrent() {
  float measure = analogRead(AUX_CURRENT_SENSE_PIN);
  auxCurrent = PMSmap(measure, 0, 1023, 0, 15);
}

/**
 * @note Send data to rovecom
 */
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

void errorPackOvercurrent() {
  RoveComm.write(RC_PMSBOARD_PACKOVERCURRENT_DATA_ID, dummy);
  eStop();
}
void errorAuxOvercurrent() {
  RoveComm.write(RC_PMSBOARD_AUXOVERCURRENT_DATA_ID, dummy);
  disableBusses(AUX_ENABLE_BIT);
  //buzzer start
}
void errorCellUnderVoltage() {
  RoveComm.write(RC_PMSBOARD_CELLUNDERVOLTAGE_DATA_ID, dummy);
  eStop();
}
void errorCellCritical() {
  RoveComm.write(RC_PMSBOARD_CELLCRITICAL_DATA_ID, dummy);
  delay(1000);
}