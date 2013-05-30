/**
 * CtrlM firmware v1
 *
 * Copyright 2009-2010, Tod E. Kurt, tod@thingm.com
 *
 *
 * CtrlM I2C Commands:
 * -------------------
 * In general, all BlinkM commands sent to CtrlM are sent out verbatim via IR,
 * via the 8-byte CtrlM IR protocol, and received by FreeM.
 * Thus CtrlM is like a virtual wire, in that you can send commands like:
 *
 * {cmd, arg1,arg2,arg3 } -- send arbitrary blinkm command on existing addr
 *
 * But there are a few exceptions.
 *
 * First, there are a few CtrlM-only commands:
 * {'@', freem_addr, blinkm_addr, 0 } -- set freem & blinkm addr to send to
 * {'#', freq_msb, freq_lsb, duty_percent }  -- set IR freq in kHz
 * //{'&', pair_msec, wait_after, } -- set time between 4-byte packets, FIXME
 * {'$', cmd_type,cmd3,cmd2,cmd1,cmd0}--send IR remote cmd (cmd_type=sony,nec,)
 * {'!',  freemaddr, blinkmaddr, cmd,arg1,arg2,arg3, 0,chksum} -- send arb data
 *
 * Second, some commands are not sent down the IR "wire". These commands are:
 * {'a' }       -- get i2c addr of CtrlM
 * {'A', addr}  -- set i2c addr of CtrlM
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
 * buffer layout is like:  (na == don't matter, set to zero)
 *                  buf[0], buf[1],     buf[2],      [3],  [4], [5], [6],  [7]
 *  std command:     {0x55,  freem_addr, blinkm_addr, cmd,  a1,  a2,  a3,   chk}
 *  write freemaddr: {0x55,  freem_addr, 0xff,        0xff, na,  na,  na,   chk}
 *  set colorspot:   {0x55,  freem_addr, 0xfe,        pos,  a1,  a2,  a3,   chk}
 *  play colorspot:  {0x55,  freem_addr, 0xfd,        pos, cmd,  na,  na,   chk}
 *
 *
 * Some LinkM.sh commands to try: (0x21 == '!'):
 *
 * Fade to purple (0xff,0x00,0xff):
 * ./linkm.sh -v --cmd "0x21,0x55,0,0,'c',0xff,0x00,0xff,0xb6" 
 *
 * Set colorspot 3 to 0x00,0xff,0xff:
 * ./linkm.sh -v --cmd "0x21,0x55,0,0xfe,3,0x00,0xff,0xff,0x54" 
 *
 * Play colorspot 3: 2nd way gives command, could be 'p'lay script
 * ./linkm.sh -v --cmd "0x21,0x55,0,0xfd,3,0x00,0x00,0x00,0x55" 
 * ./linkm.sh -v --cmd "0x21,0x55,0,0xfd,3,'c',0x00,0x00,0xb8" 
 *
 * Can also do: (with new CtrlM firmware that supports '^' and '*', 0x2a='*')
 * ./linkm.sh -v --cmd "0x2a,3,'p',0" 
 *
 * And, Set Colorspot:
 * ./linkm.sh -v --cmd "0x5e,3,0x98,0x76,0x54"  // with new ctrlm '^' == 0x53
 * ./linkm.sh -v --cmd "0x21,0x55,0,0xfe,3,0x98,0x76,0x54,0xb8" // by hand
 *
 *
 * EEPROM layout:
 * --------------
 * EEPROM is mapped out like so:
 *   addr 0: i2c address
 *   addr 1: boot mode 
 *   addr 2: script_id
 *   addr 3: script_reps
 *   addr 4: 
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

#include "usiTwiSlave.h"

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


uint8_t inputs[4];  // holder for 8-bit a/d inputs

int8_t timeadj;

script_line curr_line;
uint8_t curr_script_len;
uint8_t curr_script_reps;
uint8_t curr_script_id;   // id number of script being run

volatile uint8_t script_tick;       // incremented by interrupt
script_line* script_addr;  // addr
uint8_t script_pos;        // current position in a sequence

// vars used when parsing either i2c or script playback
uint8_t cmd;
uint8_t cmdargs[8];
    

uint8_t bigIinput = 0xff;
uint8_t bigIval = 0xff;
uint8_t bigIjump;

uint16_t wait_tick;   // big delay

// ctrlm-only vals
uint8_t blinkm_addr = 0x09;   // i2c address of blinkm on freem (0 = all)
uint8_t freem_addr = 0x00;    // "address" of freem (0 = all)

uint8_t ir_freqval = DEFAULT_FREQVAL;  // for timer1, FIXME: use 
uint8_t ir_dutyval = DEFAULT_FREQVAL/3;  // 33% duty cycle

//static uint8_t packet_millis = 5; // time between 4-byte packets
//static uint8_t wait_after = 0;    // 1 = wait packet_millis after sending

#include "IRsend.h"


// function prototypes
static void read_i2c_vals(uint8_t num);
static void handle_script_cmd(void);
static void handle_i2c(void);
static void script_get_next_line_ee(void);
static void script_do_next_line(void);
static void play_script_ee(uint8_t reps);
static void play_script(uint8_t script_id, uint8_t reps, uint8_t fadespeed);
static void handle_script(void);

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

static inline uint8_t compute_checksum(uint8_t* buf, int c)
{
    uint8_t chksum = 0;  // chksum, start at zero
    for( uint8_t i=0; i<c; i++ ) {
        chksum += buf[i]; 
    }
    return chksum;
}
    
// ----------------------------------------------------

// read the specified number of values off i2c bus and into cmdargs array
static void read_i2c_vals(uint8_t num)
{
    for( uint8_t i=0; i<num; i++)
        cmdargs[i] = usiTwiReceiveByte();
}

static void handle_script_cmd(void)
{
    uint8_t wait_cmd = 0;
    int8_t tmp; 
    uint16_t val;
    switch(cmd) {
    case('@'):
        freem_addr  = cmdargs[0];
        blinkm_addr = cmdargs[1];
        break;
    case('#'):     // set IR frequency {'#', freq_msb,freq_lsb,duty_percent}
        val = cmdargs[0] << 8 | cmdargs[1];
        val = freq_to_timer1val( val );
        ir_freqval = val;
        ir_dutyval = (val*100)/cmdargs[2];  
        break;
    case('%'):    // turn IR LED on or off, at pwm freq
        if( cmdargs[0] != 0 ) {
            IRsend_enableIROut();
            IRsend_iron();
        } else {
            IRsend_iroff();
        }
        break;
//  case('&'):     // adjust packet_millis
//      packet_millis = cmdargs[0];
//      wait_after    = cmdargs[1];
//      break;
    case('$'):     // send sony IR code
        val = cmdargs[0] << 8 | cmdargs[1];
        IRsend_sendSony( val, 12);
        break;

        //
        // blinkm cmds
        // 
// new v2 commands
    case('j'):    // jump relative, {'j',-2} -- jump 2 back
        script_pos += cmdargs[0] - 1;
        break;
    case('i'):   // read inputs {'i',0x2,0x80,0xfe} -- jmp -2 if inp#2 > 0x80
        tmp = 0x80;  // val which means 'not set'
        if( cmdargs[0] >= 0x40 ) {  // inputnum 0x40 == inputs on i2c pin SDA
            tmp -= 0x40;            // remove offset, blinkm ins start at 0x40
        }

        if( tmp != 0x80 ) {  
            // if input# value is greater than setpoint, jump 
            if( inputs[ tmp ] > cmdargs[1] ) { 
                script_pos += cmdargs[2] - 1;
            }
        }
        break;        
    case('I'):          // immediate absolute jump based on input: 
        bigIinput =  cmdargs[0] - 0x40;  // blinkm inputs start at 0x40
        bigIval   = cmdargs[1];
        bigIjump  = cmdargs[2];
        break;
    case('w'):        // wait for some number of maxi-ticks?
        wait_tick = cmdargs[0]  + (cmdargs[1] <<8);
        wait_cmd = 1; 
        break;
        /*
    case('s'):       // send sync (i2c): {'s','p',n,p},{'s','o',0,0},
        USICR = 0; // turn off i2c slave
        i2c_init();
        if( cmdargs[0] == 'p' ) {         // play
            i2c_start();
            i2c_write((i2csendaddr<<1));  // FIXME: check return value
            i2c_write('p');  
            i2c_write( cmdargs[1] );       
            i2c_write( 0 );       
            i2c_write( cmdargs[2] );       
            i2c_stop();
        }
        else if( cmdargs[0] == 'o' ) {    // stop script
            i2c_start();
            i2c_write((i2csendaddr<<1));  // FIXME: check return value
            i2c_write('o');  
            i2c_stop();
        }
        else if( cmdargs[0] == 'f' || cmdargs[0] == 't' ) { // set fadespeed
            i2c_start();
            i2c_write((i2csendaddr<<1));  // FIXME: check return value
            i2c_write(cmdargs[0]);  
            i2c_write(cmdargs[1]);  
            i2c_stop();
        }
        else if( cmdargs[0] == 'a' ) {    // set address 
            i2csendaddr = cmdargs[1];
        }
        usiTwiSlaveInit( slaveAddress );  // back to slaving, blinkm
        break;
        */

    case('O'):
        wait_tick = 0;
        curr_script_len = 0; 
        //goto default;


    case('*'):  // play colorspot
        cmdargs[6] = cmdargs[3]; // 0
        cmdargs[5] = cmdargs[2]; // 0  arg?
        cmdargs[4] = cmdargs[1]; // cmd
        cmdargs[3] = cmdargs[0]; // pos
        
        cmdargs[2] = 0xfd;       // 0xfd == play colorspot 
        cmdargs[1] = freem_addr;
        cmdargs[0] = 0x55;       // magic start byte
        
        cmdargs[7] = compute_checksum(cmdargs,7);
            
        IRsend_sendSonyData64bit( cmdargs );
            
        break;

    default: // all other cases, treat as sending blinkm cmd (FIXME?)
        cmdargs[6] = cmdargs[2]; // arg3
        cmdargs[5] = cmdargs[1]; // arg2
        cmdargs[4] = cmdargs[0]; // arg1

        cmdargs[3] = cmd;        // cmd
        cmdargs[2] = blinkm_addr;
        cmdargs[1] = freem_addr;
        cmdargs[0] = 0x55;       // magic start byte

        cmdargs[7] = compute_checksum(cmdargs,7);

        //IRsend_sendSonyData64bit( cmdargs, packet_millis, wait_after );
        IRsend_sendSonyData64bit( cmdargs );
        break;

    } // switch(cmd)

    if( !wait_cmd ) {  // turn off wait_tick on any other command, just in case
        wait_tick = 0;
    }


}

