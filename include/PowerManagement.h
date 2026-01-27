#include <RoveComm.h>
#include "PinAssignments.h"
#include "Buzzer.h"

//define rovecomm
RoveCommEthernet RoveComm;
RoveCommPacket packet;

Buzzer buzzer(BUZZER_PIN);
//Constants
#define NUM_CELLS 6

//cell sense pins
  //[PackCurrent, AuxCurrent, LowCurrent, NetworkCurrent, RadioM2Current, RadioM9Current, Cell1Voltage, Cell2Voltage, Cell3Voltage, Cell4Voltage, Cell5Voltage, Cell6Voltage] (A, A, A, A, A, A, V, V, V, V, V, V)
int cellPins[6] = {CELL_SENSE_1_PIN, CELL_SENSE_2_PIN,CELL_SENSE_3_PIN, CELL_SENSE_4_PIN, CELL_SENSE_5_PIN, CELL_SENSE_6_PIN};
float cellVoltages[6] = {0};
  
  
  
float packVoltage = 0.0f;
float packCurrent = 0.0f;
float auxCurrent = 0.0f;
float nsCurrent = 0.0f;
float m9Current = 0.0f;
float m2Current = 0.0f;
float lcCurrent = 0.0f;
  
float telemArray[12] = {packCurrent, auxCurrent, lcCurrent, nsCurrent, m2Current, m9Current,0,0,0,0,0,0};

#define MOTOR_ENABLE_BIT (1 << 0)
#define LC_ENABLE_BIT (1 << 1)
#define AUX_ENABLE_BIT (1 << 2)
#define M2_ENABLE_BIT (1<<3)
#define M9_ENABLE_BIT (1<<4)
#define NETWORK_ENABLE_BIT (1 << 5)
//#define POE_ENABLE_BIT = (1<<4)

//boolean states of Rover
bool motorEnabled = false;
bool lowCurrentEnabled = false;
bool auxEnabled = false;
bool m2Enabled = false;
bool m9Enabled = false;
bool nsEnabled = false;

//time variables
uint32_t lastAcceptableTimePackCurrent = 0;
uint32_t lastAcceptableTimeAuxCurrent = 0;

//Interval Timer
#define TELEMETRY_PERIOD 500000
IntervalTimer telemetryRunner;
void telemetry();

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
