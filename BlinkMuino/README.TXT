BlinkMuino -- Small Arduino on an BlinkM using ATtiny85/ATtiny45
==========
Tod E. Kurt - http://todbot.com/blog/ - http://thingm.com/

BlinkM Smart LEDs can be used as a tiny Arduino or AVR ATtiny85/45 dev board.


Capabilities
------------
- 8MHz clock speed
- 3 high-brightness LEDs: red, green, & blue
- 2 digital I/O lines 
- 1 analog I/O line (shared with digital)
- 8k bytes Flash ROM (4kB on ATtiny45)
- 512 bytes RAM      (256 bytes on ATtiny45)
- 512 bytes EEPROM   (256 bytes on ATtiny45)


Pin out
-------
Description - Arduino pin -  ATtiny85 pin 
-----------   -----------    ------------
red LED         3             PBx
grn LED         4             PBx
blu LED         1             PBx
SDA pin, digital I/O          0             PBx
SCL pin, analog & digital I/O         2             PBx 


Setup
-----
You will need some sort of AVR ISP programmer is needed.
If you have an Arduino, the ArduinoISP might work for you.
Otherwise, you'll need a real AVRISP programmer.


Use
---
1. Create a "hardware" folder in your Arduino sketchbook folder.
You can find the location of your sketchbook by opening the Arduino preferences
and looking at the "Sketchbook location" field.

2. Copy the contents of the "blinkm" folder here into the "hardware" folder.

3. Restart the Arduino software




Links
-----
- Arduino core for ATtiny45/85
-- http://hlt.media.mit.edu/wiki/pmwiki.php?n=Main.ArduinoATtiny4585
- Arduino core for ATtiny45/85
-- http://sites.google.com/site/elettronicaarduinoesperimenti/Home/arduino-ide-and-attiny45
- ArduinoISP
-- http://arduino.cc/en/Tutorial/ArduinoISP
- AVRISP mkII 
-- http://search.digikey.com/scripts/DkSearch/dksus.dll?Cat=2621880&k=avrisp
- USBtinyISP
-- http://www.adafruit.com/index.php?main_page=product_info&cPath=16&products_id=46