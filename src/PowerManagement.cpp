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

  //turn everything on
  enableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | M2_ENABLE_BIT | M9_ENABLE_BIT | NETWORK_ENABLE_BIT);

  //initialization code

  //initialize LCD
  LCD.begin(20,4);

  //initialize buzzer
  buzzer.init();

  telemetryRunner.begin(telemetry, TELEMETRY_PERIOD);

  //start up rovecomm
  Serial.println("Starting rovecomm...");
  RoveComm.begin(RC_PMSBOARD_IPADDRESS);
  Serial.println("Rovecomm has been initialized.");
  
}

void loop() {
  //update current values
  readCellVoltages();
  readPackCurrent();
  readPackVotlage();
  readAuxCurrent();
  readLCCurrent();
  readM2Current();
  readM9Current();
  readNSCurrent();

  Serial.print("BattVolts: ");
  Serial.println(packVoltage);
  Serial.println("Cell Voltages: ");
  for(int i = 0; i < NUM_CELLS; i++) {
    Serial.print("Cell ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(cellVoltages[i]);
  }

  Serial.print("Battery Current: ");
  Serial.println(packCurrent);
  Serial.println("Ports Current: ");
  Serial.print("Network switch: ");
  Serial.println(nsCurrent);
  Serial.print("Low current: ");
  Serial.println(lcCurrent);
  Serial.println("Aux: ");
  Serial.println(auxCurrent);
  Serial.println("M9: ");
  Serial.println(m9Current);
  Serial.println("M2: ");
  Serial.println(m2Current);

  
  for (int i = 0; i < 6; i++) {
    if (cellVoltages[i] < 2.8 && millis() - lastCellUndervoltage > 1000) {
      errorCellUnderVoltage(1 << i);
    } else if (cellVoltages[i] < 2.7) {
      errorCellCritical(1 << i);
    }
  }
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
    {  
      uint8_t data = ((uint8_t*)packet.data)[0] & ~NETWORK_ENABLE_BIT;
      disableBusses(data);
      break;
    }
    case RC_PMSBOARD_SETBUS_DATA_ID:
    {
      uint8_t data = ((uint8_t*)packet.data)[0] | NETWORK_ENABLE_BIT; // make sure network switch is also enabled
      enableBusses(data);
      disableBusses(~data);
      break;
    }
  }
  LCD.write("BattVolts: " );
  LCD.write(packVoltage);
  LCD.setCursor(0,1);
  LCD.write("BattCurr: ");
  LCD.write(packCurrent);
  buzzer.update();
}

/*
 * Mapping functions derived from current sensor calibration data. See sharepoint:
 * 2026 -> Arch Electrical -> Documentation -> Core -> Power Management System -> PMS Current Calibration
 */
float LCMap(uint16_t measured) {
  // y = 23.925x - 39.528
  float temp = (3.3 / 1023) * measured;
  float output ((23.925 * temp) - 39.528);
  if (output < 0) {
      output = 0;
  }
  return output;
}
float PackMap(uint16_t measured) {
  // 38.765 - 12.637
  float temp = (3.3 / 1023) * measured;
  float output ((38.765 * temp) - 12.637);
  if (output < 0) {
      output = 0;
  }
  return output;
}
float AuxMap(uint16_t measured) {
  // y = 23.214x - 38.369
  float temp = (3.3 / 1023) * measured;
  float output ((23.214 * temp) - 38.369);
  if (output < 0) {
      output = 0;
  }
  return output;
}
float POEMap(uint16_t measured) {
  // Uses equation from M9 testing
  // y = 21.63x - 35.703
  float temp = (3.3 / 1023) * measured;
  float output = ((21.63 * temp) - 35.703);
  if (output < 0) {
      output = 0;
  }
  return output;
}

float NSMap(uint16_t measured) {
  // y = 21.378x - 35.296
  float temp = (3.3 / 1023) * measured;
  float output = ((21.378 * temp) - 35.296);
  if (output < 0) {
    output = 0;
  }
  return output;
}

void readCellVoltages() {
  for (int i = 0; i < NUM_CELLS; i++) {
    for (int j = 0; j < 1000; j++) {
      float newVoltage = mapCellVoltage((uint16_t) analogRead(cellPins[i])); //how do I get full range of voltage
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
 * @param data bitmask of system states (1 or 0) in order of MOTOR, LC, AUX, NS, M2, M9
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
  if (data & M2_ENABLE_BIT) {
    digitalWrite(M2_ENABLE, HIGH);
    m2Enabled = true;
    delay(500);
  }
  if (data & M9_ENABLE_BIT) {
    digitalWrite(M9_ENABLE, HIGH);
    m9Enabled = true;
    delay(500);
  }
  if (data & NETWORK_ENABLE_BIT) {
    digitalWrite(NS_ENABLE, HIGH);
    nsEnabled = true;
    delay(500);
  }
}
/**
 * @param data bitmask of system states (1 or 0) in order of MOTOR, LC, AUX, NS, M2, M9
 */
void disableBusses(uint8_t data) {
  if (data & MOTOR_ENABLE_BIT) {
    digitalWrite(Motor_ENABLE, LOW);
    motorEnabled = false;
  }
  if (data & LC_ENABLE_BIT) {
    digitalWrite(LC_ENABLE, LOW);
    lowCurrentEnabled = false;
  }
  if (data & AUX_ENABLE_BIT) {
    digitalWrite(AUX_ENABLE, LOW);
    auxEnabled = false;
  }
  if (data & NETWORK_ENABLE_BIT) {
    digitalWrite(NS_ENABLE, LOW);
    nsEnabled = false;
  }
  if (data & M2_ENABLE_BIT) {
    digitalWrite(M2_ENABLE, LOW);
    m2Enabled = false;
  }
  if (data & M9_ENABLE_BIT) {
    digitalWrite(M9_ENABLE, LOW);
    m9Enabled = false;
  }
}

/**
 * @note Turns off all systems for 1 second
 */
void restart() {
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT | M9_ENABLE_BIT | M2_ENABLE_BIT);
  buzzer.buzz("beeeeep");
  while (buzzer.isBuzzing()) {
    buzzer.update();
    delay(5);
  }
  buzzer.buzz("beeeeep");
  enableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT | M9_ENABLE_BIT | M2_ENABLE_BIT);
}

