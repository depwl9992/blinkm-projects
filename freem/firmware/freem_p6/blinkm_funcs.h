//-----------------------------------------------------
// blinkm funcs
//

static void blinkm_stopScript(void)
{
    i2c_start();
    i2c_write((i2csendaddr<<1));  // FIXME: check return value
    i2c_write('o');  
    i2c_stop();
}

static void blinkm_sendCmd3( uint8_t c, uint8_t a1, uint8_t a2, uint8_t a3 )
{
    i2c_start();
    i2c_write((i2csendaddr<<1));  // FIXME: check return value
    i2c_write( c );  
    i2c_write( a1 );       
    i2c_write( a2 );       
    i2c_write( a3 );       
    i2c_stop();        
}

static void blinkm_setFadespeed( uint8_t f ) 
{
    i2c_start();
    i2c_write((i2csendaddr<<1));  // FIXME: check return value
    i2c_write('f');
    i2c_write(f);
    i2c_stop();    

}
static void blinkm_fadeToRGB( uint8_t r, uint8_t g, uint8_t b )
{
    blinkm_sendCmd3( 'c', r,g,b );
}

static void blinkm_fadeToHSB( uint8_t h, uint8_t s, uint8_t b )
{
    blinkm_sendCmd3( 'h', h,s,b );
}

static void blinkm_setRGB(  uint8_t r, uint8_t g, uint8_t b )
{
    blinkm_sendCmd3( 'n', r,g,b );
}

static void blinkm_playScript( uint8_t p, uint8_t n )
{
    blinkm_sendCmd3( 'p', p,0,n );
}

static void blinkm_turnOff(void)
{
    blinkm_stopScript();
    blinkm_setRGB( 0,0,0 );
}
