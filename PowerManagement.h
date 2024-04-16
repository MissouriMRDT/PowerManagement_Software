#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include "RoveComm.h"

#include "PinAssignments.h"

RoveCommEthernet RoveComm
rovecomm_packet packet;

EthernetServer TCPServer(RC_ROVECOMM_PMSBOARD_PORT);

// BMS constants

const int CELL_SENSE_PINS[] = {C1_V_SENSE_PIN, C2_V_SENSE_PIN, C3_V_SENSE_PIN, C4_V_SENSE_PIN, C5_V_SENSE_PIN, C6_V_SENSE_PIN};

/* Sensor Measurement: Constants and Calculations */

#define VCC				3300 // mV
// Teensy 4.1 specs
	// Find at: https://www.pjrc.com/store/teensy41.html
	// find at: https://www.pjrc.com/store/teensy41_pins.html
#define SENSOR_SENSITIVITY		0.0066 //V  3.3V / 1024 levels = 0.0032
#define VOLTAGE_TO_SIGNAL_RATIO		1269

// timekeeping variables
IntervalTimer Telemetry;

// State Variables /////////////////////////////////////////////////////////////////

float cellVoltages[CELL_COUNT];

// Function Declarations ///////////////////////////////////////////////////////////

void setInputPins();

void setOutputPins();

void setOutputStates();

void getMainCurrent(float &main_current);

void getCellVoltage(float cell_voltages);

void getPackVoltage(float &pack_out_voltage);

// void getBattTemp(uint32_t &batt_temp);

void reactOverCurrent();

void reactUnderVoltage();

void reactOverTemp();

void reactForgottenLogicSwitch();

void reactEstopReleased();

void reactLowVoltage(float cell_voltage[RC_BMSBOARD_CELLV_MEAS_DATA_COUNT]);

void setEstop(uint8_t data);

void notifyEstop(); //Buzzer sound: beeeeeeeeeeeeeeeeeeeep beeeeeeeeeep beeeeep beeep bep

void notifyLogicSwitch(); //Buzzer sound: beeep beeep

void notifyEstopReleased(); //Buzzer sound: beep

void notifyReboot(); //Buzzer sound: beeeeeeeeeep beeep beeep

void notifyOverCurrent(); //Buzzer Sound: beeeeeeeeeeeeeeeeeeeeeeeeeeeeeep

void notifyUnderVoltage(); //Buzzer Sound: beeep beeep beeep beeep beeeeeeeeeeeeeeeeeeeep

void notifyLowVoltage(); //Buzzer Sound: beeep beeep beeep

void telemetry();


#endif
