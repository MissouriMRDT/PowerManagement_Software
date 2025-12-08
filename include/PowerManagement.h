#include <RoveComm.h>
#include "PinAssignments.h"

//define rovecomm
RoveCommEthernet RoveComm;
RoveCommPacket packet;


/**
 * @note map pin vals to real-world vals
 */
float PMSmap(float startVal, float fromMin, float fromMax, float toMin, float toMax);


void suicide();
void eStop();
void restart();

uint8_t enableBus(uint8_t data);
uint8_t disableBus(uint8_t data);

void errorPackOvercurrent();
void errorAuxOvercurrent();
void errorCellOverVoltage();