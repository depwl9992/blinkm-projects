/*
 *
 *
 *
 */

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

#define GAP_TICKS 200

// there are 13 LOW transitions in a standard Sony IR packet
//#define SAMPLE_SIZE 12
#define SAMPLE_SIZE 32
static volatile uint16_t irtime[SAMPLE_SIZE];
static volatile uint8_t  irpos;  // pos in irtime
static volatile uint8_t  t0over; // overflow counts
#static volatile uint32_t irkey;  // last IR key read
static volatile uint32_t irdata[2];
#define irkey irdata[0]

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
    t0over = 0;
    //TCCR0B = _BV(CS01);              // start timer0 with /8
    TCCR0B = _BV(CS01) | _BV(CS00);    // start timer0 with /64
    IR_PORT |= _BV(IR_RCV); // turn on internal pullup
}
// get the last key read (if any) , return 0 if no key
static uint32_t ir_getkey()
{
    uint32_t k = irkey;
    irkey = 0;
    return k;
}

// counts number of times TCNT0 overflows
// normally only ever gets to 1 on start burst
ISR(TIMER0_OVF_vect)
{
    t0over++;
}

// connect to pin change interrupt masked for PBx only
ISR(PCINT0_vect)
{
    uint8_t in = (IR_PIN & _BV(IR_RCV));  // read in level
    uint16_t deltaTime = TCNT0;          // get time since last pin change
    uint8_t tover = t0over;
    TCNT0  = 0;                         // reset timer value for next pass
    t0over = 0;

    deltaTime = 256*tover + deltaTime;
    
    if( !in )  // only measure low times ('in' is newly changed-to pin val)
        return;
    /*
    if( in && irpos == 0 ) { // on high
        if( tover > 0 ) { 
            softuart_putc('*');
        }
    }
    */

    if( deltaTime > GAP_TICKS && irpos > 0 ) { // timeout
        softuart_putc(':');
        goto handlesample;
    }

    // FIXME
    if( tover > 0 ) {  // got start bit?  // 1MHz
    //if( tover > 0 || deltaTime > 0x90 ) { // got start bit // 500kHz
        irpos = 1;  // irpos!=0 means we're recording
        softuart_putc('\n');
        softuart_putc('*');
        return;
    }
    else if( irpos == 0 ) {  // FIXME: why this?
        return;
    }

    // and if irpos!=0, capture the bits
    irtime[irpos-1] = deltaTime;
    irpos++;

    if( irpos== SAMPLE_SIZE ) {
        goto handlesample;
    }

    return;

    // FIXME: this is kind of a mess
 handlesample:
    //    if( irpos == SAMPLE_SIZE ) {  // got all the bits?
        for( int i=0; i<irpos; i++) {   // convert bits to byte
            softuart_printDec16(irtime[i]);
            softuart_putc(',');
            if( irtime[i] > 0x60 ) {   // FIXME: no hardcoded bit times (8MHz)
                irkey = (irkey<<1) | 1;
            } else {
                irkey <<= 1;
            }
        }
        //if( irpos < 16 ) k >>= 20;  // rc remote code
        irpos = 0;

        //irkey = k;
        softuart_putc('=');
        softuart_printHex16( (irkey>>16) );
        softuart_printHex16( (irkey & 0xffff) );
        softuart_putc('\n');
        //} // if( irpos == SAMPLE_SIZE)
}
