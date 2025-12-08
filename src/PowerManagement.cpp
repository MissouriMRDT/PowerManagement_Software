#include <Arduino.h>
#include "PowerManagement.h"
#include <LiquidCrystal.h>

LiquidCrystal LCD(LCD_RS_PIN, LCD_EN_PIN, LCD_D0_PIN, LCD_D1_PIN, LCD_D2_PIN, LCD_D3_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("PMS starting...");


  //pin setup
  pinMode(LC_ENABLE, OUTPUT);
  pinMode(M2_ENABLE, OUTPUT);
  pinMode(Motor_ENABLE, OUTPUT);
  pinMode(M9_ENABLE, OUTPUT);
  pinMode(NS_ENABLE, OUTPUT);
  pinMode(AUX_ENABLE, OUTPUT);
  //initialization code

  //start up rovecomm
  RoveComm.begin(RC_PMSBOARD_IPADDRESS);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  
}

float PMSmap(float startVal, float fromMin, float fromMax, float toMin, float toMax) {
  
}

//Not right
void readCellVoltages() {
  for (int i = 0; i < NUM_CELLS; i++) {
    float readVal = analogRead(cellPins[i]);
    float newVal = PMSmap(readVal, 0, 3.3, 0, 3.3);
    cellVoltages[i] = newVal;

  }
}

void readPackVotlage() {
  packVoltage = 0;
  for (int i = 0; i < NUM_CELLS; i++) {
    packVoltage += cellVoltages[i];
  }
}