/*
 *
 *
 *
 */

#include <string.h>  // memset

#define IRKEY_FREEM_DATA_ON  0x07F7  // special addon escapes to datamode
#define IRKEY_FREEM_DATA_OFF 0x07F8  // special addon escapes out datamode

/*
#define IRKEY_ONE    0x80  // 128
#define IRKEY_TWO    0x81  // 129
#define IRKEY_THREE  0x82  // 130
#define IRKEY_FOUR   0x83  // 131
#define IRKEY_FIVE   0x84  // 132
#define IRKEY_SIX    0x85  // 133
#define IRKEY_SEVEN  0x86  // 134
#define IRKEY_EIGHT  0x87  // 135
#define IRKEY_NINE   0x88  // 136
#define IRKEY_ZERO   0x89  // 137
#define IRKEY_ENTER  0x8b  // 139
#define IRKEY_CHUP   0x90  // 144
#define IRKEY_CHDN   0x91  // 145
#define IRKEY_VOLUP  0x92  // 146
#define IRKEY_VOLDN  0x93  // 147
#define IRKEY_MUTE   0x94  // 148
#define IRKEY_POWER  0x95  // 149

#define IRKEY_TVVCR  0xa5  // 165
#define IRKEY_PREVCH 0xbb  // 187
#define IRKEY_MENU   0xe0  // 224
#define IRKEY_ENTER2 0xe5  // 229
*/

// measure values from remote on T0/64:  310, 157, 82 
// measure values from remote on T0/256:  78,  40, 22
// measure values from remote on T0/1024: 20,  10,  6
// corresponds to 2500us, 1240us, 680us,  
// ==> 8us per tick @T0/64   ; 5000us = 625ticks, 3000us = 375 ticks
// ==> 32us per tick @T0/256 ; 5000us = 156ticks, 3000us = 93.75 ticks
//#define USEC_PER_TICK 8  // IRremote is 50
#define USECPERTICK 32 // IRremote is 50
#define TICKS(us) ((us)/USECPERTICK)

#define GAP_USEC 3000
#define GAP_TICKS (GAP_USEC/USECPERTICK)

//#define SONY_HDR_MARK_TICKS  255
//#define SONY_ONE_MARK_TICKS  100
//#define SONY_ZERO_MARK_TICKS  50  //unused

#define SONY_HDR_MARK_REAL  2400
#define SONY_ONE_MARK_REAL  1200
#define SONY_ZERO_MARK_REAL  600

#define SONY_HDR_MARK  2000
#define SONY_ONE_MARK   900
#define SONY_ZERO_MARK  700

#define IRKEY_ONE    0x010  
#define IRKEY_TWO    0x810
#define IRKEY_THREE  0x410
#define IRKEY_FOUR   0xc10
#define IRKEY_FIVE   0x210
#define IRKEY_SIX    0xa10
#define IRKEY_SEVEN  0x610
#define IRKEY_EIGHT  0x310
#define IRKEY_NINE   0x110
#define IRKEY_ZERO   0x910
#define IRKEY_ENTER  0xa170 // FIXME
#define IRKEY_CHUP   0x090 
#define IRKEY_CHDN   0x890
#define IRKEY_VOLUP  0x490
#define IRKEY_VOLDN  0xc90
#define IRKEY_MUTE   0x290
#define IRKEY_POWER  0xa90

#define IRKEY_TVVCR  0xa50
#define IRKEY_PREVCH 0xdd0
#define IRKEY_MENU   0x070
#define IRKEY_ENTER2 0xa70


// there are 13 LOW transitions in a standard Sony IR packet
//#define SAMPLE_SIZE 12
#define SAMPLE_SIZE 32
static volatile uint8_t irtime[SAMPLE_SIZE+1];  // +1 for header
static volatile uint8_t  irpos;  // pos in irtime
//static volatile uint8_t  t0over; // overflow counts
//static volatile uint32_t irkey;  // last IR key read
//static volatile uint32_t irdata[2];
//static volatile uint8_t irdatapos;
////#define irkey0 irdata[0]
//#define irkey1 irdata[1]
//#define irkey irkey0

