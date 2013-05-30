/**
 * CtrlM as IR repeater firmware v1
 *
 * Copyright 2009-2010, Tod E. Kurt, tod@thingm.com
 *
 *
 * CtrlM layout on ATtiny85
 * ------------------------
 * ATtiny85 has 8 pins:
 *   pin 1  -  PB5 - Reset
 *   pin 2  -  PB3 - XTAL1
 *   pin 3  -  PB4 - XTAL2
 *   pin 4  -  GND - Ground
 *   pin 5  -  PB0 - I2C SDA  / button
 *   pin 6  -  PB1 - IRLED / STATLED / OC0B/OC1A
 *   pin 7  -  PB2 - I2C SCL 
 *   pin 8  -  VCC - +5VDC
 *
 *
 * Sony IR codes, see:
 *  http://lirc.sourceforge.net/remotes/sony/V164G-TV
 *  http://www.arcfn.com/2010/03/understanding-sony-ir-remote-codes-lirc.html
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>    // for memcpy_P()
#include <avr/eeprom.h>
#include <util/delay.h>      
#include <avr/boot.h>
#include <string.h>          // for memcpy()

#include "ctrlm_nonvol_data.h"

#define BLINKM_PROTOCOL_VERSION_MAJOR 'b'
#define BLINKM_PROTOCOL_VERSION_MINOR 'b'

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || \
      defined(__AVR_ATtiny85__)

#define PIN_RED  PB3
#define PIN_GRN  PB4
#define PIN_BLU  PB1

#define PIN_IRLED   PB1
#define PIN_STATLED PB1   // FIXME: note, the same as IRLED

#define PIN_SDA     PB0
#define PIN_SCL     PB2

#define LED_MASK  ((1<< PIN_RED) | (1<< PIN_GRN) | (1<< PIN_BLU))
#define INPI2C_MASK ((1<< PIN_SDA) | (1<< PIN_SCL))

#define DEFAULT_FREQVAL 210

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || \
      defined(__AVR_ATtiny84__)

#define PIN_RED  PB2     // MaxM
#define PIN_GRN  PA7     // MaxM
#define PIN_BLU  PA5     // MaxM

#define PIN_IRLED   PA5
//#define PIN_STATLED PB2
#define PIN_STATLED PA5

#define PIN_SDA     PA6
#define PIN_SCL     PA4

// FIXME: this is invalid, since color pins span ports
#define LEDA_MASK  ((1<< PIN_GRN) | (1<< PIN_BLU))
#define LEDB_MASK  ((1<< PIN_RED))
#define INPI2C_MASK ((1<< PIN_SDA) | (1<< PIN_SCL))

#define DEFAULT_FREQVAL 105

#endif


uint8_t ir_freqval = DEFAULT_FREQVAL;  // for timer1, FIXME: use 
uint8_t ir_dutyval = DEFAULT_FREQVAL/3;  // 33% duty cycle

#include "IRsend.h"


// ----------------------------------------------------

// This function quickly pulses the visible LED 
// NOTE: we can only flash quickly and not full-on because 
// no current-limiting resistor on IR LED connected to same pin as stat LED
//
// at 60us cycle * 100 = 6000us == 6ms
static void led_flash( void )
{
    for( uint8_t i=0; i<20; i++ ) {  // was 100
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || \
      defined(__AVR_ATtiny85__)
        PORTB |= _BV(PIN_STATLED);
        _delay_ms(1);  // was 30us
        PORTB &=~ _BV(PIN_STATLED);
        _delay_us(10);
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || \
      defined(__AVR_ATtiny84__)
        PORTA |= _BV(PIN_STATLED);
        _delay_ms(1);  // was 30us
        PORTA &=~ _BV(PIN_STATLED);
        _delay_us(10);
#endif
    }
}

static void fanfare( uint8_t n, int millis ) 
{
    for( uint8_t i=0; i<n; i++ ) {
        led_flash();
        for( int j=0; j<millis; j++) {
            _delay_ms(1); // why does _delay_ms(millis) load extra 900 bytes?
        }
    }
}

// for testing, see below
static void basic_tests(void);

/*
 *
 */
int main( void )
{    
    _delay_ms(100);
    // set clock speed
    CLKPR = _BV( CLKPCE );                 // enable clock prescale change
    CLKPR = 0;                             // full speed (8MHz);

    // set up periodic timer for state machine ('script_tick')
    TCCR0B = _BV( CS02 ) | _BV(CS00); // start timer, prescale CLK/1024
    //TIFR   = _BV( TOV0 );          // clear interrupt flag
    //TIMSK  = _BV( TOIE0 );         // enable overflow interrupt

    // set up output pins   
    PORTB = INPI2C_MASK;          // turn on pullups 
    DDRB  = LED_MASK;             // set LED port pins to output

    fanfare( 3, 300 );

    IRsend_enableIROut();
    IRsend_iroff();

    // This loop runs forever. 
    for(;;) {
        if( PINB & _BV(PIN_SDA) ) { 
            IRsend_iroff();
        } else { 
            IRsend_iron();
        }
    }
    
} // end



