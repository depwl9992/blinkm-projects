#include <avr/io.h>             // this contains all the IO port definitions
#include <avr/interrupt.h>      // definitions for interrupts
#include <avr/pgmspace.h>
//#include "main.h"
//#include "util.h"

#define TXPORT PORTB
#define TX PB1

#define NOP __asm__ __volatile__ ("nop")
//#if (DEBUG == 1)

#define SERIALDELAY 57  // to make 19200 baud  @ 8MHz
void serialdelay(void) {
  uint16_t timer;
  for (timer=0; timer <= SERIALDELAY; timer++) {
    NOP;
  }
}

// nothing needed
#define softuart_init()

void softuart_putchar(char d) {
  int i;
  cli();  // turn off interrupts, make it nice & kleen

  TXPORT &= ~_BV(TX);
  serialdelay();
  for (i=0; i< 8; i++) {
    if (d & 0x1) {
      TXPORT |= _BV(TX);
    } else {
      TXPORT &= ~_BV(TX);
    }
    serialdelay();
    d >>= 1;
  }
  TXPORT |= _BV(TX);
  sei();   // turn on interrupts
  serialdelay();
} 
#define softuart_putc(x) softuart_putchar(x)


void printhex(uint8_t hex) {
  hex &= 0xF;
  if (hex < 10)
    softuart_putchar(hex + '0');
  else
    softuart_putchar(hex + 'A' - 10);
}
#define softuart_printHex(x) printhex(x)

void putnum_uh(uint16_t n) {
  if (n >> 12)
    printhex(n>>12);
  if (n >> 8)
    printhex(n >> 8);
  if (n >> 4)
    printhex(n >> 4);
  printhex(n);

  return;
}
#define softuart_printHex16(x) putnum_uh(x)


void putnum_ud(uint16_t n) {
	uint8_t cnt=0, flag=0;
	
	while (n >= 10000UL) { flag = 1; cnt++; n -= 10000UL; }
	if (flag) softuart_putchar('0'+cnt); 
	cnt = 0;
	while (n >= 1000UL) { flag = 1; cnt++; n -= 1000UL; }
	if (flag) softuart_putchar('0'+cnt); 
	cnt = 0;
	while (n >= 100UL) { flag = 1; cnt++; n -= 100UL; }
	if (flag) softuart_putchar('0'+cnt); 
	cnt = 0;
	while (n >= 10UL) { flag = 1; cnt++; n -= 10UL; }
	if (flag) softuart_putchar('0'+cnt); 
	cnt = 0;
	softuart_putchar('0'+n); 
	return;
}

void ROM_putstring(const char *str, uint8_t nl) {
	uint8_t i;

	for (i=0; pgm_read_byte(&str[i]); i++) {
		softuart_putchar(pgm_read_byte(&str[i]));
  }
  if (nl) {
		softuart_putchar('\n'); softuart_putchar('\r');
  }
}

//#endif
