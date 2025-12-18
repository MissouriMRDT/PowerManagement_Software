#include <Arduino.h>

class Buzzer {
public:
    Buzzer(int setBuzzerPin) {buzzerPin = setBuzzerPin;}
    void init() {
        pinMode(buzzerPin, OUTPUT); 
        digitalWrite(buzzerPin, LOW);
    };
    void update() {
        
    }
    
private:
    int buzzerPin = 0;
    uint32_t beginActionTimestamp = 0;
    uint32_t nextActionTImestamp = 0;
    size_t position = 0;

};