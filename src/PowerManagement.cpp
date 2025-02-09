#include "PowerManagement.h"

#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial.println("PMS Setup");

  // I/O Pins

  // always on unless suicide()
  pinMode(POE_ENABLE_PIN, OUTPUT);
  pinMode(NS_ENABLE_PIN, OUTPUT);
  // turn off when estop() or suicide()
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  pinMode(LOW_CURRENT_ENABLE_PIN, OUTPUT);
  pinMode(AUX_ENABLE_PIN, OUTPUT);

  // turn everything on
  enableBusses(NETWORK_ENABLE_BIT | MOTOR_ENABLE_BIT | CORE_ENABLE_BIT | AUX_ENABLE_BIT);

  // RoveComm
  Serial.println("RoveComm Initializing...");
  RoveComm.begin(RC_PMSBOARD_IPADDRESS);
  Serial.println("Complete.");

  // Telemetry
  Telemetry.begin(telemetry, TELEMETRY_PERIOD);

  buzzer.init();
}

void loop() {
  // Read voltage and current from battery
  readCellVoltages();
  readPackCurrent();

  // Check for critical cells
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    if (cellVoltages[i] <= CELL_CRITICAL_THRESHOLD) {
      errorCellCritical();
    }
  }
  // Check for undervoltage cells
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    if (cellVoltages[i] <= CELL_UNDERVOLT_THRESHOLD) {
      errorCellUndervoltage();
    }
  }
  // Check for overcurrent
  // If it has been a long time since the last normal current value, it is likely not a spike]
  uint32_t now = millis();
  if (packCurrent >= MAX_PACK_CURRENT) {
    if (now - lastAcceptableCurrentTimestamp >= ALLOWABLE_OVERCURRENT_PERIOD) {
      errorPackOvercurrent();
    }
  } else {
    lastAcceptableCurrentTimestamp = now;
  }

  if (auxCurrent >= MAX_AUX_CURRENT) {
    errorAuxOvercurrent();
  }

  // Handle incoming RoveComm packets
  RoveComm.read(packet);
  
  switch (packet.dataId) {
    //Estop
    case RC_PMSBOARD_ESTOP_DATA_ID:
    {
      roverEStop();
      break;
    }
    //Suicide
    case RC_PMSBOARD_SUICIDE_DATA_ID:
    {
      roverSuicide();
      break;
    }
    //Reboot
    case RC_PMSBOARD_REBOOT_DATA_ID:
    {
      roverRestart();
      break;
    }
    //EnableBus
    case RC_PMSBOARD_ENABLEBUS_DATA_ID: 
    {
      enableBusses(((uint8_t*)packet.data)[0]);
      break;
    }
    //DisableBus
    case RC_PMSBOARD_DISABLEBUS_DATA_ID:
    {
      uint8_t data = ((uint8_t*)packet.data)[0] & ~NETWORK_ENABLE_BIT; // do not turn off network
      disableBusses(data);
      break;
    }
    //SetBus
    case RC_PMSBOARD_SETBUS_DATA_ID:
    {
      uint8_t data = ((uint8_t*)packet.data)[0] | NETWORK_ENABLE_BIT; // make sure network switch is also enabled
      enableBusses(data);
      disableBusses(~data);
      break;
    }
  }
  
  buzzer.update();
}

void telemetry() {
  RoveComm.write(RC_PMSBOARD_CELLVOLTAGE_DATA_ID, RC_PMSBOARD_CELLVOLTAGE_DATA_COUNT, cellVoltages);
  RoveComm.write(RC_PMSBOARD_PACKVOLTAGE_DATA_ID, packVoltage);
  RoveComm.write(RC_PMSBOARD_PACKCURRENT_DATA_ID, packCurrent);
  uint8_t busStatus = 
    (motorEnabled   ? MOTOR_ENABLE_BIT   : 0) |
    (coreEnabled    ? CORE_ENABLE_BIT    : 0) |
    (auxEnabled     ? AUX_ENABLE_BIT     : 0) |
    (networkEnabled ? NETWORK_ENABLE_BIT : 0);
  RoveComm.write(RC_PMSBOARD_BUSSTATUS_DATA_ID, busStatus);
}

float analogMap(uint16_t measurement, uint16_t fromADC, uint16_t toADC, float fromAnalog, float toAnalog) {
  float slope = (toAnalog - fromAnalog) / (toADC - fromADC);
  return (measurement - fromADC) * slope + fromAnalog;
}

