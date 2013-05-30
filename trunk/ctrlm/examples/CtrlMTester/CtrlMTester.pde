/*
 * CtrlMTester -- Simple "command-line" tool to play with CtrlMs
 *
 *  Once you load this sketch on to your Arduino, open the Serial Monitor
 *  and you'll see a menu of things to do.
 *
 *
 * CtrlM connections to Arduino
 * PWR - -- gnd -- black -- Gnd
 * PWR + -- +5V -- red   -- 5V
 * I2C d -- SDA -- green -- Analog In 4
 * I2C c -- SCK -- blue  -- Analog In 5
 *
 * 2010, Tod E. Kurt, ThingM, http://thingm.com/
 *
 */


#include "Wire.h"
#include "CtrlM_funcs.h"

#include <avr/pgmspace.h>  // for progmem stuff


byte ctrlm_addr = 0x09; // the default address of all CtrlMs

char serInStr[30];  // array that will hold the serial input string

const char helpstr[] PROGMEM = 
    "\nCtrlMTester!\n"
    "'c<rrggbb>'  send fade to an rgb color\n"
    "'h<hhssbb>'  send fade to an hsb color\n"
    "'o'          stop script (both local & remote)\n"
    "'$<code>'    send IR remote control code \n"
    "'#<ff>'      set IR PWM freq val\n"
    "'@<addr><freemaddr>'    set I2C addr to send to\n"
    "%<1|0>       turn IR LED on or off at PWM freq\n"
    "'a'          get I2C address\n"
    "'A<n>'       set I2C address\n"
    "'s'/'S'      scan i2c bus for 1st CtrlM / search for devices\n"
    "'?'  for this help msg\n\n"
  ;

// There are cases where we can't use the obvious Serial.println(str) because
// the string is PROGMEM and Serial doesn't know how to deal with that
// (we store strings in PROGMEM instead of normal RAM to save RAM space)
void printProgStr(const prog_char str[])
{
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++)))
    Serial.print(c,BYTE);
}

void help()
{
  printProgStr( helpstr );
}

// called when address is found in CtrlM_scanI2CBus()
void scanfunc( byte addr, byte result ) {
  Serial.print("addr: ");
  Serial.print(addr,DEC);
  Serial.print( (result==0) ? " found!":"       ");
  Serial.print( (addr%4) ? "\t":"\n");
}

void lookForCtrlM() {
  Serial.print("Looking for a CtrlM: ");
  int a = CtrlM_findFirstI2CDevice();
  if( a == -1 ) {
    Serial.println("No I2C devices found");
  } else { 
    Serial.print("Device found at addr ");
    Serial.println( a, DEC);
    ctrlm_addr = a;
  }
}

// arduino setup func
void setup()
{
  Serial.begin(19200);

  CtrlM_beginWithPower();
  delay(1000);

  help();
  
  lookForCtrlM();

  Serial.print("cmd>");
}

