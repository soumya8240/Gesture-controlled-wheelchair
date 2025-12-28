#include <SoftwareSerial.h>
SoftwareSerial BTSerial(10, 11); // CONNECT BT RX PIN TO ARDUINO 11 PIN | CONNECT BT TX PIN TO ARDUINO 10 PIN
char tiltDirection;
int motorInput1 = 5;
int motorInput2 = 6;
int motorInput3 = 3;
int motorInput4 = 9;
const int TILT_PIN   = 8;  // Tilt switch on D8
const int BUZZER_PIN = 7; 
const bool TILT_TRIGGER_STATE = HIGH;
void setup() {
  pinMode(motorInput1, OUTPUT);
  pinMode(motorInput2, OUTPUT);
  pinMode(motorInput3, OUTPUT);
  pinMode(motorInput4, OUTPUT);
  digitalWrite(motorInput1, LOW);
  digitalWrite(motorInput2, LOW);
  digitalWrite(motorInput3, LOW);
  digitalWrite(motorInput4, LOW);
  pinMode(TILT_PIN, INPUT_PULLUP);  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.begin(38400);      // Serial communication is activated at 38400 baud/s.
  BTSerial.begin(38400);    // HC-05 default speed in AT command more
}
void loop() {
  bool tiltTriggered = (digitalRead(TILT_PIN) == TILT_TRIGGER_STATE);

  if (tiltTriggered) {
    // Emergency stop: motors OFF + buzzer ON
    stopCar();
    digitalWrite(BUZZER_PIN, HIGH);

    // Ignore BT commands while tilted
    return;
  } else {
    // Normal mode: buzzer OFF
    digitalWrite(BUZZER_PIN, LOW);
  }
  if (BTSerial.available()) {
    tiltDirection = BTSerial.read();
    if(tiltDirection == 'F'){
      Serial.println("reverse");
       reverse();
    }else if(tiltDirection == 'B'){
      Serial.println("forward");
      forward();
    }else if(tiltDirection == 'R'){
      Serial.println("left");
      left();
    }else if(tiltDirection == 'L'){
      Serial.println("right");
      right();
    }else if(tiltDirection == 'S'){
      Serial.println("Stop");
      stopCar();
    }
  }
}
//Robot lk
void left()
{
  digitalWrite(motorInput1, LOW);
  digitalWrite(motorInput2, HIGH);
  digitalWrite(motorInput3, LOW);
  digitalWrite(motorInput4, HIGH);
}
void right()
{
  digitalWrite(motorInput1, HIGH);
  digitalWrite(motorInput2, LOW);
  digitalWrite(motorInput3, HIGH);
  digitalWrite(motorInput4, LOW);
}
void reverse()
{
  digitalWrite(motorInput1, LOW);
  analogWrite(motorInput2, 150);
  analogWrite(motorInput3, 150);
  digitalWrite(motorInput4, LOW);
}
void forward()
{
  analogWrite(motorInput1, 150);
  digitalWrite(motorInput2, LOW);
  digitalWrite(motorInput3, LOW);
  analogWrite(motorInput4, 150);
}
void stopCar() {
  digitalWrite(motorInput1, LOW);
  digitalWrite(motorInput2, LOW);
  digitalWrite(motorInput3, LOW);
  digitalWrite(motorInput4, LOW);
}
