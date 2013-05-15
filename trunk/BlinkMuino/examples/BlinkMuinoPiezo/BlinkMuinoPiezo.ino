/*
  BlinkMuinoPiezo
 
 Taken from:
 Knock Sensor
 
 This sketch reads a piezo element to detect a knocking sound. 
 It reads an analog pin and compares the result to a set threshold. 
 If the result is greater than the threshold, it writes
 "knock" to the serial port, and toggles the LED on pin 13.
 
 The circuit:
 	* + connection of the piezo attached to analog in 0
 	* - connection of the piezo attached to ground
 	* 1-megohm resistor attached from analog in 0 to ground
 
 http://www.arduino.cc/en/Tutorial/Knock
 
 created 25 Mar 2007
 by David Cuartielles <http://www.0j0.org>
 modified 30 Aug 2011
 by Tom Igoe
 
 This example code is in the public domain.
 
 */

// BlinkM / BlinkM MinM pins
const int redPin = 3;  // 
const int grnPin = 4;  //
const int bluPin = 1;  // PWM, will blink when programming
const int sdaPin = 0;  // PWM, 'd' pin, can be digital I/O
const int sclPin = 2;  // A/D, 'c' pin, can be digital I/O, or analog input

// these constants won't change:
const int ledPin = grnPin;      // led connected to digital pin 13
const int knockSensor = A1; // the piezo is connected to analog input 1, pin2
const int threshold = 100;  // threshold value to decide when the detected sound is a knock or not

// these variables will change:
int sensorReading = 0;      // variable to store the value read from the sensor pin

void setup() {
  pinMode(knockSensor, INPUT);
  pinMode(ledPin, OUTPUT); // declare the ledPin as as OUTPUT
  // Serial.begin(9600);       // use the serial port
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
}

void loop() {
  // read the sensor and store it in the variable sensorReading:
  sensorReading = analogRead(knockSensor);    

  // if the sensor reading is greater than the threshold:
  if (sensorReading >= threshold) {
    digitalWrite(ledPin, HIGH);      // update the LED pin 
    delay(200);  // delay to avoid overloading the serial port buffer
    digitalWrite(ledPin,LOW);
  }
}



