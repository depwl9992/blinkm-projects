/*
 * CtrlMSimple1 -- Simple test of CtrlM
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


byte ctrlm_addr = 0x00;

///byte freem_addr = 12;
byte freem_addr = 0; 

// arduino setup func
void setup()
{
  Serial.begin(19200);
  Serial.println("CtrlMSimple!");

  CtrlM_beginWithPower();
  delay(1000);

  /*
  Serial.print("setting new addr...");
  CtrlM_writeFreeMAddress( ctrlm_addr, freem_addr);
  Serial.println("done");
  delay(1000);
  */

  //CtrlM_setSendAddress( ctrlm_addr, 0,0 ); // send on all addrs
  CtrlM_setSendAddress( ctrlm_addr, freem_addr, 0 );

  CtrlM_setFadeSpeed( ctrlm_addr, 200 );


}

byte n = 255;
int dtime = 70;
// arduino loop func
void loop()
{
    Serial.print("Sending color: ");
    Serial.println( n, HEX);
#if 1
    CtrlM_setRGB( ctrlm_addr, n, n, n );
    delay( dtime );
    CtrlM_setRGB( ctrlm_addr, 0,0,0 );
    //n++;
#endif

#if 0
    CtrlM_fadeToRGB( ctrlm_addr, 0xff,0xff,n );
    delay( dtime );
    CtrlM_fadeToRGB( ctrlm_addr, 0,0,n );
    n++;
#endif

    //delay(20); // delay(20) causes missing of 3-4 packets
    delay( dtime );
}



