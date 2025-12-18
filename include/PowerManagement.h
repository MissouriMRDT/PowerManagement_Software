#include <RoveComm.h>
#include "PinAssignments.h"

//define rovecomm
RoveCommEthernet RoveComm;
RoveCommPacket packet;

//Constants
#define NUM_CELLS 6

//cell sense pins
int cellPins[6] = {CELL_SENSE_1_PIN, CELL_SENSE_2_PIN,CELL_SENSE_3_PIN, CELL_SENSE_4_PIN, CELL_SENSE_5_PIN, CELL_SENSE_6_PIN};
float cellVoltages[6] = {0};



float packVoltage = 0.0f;
float packCurrent = 0.0f;
float auxCurrent = 0.0f;

#define MOTOR_ENABLE_BIT (1 << 0)
#define LC_ENABLE_BIT (1 << 1)
#define AUX_ENABLE_BIT (1 << 2)
#define NETWORK_ENABLE_BIT (1 << 3)
//#define POE_ENABLE_BIT = (1<<4)

//boolean states of Rover
bool motorEnabled = false;
bool lowCurrentEnabled = false;
bool auxEnabled = false;
bool nsEnabled = false;

//time variables
uint32_t lastAcceptableTimePackCurrent = 0;
uint32_t lastAcceptableTimeAuxCurrent = 0;

/**
 * @note map pin vals to real-world vals
 */
float PMSmap(float startVal, float fromMin, float fromMax, float toMin, float toMax);


void suicide();
void eStop();
void restart();

void enableBusses(uint8_t data);
void disableBusses(uint8_t data);

void errorPackOvercurrent();
void errorAuxOvercurrent();
void errorCellUnderVoltage();
void errorCellCritical();

void readCellVoltage();
void readPackVotlage();
void readPackCurrent();
void readAuxCurrent();

void telemetry();
