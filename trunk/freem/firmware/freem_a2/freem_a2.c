/**
 * freem_a2.c -- FreeM: BlinkM Battery Remote demonstration firmware
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
 * - Sending keycode of 0x1234 escapes into data mode
 * - Sending keycode of 0x1235 escapes out of data mode
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
#define DEBUG 1

#define LED_PIN  PB1

#define IRRCV    PB4
#define IRPORT   PORTB
#define IRDDR    DDRB
#define IRPIN    PINB

#if DEBUG>0
// must define this before including softuart.h
#if F_CPU == 500000
#define _baudRate  2400 // if F_CPU=500kHz
#endif
#if F_CPU == 8000000
#define _baudRate  38400  // if F_CPU=8MHz
#endif
#include "softuart.h"
#endif

#include "i2cmaster_bitbang.c"
#include "irsony.c"


//  The timing information for timer to fire periodically to measure touch  
#define MEASUREMENT_PERIOD_MS       49
#define T1OVF_PER_MEASUREMENT        3

// flag set by timer ISR when it's time to measure touch 
static volatile uint8_t time_to_act = 0;

// current time, set by timer ISR 
static volatile uint16_t millis = 0;
static volatile uint8_t t1ovf_count;
static volatile uint8_t timertick;

//-----------------------------------------------------

//#define MICROSEC_PER_TICK 8   1MHz with  /8 timer0 clock 
#define MICROSEC_PER_TICK 64    1MHz with /64 timer0 clock

#define FADESPEED_DEFAULT 10
#define FADESPEED_RANDMOOD 3
#define FADESPEED_RANDFAST 100

#define MILLIS_MOOD  5000
#define MILLIS_FAST  250

// the different modes FreeM can be in
enum {
    MODE_OFF,
    MODE_HUE,
    MODE_RANDMOOD,
    MODE_RANDFAST,
    MODE_FLASH,
    MODE_NUMKEY,
    MODE_PLAYSCRIPT,
    MODE_DATA,
};
uint8_t mode = MODE_OFF;

uint16_t key;
uint8_t hue;
uint8_t bri;
uint16_t changemillis;

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

//
static void handle_key(void)
{
    key = ir_getkey();
    if( key==0 )  // no key
        return;
    
#if DEBUG > 1
    softuart_puts("k:"); softuart_printHex16( key ); softuart_putc('\n');
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
    else if( key == IRKEY_FREEM_DATA_ON ) { 
        mode = MODE_DATA;
    }
    else if( key == IRKEY_FREEM_DATA_OFF ) {
        mode = MODE_OFF;
    }
#if DEBUG>0
    softuart_printHex(mode); softuart_puts(" hb:");
    softuart_printHex(hue);  softuart_putc(',');
    softuart_printHex(bri);  softuart_putc('\n');
#endif
}

// ------------------------------------------------------

//
//   initialise host app, pins, watchdog, etc
//
static void init_system( void )
{
#if F_CPU == 500000
    CLKPR = _BV( CLKPCE );                 // enable clock prescale change
    CLKPR = _BV(CLKPS2) ;                  // div clock by 16
#endif
}

//
// configure periodic timer firing
//
static void init_timer_isr( void )
{
    TCCR1 |= _BV(CS12)|_BV(CS11)|_BV(CS10); // clock/64   see calc below @.5MHz
    //TCCR1 |= _BV(CS13);                   // clock/128  see calc below @1MHz
    //TCCR1 |= _BV(CS13) | _BV(CS11);       // clock/1024 see calc below @8MHz
    //TCCR1 |= _BV(CS13) | _BV(CS12) | _BV(CS11);  // clock/8192 // testing
    TIFR  |= _BV( TOV1 );       // clear interrupt flag
    TIMSK |= _BV( TOIE1 );      // enable timer 1 overflow interrupt
}


int main( void )
{
    init_system();      // initialise host app, pins, watchdog, etc 
    
    init_timer_isr();      // configure timer ISR to fire regularly 

    sei();       // enable interrupts 

#if DEBUG>0
    softuart_init();
    softuart_puts("\nfreem_a2\n");
#endif
    ir_init();

    i2c_init();
    
    _delay_ms(300);

    blinkm_turnOff();

    // a little hello fanfare
    for( int i=0;i<2; i++ ) {
#if DEBUG>0
        softuart_puts("!");
#endif
        blinkm_setRGB( 0x09,0x09,0x09);
        _delay_ms(150);
        blinkm_setRGB( 0x00,0x00,0x00);
        _delay_ms(150);
    }

    hue = 0;
    bri = 255;

    mode = MODE_RANDMOOD;  // start out in moodlight mode
    blinkm_fadeToHSB( rand(),255,bri );
    time_to_act = 1; 

    // loop forever 
    for( ; ; ) {
        if( time_to_act ) {
            time_to_act = 0; // clear flag

            handle_key();

        }
       
        // FIXME: this could be refactored to be more efficient
        if( mode == MODE_RANDMOOD ) { 
            if( millis - changemillis > MILLIS_MOOD ) {
                changemillis = millis;
                hue = rand();
#if DEBUG>0
                softuart_puts("rhb:");
                softuart_printHex(hue); softuart_putc(',');
                softuart_printHex(bri); softuart_putc('\n');
#endif
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
        else if( mode == MODE_DATA ) {
#if DEBUG>0
            softuart_puts("data mode\n");
            key = ir_getkey();
            for( int i=0;i<key; i++ ) {  // debug: blink num received
                softuart_printHex16( i );
                blinkm_setRGB( 0x20,0x20,0x20);
                _delay_ms(100);
            }
#endif
        }

    } // for
}


//
// Timer1 Overflow interrupt
// Called on overflow (256 ticks) of Timer1.  8MHz internal clock  FIXME
// We want approximately 50 milliseconds between measurements
// calculate with '1/(8e6/256/_1024_) * 1000' to get milliseconds
// With CS   12       set (clk/    8), Timer1 overflows @ 
// With CS13       10 set (clk/  256), Timer1 overflows @ 122 Hz       (8.2ms)
// With CS13    11    set (clk/  512), Timer1 overflows @  61 Hz      (16.3ms) *
// With CS13    11 10 set (clk/ 1024), Timer1 overflows @  30.517 Hz  (32.7ms)
// With CS13 12 11    set (clk/ 8192), Timer1 overflows @  3.814 Hz   (262ms)
// See table 15-2  in ATtiny45 datasheet for more
ISR(TIMER1_OVF_vect)
{
    timertick++;
    t1ovf_count++;
    if( t1ovf_count == T1OVF_PER_MEASUREMENT ) {
        t1ovf_count = 0;
        time_to_act = 1;    //  set flag: time to do something
        millis += MEASUREMENT_PERIOD_MS;  // update current time 
    }
}