//
// called infinitely in main() along with handle_script()
//
static void handle_i2c(void)
{
    //uint8_t tmp;
    if( usiTwiDataInReceiveBuffer() ) {
        cmd  = usiTwiReceiveByte();
        switch(cmd) {
        case('@'):         // set addr to send to {'@',i2caddr,freemaddr}
        case('#'):         // script cmd: set ir pwm frequency & duty cycle
        case('%'):         // IR light on/off
        case('&'):         // packet_wait_millis, wait_after
            read_i2c_vals(3);  // all these take 3 args
            handle_script_cmd();
            break;
        case('$'):         // script cmd: send ir code
            read_i2c_vals(5);
            if( cmdargs[0] == 0 ) { // FIXME: 0 == sony command type 
                //uint32_t data = *(cmdargs+1);
                IRsend_sendSony( (cmdargs[1]<<8) | cmdargs[2],12 );  // FIXME
            } 
            else if( cmdargs[0] == 1 ) {  // 1 == RC5
                IRsend_sendSony( (cmdargs[1]<<8) | cmdargs[2],12 );  // FIXME
            }
            break;
        case('!'):           // send arbitrary i2c data 
            read_i2c_vals(8);  
            IRsend_sendSonyData64bit( cmdargs );
            //fanfare(3, 100 );
            break;
        case('^'):           // set colorspot {'^', 13, r,g,b }
            read_i2c_vals(4);

            cmdargs[6] = cmdargs[3]; // b
            cmdargs[5] = cmdargs[2]; // g
            cmdargs[4] = cmdargs[1]; // r
            cmdargs[3] = cmdargs[0]; // pos
            
            cmdargs[2] = 0xfe;       // 0xfe == set colorspot 
            cmdargs[1] = freem_addr;
            cmdargs[0] = 0x55;       // magic start byte

            cmdargs[7] = compute_checksum(cmdargs,7);

            IRsend_sendSonyData64bit( cmdargs );

            break;
        case('*'):           // play colorspot {'*', 13, 0, 0 }
            read_i2c_vals(3);
            handle_script_cmd();
            /*
            tmp = blinkm_addr;  // FIXME: bit of a hack here
            blinkm_addr = 0xfd; // 0xfd == play colorspot 
            cmd = cmdargs[0];
            cmdargs[0] = cmdargs[1]; 
            cmdargs[1] = cmdargs[2];
            //cmdargs[2] = cmdargs[3];  // no, only have 3 args to use
            handle_script_cmd();
            blinkm_addr = tmp;
            */
            break;

            // stolen from blinkm.c
        case('a'):         // get address 
            usiTwiTransmitByte( eeprom_read_byte(&ee_i2c_addr) );
            break;
        case('A'):         // set address
            cmdargs[0] = cmdargs[1] = cmdargs[2] = cmdargs[3] = 0;
            read_i2c_vals(4);  // address, 0xD0, 0x0D, address
            if( cmdargs[0] != 0 && cmdargs[0] == cmdargs[3] && 
                cmdargs[1] == 0xD0 && cmdargs[2] == 0x0D ) {  // 
                eeprom_write_byte( &ee_i2c_addr, cmdargs[0] ); // write address
                usiTwiSlaveInit( cmdargs[0] );                 // re-init
                _delay_ms(5);  // wait a bit so the USI can reset
            }
            break;
        case('Z'):        // return protocol version
            usiTwiTransmitByte( BLINKM_PROTOCOL_VERSION_MAJOR );
            usiTwiTransmitByte( BLINKM_PROTOCOL_VERSION_MINOR );
            break;
        case('P'):       // play ctrlm script
            read_i2c_vals(3);
            play_script(0, cmdargs[1], cmdargs[2]);
            break;

        // blinkm cmds
        case('n'):         // script cmd: set rgb color now
        case('c'):         // script cmd: fade to rgb color
        case('C'):         // script cmd: fade to random rgb color
        case('h'):         // script cmd: fade to hsv color
        case('H'):         // script cmd: fade to random hsv color
        case('p'):         // script cmd: play script
            read_i2c_vals(3);
            handle_script_cmd();
            break;
        case('f'):
        case('t'):
            read_i2c_vals(1);
            handle_script_cmd();
        case('o'):
        case('O'):
            handle_script_cmd();
            break;
//
// new v2 commands
//
        case('l'):         // return script len & reps
            read_i2c_vals(1);  // script_id
            if( cmdargs[0] == 0 ) { // eeprom script
                usiTwiTransmitByte( eeprom_read_byte( &ee_script.len ) );
                usiTwiTransmitByte( eeprom_read_byte( &ee_script.reps ) );
            }
            else {
               //script* s=(script*)pgm_read_word(&(fl_scripts[cmdargs[1]-1]));
               //curr_script_len  = pgm_read_byte( (&s->len) );
               //curr_script_reps = pgm_read_byte( (&s->reps) );
            }
        case('i'):         // return current input values
            usiTwiTransmitByte( inputs[0] );
            usiTwiTransmitByte( inputs[1] );
            usiTwiTransmitByte( inputs[2] );
            usiTwiTransmitByte( inputs[3] );
            break;
        
        } // switch(cmd)
        
    } // if(usiTwi)
}

