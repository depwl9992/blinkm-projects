
// ------------------------------------------------------------

// pulse parameters in tens usec
#define RC5_T1		    89
#define RC5_RPT_LENGTH	4600

#define RC6_HDR_MARK	266
#define RC6_HDR_SPACE	89
#define RC6_T1          44
#define RC6_RPT_LENGTH	4600

#define NEC_HDR_MARK	900
#define NEC_HDR_SPACE	450
#define NEC_BIT_MARK	56
#define NEC_ONE_SPACE	160
#define NEC_ZERO_SPACE	56
#define NEC_RPT_SPACE	225

#define SONY_HDR_MARK	240
#define SONY_HDR_SPACE	60
#define SONY_ONE_MARK	120
#define SONY_ZERO_MARK	60
#define SONY_RPT_LENGTH 4500
#define SONY_BITS 12

// this gives us a transmit rate of ~771 bits/second. 
#define DATA_HDR_MARK   240
#define DATA_HDR_SPACE  30
#define DATA_ZERO_MARK  30
#define DATA_ZERO_SPACE 30
#define DATA_ONE_MARK   60
#define DATA_ONE_SPACE  30

//#define Data0_Time_ON   30    // carrier on time for data 0 (300uS)
//#define Data0_Time_OFF  30    // carrier off time for data 0
//#define Data1_Time_ON   60    // carrier on time for data 1 
//#define Data1_Time_OFF  30    // carrier off time for data 1

#define TOPBIT 0x80000000
#define TOPBIT8 0x80



//#define freq_to_timer0val(x) ((F_CPU / x - 1)/ 2)
#define freq_to_timer1val(x) ((F_CPU / x) - 1 )

// Shortcut to insert single, non-optimized-out nop
#define NOP __asm__ __volatile__ ("nop")
// Tweak this if neccessary to change timing
#define DELAY_CNT 11

// This function delays the specified number of 10 microseconds
// it is 'hardcoded' and is calibrated by adjusting DELAY_CNT 
// in main.h Unless you are changing the crystal from 8mhz, dont
// mess with this.
static void delay_ten_us(uint16_t us)
{
  uint8_t timer;
  while (us != 0) {
    // for 8MHz we want to delay 80 cycles per 10 microseconds
    // this code is tweaked to give about that amount.
    for (timer=0; timer <= DELAY_CNT; timer++) {
      NOP;
      NOP;
    }
    NOP;
    us--;
  }
}

//
inline static void IRsend_iron(void)
{
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || \
      defined(__AVR_ATtiny85__)

    //TCCR1 = _BV(PWM1A) | _BV(COM1A0) | _BV(CS10);
    TCCR1 |= _BV(COM1A0);

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || \
      defined(__AVR_ATtiny84__)

    TCCR1A |=  _BV(COM1B1);

#endif
}

//
inline static void IRsend_iroff(void)
{
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || \
      defined(__AVR_ATtiny85__)

    TCCR1 &=~ _BV(COM1A0);
    //PORTB &=~ _BV(PIN_IRLED); // turn off IR LED

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || \
      defined(__AVR_ATtiny84__)

    TCCR1A &=~  _BV(COM1B1);

#endif

}

//
static void IRsend_mark(uint16_t time)
{
    // Sends an IR mark for the specified number of microseconds.
    // The mark output is modulated at the PWM frequency.
    IRsend_iron();
    delay_ten_us(time);
}
// Leave pin off for time (given in microseconds) 
static void IRsend_space(uint16_t time)
{
    // Sends an IR space for the specified number of microseconds.
    // A space is no output, so the PWM output is disabled.
    IRsend_iroff();
    delay_ten_us(time);
}

