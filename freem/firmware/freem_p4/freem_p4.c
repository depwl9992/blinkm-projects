/**
 * freem_p4.c -- FreeM: BlinkM Battery Remote demonstration firmware
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
 *   2 - fast random colors
 *   3 - flashing beacon, use ch up/dn to change hue, press 3 again to start
 *   4 - yellow
 *   5 - aqua
 *   6 - purple
 *   7 - dim white
 *   8 - med white
 *   9 - bright white
 *   0 - play script 0 on BlinkM
 * 
 * Data Mode:
 * - Sending keycode of 0x07F7 escapes into data mode
 * - Sending keycode of 0x07F8 escapes out of data mode
 * - data format is: len,byte_0,byte_1,byte_2,...byte_len-1
 *
 * "BlinkM Battery Remote" - battir3 pcb layout
 * ---------------------------------------------
 * attiny45       -- function
 * pin 1 -- PB5   -- RESET
 * pin 2 -- PB3   -- PHOTOCELL  (unused)
 * pin 3 -- PB4   -- IRDETECT
 * pin 4 -- GND   -- GND
 * pin 5 -- PB0   -- SDA/MOSI    -- 
 * pin 6 -- PB1   -- MISO        -- LED1 & serial TX out
 * pin 7 -- PB2   -- SCK/SCL     -- 
 * pin 8 -- VCC   -- VCC
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>    // for _delay_ms()
#include <stdint.h>        // for types like uint8_t, etc.
#include <stdlib.h>        // for rand()

// set DEBUG to 0 to disable all debug info and save code space
// set DEBUG to 1 to enable serial debugging on pin 6 (PB1)
// set DEBUG to 2 to enable serial logging off all keys received 
#define DEBUG 2

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
#define softuart_printHex(x)
#define softuart_printHex16(x)
#endif

//#include "softuart_util.h"


#include "i2cmaster_bitbang.h"
#include "irsony.h"


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

// the different modes FreeM can be in
enum {
    MODE_OFF,
    MODE_RANDMOOD,
    MODE_RANDFAST,
    MODE_FLASH,
    MODE_NUMKEY,
    MODE_HUE,
    MODE_PLAYSCRIPT,
    MODE_DATA,
};
uint8_t mode = MODE_OFF; 

uint16_t key;            // keypress
uint8_t hue;             // current hue
uint8_t bri;             // current brightness
uint16_t changemillis;   // the last time we changed, for RANDMOOD, etc.

// I2C address of blinkm, or 0 to address all blinkms
uint8_t i2csendaddr = 0;

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

//-----------------------------------------------------
// blinkm funcs
//

static void blinkm_stopScript(void)
{
    i2c_start();
    i2c_write((i2csendaddr<<1));  // FIXME: check return value
    i2c_write('o');  
    i2c_stop();
}

static void blinkm_sendCmd3( uint8_t c, uint8_t a1, uint8_t a2, uint8_t a3 )
{
    i2c_start();
    i2c_write((i2csendaddr<<1));  // FIXME: check return value
    i2c_write( c );  
    i2c_write( a1 );       
    i2c_write( a2 );       
    i2c_write( a3 );       
    i2c_stop();        
}

static void blinkm_setFadespeed( uint8_t f ) 
{
    i2c_start();
    i2c_write((i2csendaddr<<1));  // FIXME: check return value
    i2c_write('f');
    i2c_write(f);
    i2c_stop();    

}
static void blinkm_fadeToRGB( uint8_t r, uint8_t g, uint8_t b )
{
    blinkm_sendCmd3( 'c', r,g,b );
}

static void blinkm_fadeToHSB( uint8_t h, uint8_t s, uint8_t b )
{
    blinkm_sendCmd3( 'h', h,s,b );
}

static void blinkm_setRGB(  uint8_t r, uint8_t g, uint8_t b )
{
    blinkm_sendCmd3( 'n', r,g,b );
}

static void blinkm_playScript( uint8_t p, uint8_t n )
{
    blinkm_sendCmd3( 'p', p,0,n );
}

static void blinkm_turnOff(void)
{
    blinkm_stopScript();
    blinkm_setRGB( 0,0,0 );
}


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

//
// Attempt to have a litle datapath in there for sending blinkm cmds over IR
// Doesn't work very reliably yet
//
static void get_data(void)
{
    uint8_t i,j,d[8];                  // the data, at most 8 bytes
    uint16_t v,len=0;

    j=200;
    while( !(len = ir_getkey()) ) {      // length of packet
        _delay_ms(1);
        j--;
        if( j == 0 )                     // FIXME: dumb timeout
            goto dataerror;
    }
    len = (len>8) ? 8 : len;             // enforce bounds

    for( i=0; i<len; i++ ) {  
        j = 200;
        while( !(v=ir_getkey()) ) {     // wait for key then store it
            _delay_ms(1);
            j--;
            if( j == 0 )                // FIXME: dumb timeout
                goto dataerror2;
        }
        d[i] = v;
    }


 datadone:
    /* // this takes too long, 2400 bps sucks
    softuart_printHex(len);
    softuart_putc('=');
    for( i=0;i<len;i++) {
        softuart_printHex( d[i] ); softuart_putc(',');
    }
    */
    // now do something with that data
    if( len == 4 ) { 
        softuart_putc('*');
        blinkm_sendCmd3( d[0], d[1],d[2],d[3] );
    }
    mode = MODE_OFF;
    return;

 dataerror2:
    softuart_putc('!');
 dataerror:
    softuart_putc('!');
    goto datadone;  // so we can at least see the partial data
}

