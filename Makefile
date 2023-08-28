all: comp obj firm

comp:
	avr-gcc -mmcu=atmega328p -Os -o main.elf main.c

obj:
	avr-objcopy -O ihex -R .eeprom main.elf main.hex

firm:
	avrdude -c arduino -p atmega328p -P /dev/ttyUSB0 -b 115200 -U flash:w:main.hex

tool:
	sudo apt install gcc-avr avr-libc avrdude