// configure PWM module 
// PWM1B mode:
// to enable 50% PWM on OC1B and not on /OC1B:
// TCCR1 = _BV(CS10);
// GTCCR = _BV(PWM1B);
// OCR1C = ((F_CPU / freq - 1));
// OCR1B = OCR1C/2
static void IRsend_enableIROut(void) 
{

    //const int freq = freq_to_timer1val( hz );

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || \
      defined(__AVR_ATtiny85__)

    //OCR1C = ir_freqval;
    //OCR1B = OCR1C/2; // 50% duty cycle
    //OCR1B = OCR1C/3; // 33% duty cycle
    //OCR0B = ir_freqval; 

    GTCCR = 0;
    TCCR1 = _BV(PWM1A) | _BV(CS10);
    OCR1C = ir_freqval;
    OCR1A = ir_dutyval; ///OCR1C / 3; // 33% duty cycle
    //OCR1A = OCR1C / 4; // 25% duty cycle

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || \
      defined(__AVR_ATtiny84__)

    // cyz_rgb - "Fast PWM, ICR1 = TOP"
    //TCCR1A = _BV(COM1B1) | _BV(COM1B0)| _BV(WGM11);
    //TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS00);
    //ICR1  = 0xFFFF;
    //OCR1B = 0xFFFF;

    // blinkm maxm - pwm, phase correct, 8-bit
    //TCCR1A = _BV(WGM10) |  _BV(COM0B1); //| _BV(COM0B0);
    //TCCR1B = _BV(CS11) | _BV(CS10);
    //OCR1B = 0x80;

    // pwm, phase & frequency correct, ICR1 = TOP
    TCCR1A = _BV(COM1B1) | _BV(WGM10);
    TCCR1B = _BV(WGM13) | _BV(CS10) ; //| _BV(CS10);
           
    // supposedly: Foc = fclk / (2*N*top)
    // or, Foc = F_CPU/( 2 * 1 * OCR1A ) =  8e6/(2*1*0x65) = 39.6 kHz
    OCR1A = ir_freqval;
    OCR1B = ir_dutyval; // OCR1A/3; // 33% duty cycle
        
#endif

}

// From IRremote.cpp
static void IRsend_sendSony(uint32_t data, uint16_t nbits)
{
    IRsend_enableIROut();
    IRsend_mark(SONY_HDR_MARK);
    IRsend_space(SONY_HDR_SPACE);
    data = data << (32 - nbits);
    for (int i = 0; i < nbits; i++) {
        if (data & TOPBIT) {
            IRsend_mark(SONY_ONE_MARK);
            IRsend_space(SONY_HDR_SPACE);
        } 
        else {
            IRsend_mark(SONY_ZERO_MARK);
            IRsend_space(SONY_HDR_SPACE);
        }
        data <<= 1;
    }
}
//
static void IRsend_sendSonyData4(uint32_t data )
{
    uint8_t nbits = 32;
    IRsend_enableIROut();
    IRsend_mark(SONY_HDR_MARK);
    IRsend_space(SONY_HDR_SPACE);
    data = data << (32 - nbits);
    for (int i = 0; i < nbits; i++) {
        if (data & TOPBIT) {
            IRsend_mark(SONY_ONE_MARK);
            IRsend_space(SONY_HDR_SPACE);
        } 
        else {
            IRsend_mark(SONY_ZERO_MARK);
            IRsend_space(SONY_HDR_SPACE);
        }
        data <<= 1;
    }
}

// an experiment, in sending all 8-bytes at once
// and it appears to work
// THIS IS THE MAIN DATA SENDING FUNCTION
// public
static void IRsend_sendSonyData64bit(uint8_t* data )
{
    //uint16_t wait_ten_us = wait_millis * 100; // convert millis to "10usecs" 

    IRsend_enableIROut();
    IRsend_mark(SONY_HDR_MARK);
    IRsend_space(SONY_HDR_SPACE);

    for( int i=0; i< 8; i++ ) {     // 8 bytes
        uint8_t dat = data[i];
        for( int j=0; j<8; j++ ) {  // 8 bits in a byte
            if (dat & TOPBIT8) {
                IRsend_mark(SONY_ONE_MARK);
                IRsend_space(SONY_HDR_SPACE);
            } 
            else {
                IRsend_mark(SONY_ZERO_MARK);
                IRsend_space(SONY_HDR_SPACE);
            }
            dat <<= 1;
        }
    }

    //if( wait_after) 
    //    delay_ten_us( wait_ten_us );
}

//
// This was the main workhorse for ctrlm data sending:
// Send an 8-byte data buffer out, using two 32-bit (4-byte) packets
static void IRsend_sendSonyData8(uint8_t* data, uint8_t wait_millis, uint8_t wait_after)
{
    uint16_t wait_ten_us = wait_millis * 100; // convert millis to "10usecs" 
    uint32_t* dp0 = ((uint32_t*)(void*)data+0);
    uint32_t* dp1 = ((uint32_t*)(void*)data+1);

    IRsend_sendSonyData4( *dp0 );
    delay_ten_us( wait_ten_us );

    IRsend_sendSonyData4( *dp1 );

    if( wait_after) 
        delay_ten_us( wait_ten_us );
}

