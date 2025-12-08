#include <Arduino.h>
#include "PowerManagement.h"


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("PMS starting...");

  //initialization code

  //start up rovecomm
  RoveComm.begin(RC_PMSBOARD_IPADDRESS);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  
}

float PMSmap(float startVal, float fromMin, float fromMax, float toMin, float toMax) {
  
}
