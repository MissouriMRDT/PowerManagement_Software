#include <RoveComm.h>

#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include "RoveComm.h"

#include "PinAssignments.h"
#include "Buzzer.h"

// SENSORS ////////////////////////////////////////////////////////////////////////

const int CELL_V_SENSE_PINS[] = {C1_V_SENSE_PIN, C2_V_SENSE_PIN, C3_V_SENSE_PIN, C4_V_SENSE_PIN, C5_V_SENSE_PIN, C6_V_SENSE_PIN};

// Current conversion factor (PACK CURRENT)
//(512 .. 1023) -> (0 .. 200)
#define PACK_I_SENSE_ADC_MIN 512
#define PACK_I_SENSE_ADC_MAX 1023
#define PACK_I_SENSE_REAL_MIN 0.0
#define PACK_I_SENSE_REAL_MAX 200.0

// Current conversion factor (AUX CURRENT)
//(512 .. 1023) -> (0 .. 20)
#define AUX_I_SENSE_ADC_MIN 512
#define AUX_I_SENSE_ADC_MAX 1023
#define AUX_I_SENSE_REAL_MIN 0.0
#define AUX_I_SENSE_REAL_MAX 20.0

// Cell voltage conversion factor
//(0 .. 1023) -> (0 .. 4.2)
#define CELL_V_SENSE_ADC_MIN 0
#define CELL_V_SENSE_ADC_MAX 1023
#define CELL_V_SENSE_REAL_MIN 0.0
#define CELL_V_SENSE_REAL_MAX 4.2

////////////////////////////////////////////////////////////////////////////////////////

// Important constants
#define NUM_CELLS 6
#define NUM_VOLTAGE_READINGS 1000
#define NUM_MISC_CURRENTS 3
#define RESTART_DELAY 1000 // millis
#define MAX_PACK_CURRENT 50 // Amps
#define MAX_AUX_CURRENT 15 // Amps
#define ALLOWABLE_OVERCURRENT_PERIOD 30000 // millis
#define CELL_UNDERVOLT_THRESHOLD 2.7
#define CELL_CRITICAL_THRESHOLD 2.5


// Global Variables /////////////////////////////////////////////////////////////////

// Telemetry
#define TELEMETRY_PERIOD 500000
IntervalTimer Telemetry;
void telemetry();

// RoveComm
RoveCommEthernet RoveComm;
rovecomm_packet packet;
EthernetServer TCPServer(RC_ROVECOMM_ETHERNET_TCP_PORT);

float cellVoltages[NUM_CELLS] = {0};
float packVoltage = 0;
float packCurrent = 0;
float auxCurrent = 0;
float miscCurrents[NUM_MISC_CURRENTS] = {0};

uint32_t lastPackOvercurrentErrorTimestamp = 0;
uint32_t lastAuxOvercurrentErrorTimestamp = 0;

//Buzzer buzzer(BUZZER_CTL_PIN); // unused for now

// Function Declarations ///////////////////////////////////////////////////////////

void readCells();
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

// Enable Bitmasks -- [Motor, Core, Aux]
#define MOTOR_ENABLE_BIT  (1 << 2)
#define CORE_ENABLE_BIT   (1 << 1)
#define AUX_ENABLE_BIT    (1 << 0)
uint8_t enables = MOTOR_ENABLE_BIT | CORE_ENABLE_BIT | AUX_ENABLE_BIT;
void roverSetEnables(uint8_t bitfield);

#endif