static volatile uint32_t ird;  // = irdata[irdatapos];
static volatile uint16_t deltaTicks;
//static volatile uint8_t tover;
#define irkey ird

static volatile uint8_t ir_ready ;

//call this to initialize hardware, enable global interrupts in mainline code
//timer 0 (8 bit), normal mode, /256 clock, @8Mhz - 32us/bit,rollover in 8.192ms
//#define t0_on()             TCCR0B = _BV(CS02)
//timer 0 (8 bit), normal mode, /8 clock, @1Mhz - 8us/bit, rollover in 2.048ms
//#define t0_on()             TCCR0B = _BV(CS01); //|_BV(CS00)
//timer 0 (8 bit), normal mode, /64 clock, @1Mhz - 64us/bit,rollover in 16.384ms
//#define t0_on()             TCCR0B = _BV(CS01); //|_BV(CS00)
static void ir_init()
{
    // FIXME: make IRPIN input? nahh
    PCMSK  = _BV(PCINT4);  // FIXME  // set up a pin change mask for PB3 only
    GIMSK |= _BV(PCIE);              // pin change interrupt enable
    TIMSK |= _BV(TOIE0);             // enable timer0 overflow interrupt    
    irpos  = 0;
    //t0over = 0;
    //TCCR0B = _BV(CS01);                // start timer0 with /8
    //TCCR0B = _BV(CS01) | _BV(CS00);    // start timer0 with /64 
    TCCR0B = _BV(CS02);                  // start timer0 with /256 
    IR_PORT |= _BV(IR_RCV); // turn on internal pullup
    TCNT0=0;
}


static void handle_sample()
{
    softuart_puts("irpos:");
    softuart_printDec16(irpos);
    softuart_putc(':');

    // it starts at 1 because irpos=0 contains header time
    for( int i=1; i<irpos; i++) {   // convert bits to byte
        softuart_printDec16(irtime[i]);
        softuart_putc(',');
        if( irtime[i] > TICKS(SONY_ONE_MARK) ) {
            ird = (ird<<1) | 1;
        } else {
            ird = (ird<<1) | 0;
        }
    }

    //irdatapos++;
    irpos = 0;  // not recording now
    for( int i=0; i<SAMPLE_SIZE; i++) { irtime[i]=0; } // bzero()

    softuart_putc('=');
    softuart_printHex16( (ird >>16) );
    softuart_printHex16( (ird & 0xffff) );
    softuart_putc('\n');

    ird = 0;   // data used up
    softuart_putc('$');
}

// get the last key read (if any) , return 0 if no key
static uint32_t ir_getkey()
{
    if( ir_ready ) { 
        handle_sample();
        ir_ready = 0; // consume
    }
    uint32_t k = irkey;
    irkey = 0;
    return k;
}


// this is unneeded when Timer0 is /1024 and 32usec/tick
// counts number of times TCNT0 overflows
// normally only ever gets to 1 on start burst
ISR(TIMER0_OVF_vect)
{
    if( irpos> 0 ) {
        ir_ready = 1;  // catches case for remotes
    }

}

// connect to pin change interrupt masked for PBx only
ISR(PCINT0_vect)
{
    uint8_t in = (IR_PIN & _BV(IR_RCV));  // read in level
    deltaTicks = TCNT0;          // get time since last pin change
    TCNT0  = 0;                  // reset timer for next pass
    
    // first,  (FIXME: this is not in use)
    if( deltaTicks > GAP_TICKS && irpos > 0 ) { // gap between codes & recording
        softuart_putc('!');
        return;
    }

    // don't record the constant ("high") pulses of the sony protocol
    if( in==0 ) {  // inverted logic
        return;
    }
    
    // FIXME
    if( deltaTicks > TICKS(SONY_HDR_MARK) ) {      // got start bit? 
        irpos = 0;  // reset in case it's not
        softuart_putc('\n');  // say we got header
        softuart_putc('*');
    }
    
    // otherwise deltaTime is < SONY_HDR_MARK and thus maybe data
    irtime[irpos] = deltaTicks;
    irpos++;  // now we're recording

    if( irpos == SAMPLE_SIZE ) {
        ir_ready = 1;
    }

}