//
// Parse the key presses from IR remote
//
static void handle_key(void)
{
    key = ir_getkey();
    if( key==0 )  // no key
        return;
    
#if DEBUG > 1
    softuart_puts("k:"); softuart_printHex16( key ); softuart_putc('\n');
#endif
#if DEBUG==0
        statled_toggle();
#endif

    if( !(key == IRKEY_VOLUP || key == IRKEY_VOLDN) ) { 
        blinkm_stopScript();
        blinkm_setFadespeed( FADESPEED_DEFAULT );
    }

    if(      key == IRKEY_POWER ) {
        mode = MODE_OFF;
        blinkm_turnOff();
    }
    else if( key == IRKEY_CHUP ) {
        mode = MODE_HUE;
        if( bri==0 ) bri = 120;
        hue +=5;
    }
    else if( key == IRKEY_CHDN ) { 
        mode = MODE_HUE;
        if( bri==0 ) bri = 120;
        hue -=5;
    }
    else if( key == IRKEY_VOLUP ) { 
        if( bri >= 250 )
            bri = 255;
        else bri += 10;
    }
    else if( key == IRKEY_VOLDN ) { 
        if( bri < 5 )
            bri = 0;
        else bri -= 10;
    }
    else if( key == IRKEY_ONE ) {
        mode = MODE_RANDMOOD;
        if( bri == 0 ) bri = 127;
    }
    else if( key == IRKEY_TWO ) {
        mode = MODE_RANDFAST;
        if( bri == 0 ) bri = 127;
    }
    else if( key == IRKEY_THREE ) {
        mode = MODE_FLASH;
    }
    else if( key == IRKEY_FOUR ) {
        mode = MODE_NUMKEY;
        blinkm_fadeToRGB( 0xff,0xff,0x00 );
    }
    else if( key == IRKEY_FIVE ) {
        mode = MODE_NUMKEY;
        blinkm_fadeToRGB( 0x00,0xff,0xff );
    }
    else if( key == IRKEY_SIX ) {
        mode = MODE_NUMKEY;
        blinkm_fadeToRGB( 0xff,0x00,0xff );
    }
    else if( key == IRKEY_SEVEN ) {
        mode = MODE_NUMKEY;
        blinkm_fadeToRGB( 0x11,0x11,0x11 );
    }
    else if( key == IRKEY_EIGHT ) {
        mode = MODE_NUMKEY;
        blinkm_fadeToRGB( 0x80,0x80,0x80 );
    }
    else if( key == IRKEY_NINE ) {
        mode = MODE_NUMKEY;
        blinkm_fadeToRGB( 0xff,0xff,0xff );
    }
    else if( key == IRKEY_ZERO ) {
        mode = MODE_PLAYSCRIPT;
        blinkm_setFadespeed( FADESPEED_DEFAULT );
        blinkm_playScript( 0,0 );
    }
    //else if( key == IRKEY_FREEM_DATA_ON ) { 
    //    mode = MODE_DATA;
    //    get_data();
    // }
    //else if( key == IRKEY_FREEM_DATA_OFF ) {
    //    mode = MODE_OFF;
    //}

    /*
    softuart_puts("m:"); softuart_printHex(mode); 
    softuart_puts(" hb:"); // print little summary
    softuart_printHex(hue);  softuart_putc(',');
    softuart_printHex(bri);  softuart_putc('\n');
    */
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
    softuart_puts("\nfreem_p4\n");

    ir_init();

    i2c_init();
    
    softuart_puts("300ms? ");
    lastmillis = millis;
    _delay_ms(300);     // just chill a bit, yo
    softuart_printDec16( millis-lastmillis );   // debug timekeeping

    blinkm_turnOff();

    // a little hello fanfare
    statled_set(1);
    for( int i=0;i<2; i++ ) {
        softuart_puts("!");
        blinkm_setRGB( 0x09,0x09,0x09);
        _delay_ms(150);
        blinkm_setRGB( 0x00,0x00,0x00);
        _delay_ms(150);
    }

    hue = 0;
    bri = 255;

    //mode = MODE_RANDMOOD;  // start out in moodlight mode
    lastmillis = 0-MILLIS_MOOD;
    blinkm_fadeToHSB( rand(),255,bri );


    // loop forever 
    for( ; ; ) {

        handle_key();  // read a key, maybe change mode, or hue/bri
    
        // FIXME: this could be refactored to be more efficient
        if( mode == MODE_RANDMOOD ) { 
            if( millis - changemillis > MILLIS_MOOD ) {
                changemillis = millis;
                hue = rand();
                softuart_puts("rhb:");
                softuart_printHex(hue); softuart_putc(',');
                softuart_printHex(bri); softuart_putc('\n');
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
        else if( mode == MODE_HUE ) {
            blinkm_setFadespeed( FADESPEED_RANDMOOD );
            blinkm_fadeToHSB( hue, 255, bri );
        }
        else if( mode == MODE_FLASH ) {
            if( millis - changemillis > MILLIS_FAST ) { 
                changemillis = millis;
                bri = (bri==255) ? 0:255;
                blinkm_setFadespeed( FADESPEED_RANDFAST );
                blinkm_fadeToHSB( hue, 255, bri);
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