//
static void basic_tests(void)
{

#if 0 
    // test basic PWM functionality, put a 2MHz 33% dutycycle sqwave out
    TCCR1 = _BV(PWM1A) | _BV(COM1A0) | _BV(CS10);
    GTCCR = 0;
    OCR1C = 3;
    OCR1A = 1;
    while(1) { ; }
#endif

#if 0
    // test based IR frequency output
    IRsend_enableIROut();
    IRsend_iron();
    while( 1 ) { ; }
#endif

#if 0 
    // test basic IR sending capability
    while( 1 ) { 
        IRsend_sendSony( 0x610, 12); // 7 key
        //IRsend_sendSony( 0x910, 12); // 0 key
        _delay_ms(1500);
        IRsend_sendSony( 0xa90, 12); // power key 
        _delay_ms(1500);
    }
#endif

#if 0 
    // test basic IR sending capability
    int c = 0;
    while( 1 ) { 
        IRsend_sendSony( c, 16);
        _delay_ms(50);
        c--;
    }

#endif

#if 0 
    // test basic IR sending capability
    while( 1 ) { 
        IRsend_sendRC5( TOPBIT + 0x1234, 32); // 
        _delay_ms(1500);
        IRsend_sendRC5( TOPBIT + 0xABCD, 32); // 
        _delay_ms(1500);
    }
#endif

#if 0
    // test basic IR sending capability
    int c = 0;
    while( 1 ) { 
        IRsend_sendRC6( c, 32); // 
        _delay_ms(50);
        c--;
    }
#endif

#if 0 
    // test basic IR sending capability
    int c = 0;
    while( 1 ) { 
        IRsend_txByte( reverse8(c) ); // 
        _delay_ms(50);
        c++;
    }
#endif

#if 0 
    // test basic IR sending capability
    int c = 0;
    while( 1 ) { 
        IRsend_sendSonyData4( c ); // 
        c += 0x100;    //c--;
        //_delay_ms(50); // 50msec needed when using IRrecvDump
        _delay_ms(5);   // 3msec GAP in ctrlm source
        IRsend_sendSonyData4( c ); // 
        c++;          //c--;
        /*
        _delay_ms(5);
        IRsend_sendSonyData4( c-- ); // 
        _delay_ms(5);
        IRsend_sendSonyData4( c-- ); // 
        */
        _delay_ms(100);
    }
#endif

#if 1 
    // test basic IR sending capability
    uint8_t b[8] = {0x01, 0x23, 0x45, 0x67,  0x89, 0xAB, 0xCD, 0xEF};
    //    uint64_t* dp = (uint64_t*) b;
    while( 1 ) { 
        IRsend_sendSonyData64bit( b ); // 
        //c += 0x1;    //c--;
        b[0]++; b[7]++;
        _delay_ms(20);
    }
#endif

}

    // testing 38kHz output
    // Timer0 on OC0B -- doesn't work
    //OCR0B = 100; 
    //TCCR0A =_BV(COM0B0) | _BV(WGM01);          // set up timer 0
    //TCCR0B = _BV(CS00);
    
    // Timer1 on OC1B -- works
    //OCR1C = ir_freqval;
    //OCR1B = OCR1C/2; // 50% duty cycle
    //GTCCR = _BV(PWM1B) | _BV(COM1B1);
    //TCCR1 = _BV(CS10);   // clock/1, no prescale

    // Timer1 on OC1A -- works
    // fpwm = ftck1 / (ocr1c + 1) -> ocr1c = (ftck1/fpwm) - 1
    // (8e6/38e3) - 1 = 209
    //OCR1C = 209;
    //OCR1A = OCR1C/3; // 33% duty cycle
    //TCCR1 = _BV(PWM1A) | _BV(COM1A0) | _BV(CS10);
    //GTCCR = 0;


    /*
    // set up periodic timer for sotware PWM
    TCCR0B = _BV( CS00 );          // start timer, no prescale
    TIFR   = _BV( TOV0 );          // clear interrupt flag
    TIMSK  = _BV( TOIE0 );         // enable overflow interrupt
    */

    /*
    // set up periodic timer for state machine ('script_tick')
    TCCR1 =  _BV( CS13 ) | _BV( CS11 ) | _BV( CS10 ); 
    //TCCR1 =  _BV( CS13 ) | _BV( CS11 ); 
    TIFR  |= _BV( TOV1 );
    TIMSK |= _BV( TOIE1 );
    */

/*
// 
// State machine pulse
// Slews the current value of red,grn,blu towards red_dest,grn_dest,blu_dest
// Called on overflow (256 ticks) of Timer1.  8MHz internal clock
//
// With CS   12,11,10  set (clk/   64), Timer 1 overflows @  488 Hz
// With CS13           set (clk/  128), Timer 1 overflows @  244 Hz
// With CS13,      10  set (clk/  256), Timer 1 overflows @  122 Hz
// With CS13,   11     set (clk/  512), Timer 1 overflows @   61 Hz 
// With CS13,   11 10  set (clk/ 1024), Timer 1 overflows @   30.517 Hz (3.33 ms)
// With CS13,12,11,10  set (clk/16384), Timer 1 overflows @    1.9 Hz
// We want around 10 ms (100Hz) between evals,
//
ISR(SIG_OVERFLOW1)
{
    cmd = 1;
}

*/

