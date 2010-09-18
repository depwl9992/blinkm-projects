/**
 * freem_p7.c -- FreeM: BlinkM Battery Remote demonstration firmware
 *               Use with "BlinkM Battery Remote" (battir3) circuit
 * 
 * 2010 Tod E. Kurt, labs.thingm.com
 *
 * Remote control functions:  (Sony TV remotes only right now)
 * - POWER     - stop script & turn off blinkm
 * - CH  UP/DN - solid hue mode, cycle hue if held down
 * - VOL UP/DN - brightness up/down
 * - ENTER     -
 * - MUTE      - 
 * - Num keys  - do something interesting
 *   1 - random slowly changing mood light (calc'd in FreeM)
 *   2 - fast random colors, ch up/dn to change speed
 *   3 - flashing beacon, use ch up/dn to change hue, press 3 again to start
 *   4 - fixed color, hold to change hue, vol/up dn to change brightness
 *   5 - flash once
 *   6 - purple
 *   7 - dim white, hold to change bright
 *   8 - med white, hold to change bright
 *   9 - bright white, hold to change bright
 *   0 - play script 0 on BlinkM
 * 
 *
 * CtrlM IR protocol:
 * ------------------
 * CtrlM Commands are 8 bytes long. 
 * These 8 bytes are sent out as two 32-bit word packets, 
 * using a modified 32-bit version of the Sony IR protocol.
 * Consisting of the bytes:
 * byte0 -- 0x55        - start byte
 * byte1 -- freem_addr  -
 * byte2 -- blinkm_addr -
 * byte3 -- blinkm_cmd  - 
 * byte4 -- blinkm_arg1 - blinkm arg1 (if not used, set to 0)
 * byte5 -- blinkm_arg2 - blinkm arg2 (if not used, set to 0)
 * byte6 -- blinkm_arg3 - blinkm arg3 (if not used, set to 0)
 * byte7 -- checksum    - simple 8-bit sum of bytes 0-6
 *
 *
 * FreeM ATtiny85 pinout
 * ----------------------
 * attiny85       -- function                         -- header
 * pin 1 -- PB5   -- RESET                            --  R
 * pin 2 -- PB3   -- PHOTOCELL  (unused)
 * pin 3 -- PB4   -- IRDETECT
 * pin 4 -- GND   -- GND                              --  -
 * pin 5 -- PB0   -- SDA/MOSI                         --  d 
 * pin 6 -- PB1   -- MISO  -- LED1 / serial TX out    --  I
 * pin 7 -- PB2   -- SCK/SCL                          --  c
 * pin 8 -- VCC   -- VCC                              --  +
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>    // for _delay_ms()
#include <stdint.h>        // for types like uint8_t, etc.
#include <stdlib.h>        // for rand()

// set DEBUG to 0 to disable all debug info and save code space
// set DEBUG to 1 to enable serial debugging on pin 6 (PB1)
// set DEBUG to 2 to enable serial logging off all keys received 
#define DEBUG 1
#define DEBUG_IR 0

#define LED_PIN  PB1
#define LED_PORT PORTB
#define LED_DDR  DDRB

// these must be defined before including i2cmaster_bitbang
#define I2C_DDR  DDRB
#define I2C_PIN  PINB
#define I2C_PORT PORTB
#define I2C_CLK  PB2    
#define I2C_DAT  PB0

// these must be defined before inclding irsony
#define IR_RCV   PB4
#define IR_PORT  PORTB
#define IR_DDR   DDRB
#define IR_PIN   PINB


#if DEBUG>0
// must define this before including softuart.h
#if F_CPU == 500000
#define _baudRate  2400   // if F_CPU=500kHz, doesn't work at other baudrates
#endif
#if F_CPU == 8000000
#define _baudRate  38400  // if F_CPU=8MHz
#endif
#include "softuart.h"
#else  // if DEBUG==0 nuke the func calls so they're just ignored
#define softuart_init()
#define softuart_putc(x)
#define softuart_puts(x)
#define softuart_putsP(x)
#define softuart_printHex(x)
#define softuart_printHex16(x)
#endif

//#include "softuart_util.h"


#include "i2cmaster_bitbang.h"
#include "irsony.h"


// I2C address of blinkm, or 0 to address all blinkms
uint8_t i2csendaddr = 0;

#include "blinkm_funcs.h"   // uses i2csendaddr


//  The timing information for timer to fire periodically 
#if F_CPU == 500000
#define MILLIS_PER_T1OVF       4      // actually 4.1msec
#endif
#if F_CPU == 8000000
#define MILLIS_PER_T1OVF       2      // actually 4.048 msec
#endif

static volatile uint16_t millis = 0;  // current time, set by timer ISR 
static volatile uint8_t t1ovf_count;  
static uint16_t lastmillis;           // the previous time we did something

//-----------------------------------------------------

#define FADESPEED_DEFAULT    10
#define FADESPEED_RANDMOOD    2
#define FADESPEED_RANDFAST  100

#define MILLIS_MOOD  4000
#define MILLIS_FAST   300

//#define KEYCNT_HOLD  6   // number of 'keycnt's to be considered a hold press
#define KEYMILLIS_HOLD  1000 // number of millis to be considered a hold press

// the different modes FreeM can be in
enum {
    MODE_OFF,
    MODE_RANDMOOD,
    MODE_RANDFAST,
    MODE_FLASH,
    MODE_FLASHONCE,
    MODE_NUMKEY,
    MODE_WHITE,
    MODE_HSB,
    MODE_PLAYSCRIPT,
};
uint8_t mode = MODE_OFF; 

uint16_t key;            // keypress
uint16_t key_last;       // the previous keypress
uint16_t keymillis;      // last time a key was pressed

uint8_t  hue = 0;        // current hue
uint8_t  bri = 255;      // current brightness

uint16_t changemillis;   // the last time we changed, for RANDMOOD, etc.

// address of FreeM
uint8_t freem_addr  = 0x80;

// buf to store copy of incoming data packet
uint8_t buf[8];


// address of this FreeM
uint8_t  ee_freem_addr         EEMEM = 0;


// -----------------------------------------------------

static void statled_set(uint8_t v)
{
    if(v) LED_PORT |=  _BV( LED_PIN );
    else  LED_PORT &=~ _BV( LED_PIN );
} 
static void statled_toggle(void)
{
    LED_PORT ^= _BV(LED_PIN);
}

// a little hello fanfare
static void fanfare( uint8_t times ) 
{
    for( int i=0;i<times; i++ ) {
#if DEBUG > 0
        softuart_puts("*");
#else
        statled_set(1);
#endif
        blinkm_setRGB( 0x09,0x09,0x09);
        _delay_ms(150);
#if DEBUG > 0
#else
        statled_set(0);
#endif
        blinkm_setRGB( 0x00,0x00,0x00);
        _delay_ms(150);
    }
}


//
// Parse the key presses from IR remote
//
static void handle_key(void)
{
    key = ir_getdata( buf );
    if( key == 0 ) {       // no IR data of any kind
        return;
    }
    else if( key == 1 ) {  // got CtrlM 8-byte packet
#if DEBUG >0
        softuart_putsP("ctrlm:");
        //softuart_printHex( buf[0] ); softuart_putc(',');
        softuart_printHex( buf[1] ); softuart_putc(',');
        softuart_printHex( buf[2] ); softuart_putc(',');
        softuart_printHex( buf[3] ); softuart_putc(',');
        softuart_printHex( buf[4] ); softuart_putc(',');
        softuart_printHex( buf[5] ); softuart_putc(',');
        softuart_printHex( buf[6] ); softuart_putc('\n');
        //softuart_printHex( buf[7] ); softuart_putc('\n');
#else
        statled_set(1);
#endif
        if( buf[2] == 0xff && buf[3] == 0xff ) {  // set freem addr cmd
            freem_addr = buf[1];
            eeprom_write_byte( &ee_freem_addr, freem_addr ); // write address
            fanfare(5);  // let people know we did the deed
        }
        else if( buf[1] == freem_addr || buf[1] == 0 ) {

            i2csendaddr = buf[2];
            blinkm_sendCmd3( buf[3], buf[4], buf[5], buf[6] );
        }
#if DEBUG >0
#else 
        statled_set(0);
#endif
        return;  // done with data mode
    }

    // Otherwise, it's an RC code

    uint16_t keymillisdiff = (millis-keymillis);
    if( keymillisdiff > 5000 ) { // 5 sec, reset
        keymillis = millis;
        keymillisdiff = 0;
    }
    if( key != key_last ) {
        keymillis = millis;
    }
    key_last = key;
    uint8_t keyheld = (keymillisdiff > KEYMILLIS_HOLD);

#if DEBUG > 1
    softuart_puts("k:"); softuart_printHex16( key ); softuart_putc('\n');
#else
    statled_set(1);
#endif

    if( !(key == IRKEY_VOLUP || key == IRKEY_VOLDN) ) { 
        blinkm_stopScript();
        blinkm_setFadespeed( FADESPEED_DEFAULT );
    }

    if(      key == IRKEY_POWER ) {
        softuart_putsP("off\n");
        mode = MODE_OFF;
        blinkm_turnOff();
    }
    else if( key == IRKEY_CHUP ) {    // increase hue, for current mode
        if( mode == MODE_OFF ) mode = MODE_HSB;
        hue+=2;
        softuart_putsP("hue+:"); softuart_printHex(hue); softuart_putc('\n');
    }
    else if( key == IRKEY_CHDN ) {    // decrease hue, for current mode
        if( mode == MODE_OFF ) mode = MODE_HSB;
        softuart_putsP("hue-:"); softuart_printHex(hue); softuart_putc('\n');
    }
    else if( key == IRKEY_VOLUP ) {   // increase brightness, for current mode
        if( mode == MODE_OFF ) mode = MODE_HSB;
        if( bri < 253 ) bri+=2;
        softuart_putsP("bri+:"); softuart_printHex(bri); softuart_putc('\n');
    }
    else if( key == IRKEY_VOLDN ) {   // increase brightness, for current mode
        if( mode == MODE_OFF ) mode = MODE_HSB;
        if( bri > 2 ) bri-=2;
        softuart_putsP("bri-:"); softuart_printHex(bri); softuart_putc('\n');
    }
    else if( key == IRKEY_ONE ) {     // mode: random mood light 
        softuart_putsP("moodlight\n");
        mode = MODE_RANDMOOD;
        if( bri == 0 ) bri = 255;     // fix brightness on startup
    }
    else if( key == IRKEY_TWO ) {     // mode: fast random 
        softuart_putsP("random\n");
        mode = MODE_RANDFAST;
        if( bri == 0 ) bri = 255;     // fix brightness on startup
    }
    else if( key == IRKEY_THREE ) {   // mode: beacon flash
        softuart_putsP("flash\n");
        mode = MODE_FLASH;
        if( keyheld ) {  // reset hue/bri on holding key
            hue = 0;  
            bri = 255;
            softuart_putsP("hue reset\n");
        }
    }
    else if( key == IRKEY_FOUR ) {    // mode: fixed color
        softuart_putsP("hsb\n");
        mode = MODE_HSB;
        if( bri == 0 ) bri = 255;
        if( keyheld ) {  // reset hue/bri on holding key
            hue+=2; 
        }
        //blinkm_fadeToHSB( hue, 0xff, bri );
    }
    else if( key == IRKEY_FIVE ) {   // mode: single flash at color
        softuart_putsP("flash once\n");
        //mode = MODE_NUMKEY;
        //blinkm_fadeToRGB( 0x00,0xff,0xff );
        mode = MODE_FLASHONCE;
        changemillis = 0;
        if( keyheld ) {  // reset hue/bri on holding key
            hue = 0;
            bri = 255;
            softuart_putsP("hue reset\n");
        }
    }
    else if( key == IRKEY_SIX ) {    // purple, no, need better
        softuart_putsP("purple, fixme\n");
        mode = MODE_NUMKEY;
        blinkm_fadeToRGB( 0xff,0x00,0xff );
    }
    else if( key == IRKEY_SEVEN ) {  // low-white at vol up/dn brightness
        softuart_putsP("low-white\n");
        mode = MODE_WHITE;
        bri = 0x11;
        blinkm_fadeToRGB( bri, bri, bri );
    }
    else if( key == IRKEY_EIGHT ) {  // mid-white at vol up/dn brightness
        softuart_putsP("mid-white\n");
        mode = MODE_WHITE;
        bri = 0x80;
        blinkm_fadeToRGB( bri, bri, bri );
    }
    else if( key == IRKEY_NINE ) {   // low-white at vol up/dn brightness
        softuart_putsP("white\n");
        mode = MODE_WHITE;
        blinkm_fadeToRGB( bri, bri, bri );
        if( keyheld ) { // make brighter on key hold
            bri += 10;               // will just wrap around
        }
    }
    else if( key == IRKEY_ZERO ) {
        softuart_putsP("play0\n");
        mode = MODE_PLAYSCRIPT;
        blinkm_setFadespeed( FADESPEED_DEFAULT );
        blinkm_playScript( 0,0 );
    }
#if DEBUG > 0
#else
    statled_set(0);
#endif
    key = 0;
}

// ------------------------------------------------------


//
// 
//
int main( void )
{
    CLKPR = _BV( CLKPCE );                  // enable clock prescale change
#if F_CPU == 500000
    CLKPR = _BV(CLKPS2) ;                   // div clock by 16 (8MHz -> 0.5MHz)
#else
    CLKPR = 0;                              // full speed (8MHz)
#endif

#if F_CPU == 500000
    TCCR1 |= _BV(CS12);                     // clock/8  see calc below @0.5MHz
#endif
#if F_CPU == 8000000
    TCCR1 |= _BV(CS12)|_BV(CS11)|_BV(CS10); // clock/64 see calc below @8MHz
#endif

    TIFR  |= _BV( TOV1 );       // clear interrupt flag
    TIMSK |= _BV( TOIE1 );      // enable timer 1 overflow interrupt

    sei();              // enable interrupts 

    LED_DDR |= _BV( LED_PIN );  // make output

    softuart_init();
    softuart_putsP("\nfreem_p7\n");

    _delay_ms(300);  // wait for power to stabilize before doing i2c

    ir_init();

    i2c_init();


#if DEBUG > 0
    softuart_puts("300ms? ");
    lastmillis = millis;
    _delay_ms(300);     // timing test, just chill a bit, yo
    softuart_printDec16( millis-lastmillis );   // should be close to 300
#endif

    blinkm_turnOff();

    fanfare( 3 );

    //mode = MODE_RANDMOOD;  // start out in moodlight mode
    lastmillis = 0-MILLIS_MOOD;
    blinkm_fadeToHSB( rand(),255,bri );


    // loop forever 
    for( ; ; ) {

        handle_key();  // read a key, maybe change mode, or hue/bri
        //get_ir_data();

        // FIXME: this could be refactored to be more efficient
        if( mode == MODE_RANDMOOD ) { 
            if( millis - changemillis > MILLIS_MOOD ) {
                softuart_putsP("hsb:");
                softuart_printHex(hue); softuart_putc(',');
                softuart_printHex(bri); softuart_putc('\n');
                changemillis = millis;
                hue = rand();
                softuart_putsP("rhb:");
            }
            blinkm_setFadespeed( FADESPEED_RANDMOOD );
            blinkm_fadeToHSB( hue, 255, bri );
        }
        else if( mode == MODE_RANDFAST ) { 
            if( millis - changemillis > MILLIS_FAST ) { 
                changemillis = millis;
                hue = rand();
                blinkm_setFadespeed( FADESPEED_RANDFAST );
                blinkm_fadeToHSB( hue, 255, bri );
            }
            blinkm_setFadespeed( FADESPEED_RANDFAST );
            blinkm_fadeToHSB( hue, 255, bri );
        }
        else if( mode == MODE_WHITE ) {
            blinkm_setFadespeed( FADESPEED_RANDMOOD );
            blinkm_fadeToRGB( bri, bri, bri );
        }
        else if( mode == MODE_HSB ) {
            blinkm_setFadespeed( FADESPEED_RANDMOOD );
            blinkm_fadeToHSB( hue, 255, bri );
        }
        else if( mode == MODE_FLASH ) {
            if( millis - changemillis > MILLIS_FAST ) { 
                changemillis = millis;
                bri = (bri==0) ? 255:0;
                blinkm_setFadespeed( FADESPEED_RANDFAST );
                blinkm_fadeToHSB( hue, 255, bri);
            }
        }
        else if( mode == MODE_FLASHONCE ) {
            if( changemillis == 0 ) { // start
                blinkm_setFadespeed( FADESPEED_RANDFAST );
                blinkm_fadeToHSB( hue, 255, bri);
                changemillis = millis;;
            }
            else if( millis - changemillis > MILLIS_FAST ) {  //stop
                blinkm_fadeToHSB( 0,0,0);
                mode = MODE_OFF;
                changemillis = 0;
            }
        }

        
    } // for
}


//
// Timer1 Overflow interrupt
// Called on overflow (256 ticks) of Timer1. 0.5MHz internal clock 
// We want approximately 50 milliseconds between measurements
// calculate with '1/(0.5e6/256/_1024_) * 1000' to get milliseconds
// At 0.5MHz:
//  CS      11    set (clk/    2), Timer1 ovfs @976.6 Hz   (  1.0ms)
//  CS   12       set (clk/    8), Timer1 ovfs @244.1 Hz   (  4.1ms)
//  CS   12 11 10 set (clk/   64), Timer1 ovfs @ 30.5 Hz   ( 32.7ms)*
//  CS13       10 set (clk/  256), Timer1 ovfs @  7.6 Hz   (131.0ms)
//  CS13    11    set (clk/  512), Timer1 ovfs @  3.8 Hz   (262.1ms) 
//  CS13    11 10 set (clk/ 1024), Timer1 ovfs @  1.9 Hz   (524.3ms)
//  CS13 12 11    set (clk/ 8192), Timer1 ovfs @
//
// At 8MHz:
//  CS   12 11 10 set (clk/   64), Timer1 ovfs @ 30.5 Hz   ( 32.7ms)*
//
// See table 15-2  in ATtiny45 datasheet for more
ISR(TIMER1_OVF_vect)
{
    millis += MILLIS_PER_T1OVF;
}


//-----------------------------------------------------
// utility funcs
//

// this is a really cheapie rand
// using this instead of real rand() saves 300 bytes
/*
static uint8_t rand8bit(void)
{
    static int16_t rctx;
    if( rctx == 0 ) rctx = 1234; // 123467;
    int hi = (rctx<<1) / 123;
    int lo = (rctx<<1) % 123;
    int  x = 1687 * lo - 286 * hi;
    rctx = x;
    return x;
}
#define rand(x) rand8bit(x)
*/