// go to the next line in the script
// called by scirpt_do_next_line() for eeprom-based scripts
static void script_get_next_line_ee(void)
{
    eeprom_read_block( &curr_line, (script_addr + script_pos),
                       sizeof(script_line));
}

// load and execute the next script line
static void script_do_next_line(void)
{
    if( curr_script_id == 0 )
        script_get_next_line_ee();
    //else 
    //    script_get_next_line_fl();

    // FIXME: doesn't do the right thing when wrapping  FIXED: now it does?
    if( timeadj < 0 && curr_line.dur < -timeadj ) {
        curr_line.dur = 0;
    } else { 
        curr_line.dur = curr_line.dur + timeadj;
    }

    cmd        = curr_line.cmd[0];  // FIXME?
    cmdargs[0] = curr_line.cmd[1];
    cmdargs[1] = curr_line.cmd[2];
    cmdargs[2] = curr_line.cmd[3];

    handle_script_cmd();
}

// called by play_script() for eeprom-based scripts
static void play_script_ee(uint8_t reps)
{
    curr_script_len  = eeprom_read_byte( &ee_script.len );
    curr_script_reps = eeprom_read_byte( &ee_script.reps );
    script_addr = ee_script.lines;
}

// this just sets things up, and runs the first line,
// then handle_script() does the rest
static void play_script(uint8_t script_id, uint8_t reps, uint8_t pos)
{
    //wait_tick = 0;              // disable any pending waiting
    script_tick = 0;
    script_pos = pos;
    curr_script_id = script_id;

    if( curr_script_id == 0 )
        play_script_ee(reps);
    //else
    //    play_script_fl(reps);
    
    if( reps != 0 ) // override 
        curr_script_reps = reps;

    script_do_next_line();
}

