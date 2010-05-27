FreeM Kit: aka "BlinkM Battery Remote"
======================================

Documentation is kind of sparse at the moment.
So this kit is aimed at more advanced users right now.

The firmware "freem_a2" is the one you should use.
Tune the Makefile for your programmer if you're not using an AVRISP mkII.
Then type "make program" to compile and program your chip.

From the header of the firmware, these are the basic features:
 * Remote control functions:  (Sony TV remotes only right now)
 * - POWER     - stop script & turn off blinkm
 * - CH  UP/DN - solid hue mode, cycle hue if held down
 * - VOL UP/DN - brightness up/down
 * - ENTER     -
 * - MUTE      - 
 * - Num keys  - do something interesting
 *   1 - random slowly changing mood light (calc'd in FreeM)
 *   2 - fast random colors
 *   3 - flashing beacon, use ch up/dn to change hue, press 3 again to start
 *   4 - yellow
 *   5 - aqua
 *   6 - purple
 *   7 - dim white
 *   8 - med white
 *   9 - bright white
 *   0 - play script 0 on BlinkM
 * 
 * Data Mode:
 * - Sending keycode of 0x07F7 escapes into data mode
 * - Sending keycode of 0x07F8 escapes out of data mode
 * - data format is: len,byte_0,byte_1,byte_2,...byte_len-1
 * - if len==4, then command is sent as a straight 3-arg BlinkM command,
 *   e.g. send binary 0x7F7,'c',0xFF,0x00,0xFF to fade BlinkM to purple
*  - data mode is still buggy and aggravating, but maybe interesting
 *
 * "BlinkM Battery Remote" - battir3 pcb layout
 * ---------------------------------------------
 * attiny45       -- function
 * pin 1 -- PB5   -- RESET
 * pin 2 -- PB3   -- PHOTOCELL  (unused)
 * pin 3 -- PB4   -- IRDETECT
 * pin 4 -- GND   -- GND
 * pin 5 -- PB0   -- SDA/MOSI    -- 
 * pin 6 -- PB1   -- MISO        -- LED1 & serial TX out
 * pin 7 -- PB2   -- SCK/SCL     -- 
 * pin 8 -- VCC   -- VCC






Contact tod at thingm.com if you have any questions.