// From IRremote.cpp
void IRsend_sendNEC(uint32_t data, int nbits)
{
    IRsend_enableIROut();
    IRsend_mark(NEC_HDR_MARK);
    IRsend_space(NEC_HDR_SPACE);
    for (int i = 0; i < nbits; i++) {
        if (data & TOPBIT) {
            IRsend_mark(NEC_BIT_MARK);
            IRsend_space(NEC_ONE_SPACE);
        } 
        else {
            IRsend_mark(NEC_BIT_MARK);
            IRsend_space(NEC_ZERO_SPACE);
        }
        data <<= 1;
    }
    IRsend_mark(NEC_BIT_MARK);
    IRsend_space(0);
}

// From IRremote.cpp
// Note: first bit must be a one (start bit)
void IRsend_sendRC5(uint32_t data, int nbits)
{
    IRsend_enableIROut();  // 36);
    data = data << (32 - nbits); // the first bit
    IRsend_mark(RC5_T1);  // First start bit
    IRsend_space(RC5_T1); // Second start bit
    IRsend_mark(RC5_T1);  // Second start bit
    for (int i = 0; i < nbits; i++) {
        if (data & TOPBIT) {
            IRsend_space(RC5_T1); // 1 is space, then mark
            IRsend_mark(RC5_T1);
        }
        else {
            IRsend_mark(RC5_T1);
            IRsend_space(RC5_T1);
        }
        data <<= 1;
    }
    IRsend_space(0); // Turn off at end
}


// From IRremote.cpp
// Caller needs to take care of flipping the toggle bit
void IRsend_sendRC6(unsigned long data, int nbits)
{
    IRsend_enableIROut(); //36);
    data = data << (32 - nbits);
    IRsend_mark(RC6_HDR_MARK);
    IRsend_space(RC6_HDR_SPACE);
    IRsend_mark(RC6_T1); // start bit
    IRsend_space(RC6_T1);
    int t;
    for (int i = 0; i < nbits; i++) {
        if (i == 3) {
            t = 2 * RC6_T1;      // double-wide trailer bit
        } 
        else {
            t = RC6_T1;
        }
        if (data & TOPBIT) {
            IRsend_mark(t);
            IRsend_space(t);
        } 
        else {
            IRsend_space(t);
            IRsend_mark(t);
        }
        data <<= 1;
    }
    IRsend_space(0); // Turn off at end
}


// from defcon-badge-16 ir.c
static void IRsend_txByte(uint8_t val)
{
    uint8_t whichBit, temp;
    
    IRsend_enableIROut(); 
    IRsend_mark( DATA_HDR_MARK );
    IRsend_space( DATA_HDR_SPACE ); 
    
    whichBit = 0;
    while (whichBit < 8) {
        temp = 1 << whichBit;
        
        if ((val & temp) == 0) {
            //ir_tx_data_0();
            IRsend_mark(DATA_ZERO_MARK);
            IRsend_space(DATA_ZERO_SPACE);
        } else {
            //ir_tx_data_1();
            IRsend_mark(DATA_ONE_MARK);
            IRsend_space(DATA_ONE_SPACE);
        }

        whichBit++;
    }
}
// send a buffer, e.g.
// IRsend_txBuff( {'c', 0xff,0xff,0xff}, 4 );
static void IRsend_txBuff(uint8_t *buf, int len)
{
    for( int i=0; i<len; i++) {
        IRsend_txByte(*buf);
        buf++;
        _delay_ms(3); 
        // need delay in between bytes to prevent IR receiver from locking up
  }	
}
// from defcon-badge-16 ir.c
static void IRsend_txString(uint8_t *Array_to_send)
{
  while(*Array_to_send != '\0')	{
	  IRsend_txByte(*Array_to_send);
      Array_to_send++;
      _delay_ms(3);
      // need delay in between bytes to prevent IR receiver from locking up
  }	
}

// from IRremote.cpp
// buf contains on/off times
static void IRsend_sendRaw(unsigned int buf[], int len)
{
    IRsend_enableIROut();
    for (int i = 0; i < len; i++) {
        if (i & 1) 
            IRsend_space(buf[i]);
        else 
            IRsend_mark(buf[i]);
    }
    IRsend_space(0); // Just to be sure
}


