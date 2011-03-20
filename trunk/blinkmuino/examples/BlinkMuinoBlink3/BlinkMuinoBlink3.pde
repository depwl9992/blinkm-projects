/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
 
  This example code is in the public domain.
 */

// 
const int grnPin = 4;
const int redPin = 3; 
const int bluPin = 1;

const int sdaPin = 0;
const int sclPin = 2;

void setup() {
  pinMode(redPin, OUTPUT);     
  pinMode(grnPin, OUTPUT);     
  pinMode(bluPin, OUTPUT);     
}

void loop() {
  digitalWrite(redPin, HIGH);
  delay(100);
  digitalWrite(redPin, LOW);
  delay(400);

  digitalWrite(grnPin, HIGH);
  delay(100);
  digitalWrite(grnPin, LOW);
  delay(400);

  digitalWrite(bluPin, HIGH);
  delay(100);
  digitalWrite(bluPin, LOW);
  delay(400);
 
 delay(4000); 
}
