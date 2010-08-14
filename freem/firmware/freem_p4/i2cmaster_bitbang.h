//
// originally from 
//   http://codinglab.blogspot.com/2008/10/i2c-on-avr-using-bit-banging.html
// but highly modified
//

#include <util/delay.h>      

// MUST define 
// I2C_DDR, I2C_PIN, I2C_PORT
// I2C_CLK and I2C_DAT


//#define DELAYUS 50   // should be ~5.0 but whatever
const int i2cbitdelay = 50;

// to enable internal pullups
// when pin is set to input, set pull-up (set PORT to 1)
// when pin is set to output, clr pul-up (set PORT to 0)

#define I2C_DATA_HI()  I2C_DDR  &=~ _BV( I2C_DAT ); I2C_PORT |= _BV( I2C_DAT ); 
#define I2C_DATA_LO()  I2C_PORT &=~ _BV( I2C_DAT ); I2C_DDR  |= _BV( I2C_DAT ); 

#define I2C_CLOCK_HI() I2C_DDR  &=~ _BV( I2C_CLK ); I2C_PORT |= _BV( I2C_CLK );
#define I2C_CLOCK_LO() I2C_PORT &=~ _BV( I2C_CLK ); I2C_DDR  |= _BV( I2C_CLK );

//#define I2C_DATA_HI()  I2C_DDR &=~ _BV( I2C_DAT )
//#define I2C_DATA_LO()  I2C_DDR |=  _BV( I2C_DAT )

//#define I2C_CLOCK_HI() I2C_DDR &=~ _BV( I2C_CLK )
//#define I2C_CLOCK_LO() I2C_DDR |=  _BV( I2C_CLK )

void i2c_writebit( unsigned char c )
{
    if ( c > 0 ) {
        I2C_DATA_HI();
    } else {
        I2C_DATA_LO();
    }

    I2C_CLOCK_HI();
    _delay_us(i2cbitdelay);

    I2C_CLOCK_LO();
    _delay_us(i2cbitdelay);

    if ( c > 0 ) {
        I2C_DATA_LO();
    }
    _delay_us(i2cbitdelay);
}

unsigned char i2c_readbit(void)
{
    I2C_DATA_HI();
    I2C_CLOCK_HI();
    _delay_us(i2cbitdelay);

    unsigned char c = I2C_PIN;

    I2C_CLOCK_LO();
    _delay_us(i2cbitdelay);

    return ( c >> I2C_DAT ) & 1;
}

// Inits bitbanging port, must be called before using the functions below
//
void i2c_init(void)
{
    I2C_PORT &=~ (_BV( I2C_DAT ) | _BV( I2C_CLK ));

    I2C_CLOCK_HI();
    I2C_DATA_HI();

    _delay_us(i2cbitdelay);
}

// Send a START Condition
//
void i2c_start(void)
{
    // set both to high at the same time
    I2C_DDR &=~ (_BV( I2C_DAT ) | _BV( I2C_CLK ));
    _delay_us(i2cbitdelay);
   
    I2C_DATA_LO();
    _delay_us(i2cbitdelay);

    I2C_CLOCK_LO();
    _delay_us(i2cbitdelay);
}

// Send a STOP Condition
//
void i2c_stop(void)
{
    I2C_CLOCK_HI();
    _delay_us(i2cbitdelay);

    I2C_DATA_HI();
    _delay_us(i2cbitdelay);
}

// write a byte to the I2C slave device
//
unsigned char i2c_write( unsigned char c )
{
    for ( char i=0;i<8;i++)
    {
        i2c_writebit( c & 128 );
   
        c<<=1;
    }

    return i2c_readbit();
}


// read a byte from the I2C slave device
//
unsigned char i2c_read( unsigned char ack )
{
    unsigned char res = 0;

    for ( char i=0;i<8;i++)
    {
        res <<= 1;
        res |= i2c_readbit();  
    }

    if ( ack > 0)
        i2c_writebit( 0 );
    else
        i2c_writebit( 1 );

    _delay_us(i2cbitdelay);

    return res;
}
