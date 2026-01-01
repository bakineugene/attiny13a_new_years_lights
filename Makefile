build-avr: main.c
	mkdir -p build
	avr-g++ -I. -Imicro_led/src/ main.c -o ./build/main.elf -mmcu=attiny13a -DF_CPU=8000000UL -Og
	avr-objcopy ./build/main.elf -O ihex ./build/main.hex

upload-avr: build-avr
	avrdude -c usbasp -p t13a -U flash:w:"./build/main.hex":a

clean:
	rm -rf build
