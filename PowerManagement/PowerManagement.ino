#include "PowerManagement.h"

void setup() {
  Serial.begin(115200);
  Serial.println("PMS Setup");

  //I/O Pins
  pinMode(BUZZER_CTL_PIN, OUTPUT);
  digitalWrite(BUZZER_CTL_PIN, LOW);

  // always on unless suicide()
  pinMode(POE_ENABLE_PIN, OUTPUT);
  pinMode(NS_ENABLE_PIN, OUTPUT);
  // turn off when estop() or suicide()
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  pinMode(CORE_ENABLE_PIN, OUTPUT);
  pinMode(AUX_ENABLE_PIN, OUTPUT);

  // low side enable
  digitalWrite(POE_ENABLE_PIN, LOW);
  digitalWrite(NS_ENABLE_PIN, LOW);
  // high side enable
  roverSetEnables(MOTOR_ENABLE_BIT | CORE_ENABLE_BIT | AUX_ENABLE_BIT);

  //RoveComm
  Serial.println("RoveComm Initializing...");
  RoveComm.begin(RC_PMSBOARD_FIRSTOCTET, RC_PMSBOARD_SECONDOCTET, RC_PMSBOARD_THIRDOCTET, RC_PMSBOARD_FOURTHOCTET, &TCPServer);
  Serial.println("Complete.");

  //Telemetry
  Telemetry.begin(telemetry, TELEMETRY_PERIOD);
}


void loop() {
  // Read voltage and current from battery
  readCells();
  packCurrent = analogMap(analogRead(PACK_I_SENSE_PIN), PACK_I_SENSE_ADC_MIN, PACK_I_SENSE_ADC_MAX, PACK_I_SENSE_REAL_MIN, PACK_I_SENSE_REAL_MAX);
  auxCurrent = analogMap(analogRead(AUX_I_SENSE_PIN), AUX_I_SENSE_ADC_MIN, AUX_I_SENSE_ADC_MAX, AUX_I_SENSE_REAL_MIN, AUX_I_SENSE_REAL_MAX);

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
  //Check for overcurrent
  if (packCurrent >= MAX_PACK_CURRENT) {
    errorPackOvercurrent();
  }
  if (auxCurrent >= MAX_AUX_CURRENT) {
    errorAuxOvercurrent();
  }

  // Handle incoming RoveComm packets
  rovecomm_packet packet = RoveComm.read();

  switch (packet.data_id) {
    //Estop
    case RC_PMSBOARD_ESTOP_DATA_ID:
      roverEStop();
      break;
    //Suicide
    case RC_PMSBOARD_SUICIDE_DATA_ID:
      roverSuicide();
      break;
    //Reboot
    case RC_PMSBOARD_REBOOT_DATA_ID:
      roverRestart();
      break;
    
    //EnableBus
    case RC_PMSBOARD_ENABLEBUS_DATA_ID:
      roverSetEnables(enables | ((uint8_t*)packet.data)[0]);
      break;
    //DisableBus
    case RC_PMSBOARD_DISABLEBUS_DATA_ID:
      roverSetEnables(enables & ~((uint8_t*)packet.data)[0]);
      break;
    //SetBus
    case RC_PMSBOARD_SETBUS_DATA_ID:
      roverSetEnables(enables);
      break;
  }
  
  //buzzer.update();
}


void telemetry() {
  RoveComm.write(RC_PMSBOARD_CELLVOLTAGE_DATA_ID, RC_PMSBOARD_CELLVOLTAGE_DATA_COUNT, cellVoltages);
  RoveComm.write(RC_PMSBOARD_PACKVOLTAGE_DATA_ID, RC_PMSBOARD_PACKVOLTAGE_DATA_COUNT, packVoltage);
  RoveComm.write(RC_PMSBOARD_PACKCURRENT_DATA_ID, RC_PMSBOARD_PACKCURRENT_DATA_COUNT, packCurrent);
  RoveComm.write(RC_PMSBOARD_AUXCURRENT_DATA_ID, RC_PMSBOARD_AUXCURRENT_DATA_COUNT, auxCurrent);
  RoveComm.write(RC_PMSBOARD_MISCCURRENT_DATA_ID, RC_PMSBOARD_MISCCURRENT_DATA_COUNT, miscCurrents);
}


