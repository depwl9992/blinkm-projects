BlinkMuino -- Small Arduino on an BlinkM using ATtiny85/ATtiny45/ATtiny84
==========
2011-2013 - Tod E. Kurt - http://todbot.com/blog/ - http://thingm.com/

BlinkM and BlinkM MinM Smart LEDs can be used as a tiny Arduino.
BlinkMs can also be used as a general AVR ATtiny85 Arduino dev board.
This project is based on the efforts of the High-Low Tech group at MIT:
http://hlt.media.mit.edu/?p=1695


BlinkM Capabilities
--------------------
- 0.4" square (MinM), or 0.62" square (BlinkM)
- 8MHz clock speed
- 3 high-brightness LEDs: red, green, & blue
- 2 digital I/O lines 
- 1 analog I/O line  (shared with digital)
- 8k bytes Flash ROM 
- 512 bytes RAM      
- 512 bytes EEPROM   


Pin out
-------
Description - Arduino pin -  ATtiny85 pin 
-----------   -----------    ------------
red LED          3              PB3
grn LED          4              PB4
blu LED          1              PB1
SDA pin,         0              PB0
SCL pin,         2              PB2 


Hardware Setup
--------------
You will need an AVR-ISP programmer to program the BlinkM boards.
These are fairly low-cost: the official AVRISPmkII from Atmel 
is 35 USD, while the USBtinyISP is 22 USD.

If you have an Arduino board, the ArduinoISP sketch turns the Arduino 
into an AVR-ISP programmer.  For more details on it, see:
http://arduino.cc/en/Tutorial/ArduinoISP

With a programmer acquired, wire up the BlinkM to it.  If using AruinoISP, 
see the diagrams and photos here:
  http://code.google.com/p/blinkm-projects/wiki/ReflashBlinkM
If using a regular AVR-ISP programmer, see the wiring diagrams here:
  http://code.google.com/p/blinkm-projects/wiki/BlinkMuino


Installation
-------------
0. Find your Arduino sketchbook folder.
   The loccation of your sketchbook is shown in the the Arduino preferences
   page at the "Sketchbook location" field.

1. Go to your Arduino sketchbook folder and create a folder called "hardware".
   This folder might already exist.

2. Copy the "blinkm" folder here into the "hardware" folder.  
   The end result should be: Arduino/hardware/blinkm

3. Restart the Arduino software.

4. The Tools -> Board menu should now have entries for BlinkMs. 
   Select the entry that is correct for your BlinkM type.

5. In the Tools -> Programmers menu, select your type of AVR-ISP programmer.

6. If you're using ArdiunoISP, set the Tools -> Serial Port to your Arduino.


Use
---
Open up one of the example sketches in the "examples" directory, say the
"BlinkMuinoBlink3" sketch.  Click the Run button to test compile it.

Make sure your BlinkM is connected to the programmer and click "Upload".
If you have problems, press the Shift key while clicking Upload to see verbose
messages.



Links
-----
- ReflashBlinkM
-- http://code.google.com/p/blinkm-projects/wiki/ReflashBlinkM

- Programming an ATtiny w/ Arduino 1.0
-- http://hlt.media.mit.edu/?p=1695

- ArduinoISP
-- http://arduino.cc/en/Tutorial/ArduinoISP

- AVRISP mkII 
-- http://search.digikey.com/scripts/DkSearch/dksus.dll?Cat=2621880&k=avrisp

- USBtinyISP
-- http://www.adafruit.com/index.php?main_page=product_info&cPath=16&products_id=46

- BlinkM datasheet
-- http://thingm.com/fileadmin/thingm/downloads/BlinkM_datasheet.pdf