//
// called infinitely in main() along with handle_i2c()
//
static void handle_script(void)
{
    if( curr_script_len == 0 )            // are we playing?
        return;                           // no, we're not

    if( wait_tick ) {                     // for new 'w'ait ommand
        if( script_tick >= SCRIPT_TICKS_PER_WAIT_TICK ) {  // wait for ticks 
            wait_tick--;                  // since each wait tick is 150 ticks
            script_tick = 0;
        }
        return;
    }

    //if( script_tick > curr_line.dur ) {  // yes! time for a new line?
    if( script_tick >= curr_line.dur ) {  // yes! time for a new line?
        script_tick = 0;                  // yup, reset clock
        script_pos++;                     // and go to next line
        if( script_pos == curr_script_len ) {  // oh wait, we're at the end
            script_pos = 0;               // reset script position
            curr_script_reps--;           // finished a repeat
            if( curr_script_reps == 255 ) {// if this wraps us we're inf reps
                curr_script_reps = 0;     // so reset
                script_do_next_line();
            }
            else if( curr_script_reps == 0 ) // or, no more repeats
                curr_script_len = 0;      // so we're done, turn off script
            else 
                script_do_next_line();    // otherwise, keep going
        }
        else {                            // otherwise, 
            script_do_next_line();        // execute next line
        }
    }
}

