/**
 * freem_a1.c -- BlinkM Battery Remote demonstration firmware
 * 
 *  2010 Tod E. Kurt, labs.thingm.com
 *
 * Remote control function:  (Sony TV remotes only right now)
 * - Keys 0-9:
 *   1 - red
 *   2 - green
 *   3 - blue
 *   4 - yellow
 *   5 - aqua 
 *   6 - purple
 *   7 - dim white
 *   8 - med white
 *   9 - bright white
 *   0 - off
 * pwr - play script or off toggle
 *
 *
 * battir3 pcb layout
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


#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#if F_CPU == 500000
#define _baudRate  2400 // if F_CPU=500kHz
#endif
#if F_CPU == 8000000
#define _baudRate  38400  // if F_CPU=8MHz
#endif

#include "softuart.h"

#include "i2cmaster_bitbang.c"

#define LED_PIN  PB1

#define IRRCV   PB4
#define IRPORT  PORTB
#define IRDDR   DDRB
#define IRPIN   PINB

#include "irsony.c"


//  The timing information for timer to fire periodically to measure touch  
#define MEASUREMENT_PERIOD_MS       49
#define T1OVF_PER_MEASUREMENT        3
// testing
//#define MEASUREMENT_PERIOD_MS       (262*4)
//#define T1OVF_PER_MEASUREMENT        4

// flag set by timer ISR when it's time to measure touch 
static volatile uint8_t time_to_measure = 0;

// current time, set by timer ISR 
static volatile uint16_t current_time_ms = 0;
static volatile uint8_t t1ovf_count;
static volatile uint8_t timertick;

//-----------------------------------------------------

//#define MICROSEC_PER_TICK 8   1MHz with  /8 timer0 clock 
#define MICROSEC_PER_TICK 64    1MHz with /64 timer0 clock


//-----------------------------------------------------

#define i2csendaddr 0

static void blinkm_stop(void)
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

static void blinkm_setRGB(  uint8_t r, uint8_t g, uint8_t b )
{
    blinkm_sendCmd3( 'n', r,g,b );
}

static void blinkm_playScript( uint8_t p, uint8_t n )
{
    blinkm_sendCmd3( 'p', p,0,n );
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
    //CLKPR = _BV(CLKPS1) | _BV(CLKPS0);   // div clock by 8
    //CLKPR = _BV(CLKPS2) | _BV(CLKPS0);   // div clock by 32

    //MCUCR |= (1 << PUD);   //  disable pull-ups 
    
    //DDRB  |= _BV(LED_PIN);     // make output 
    //PORTB |= _BV(LED_PIN);     // make HIGH
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

    softuart_init();
    softuart_puts("\nbattir3\n");

    ir_init();

    i2c_init();
    
    blinkm_stop();
    blinkm_setRGB( 0,0,0 );
    _delay_ms(300);

    // a little hello fanfare
    for( int i=0;i<2; i++ ) {
        softuart_printHex16( i );
        softuart_puts("!");
        blinkm_setRGB( 0x02,0x02,0x02);
        _delay_ms(50);
        blinkm_setRGB( 0x01,0x01,0x01);
        _delay_ms(50);
    }
    blinkm_setRGB( 0,0,0 );
    blinkm_playScript(0,0);

    sei();       // enable interrupts 

    softuart_puts(":\n");

    int power_on = 1;
    // loop forever 
    for( ; ; ) {
        if( time_to_measure ) {
            time_to_measure = 0; // clear flag: time to measure touch 

            uint16_t key = ir_getkey();
            if( key==0 )  // no key
                continue;

            softuart_puts("k:");
            softuart_printHex16( key );
            softuart_putc('\n');

            if(      key == IRKEY_ONE ) {
                blinkm_fadeToRGB( 0xff,0x00,0x00 );
            }
            else if( key == IRKEY_TWO ) {
                blinkm_fadeToRGB( 0x00,0xff,0x00 );
            }
            else if( key == IRKEY_THREE ) {
                blinkm_fadeToRGB( 0x00,0x00,0xff );
            }
            else if( key == IRKEY_FOUR ) {
                blinkm_fadeToRGB( 0xff,0xff,0x00 );
            }
            else if( key == IRKEY_FIVE ) {
                blinkm_fadeToRGB( 0x00,0xff,0xff );
            }
            else if( key == IRKEY_SIX ) {
                blinkm_fadeToRGB( 0xff,0x00,0xff );
            }
            else if( key == IRKEY_SEVEN ) {
                blinkm_fadeToRGB( 0x11,0x11,0x11 );
            }
            else if( key == IRKEY_EIGHT ) {
                blinkm_fadeToRGB( 0x80,0x80,0x80 );
            }
            else if( key == IRKEY_NINE ) {
                blinkm_fadeToRGB( 0xff,0xff,0xff );
            }
            else if( key == IRKEY_ZERO ) {
                blinkm_fadeToRGB( 0x00,0x00,0x00 );
            }
            else if( key == IRKEY_VOLUP ) {
                blinkm_setFadespeed( 80 );
            }
            else if( key == IRKEY_VOLDN ) {
                blinkm_setFadespeed( 5 );
            }
            else if( key == IRKEY_POWER ) {
                if( power_on ) { 
                    blinkm_stop();
                    blinkm_fadeToRGB( 0x00,0x00,0x00 );
                    power_on = 0;
                } else {
                    blinkm_playScript(0,0);
                    power_on = 1;
                }
                _delay_ms(500);  // arbitrary wait to approximate key debounce
            }
             else if( key == IRKEY_PLAY ) { 
                blinkm_playScript( 0, 0 );
            }
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
        time_to_measure = 1;    //  set flag: time to measure 
        current_time_ms += MEASUREMENT_PERIOD_MS;  // update current time 
    }
}

