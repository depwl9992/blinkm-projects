/*
  BlinkMuinoPiezoServo
 
 The circuit:
        * servo control pin attached to "c" pin on BlinkM (digital pin 0)
 	* + connection of the piezo attached to "d" pin on BlinkM (analog pin A1)
 	* - connection of the piezo attached to ground
 	* 1-megohm resistor attached from analog in 0 to ground
        * 10uF capactor across "+" and "-" to absorb motor noise
        * servo power & ground hooked to BlinkM power & ground
        * 4xAA 1.2V NiMH battery pack
        
 See it in action at: https://www.youtube.com/watch?v=m3fmMM_1wBc
 
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
const int servoPin = sdaPin;
const int knockSensor = A1; // the piezo is connected to analog input 1, pin2
const int threshold = 100;  // threshold value to decide when the detected sound is a knock or not

// these variables will change:
int sensorReading = 0;      // variable to store the value read from the sensor pin
int servoPos;

void setup() {
  pinMode(knockSensor, INPUT);
  pinMode(servoPin,OUTPUT);
  pinMode(ledPin, OUTPUT); // declare the ledPin as as OUTPUT
  // Serial.begin(9600);       // use the serial port
  digitalWrite(bluPin, HIGH);
  for( int i=0; i< 500/20; i++ ) {
    delay(20);
    servoOut(0);
  }
  digitalWrite(bluPin, LOW);
}

// pos ranges from 0-1024
void servoOut( int pos ) {  
  int pulseWidth = pos + 1000;    // convert to microseconds pulsewidth

  digitalWrite(servoPin, HIGH);   // make servo pulse
  delayMicroseconds(pulseWidth);
  digitalWrite(servoPin, LOW);
}

void loop() {
  // read the sensor and store it in the variable sensorReading:
  sensorReading = analogRead(knockSensor);    

  // if the sensor reading is greater than the threshold:
  if (sensorReading >= threshold) {
    servoPos = 800;
    digitalWrite(ledPin, HIGH);
  }
  if( servoPos < 800 ) { 
      digitalWrite(ledPin,LOW);
  }
  // if we have a position, send it out
  if( servoPos > 0 ) {
    servoPos -= 5;
    servoOut( servoPos );
  }
  delay(20);  // for servo timing
}



