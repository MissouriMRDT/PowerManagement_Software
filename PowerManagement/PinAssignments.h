#ifndef PIN_ASSIGNMENTS_H
#define PIN_ASSIGNMENTS_H

// avoid turning these off because they take a long time to start up
#define POE_ENABLE_PIN 0 // power over ethernet
#define NS_ENABLE_PIN 1  // network switch

#define MOTOR_ENABLE_PIN 32 // prop units
#define CORE_ENABLE_PIN 8 // CoreBoard, DiffGPS, and PIs
#define AUX_ENABLE_PIN 31 // panel mount

// battery
#define C1_V_SENSE_PIN 18
#define C2_V_SENSE_PIN 19
#define C3_V_SENSE_PIN 16
#define C4_V_SENSE_PIN 17
#define C5_V_SENSE_PIN 14
#define C6_V_SENSE_PIN 15

#define PACK_I_SENSE_PIN 24
#define AUX_I_SENSE_PIN 27 // panel mount only

// can be used in the future to current sense boards
#define MISC1_I_SENSE_PIN 39
#define MISC2_I_SENSE_PIN 40
#define MISC3_I_SENSE_PIN 41

#define BUZZER_CTL_PIN 7

#endif
