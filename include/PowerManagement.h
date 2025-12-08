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



/**
 * @note map pin vals to real-world vals
 */
float PMSmap(float startVal, float fromMin, float fromMax, float toMin, float toMax);


void suicide();
void eStop();
void restart();

void enableBus(uint8_t data);
void disableBus(uint8_t data);

void errorPackOvercurrent();
void errorAuxOvercurrent();
void errorCellOverVoltage();
void errorCellCritical();

void readCellVoltage();
void readPackVotlage();
void readPackCurrent();
