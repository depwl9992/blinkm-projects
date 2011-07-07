/*
 * BlinkMNet -- A network Ardiuno-connected BlinkMs
 *
 *
 * Operation:
 * - Receive command from Serial or NSS from upstream Arduino or PC
 * - Perform command
 * - Relay command via NSS to downstream Arduino
 *
 * Command format:
 *                0       1         2          3     4     5     6
 * - 7 bytes - startbye, addr, blinkm_cmdbyte, arg1, arg2, arg3, chksum
 * - startbyte = 0x85
 * - cmdbyte == 'c', 'f', 'o', etc.
 * - chksum = xor sum of array
 *
 *
 * Arduinos and BlinkMs are connected like:
 *
 * >--+   +---------+   +---------+   +--------> ...
 * rx |   | tx   rx |   | tx   rx |   | tx
 *  +-v---+--+   +--v---+--+   +--v---+--+
 *  | Arduino |   | Arduino |   | Arduino |
 *  +---+-----+   +---+-----+   +---+-----+
 *      |             |             |
 *      |             |             |
 *   +--v----+     +--v----+     +--v----+
 *   |BlinkM1|     |BlinkM3|     |BlinkM5|
 *   +--+----+     +--+----+     +--+----+
 *      |             |             |
 *   +--v----+     +--v----+     +--v----+
 *   |BlinkM1|     |BlinkM4|     |BlinkM6|
 *   +--+----+     +--+----+     +--+----+
 *      |             |             |
 *     ...           ...           ... 
 *
 *
 * 2011 - Tod E. Kurt - http://todbot.com/blog/
 *
 */

#include <NewSoftSerial.h>
#include <TimedAction.h>
#include <avr/pgmspace.h>


// control debugging output:
// debug=1 -- print 'cmd' on each cmd sent
// debug=2 -- print out each command handled
const byte debug = 2;  
const boolean enableKnobs = false;

const char VERSION[] = "h";

//const unsigned int bps = 38400;
const unsigned int bps = 19200;
//const unsigned int bps = 9600;

const byte ledPin= 13;          // for status blinking
const byte txPin = 12;          // transmit to next Arduino in chain
const byte rxPin = 11;          // receiver from previous Arduino in chain

const byte cmdEnablePin =  7;   // set LOW to make this Arduino the commander
const byte knobEnablePin = 6;   // set LOW to enable knobs
 
const byte briPin = A0;  // control overall brightness
const byte spdPin = A1;  // control speed

const byte gndPin = A2;
const byte pwrPin = A3;
const byte sdaPin = A4;   // digital pin wired to 'd' on BlinkM
const byte sclPin = A5;   // digital pin wired to 'c' on BlinkM

#include "SoftI2CMaster.h"
SoftI2CMaster i2c = SoftI2CMaster( sdaPin,sclPin );
// must define "i2c" before including BlinkM_funcs_soft.h
#include "BlinkM_funcs_soft.h"

// NewSoftSerial(recv_pin, trans_pin)
NewSoftSerial nss1(rxPin, txPin);  // 


const byte cmd_startbyte = '!'; // 0x85;
const byte cmd_len = 6;  // includes checksum,doesn't include startbytex
const int cmd_timeout = 400;

byte rx_buf[cmd_len];

TimedAction cmdAction = TimedAction(200, updateCmdAction);

typedef struct _cmdline {
  unsigned int dur;
  byte addr;
  byte cmd;
  byte a1;
  byte a2;
  byte a3;
  char desc[5];
} cmdline;

cmdline cmdline_curr;
int cmdline_pos = 0;

unsigned int bri;
unsigned int spd;

// 
// This file contains the actual "script" to play back
//
#include "Cmds.h"


//
void setup()
{
  pinMode( ledPin, OUTPUT);

  pinMode( cmdEnablePin, INPUT);
  digitalWrite(cmdEnablePin, HIGH); // turn on pullup

  pinMode( knobEnablePin, INPUT);
  digitalWrite(knobEnablePin, HIGH); // turn on pullup

  // turn A2 & A3 into a tiny voltage source, for testing BlinkMs
  pinMode( gndPin, OUTPUT);
  pinMode( pwrPin, OUTPUT);
  digitalWrite( gndPin, LOW);
  digitalWrite( pwrPin, HIGH);
  
  // a little flash of the status LED to say we're alive
  // also gives us a time delay for blinkms to come online
  for( int i=0; i<5; i++ ) {
    digitalWrite(ledPin,HIGH);
    delay(50);
    digitalWrite(ledPin,LOW);
    delay(50);
  }

  BlinkM_begin();
  BlinkM_off(0);

  nss1.begin(bps);
  delay(100);

  cmdAction.disable();  // do do actions yet

  Serial.begin(bps);
  Serial.print("BlinkMNet - version:");
  Serial.print(VERSION);
  Serial.print(" - '");
  serialPrintProgStr( cmdline_id );
  Serial.print("' len=");
  Serial.println( cmdlines_len );

  if( digitalRead(cmdEnablePin) == LOW ) { 
    Serial.println("Acting as commander, will start sending script...");
  } else {
    Serial.println("Acting as slave, will forward incoming msgs too...");
  }

  delay(100);

}

// main loop
void loop() 
{
  command_check();    // check for incoming upstream commands
  ui_check();         
  cmdAction.check();
}

