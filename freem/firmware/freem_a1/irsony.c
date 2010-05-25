/*
 *
 *
 *
 */

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

//#define IRKEY_PREV  187
// no, these are for some other thing
#define IRKEY_STOP  1432
#define IRKEY_PAUSE 1433
#define IRKEY_PLAY  1434
#define IRKEY_PREV  1435
#define IRKEY_NEXT  1436


// there are 13 LOW transitions in a standard Sony IR packet
#define SAMPLE_SIZE 13
static volatile uint16_t irtime[SAMPLE_SIZE];
static volatile uint8_t  irpos; // pos in irtime
static volatile uint8_t t0over; // overflow counts
static volatile uint16_t irkey; // last IR key read

//call this to initialize hardware, enable global interrupts in mainline code
//timer 0 (8 bit), normal mode, /256 clock, @8Mhz - 32us/bit, rollover in 8.192ms
//#define t0_on()             TCCR0B = _BV(CS02)
//timer 0 (8 bit), normal mode, /8 clock, @1Mhz - 8us/bit, rollover in 2.048ms
//#define t0_on()             TCCR0B = _BV(CS01); //|_BV(CS00)
//timer 0 (8 bit), normal mode, /64 clock, @1Mhz - 64us/bit, rollover in 16.384ms
//#define t0_on()             TCCR0B = _BV(CS01); //|_BV(CS00)
static void ir_init()
{
    // FIXME: make IRPIN input? nahh
    PCMSK  = _BV(PCINT4);  // FIXME  // set up a pin change mask for PB3 only
    GIMSK |= _BV(PCIE);              // pin change interrupt enable
    TIMSK |= _BV(TOIE0);             // enable timer0 overflow interrupt    
    irpos  = 0;
    t0over = 0;
    TCCR0B = _BV(CS01);              // start timer0 with /64
    IRPORT |= _BV(IRRCV); // turn on internal pullup
}
// get the last key read (if any) , return 0 if no key
static uint16_t ir_getkey()
{
    uint16_t k = irkey;
    irkey=0;
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
    uint8_t in = (IRPIN & _BV(IRRCV));  // read in level
    uint8_t deltaTime = TCNT0;          // get time since last pin change
    uint8_t tover = t0over;
    TCNT0  = 0;                         // reset timer value for next pass
    t0over = 0;

    if( !in )  // only measure low times ('in' is newly changed-to pin val)
        return;

    //if( tover > 0 ) {  // got start bit?  // 1MHz
    if( tover > 0 || deltaTime > 0x90 ) { // got start bit // 500kHz
        irpos = 1;  // irpos!=0 means we're recording
        return;
    }
    else if( irpos == 0 ) return;
    // and if irpos!=0, capture the bits
    irtime[irpos] = deltaTime;
    irpos++;
    if( irpos == SAMPLE_SIZE ) {  // got all the bits?
        irpos = 0;
        uint16_t k = 0;
        for( int i=1; i<SAMPLE_SIZE; i++) {   // convert bits to byte
            //if( irtime[i] > 0x80 ) {  // FIXME: no hardcoded bit times (1MHz)
            if( irtime[i] > 0x40 ) {  // FIXME: no hardcoded bit times (500kHz)
                k |= (1<<(i-1));
            }
        }
        irkey = k;
    } // if( irpos == SAMPLE_SIZE)
}