// read inputs, act on 'bigI' ('I') command
// every time called, will see if a conversion is done, and if so
// store that value into the inputs[] array.  then it will go to the
// next channel and start another conversion
static void handle_inputs(void)
{
    uint8_t tmp = PINB;             // read all pins
    inputs[0] = (tmp & (1<<PIN_SDA)) ? 255:0; // input0 is digital only
    inputs[1] = (tmp & (1<<PIN_SCL)) ? 255:0; //
    // can't do analog on SCL (the only on an ADC) because of
    // weird interaction of internal pull-ups & USI statemachine

    // handle 'bigI' logic
    if( bigIinput != 0xff && bigIval != 0xff ) { // 0xff means not enabled
        if( inputs[ bigIinput ] > bigIval ) {
            script_pos = bigIjump-1;
            script_tick = 255;  // sigh // FIXME: only works if byte
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

#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || \
      defined(__AVR_ATtiny85__)

    // set up periodic timer for state machine ('script_tick')
    TCCR0B = _BV( CS02 ) | _BV(CS00); // start timer, prescale CLK/1024
    TIFR   = _BV( TOV0 );          // clear interrupt flag
    TIMSK  = _BV( TOIE0 );         // enable overflow interrupt

    // set up output pins   
    PORTB = INPI2C_MASK;          // turn on pullups 
    DDRB  = LED_MASK;             // set LED port pins to output

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || \
      defined(__AVR_ATtiny84__)

    // set up periodic timer for state machine ('script_tick')
    //TCCR0B = _BV( CS02 ) | _BV(CS00); // start timer, prescale CLK/1024
    //TIFR0  = _BV( TOV0 );          // clear interrupt flag
    //TIMSK0 = _BV( TOIE0 );         // enable overflow interrupt
    // set up output pins   
    PORTA = INPI2C_MASK;          // turn on pullups 
    DDRA  = 0xFF; //LEDA_MASK;            // set LED port pins to output
    DDRB  = 0xFF; //LEDB_MASK;            // set LED port pins to output
    
#endif
    
    fanfare( 3, 300 );

#if 0
    // test for ATtiny44/84 MaxM
    fanfare( 3, 300 );

    IRsend_enableIROut();

    while( 1 ) {
        _delay_ms(10);
        IRsend_iroff();
        _delay_ms(10);
        IRsend_iron();
    }

    /*
    uint8_t f = OCR1B;
    while( 1 ) {
        _delay_ms(10);
        f++; 
        if( f== OCR1A ) f=0;  // OCR1A == period
        OCR1B = f;            // OCR1B == duty cycle (0-OCR1A)
    }
    */
#endif

#if 0
    // test timing of script_tick
    _delay_ms(2000);
    sei();
    _delay_ms(500);  // this should cause script_tick to equal 15
    uint8_t j = script_tick;
    for( int i=0; i<j; i++ ) {
        led_flash();
        _delay_ms(300);
    }
#endif

    //////  begin normal startup

    uint8_t boot_mode      = eeprom_read_byte( &ee_boot_mode );
    uint8_t boot_script_id = eeprom_read_byte( &ee_boot_script_id );
    uint8_t boot_reps      = eeprom_read_byte( &ee_boot_reps );
    //uint8_t boot_fadespeed = eeprom_read_byte( &ee_boot_fadespeed );
    uint8_t boot_timeadj   = eeprom_read_byte( &ee_boot_timeadj );

    // initialize i2c interface
    uint8_t i2c_addr = eeprom_read_byte( &ee_i2c_addr );
    if( i2c_addr==0 || i2c_addr>0x7f) i2c_addr = I2C_ADDR;  // just in case

    usiTwiSlaveInit( i2c_addr );

    timeadj    = boot_timeadj;
    if( boot_mode == BOOT_PLAY_SCRIPT ) {
        play_script( boot_script_id, boot_reps, 0 );
    }

    sei();                      // enable interrupts

#if 0
    basic_tests();
#endif

    // This loop runs forever. 
    // If the TWI Transceiver is busy the execution will just 
    // continue doing other operations.
    for(;;) {
        handle_i2c();
        handle_inputs();
        handle_script();
    }
    
} // end


//
// State machine pulse
// updates "script_tick" every 1/30th of a second (
//
ISR(SIG_OVERFLOW0)
{
    script_tick++;
}





// ------------------------------------------------------------------------
// testing code, not used in production
// ------------------------------------------------------------------------


// stolen from http://graphics.stanford.edu/~seander/bithacks.html
// v == value to reverse, s == number of bits in value
// doesn't mask out upper, nonsensicle values
uint8_t reverse8(uint8_t v)
{
  uint8_t r =v;
  //int s = sizeof(v) * CHAR_BIT - 1; // extra shift needed at end
  uint8_t s = 8;
  for (v >>= 1; v; v >>= 1) {   
    r <<= 1;  
    r |= v & 1;
    s--;
  }  
  r <<= s; // shift when v's highest bits are zero
  return r;
}

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