float analogMap(uint16_t measurement, uint16_t fromADC, uint16_t toADC, float fromAnalog, float toAnalog) {
  float slope = (toAnalog - fromAnalog) / (toADC - fromADC);
  return (measurement - fromADC) * slope + fromAnalog;
}
void readCells() {
  // Repeatedly read the voltage of all cells
  // To eliminate voltage drops from current spikes, store the largest value into the cellVoltages array
  for (uint16_t j = 0; j < NUM_VOLTAGE_READINGS; j++) {
    for (uint8_t i = 0; i < NUM_CELLS; i++) {
      uint16_t measurement = analogRead(CELL_V_SENSE_PINS[i]);
      float newVoltage = analogMap(measurement, CELL_V_SENSE_ADC_MIN, CELL_V_SENSE_ADC_MAX, CELL_V_SENSE_REAL_MIN, CELL_V_SENSE_REAL_MAX);
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

void roverEStop() {
  // keep POE and network switch on
  roverSetEnables(0);

  digitalWrite(BUZZER_CTL_PIN, HIGH); // blocking beep
  delay(1000);
  digitalWrite(BUZZER_CTL_PIN, LOW);
}

void roverSuicide() {
  // turn everything off
  roverSetEnables(0);
  // turn off POE and network switch
  digitalWrite(POE_ENABLE_PIN, HIGH);
  digitalWrite(NS_ENABLE_PIN, HIGH);

  digitalWrite(BUZZER_CTL_PIN, HIGH); // blocking beep
  delay(1000);
  digitalWrite(BUZZER_CTL_PIN, LOW);
}

void roverRestart() {
  // turn off POE and network switch
  digitalWrite(POE_ENABLE_PIN, HIGH);
  digitalWrite(NS_ENABLE_PIN, HIGH);

  digitalWrite(BUZZER_CTL_PIN, HIGH); // blocking beep
  delay(RESTART_DELAY);
  digitalWrite(BUZZER_CTL_PIN, LOW);

  // turn on POE and network switch
  digitalWrite(POE_ENABLE_PIN, LOW);
  digitalWrite(NS_ENABLE_PIN, LOW);
  roverSetEnables(MOTOR_ENABLE_BIT | CORE_ENABLE_BIT | AUX_ENABLE_BIT);
}

void roverSetEnables(uint8_t bitfield) {
  if ((enables ^ bitfield) & MOTOR_ENABLE_BIT)
    digitalWrite(MOTOR_ENABLE_PIN, (bitfield & MOTOR_ENABLE_BIT) > 0 ? HIGH : LOW);
  if ((enables ^ bitfield) & CORE_ENABLE_BIT)
    digitalWrite(MOTOR_ENABLE_PIN, (bitfield & CORE_ENABLE_BIT) > 0 ? HIGH : LOW);
  if ((enables ^ bitfield) & AUX_ENABLE_BIT)
    digitalWrite(MOTOR_ENABLE_PIN, (bitfield & AUX_ENABLE_BIT) > 0 ? HIGH : LOW);
  enables = bitfield;
}

uint8_t dummy = 0;

void errorPackOvercurrent() {
  RoveComm.writeReliable(RC_PMSBOARD_PACKOVERCURRENT_DATA_ID, RC_PMSBOARD_PACKOVERCURRENT_DATA_COUNT, dummy);

  // Occasional overcurrent errors are acceptable
  // If more than one occurs within a given time period, assume short circuit and estop
  uint32_t now = millis();
  if ((now - lastPackOvercurrentErrorTimestamp) >= ALLOWABLE_OVERCURRENT_PERIOD) {
    roverRestart();
    lastPackOvercurrentErrorTimestamp = now;
  } else {
    roverEStop();
  }
}
void errorAuxOvercurrent() {
  RoveComm.writeReliable(RC_PMSBOARD_AUXOVERCURRENT_DATA_ID, RC_PMSBOARD_AUXOVERCURRENT_DATA_COUNT, dummy);

  // Occasional overcurrent errors are acceptable
  // If more than one occurs within a given time period, assume short circuit and estop
  uint32_t now = millis();
  if ((now - lastAuxOvercurrentErrorTimestamp) >= ALLOWABLE_OVERCURRENT_PERIOD) {
    roverRestart();
    lastAuxOvercurrentErrorTimestamp = now;
  } else {
    roverSetEnables(MOTOR_ENABLE_BIT | CORE_ENABLE_BIT); // disable Aux
  }
}

void errorCellUndervoltage() {
  RoveComm.writeReliable(RC_PMSBOARD_CELLUNDERVOLTAGE_DATA_ID, RC_PMSBOARD_CELLUNDERVOLTAGE_DATA_COUNT, dummy);
  roverEStop();
}

void errorCellCritical() {
  RoveComm.writeReliable(RC_PMSBOARD_CELLCRITICAL_DATA_ID, RC_PMSBOARD_CELLCRITICAL_DATA_COUNT, dummy);
  roverSuicide();
}