// arduino loop func
void loop()
{
  long num;
  //read the serial port and create a string out of what you read
  if( readSerialString() ) {
    Serial.println(serInStr);
    char cmd = serInStr[0];  // first char is command
    char* str = serInStr;
    while( *++str == ' ' );  // got past any intervening whitespace
    num = atoi(str);         // the rest is arguments (maybe)
    if( cmd == '?' ) {
      help();
    }
    else if( cmd == 'c' || cmd=='h' || cmd == 'C' || cmd == 'H' ) {
      byte a = toHex( str[0],str[1] );
      byte b = toHex( str[2],str[3] );
      byte c = toHex( str[4],str[5] );
      if( cmd == 'c' ) {
        Serial.print("Fade to r,g,b:");
        CtrlM_fadeToRGB( ctrlm_addr, a,b,c);
      } else if( cmd == 'h' ) {
        Serial.print("Fade to h,s,b:");
        CtrlM_fadeToHSB( ctrlm_addr, a,b,c);
      } else if( cmd == 'C' ) {
        Serial.print("Random by r,g,b:");
        CtrlM_fadeToRandomRGB( ctrlm_addr, a,b,c);
      } else if( cmd == 'H' ) {
        Serial.print("Random by h,s,b:");
        CtrlM_fadeToRandomHSB( ctrlm_addr, a,b,c);
      }
      Serial.print(a,HEX); Serial.print(",");
      Serial.print(b,HEX); Serial.print(",");
      Serial.print(c,HEX); Serial.println();
    }
    else if( cmd == 'p' ) {
      Serial.print("Play script #");
      Serial.println(num,DEC);
      CtrlM_playScript( ctrlm_addr, num,0,0 );
    }
    else if( cmd == 'P' ) { 
      Serial.print("Play CtrlM script");
      CtrlM_playCtrlMScript( ctrlm_addr, 0,0 );
    }
    else if( cmd == 'o' ) {
      Serial.println("Stop script");
      CtrlM_stopScript( ctrlm_addr );
    }
    else if( cmd == '#' ) {
      Serial.print("Set IR freq to:"); Serial.println(num,DEC);
      CtrlM_setIRFreq( ctrlm_addr, num, num/3 );
    }
    else if( cmd == '%' ) {
      Serial.print("Turning IR LED ");
      if( num == 0 ) {
        Serial.println("off");
      } else {
        Serial.println("on");
      }
      CtrlM_turnIRLED( ctrlm_addr, num );
    }
    else if( cmd == '$' ) {
      byte codetype = 0;  // 0 == sony
      byte c0 = toHex( str[0],str[1] );
      byte c1 = toHex( str[2],str[3] );
      byte c2 = toHex( str[4],str[5] );
      byte c3 = toHex( str[6],str[7] );
      num = (c0<<24) | (c1<<16) | (c2<<8) | c3 ;
      Serial.print("Sending IR code:"); Serial.println(num,HEX);
      CtrlM_sendIRCode( ctrlm_addr, codetype, num );
    }
    else if( cmd == 'A' ) {
      if( num>0 && num<0xff ) {
        Serial.print("Setting address to: ");
        Serial.println(num,DEC);
        CtrlM_setAddress(num);
        ctrlm_addr = num;
      } else if ( num == 0 ) {
        Serial.println("Resetting address to default 9: ");
        ctrlm_addr = 9;
        CtrlM_setAddress(ctrlm_addr);
      }
    }
    else if( cmd == 'a' ) {
      Serial.print("Address: ");
      num = CtrlM_getAddress(0); 
      Serial.println(num);
    }
    else if( cmd == '@' ) {
      Serial.print("Will now talk on address: ");
      Serial.println(num,DEC);
      ctrlm_addr = num;
    }
    else if( cmd == 'Z' ) { 
      Serial.print("CtrlM version: ");
      num = CtrlM_getVersion(ctrlm_addr);
      if( num == -1 )
        Serial.println("couldn't get version");
      Serial.print( (char)(num>>8), BYTE ); 
      Serial.println( (char)(num&0xff),BYTE );
    }
    else if( cmd == 'B' ) {
      Serial.print("Set startup mode:"); Serial.println(num,DEC);
      CtrlM_setStartupParams( ctrlm_addr, num, 0,0,1,0);
    }
    else if( cmd == 's' ) { 
      lookForCtrlM();
    }
    else if( cmd == 'S' ) {
      Serial.println("Scanning I2C bus from 1-100:");
      CtrlM_scanI2CBus(1,100, scanfunc);
      Serial.println();
    }
    else { 
      Serial.println("Unrecognized cmd");
    }
    serInStr[0] = 0;  // say we've used the string
    Serial.print("cmd>");
  } //if( readSerialString() )
  
}

//read a string from the serial and store it in an array
//you must supply the array variable
uint8_t readSerialString()
{
  if(!Serial.available()) {
    return 0;
  }
  delay(10);  // wait a little for serial data

  memset( serInStr, 0, sizeof(serInStr) ); // set it all to zero
  int i = 0;
  while (Serial.available()) {
    serInStr[i] = Serial.read();   // FIXME: doesn't check buffer overrun
    i++;
  }
  //serInStr[i] = 0;  // indicate end of read string
  return i;  // return number of chars read
}

// -----------------------------------------------------
// a really cheap strtol(s,NULL,16)
#include <ctype.h>
uint8_t toHex(char hi, char lo)
{
  uint8_t b;
  hi = toupper(hi);
  if( isxdigit(hi) ) {
    if( hi > '9' ) hi -= 7;      // software offset for A-F
    hi -= 0x30;                  // subtract ASCII offset
    b = hi<<4;
    lo = toupper(lo);
    if( isxdigit(lo) ) {
      if( lo > '9' ) lo -= 7;  // software offset for A-F
      lo -= 0x30;              // subtract ASCII offset
      b = b + lo;
      return b;
    } // else error
  }  // else error
  return 0;
}


