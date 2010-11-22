/*
 *
 *
 *
 */


// Determine Optimal Timer0 settings for single-byte usec sampling
// ----------------------------------------------------------------
// measure hdr/mark/space vals from remote on T0/64:  310, 157, 82 
// measure hdr/mark/space vals from remote on T0/256:  78,  40, 22
// measure hdr/mark/space vals from remote on T0/1024: 20,  10,  6
// corresponds to 2500us, 1240us, 680us,  (from oscilloscope)
// ==>   8us per tick @T0/64   ; 5000us = 625.00 ticks, 3000us = 375 ticks
// ==>  32us per tick @T0/256  ; 5000us = 156.25 ticks, 3000us = 93.75 ticks
// ==> 128us per tick @T0/1024 ; 5000us =  39.06 ticks, 3000us = 23.43 ticks
// THUS: 32us per tick seems best, i.e. T0/256 TIMER0 setting
//#define USEC_PER_TICK 8  // IRremote is 50 usec/tick
#define USECPERTICK 32    // IRremote is 50
//#define USECPERTICK 128 
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
#define IRKEY_EIGHT  0xe10
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

#define IR_NONE      0
#define IR_GOT_DATA  1


// there are 13 LOW transitions in a standard Sony IR packet
// there are 32 LOW transisions (not counting header) in our 32-bit data packet
// there are 64 LOW transisions (not counting header) in our 64-bit data packet
//#define SAMPLE_SIZE_DATA 32
#define SAMPLE_SIZE_DATA 64
#define SAMPLE_SIZE_SONY 12

#define CTRLM_STARTBYTE 0x55

#define irdata_max 4
//static volatile uint32_t ird;       // holder of built-up data 
static volatile uint64_t ird;       // holder of built-up data 
static volatile uint8_t  irspos;    // sampling position, counts bits in data
static volatile uint8_t  irspos_last; 
//static volatile uint32_t irdata[irdata_max]; // holds finished 'ird'
static volatile uint64_t irdata[irdata_max]; // holds finished 'ird'
static volatile uint8_t irdatapos;  // pos in irdata[], !0 means data
#define irkey irdata[0] // backward compat

static volatile uint16_t deltaTicks; // measured duration of LOW pulse

// initialize pin change interrupt for IR reception
// public
static void ir_init()
{
    // FIXME: make IRPIN input? nahh
    PCMSK  = _BV(PCINT4);  // FIXME  // set up a pin change mask for PB3 only
    GIMSK |= _BV(PCIE);              // pin change interrupt enable
    TIMSK |= _BV(TOIE0);             // enable timer0 overflow interrupt    
    irspos  = 0;
    //t0over = 0;
    //TCCR0B = _BV(CS01);                // start timer0 with /8
    //TCCR0B = _BV(CS01) | _BV(CS00);    // start timer0 with /64 
    TCCR0B = _BV(CS02);                  // start timer0 with /256 
    IR_PORT |= _BV(IR_RCV); // turn on internal pullup
    TCNT0=0;
}

