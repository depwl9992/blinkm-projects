###############################################################################
# Makefile for the project avr5g3_qt_example
###############################################################################

## General Flags
TARGET = freem_v1a

#MCU = attiny45
MCU = attiny85
#F_CPU = 16000000   # 16MHz on Arduino board
F_CPU = 8000000    # internal osc
#F_CPU = 1000000    # internal osc w/ clkdiv8
#F_CPU  =  500000    # internal osc w/ clkdiv16
#F_CPU =  250000    # internal osc w/ clkdiv32

LFUSE=0xE2
HFUSE=0xDD
EFUSE=0xFE


CC = avr-gcc
CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99 
CFLAGS += -Wno-unused-function
#CFLAGS += -DQT_NUM_CHANNELS=4 -DQT_DELAY_CYCLES=1 -D_QTOUCH_ -DSNSK=B  -DSNS=B
CFLAGS += -DF_CPU=$(F_CPU)
CFLAGS += -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
#CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 
#CFLAGS += -MD -MP -MT $(*F).o 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS +=  -Wl,-Map=$(TARGET).map


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Library Directories
LIBDIRS = -L.

## Libraries
# this is qtouch lib for atmega168
#LIBS = -lavr5g3-4qt-k-0rs   
# this is qtouch lib for attiny45 (85)
# LIBS = -lavr25g1-4qt-k-0rs

## Objects that must be built in order to link
OBJECTS = $(TARGET).o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 


# Programming support using avrdude. Settings and variables.
AVRDUDE = avrdude
AVRDUDE_PROGRAMMER = avrispmkII
AVRDUDE_PORT = usb # for AVR-ISP mkII

AVRDUDE_WRITE_FLASH = -e -U flash:w:$(TARGET).hex
AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep

AVRDUDE_VERBOSE = -v -v

AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)
AVRDUDE_FLAGS += $(AVRDUDE_NO_VERIFY)
AVRDUDE_FLAGS += $(AVRDUDE_VERBOSE)
AVRDUDE_FLAGS += $(AVRDUDE_ERASE_COUNTER)


PKGDATE = `date +%Y%m%d`

## Build
all: $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).lss size

## Compile
%.o : %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
%.elf: $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET).elf

%.hex: $(TARGET).elf
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET).elf
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET).elf
	avr-objdump -h -S $< > $@

size: ${TARGET}.elf
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}.elf

packaging:
	echo "making $(TARGET)_$(PKGDATE)"
	mkdir -p $(TARGET)_$(PKGDATE)
	cp -f $(TARGET).hex $(TARGET)_$(PKGDATE)/$(TARGET)_$(PKGDATE).hex
	cp -f $(TARGET).eep $(TARGET)_$(PKGDATE)/$(TARGET)_$(PKGDATE).eep
	cp -f $(TARGET)_fuse.txt $(TARGET)_$(PKGDATE)/$(TARGET)_fuse_$(PKGDATE).txt
	zip -r $(TARGET)_$(PKGDATE).zip $(TARGET)_$(PKGDATE)

# Program the device.  
program: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)

erase:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -e -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

eeprom_read:
	$(AVRDUDE) $(AVRDUDE_FLAGS) -U eeprom:r:-:i

## Clean target
#.PHONY: clean
clean:
	-rm -rf $(OBJECTS) $(TARGET).elf $(TARGET).hex $(TARGET).eep $(TARGET).lss $(TARGET).map 


## Other dependencies
#-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

