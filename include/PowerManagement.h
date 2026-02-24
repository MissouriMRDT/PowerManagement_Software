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

//thread ids
int currentCheckID = 0;
int mainID = 0;
int animationID = 0;

//bus change check variables
bool isDisabling = false;
bool ranOnce = false;


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
#define M2_ENABLE_BIT (1 << 3)
#define M9_ENABLE_BIT (1 << 4)
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
uint32_t lastTimeAcceptablePackCurrent = 0;
uint32_t lastTimeAcceptableAuxCurrent = 0;
uint32_t lastTimeAcceptablePackVoltage = 0;
uint32_t lastTimeNoCellUnderVoltage = 0;
uint32_t lastTimeNoCellCritical = 0;

//Interval Timer
#define TELEMETRY_PERIOD 500000
IntervalTimer telemetryRunner;
void telemetry();




//function headers
void suicide();
void eStop();
void restart();

void enableBusses(uint8_t data);
void disableBusses(uint8_t data);

void errorPackOvercurrent();
void errorAuxOvercurrent();
void errorCellUnderVoltage(uint8_t bitmask);
void errorCellCritical(uint8_t bitmask);

void readCellVoltage();
void readPackVotlage();
void readPackCurrent();
void readAuxCurrent();


void readPackCurrent();
void readAuxCurrent();
void readNSCurrent();
void readLCCurrent();
void readM9Current();
void readM2Current();
void readCellVoltages();
void checkCurrent();



void telemetry();
float map_float(float x, float in_min, float in_max, float out_min, float out_max);
float mapCellVoltage(float measured);

void mainThread();
void checkCurrent();
void animationThread();
byte name0x12[] = { B00000, B10000, B10100, B10100, B01000, B01000, B01000, B11111 }; 
byte name0x13[] = { B00000, B01100, B01000, B01000, B01001, B01001, B01011, B11011 }; 
byte name0x14[] = { B00000, B00000, B00000, B10000, B11000, B01000, B00100, B00100 }; 
byte name1x11[] = { B00001, B00001, B00000, B00000, B01100, B10011, B10010, B01100 }; 
byte name1x12[] = { B00000, B00000, B11111, B00110, B11001, B00010, B00010, B00001 }; 
byte name1x13[] = { B00111, B10110, B01000, B00110, B11001, B01000, B01000, B10000 }; 
byte name1x14[] = { B00011, B00010, B00000, B00000, B10110, B01001, B01001, B00110 }; 