//
void updateCmdAction() 
{
  unsigned int a1,a2,a3;
  unsigned long newInterval;

  // get new cmdline
  memcpy_P( &cmdline_curr, &cmdlines[cmdline_pos], sizeof(cmdline) );

  // advance to next position
  cmdline_pos++;
  if( cmdline_pos == cmdlines_len ) cmdline_pos = 0;

  newInterval = cmdline_curr.dur;
  a1 = cmdline_curr.a1;
  a2 = cmdline_curr.a2;
  a3 = cmdline_curr.a3;
  if( enableKnobs ) {
    a1 = (a1 * bri) / 256;
    a2 = (a2 * bri) / 256;
    a3 = (a3 * bri) / 256;
    
    if( newInterval != 0 ) {  
      newInterval = (newInterval * spd) / 128;
      if( newInterval < 10 ) newInterval = 10;
    }
  } 

  rx_buf[0] = cmdline_curr.addr;    // blinkm i2c address
  rx_buf[1] = cmdline_curr.cmd;     // command - setRGB
  rx_buf[2] = a1;                   // arg1
  rx_buf[3] = a2;                   // arg2
  rx_buf[4] = a3;                   // arg3
  rx_buf[5] = 0;                    // checksum

  cmdAction.setInterval( newInterval );
  
  if( debug>1 ) {
    Serial.print( cmdline_curr.desc );
    Serial.print('\t');
    Serial.print(millis() );
    Serial.print('\t');
    Serial.print(newInterval,DEC);              Serial.print('\t');
    Serial.print((byte)cmdline_curr.addr,DEC);  Serial.print('\t');
    Serial.print((char)cmdline_curr.cmd);       Serial.print(',');
    Serial.print((byte)a1,HEX);                 Serial.print(',');
    Serial.print((byte)a2,HEX);                 Serial.print(',');
    Serial.print((byte)a3,HEX);                 Serial.print('\n');
    if( debug > 3 ) {
    Serial.print("bri/spd=");
    Serial.print(bri); Serial.print(','); Serial.println(spd);
    }
  }

  command_send();
  command_handle();
}

//
// Check the various UI components, if this device is a coordinator
// So far these are:
// - cmdEnable switch - LOW to make this one a command coordinator
// - brightness knob - adjust max brightness of commands (via simple hack)
// - speed knob - speed up or slow down playback 
//
void ui_check()
{
  if( digitalRead( cmdEnablePin ) == LOW ) {
    cmdAction.enable();
  } else {
    cmdAction.disable();
  }

  //enableKnobs = !digitalRead( knobEnablePin );  // active low

  if( enableKnobs ) { 
    bri = analogRead( briPin ) / 4;
    spd = analogRead( spdPin ) / 4;
    if( spd==0 ) spd = 1;
  } else { 
    bri = 255; // max bright default
    spd = 128; // midpoint in speed default
  }
}

//
// execute a command in rx_buf
//
void command_handle()
{
  byte cmd = rx_buf[1];
  // three-arg case
  if( cmd == 'n' || cmd == 'c' || cmd == 'h' || cmd == 'C' || cmd == 'H' || 
      cmd == 'p' || cmd == 'k' || cmd == 'K' || cmd == 'i' || cmd == 'I' ) {
    //               addr        cmd         arg1       arg2      arg3
    BlinkM_sendCmd3( rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4] );
  } 
  else if( cmd == 'f' || cmd == 't' || cmd == 'j' ) { // one arg case
    BlinkM_sendCmd1( rx_buf[0], rx_buf[1], rx_buf[2] );
  } 
  else if( cmd == 'o' ) {  // special case for no-argument all-off
    BlinkM_stopScript( rx_buf[0] );
  }
  else if( cmd == '0' ) { 
    BlinkM_off( rx_buf[0] );
  }

}

//
void command_send()
{
  if(debug>0 && debug!=2) { Serial.println("cmd"); }
  digitalWrite(ledPin, HIGH); // say we got something
  nss1.print( cmd_startbyte, BYTE );
  for( int i=0; i<cmd_len; i++ ) {
    nss1.print( rx_buf[i], BYTE );
  }
  digitalWrite(ledPin, LOW); // say we got something
}

// 
// check for incoming commands from upstream 
// if commmand packet found, send it downstream, and also do it
//
void command_check()
{
  boolean goodpacket = false;
  //
  if( nss1.available()  ) { 
    Serial.print('.');
    int c = nss1.read();
    if( c == cmd_startbyte ) {
      long startMillis = millis();
      while( nss1.available() < cmd_len ) {
        if( (millis()-startMillis) > cmd_timeout ) {
          return;
        }
      }
      // if we get this far, we got a whole packet in recv buffer
      // so copy it to local buffer
      for( int i=0; i< cmd_len; i++ ) {
        rx_buf[i] = nss1.read();
      }
      // do checksum, set goodpacket
      // (do nothing for now)
      goodpacket = true;
    }
  } // if( nss1.available() ) 

  // duplicate of above, but in hardware serial, for testing or host pc
  if( Serial.available()  ) { 
    int c = Serial.read();
    if( c == cmd_startbyte ) {
      long startMillis = millis();
      while( Serial.available() < cmd_len ) {
        if( (millis()-startMillis) > cmd_timeout ) {
          return;
        }
      }
      // if we get this far, we got a whole packet in recv buffer
      // so copy it to local buffer
      for( int i=0; i< cmd_len; i++ ) {
        rx_buf[i] = Serial.read();
      }
      // do checksum, set goodpacket
      // (do nothing for now)
      goodpacket = true;
    }
  } // if( Serial.available() ) 

  // pass along the command
  if( goodpacket ) {
    command_send();
    command_handle();
  } 

}

  
// given a PROGMEM string, use Serial.print() to send it out
void serialPrintProgStr(const prog_char str[])
{
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++)))
    Serial.print(c,BYTE);
}

// and then at some point
