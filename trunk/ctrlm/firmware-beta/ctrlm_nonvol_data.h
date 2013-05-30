/*
 * BlinkM non-volatile data
 *
 */

// comment out to remove ROM scripts
//#define INCLUDE_ROM_SCRIPTS

// define in makefile so each gets their own
#ifndef I2C_ADDR
#define I2C_ADDR 0x09
#endif

// number of of script ticks per 'w'ait command
//  30.52 script ticks == 1 second
//  153   script ticks == 5.013 seconds
//  255   script ticks == 8.35 seconds
//#define SCRIPT_TICKS_PER_WAIT_TICK 153
#define SCRIPT_TICKS_PER_WAIT_TICK 153


// possible values for boot_mode
#define BOOT_NOTHING     0
#define BOOT_PLAY_SCRIPT 1
#define BOOT_MODE_END    2

#define MAX_EE_SCRIPT_LEN 49

typedef struct _script_line {
    uint8_t dur; 
    uint8_t cmd[4];    // cmd,arg1,arg2,arg3
} script_line;

typedef struct _script {
    uint8_t len;  // number of script lines, 0 == blank script, not playing
    uint8_t reps; // number of times to repeat, 0 == infinite playes
    script_line lines[];
} script;


// eeprom begin: muncha buncha eeprom
uint8_t  ee_i2c_addr         EEMEM = I2C_ADDR;
uint8_t  ee_boot_mode        EEMEM = BOOT_NOTHING;
//uint8_t  ee_boot_mode        EEMEM = BOOT_PLAY_SCRIPT;
uint8_t  ee_boot_script_id   EEMEM = 0x00;
uint8_t  ee_boot_reps        EEMEM = 0x00;
uint8_t  ee_boot_fadespeed   EEMEM = 0x08;
uint8_t  ee_boot_timeadj     EEMEM = 0x00;
uint8_t  ee_unused2          EEMEM = 0xDA;

/*
script ee_script  EEMEM = {
    7, // number of seq_lines
    0, // number of repeats, also acts as boot repeats?
    {  // dur, cmd,  arg1,arg2,arg3
        {  0, {'@', 0x00,0x00,0x00}}, // address all blinkms on all freems
        {  0, {'f', 0x22,0x33,0x44}},
        {  0, {'o', 0x00,0x00,0x00}},
        {  0, {'@', 0x00,0x01,0x00}}, // freem addr 1
        {  0, {'c', 0x33,0x66,0x00}},
        {  0, {'@', 0x00,0x02,0x00}}, // freem addr 2
        { 10, {'c', 0x00,0x66,0x66}},
    }
};
*/


script ee_script  EEMEM = {
    6, // number of seq_lines
    0, // number of repeats, also acts as boot repeats?
    {  // dur, cmd,  arg1,arg2,arg3
        {  0, {'f', 0x22,0x33,0x44}},
        {  0, {'o', 0x00,0x00,0x00}},
        { 10, {'c', 0x33,0x66,0x00}},
        { 10, {'c', 0x11,0x11,0x11}},
        { 10, {'c', 0x00,0x66,0x33}},
        { 10, {'c', 0xff,0xff,0xff}},
    }
};

/*
script ee_script  EEMEM = {
    6, // number of seq_lines
    0, // number of repeats, also acts as boot repeats?
    {  // dur, cmd,  arg1,arg2,arg3
        { 1, {'f', 0x11,0x22,0x33}},
        {10, {'c', 0x11,0x11,0x11}},
        {10, {'c', 0x01,0x01,0x01}},
        {10, {'c', 0x11,0x11,0x11}},
        {10, {'c', 0x01,0x01,0x01}},
        {10, {'c', 0x33,0x33,0x33}},
    }
};
*/
/*
script ee_script  EEMEM = {
    6, // number of seq_lines
    0, // number of repeats, also acts as boot repeats?
    {  // dur, cmd,  arg1,arg2,arg3
        {  1, {'f',   10,0x00,0x00}}, // set color_step (fade speed) to 15
        {100, {'c', 0xff,0xff,0xff}},
        { 50, {'c', 0xff,0x00,0x00}},
        { 50, {'c', 0x00,0xff,0x00}},
        { 50, {'c', 0x00,0x00,0xff}},
        { 50, {'c', 0x00,0x00,0x00}}
    }
};
*/

// eeprom end