void readCellVoltages() {
  // Repeatedly read the voltage of all cells
  // To eliminate voltage drops from current spikes, store the largest value into the cellVoltages array
  for (uint16_t j = 0; j < NUM_VOLTAGE_READINGS; j++) {
    for (uint8_t i = 0; i < NUM_CELLS; i++) {
      uint16_t measurement = analogRead(CELL_V_SENSE_PINS[i]);
      float newVoltage = analogMap(measurement, CELL_V_SENSE_ADC_ZERO, CELL_V_SENSE_ADC_OTHER, CELL_V_SENSE_REAL_ZERO, CELL_V_SENSE_REAL_OTHER);
      if (j == 0 || cellVoltages[i] < newVoltage) {
        cellVoltages[i] = newVoltage;
      }
    }
  }

  // Calculate pack voltage; global variable is used in interrupt, so it should only be changed after the total is calculated
  float pv = 0;
  for (uint8_t i = 0; i < NUM_CELLS; i++) {
    pv += cellVoltages[i];
  }
  packVoltage = pv;
}

void readPackCurrent() {
  uint16_t measurement = analogRead(CURRENT_SENSE_PIN);
  packCurrent = analogMap(measurement, PACK_I_SENSE_ADC_ZERO, PACK_I_SENSE_ADC_OTHER, PACK_I_SENSE_REAL_ZERO, PACK_I_SENSE_REAL_OTHER);
}

void roverEStop() {
  // keep POE and network switch on
  disableBusses(MOTOR_ENABLE_BIT | CORE_ENABLE_BIT | AUX_ENABLE_BIT);
  buzzer.buzz("beeeeep"); // nonblocking beep
}

void roverSuicide() {
  // turn everything off
  disableBusses(NETWORK_ENABLE_BIT | MOTOR_ENABLE_BIT | CORE_ENABLE_BIT | AUX_ENABLE_BIT);

  // beep bc bms is on, but everything else off
  // infinite loop because it won't turn back on
  while (true) {
    // smoke detector beep pattern
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
    delay(30000);
  }
}

void roverRestart() {
  // turn everything off
  disableBusses(NETWORK_ENABLE_BIT | MOTOR_ENABLE_BIT | CORE_ENABLE_BIT | AUX_ENABLE_BIT);

  digitalWrite(BUZZER_PIN, HIGH); // blocking beep
  delay(RESTART_DELAY);
  digitalWrite(BUZZER_PIN, LOW);

  // turn everything on
  enableBusses(NETWORK_ENABLE_BIT | MOTOR_ENABLE_BIT | CORE_ENABLE_BIT | AUX_ENABLE_BIT);
}

uint8_t dummy = 0;

void errorPackOvercurrent() {
  RoveComm.write(RC_PMSBOARD_PACKOVERCURRENT_DATA_ID, dummy);
  roverEStop();
}

void errorAuxOvercurrent() {
  RoveComm.write(RC_PMSBOARD_AUXOVERCURRENT_DATA_ID, dummy);
  disableBusses(AUX_ENABLE_BIT); // disable Aux
  buzzer.buzz("beep beeeeep"); // nonblocking beep
}

void errorCellUndervoltage() {
  RoveComm.write(RC_PMSBOARD_CELLUNDERVOLTAGE_DATA_ID, dummy);
  roverEStop();
}

void errorCellCritical() {
  RoveComm.write(RC_PMSBOARD_CELLCRITICAL_DATA_ID, dummy);
  roverSuicide();
}

void enableBusses(uint8_t bitmask) {
  if (bitmask & NETWORK_ENABLE_BIT) {
    networkEnabled = true;
    digitalWrite(POE_ENABLE_PIN, LOW); // active low
    digitalWrite(NS_ENABLE_PIN, LOW);
    delay(500); // prevent current spikes
  }
  if (bitmask & CORE_ENABLE_BIT) {
    coreEnabled = true;
    digitalWrite(LOW_CURRENT_ENABLE_PIN, HIGH);
    delay(500);
  }
  if (bitmask & MOTOR_ENABLE_BIT) {
    motorEnabled = true;
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    delay(500);
  }
  if (bitmask & AUX_ENABLE_BIT) {
    auxEnabled = true;
    digitalWrite(AUX_ENABLE_PIN, HIGH);
    delay(500);
  }
}

void disableBusses(uint8_t bitmask) {
  if (bitmask & MOTOR_ENABLE_BIT) {
    motorEnabled = false;
    digitalWrite(MOTOR_ENABLE_PIN, LOW);
  }
  if (bitmask & CORE_ENABLE_BIT) {
    coreEnabled = false;
    digitalWrite(LOW_CURRENT_ENABLE_PIN, LOW);
  }
  if (bitmask & AUX_ENABLE_BIT) {
    auxEnabled = false;
    digitalWrite(AUX_ENABLE_PIN, LOW);
  }
  if (bitmask & NETWORK_ENABLE_BIT) {
    networkEnabled = false;
    digitalWrite(POE_ENABLE_PIN, HIGH); // active low
    digitalWrite(NS_ENABLE_PIN, HIGH);
  }
}