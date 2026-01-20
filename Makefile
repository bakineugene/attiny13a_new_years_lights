build: main.c
	mkdir -p build
	avr-g++ -I. -Imicro_led/src/ main.c -o ./build/main.elf -mmcu=attiny13a -DF_CPU=9600000UL -Og
	avr-objcopy -O ihex -R .eeprom ./build/main.elf ./build/main.hex
	avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 ./build/main.elf ./build/main.eep

attiny-upload: build
	avrdude -c usbasp -p t13a -U flash:w:"./build/main.hex":a

attiny-set-fuses:
	avrdude -c usbasp -p t13 -U lfuse:w:0x7A:m -U hfuse:w:0xFF:m

attiny-reset-eep:
	avrdude -c usbasp -p t13 -U eeprom:w:"./build/main.eep":i

attiny-dump-eep:
	mkdir -p debug
	avrdude -c usbasp -p t13 -U eeprom:r:"./debug/dump.eep":i

clean:
	rm -rf build