/**
 * @note Turns off all systems except network
 */
void eStop() {
  Serial.println("EStopping!");
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT);
  buzzer.buzz("beeeeep");
}

/**
 * @note Turns off all systems
 */
void suicide() {
  Serial.println("Suiciding!");
  disableBusses(MOTOR_ENABLE_BIT | LC_ENABLE_BIT | AUX_ENABLE_BIT | NETWORK_ENABLE_BIT | M9_ENABLE_BIT | M2_ENABLE_BIT);
  //buzz infinitely
  buzzer.buzz("bEEEp===u");
  while (buzzer.isBuzzing()) {
    buzzer.update();
    delay(5);
  }
}

void readPackCurrent() {
  float measure = analogRead(PACK_CURRENT_SENSE_PIN);
  packCurrent = PackMap(measure);
}

void readAuxCurrent() {
  float measure = analogRead(AUX_CURRENT_SENSE_PIN);
  auxCurrent = AuxMap(measure);
}

void readNSCurrent() {
  float measure = analogRead(NS_CURRENT_SENSE_PIN);
  nsCurrent = NSMap(measure);
}

void readLCCurrent() {
  float measure = analogRead(LC_CURRENT_SENSE_PIN);
  lcCurrent = LCMap(measure);
}

void readM9Current() {
  float measure = analogRead(M9_CURRENT_SENSE_PIN);
  m9Current = POEMap(measure);
}

void readM2Current() {
  float measure = analogRead(M2_CURRENT_SENSE_PIN);
  m2Current = POEMap(measure);
}

/*
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}*/

float map_float(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float mapCellVoltage(float measured) {
  // float slope = 1023 / 4.2;
  
  // return (slope * measured);
  return map_float(measured, 0, 1023, 0, 4.2);
}

/**
 * @note Send data to rovecom
 */
void telemetry() {
  uint8_t index = 0;
  telemArray[index++] = packCurrent;
  telemArray[index++] = auxCurrent;
  telemArray[index++] = lcCurrent;
  telemArray[index++] = nsCurrent;
  telemArray[index++] = m2Current;
  telemArray[index++] = m9Current;
  for (int i = 0; i < NUM_CELLS; ++i) {
    telemArray[index++] = cellVoltages[i];
  }
  RoveComm.write(RC_PMSBOARD_CURRENTANDVOLTAGE_DATA_ID,RC_PMSBOARD_CURRENTANDVOLTAGE_DATA_COUNT,telemArray);
  uint8_t busStatuses =
    (motorEnabled ? MOTOR_ENABLE_BIT : 0) |
    (lowCurrentEnabled ? LC_ENABLE_BIT : 0) |
    (auxEnabled ? AUX_ENABLE_BIT : 0) |
    (m2Enabled ? M2_ENABLE_BIT : 0) | 
    (m9Enabled ? M9_ENABLE_BIT : 0) |
    (nsEnabled ? NETWORK_ENABLE_BIT : 0);
  RoveComm.write(RC_PMSBOARD_BUSSTATUS_DATA_ID,busStatuses);
}

void errorPackOvercurrent() {
  RoveComm.write(RC_PMSBOARD_PACKOVERCURRENT_DATA_ID, dummy);
  Serial.println("Error: Pack overcurrent");
  eStop();
}
void errorAuxOvercurrent() {
  RoveComm.write(RC_PMSBOARD_AUXOVERCURRENT_DATA_ID, dummy);
  Serial.println("Error: Aux overcurrent");

  float measure = analogRead(AUX_CURRENT_SENSE_PIN);
  Serial.print("Aux current measured voltage: ");
  Serial.println(measure);
  buzzer.buzz("beeeeep");
  disableBusses(AUX_ENABLE_BIT);
  //buzzer start
}
void errorCellUnderVoltage(uint8_t bitmask) {
  RoveComm.write(RC_PMSBOARD_CELLUNDERVOLTAGE_DATA_ID, bitmask);
  Serial.println("Error: Cell under voltage");
  eStop();
  buzzer.buzz("beeeeep");
  lastCellUndervoltage = millis();
}
void errorCellCritical(uint8_t bitmask) {
  RoveComm.write(RC_PMSBOARD_CELLCRITICAL_DATA_ID, bitmask);
  Serial.println("Error: Cell critical");
  // give time for packet to be delivered
  delay(500);
  suicide();
}