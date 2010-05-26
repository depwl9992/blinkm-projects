
#include <avr/io.h>
#include <util/delay.h>


// edit these to match your implementation
#ifndef _baudRate
#error "_baudRate not set!"
//#define _baudRate  2400
//#define _baudRate  19200
//#define _baudRate  38400
//#define _baudRate  57600
#endif

#define UART_TXPORT PORTB
#define UART_TXDDR  DDRB
#define UART_TXPIN  PB1

//

#define _bitPeriod (1000000L / _baudRate)  // convert bps rate to usec period
//#define _bitPeriod (104)  // 9600 bps

//
static void softuart_init()
{
    UART_TXDDR |= _BV(UART_TXPIN) ;  // set to output
}

//
static void softuart_putc(uint8_t b)
{
    if (_baudRate == 0)  return;

    uint8_t mask;
    cli();
    
    UART_TXPORT &=~ _BV(UART_TXPIN);     // low for start bit
    _delay_us(_bitPeriod);  
    
    for (mask = 0x01; mask; mask <<= 1) {    // go from lsb to msb
        if (b & mask) { // choose bit
            UART_TXPORT |=  _BV(UART_TXPIN);   // send 1
        } else {
            UART_TXPORT &=~ _BV(UART_TXPIN);   // send 0
        }
        _delay_us(_bitPeriod);
    }

    //UART_TXPORT &=~ _BV(UART_TXPIN);   // send 0 for stop bit
    //_delay_us(_bitPeriod); 
    
    UART_TXPORT |=  _BV(UART_TXPIN);   // high for  stop bit?
    _delay_us(_bitPeriod);
    
    sei();
}

static void softuart_puts(char* s)
{
   while( *s ) {
      softuart_putc( *s++ );
   }
}
// convert nibble to ascii
static uint8_t softuart_hexAscii(uint8_t h)
{
    h &= 0xf;
    if(h >= 10)  h += 'a' - 10 - '0';  // convert a-f
    h += '0';
    return h;
}

// print byte as two ascii hex digits
static void softuart_printHex(uint8_t c)
{
    softuart_putc(softuart_hexAscii(c >> 4));
    softuart_putc(softuart_hexAscii(c));
}

static void softuart_printHex16(uint16_t c)
{

    softuart_printHex( (uint8_t)(c>>8) );
    softuart_printHex( (uint8_t)(c & 0xff) );
}
