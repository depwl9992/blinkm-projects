/*
 * BlinkMNet -- 
 *
 * Operation:
 * - Receive command from Serial or NSS
 * - Perform command
 * - Relay command to NSS 
 *
 * Command format:
 *                0       1         2          3     4     5     6
 * - 7 bytes - startbye, addr, blinkm_cmdbyte, arg1, arg2, arg3, chksum
 * - startbyte = 0x85
 * - cmdbyte == 'c', 'f', 'o', etc.
 * - chksum = xor sum of array
 *
 * 2011 - Tod E. Kurt - http://todbot.com/blog/
 *
 */

#include <NewSoftSerial.h>
#include <TimedAction.h>
#include <avr/pgmspace.h>


const byte debug = 0;  // 

const char VERSION[] = "f";

//const unsigned int bps = 38400;
const unsigned int bps = 19200;
//const unsigned int bps = 9600;

const byte ledPin= 13;
const byte txPin = 12;
const byte rxPin = 11;
const byte cmdEnablePin =  7;

const byte briPin = A0; 
const byte spdPin = A1;

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
  digitalWrite(cmdEnablePin, OUTPUT); // turn on pullup

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

  cmdAction.disable();  // not yet

  Serial.begin(bps);
  Serial.print("BlinkMNet - ");
  Serial.println(VERSION);
  delay(100);

}

// main loop
void loop() 
{
  command_check();
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
  if( cmdline_pos == cmdline_len ) cmdline_pos = 0;

  newInterval = cmdline_curr.dur;
  a1 = cmdline_curr.a1;
  a2 = cmdline_curr.a2;
  a3 = cmdline_curr.a3;
  if( 1 ) {
    a1 = (a1 * bri) / 256;
    a2 = (a2 * bri) / 256;
    a3 = (a3 * bri) / 256;
    
    if( newInterval != 0 ) {  
      newInterval = (newInterval * spd) / 128 + 10;
    }
  } 

  rx_buf[0] = cmdline_curr.addr;    // blinkm i2c address
  rx_buf[1] = cmdline_curr.cmd;     // command - setRGB
  rx_buf[2] = a1;                   // arg1
  rx_buf[3] = a2;                   // arg2
  rx_buf[4] = a3;                   // arg3
  rx_buf[5] = 0;                    // checksum

  cmdAction.setInterval( newInterval );
 
  //Serial.println(millis() );
  if( debug>1 ) {
    Serial.print(millis() );
    Serial.print(" - ");
    Serial.print(newInterval,DEC);         Serial.print(',');
    Serial.print((char)cmdline_curr.cmd);  Serial.print(',');
    Serial.print((byte)a1,HEX);            Serial.print(',');
    Serial.print((byte)a2,HEX);            Serial.print(',');
    Serial.print((byte)a3,HEX);            Serial.print('\n');
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

  if( 1 ) { 
    bri = 127;
    //bri = analogRead( briPin ) / 4;
    spd = analogRead( spdPin ) / 4;
    if( spd==0 ) spd = 1;
  } else { 
    bri = 255;
    spd = 255;
  }
}

//
// execute a command in rx_buf
//
void command_handle()
{
  byte cmd = rx_buf[1];
  if( cmd == 'n' || cmd == 'c' || cmd == 'h' || cmd == 'p' ) {
    //               addr        cmd         arg1       arg2      arg3
    BlinkM_sendCmd3( rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4] );
  } 
  else if( cmd == 'o' ) {
    BlinkM_stopScript( rx_buf[0] );
  }
  else if( cmd == '0' ) { 
    BlinkM_off( rx_buf[0] );
  }

}

//
void command_send()
{
  if(debug) { Serial.println("cmd"); }
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

  
