#include <Servo.h>

Servo myservo;  // create servo object to control a servo


int potpin = 0;  // analog pin used to connect the potentiometer

int val;    // variable to read the value from the analog pin


void setup() {

  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  Serial.begin(9600);

}

int pos = 0;
void loop() {
  // pos += 1;
  // myservo.write(pos);
  // delay(10);
  // send data only when you receive data:
  uint8_t incomingByte;
  if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    if (incomingByte == 0xFF) {
       myservo.write(120);
    }

    if (incomingByte == 0x77) {
      myservo.write(60);
    }

    if (incomingByte == 0x69) {
      myservo.write(90);
    }
  }
}