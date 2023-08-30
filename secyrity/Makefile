all: compile to_hex upload

compile:
	avr-gcc -mmcu=atmega328p -Os -o main.elf main.c

to_hex:
	avr-objcopy -O ihex -R .eeprom main.elf main.hex

upload:
	avrdude -c arduino -p atmega328p -P /dev/ttyUSB0 -b 115200 -U flash:w:main.hex

install_tools:
	sudo apt install gcc-avr avr-libc avrdude