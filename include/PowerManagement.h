#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include <RoveComm.h>
#include "PinAssignments.h"
#include "Buzzer.h"

// SENSORS ////////////////////////////////////////////////////////////////////////

const int CELL_V_SENSE_PINS[] = {CELL_SENSE_1_PIN, CELL_SENSE_2_PIN, CELL_SENSE_3_PIN, CELL_SENSE_4_PIN, CELL_SENSE_5_PIN, CELL_SENSE_6_PIN};

// Current conversion factor (PACK CURRENT)
//(512 .. 885) -> (0 .. 45)
#define PACK_I_SENSE_ADC_ZERO 512
#define PACK_I_SENSE_ADC_OTHER 885
#define PACK_I_SENSE_REAL_ZERO 0.0
#define PACK_I_SENSE_REAL_OTHER 45.0

// Cell voltage conversion factor
//(0 .. 1023) -> (0 .. 4.2)
#define CELL_V_SENSE_ADC_ZERO 0
#define CELL_V_SENSE_ADC_OTHER 1023
#define CELL_V_SENSE_REAL_ZERO 0.0
#define CELL_V_SENSE_REAL_OTHER 4.2

////////////////////////////////////////////////////////////////////////////////////////

// Important constants
#define NUM_CELLS 6
#define NUM_VOLTAGE_READINGS 1000
#define RESTART_DELAY 1000 // millis
#define MAX_PACK_CURRENT 55 // Amps
#define MAX_AUX_CURRENT 15 // Amps
#define CELL_UNDERVOLT_THRESHOLD 2.7
#define CELL_CRITICAL_THRESHOLD 2.5

// Enable Bitmasks -- [Motor, Core, Aux]
#define MOTOR_ENABLE_BIT   (1 << 0)
#define CORE_ENABLE_BIT    (1 << 1)
#define AUX_ENABLE_BIT     (1 << 2)
#define NETWORK_ENABLE_BIT (1 << 3)

// Global Variables /////////////////////////////////////////////////////////////////

// Telemetry
#define TELEMETRY_PERIOD 500000
IntervalTimer Telemetry;
void telemetry();

// RoveComm
RoveCommEthernet RoveComm;
RoveCommPacket packet;

float cellVoltages[NUM_CELLS] = {0};
float packVoltage = 0;
float packCurrent = 0;
float auxCurrent = 0; // not currently measured

// in milliseconds
#define ALLOWABLE_OVERCURRENT_PERIOD 100
// the last time in milliseconds that the current was at an acceptable level
uint32_t lastAcceptableCurrentTimestamp = 0;

bool motorEnabled = false;
bool coreEnabled = false;
bool auxEnabled = false;
bool networkEnabled = false; // NS and POE

Buzzer buzzer(BUZZER_PIN); // unused for now

// Function Declarations ///////////////////////////////////////////////////////////

void readCellVoltages();
void readPackCurrent();
// map a teensy analogRead() measurement to a range of floats
float analogMap(uint16_t measurement, uint16_t fromADC, uint16_t toADC, float fromAnalog, float toAnalog);

void roverEStop();
void roverSuicide();
void roverRestart();

// send message to lighting panel, send error packet, buzz
void errorPackOvercurrent();
void errorAuxOvercurrent();
void errorCellUndervoltage();
void errorCellCritical();

void enableBusses(uint8_t bitmask);
void disableBusses(uint8_t bitmask);

#endif
