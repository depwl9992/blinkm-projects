/*
  Fade
 
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
  for(byte i=1; i<100; i++) {
    byte on  = i;
    byte off = 100-on;
    for( byte j=0; j<100; j++ ) {
      digitalWrite(grnPin, HIGH);
      digitalWrite(bluPin, HIGH);
      delayMicroseconds(on);
      digitalWrite(grnPin, LOW);
      digitalWrite(bluPin, LOW);
      delayMicroseconds(off);
    }
  }
  for(byte i=1; i<100; i++) {
    byte on  = 100-i;
    byte off = i;
    for( byte j=0; j<100; j++ ) {
      digitalWrite(grnPin, HIGH);
      digitalWrite(bluPin, HIGH);
      delayMicroseconds(on);
      digitalWrite(grnPin, LOW);
      digitalWrite(bluPin, LOW);
      delayMicroseconds(off);
    }
  }
}