// look for ir data, 
// return 0 if no data, non-zero if data
// data is returned in 8-byte buf, even if it doesn't pass checksum
// public
static uint32_t ir_getdata(uint8_t* buf)
{
#if 0
    if( irdatapos > 0 ) {
        softuart_putc('='); softuart_printDec16( irspos_last );
        softuart_putc('\n');
    }
#endif

    // we have data and it's Sony RC code sized
    if( irdatapos > 0 && irspos_last  == SAMPLE_SIZE_SONY ) { // sony cmd
        uint32_t k = irdata[0];  // FIXME:!
        irdatapos = 0;
        return k;
    }

    // we've got some data and it's the size for a CtrlM protocol packet
    if( irdatapos > 0 && irspos_last == SAMPLE_SIZE_DATA ) {
        uint8_t* ba = (uint8_t*)irdata;
        /*         // debug
        for( int i=0; i<8; i++ ) {
            softuart_printHex( ba[7-i] ); softuart_putc(',');
        }
        softuart_putc('\n');
        */
        
        // was ba[0] but data got swizzled from doing a uint64_t on it
        if( ba[7] != CTRLM_STARTBYTE ) { // ignore if invalid start byte
            irdatapos = 0; // nuke it, wait for next packet
            return IR_NONE;
        }

        // we have TWO packets, of CtrlM packet size, so we have an entire 
        //if( irdatapos == 2 ) {
        if( irdatapos == 1 ) {
            for( int i=0; i<8; i++ ) { // copy to output array, be optimistic
                buf[i] = ba[7-i];
            }
            // compute checksum
            uint8_t c = ba[0]; // checksum that was sent // was ba[7]
            for( int i=1; i<8; i++ ) {
                c -= ba[i];    // should sum to zero
            }
            irdatapos = 0;     // consumed

            if( c == 0 ) {     // succesful!
                return IR_GOT_DATA;
            }
        }
        else { // irdatapos != 1
            //softuart_putc('.');
        }
    }
    return IR_NONE;
}

// get the last key read (if any) , return 0 if no key
// a simple version of ir_getdata
// public
static uint32_t ir_getkey()
{
    if( irdatapos > 0 ) {
#if DEBUG_IR >1
        softuart_puts("irdata0:");
        uint32_t d = irdata[0];
        softuart_printHex16( (d >>16) );
        softuart_printHex16( (d & 0xffff) );
        softuart_putc('\n');
#endif
        irdatapos = 0; // say we're done with it
    }
    return 0;
}

// put finished, parsed data packet in buffer
// private, called from interrupts
static void _ir_handle_sample()
{
    irdata[irdatapos] = ird;
    irdatapos++;
    if( irdatapos == irdata_max ) irdatapos = 0;  // just nuke and continue
    ird = 0;
    irspos_last = irspos;
    irspos = 0;
}

// this is unneeded when Timer0 is /1024 and 32usec/tick
// counts number of times TCNT0 overflows
// normally only ever gets to 1 on start burst
ISR(TIMER0_OVF_vect)
{
    if( irspos> 0 ) { // if we're recording and T0 overflowed, we're done
#if DEBUG_IR >0
        softuart_putc('~');
#endif
        _ir_handle_sample();
    }
}

// connect to pin change interrupt masked for PBx only
ISR(PCINT0_vect)
{
    uint8_t in = (IR_PIN & _BV(IR_RCV));  // read in level
    deltaTicks = TCNT0;                   // get time since last pin change
    TCNT0  = 0;                           // reset timer for next pass
    
    // first,  (FIXME: this is not in use)
    if( deltaTicks > GAP_TICKS && irspos > 0 ) { // gap b/n codes & recording
#if DEBUG_IR >0
        softuart_putc('#');  // (when will this happen?)
#endif
        return;
    }

    // don't record the constant (HIGH) pulses of the sony protocol
    if( in==0 ) {  // inverted logic
        return;
    }
    
    // check for header start
    if( deltaTicks > TICKS(SONY_HDR_MARK) ) { 
        irspos = 0;  // reset in case it's not
        ird = 0;
#if DEBUG_IR >0
        softuart_putc('*');  // at the STARt (get it?)
#endif
        return;
    }
    
    // otherwise deltaTime is < SONY_HDR_MARK and thus maybe data
    if( deltaTicks > TICKS(SONY_ONE_MARK) ) {
        ird = (ird<<1) | 1;  // parse MARK as a 1 bit
    } else {
        ird = (ird<<1) | 0;  // otherwise, parse as SPACE and thus a 0 bit
    }
    irspos++;  // go to next position (and irspos>0 means we're recording)

    if( irspos == SAMPLE_SIZE_DATA ) { // got the most bits we can store (32)
#if DEBUG_IR >0
        softuart_putc('|');  // burp i'm full
#endif
        _ir_handle_sample();
    }

}

