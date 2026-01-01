#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define byte uint8_t

#include "tinyLED.h"

#define PCINT0_CLEANUP() GIFR |= (1 << PCIF)
#define PCINT0_ENABLE() GIMSK |= (1 << PCIE)
#define PCINT0_DISABLE() GIMSK &= ~(1 << PCIE)
#define PCINT0_ENABLE_PIN(pin) PCMSK |= (1 << pin)

#define PORTB_SET_OUTPUT(pin) DDRB |= (1 << pin)
#define PORTB_SET_INPUT(pin) DDRB &= ~(1 << pin)
#define PORTB_TOGGLE(pin) PORTB ^= (1 << pin)
#define PORTB_SET_HIGH(pin) PORTB |= (1 << pin)

#define WDT_DISABLE() WDTCR = 0x0
#define WDT_PREPARE_CHANGE() WDTCR = (1 << WDCE)
#define WDT_ENABLE_INTERRUPT_250() WDTCR = (1 << WDTIE) | (1 << WDP2)

ISR(PCINT0_vect) {
    PCINT0_DISABLE();

    // CHANGE SMTH

    cli();
    WDT_PREPARE_CHANGE();
    WDT_ENABLE_INTERRUPT_250();
    sei();
}

ISR(WDT_vect) {
    WDT_DISABLE();
    PCINT0_CLEANUP();
    PCINT0_ENABLE();
}

#define WAVE_LEN 24

const uint8_t wave_r[WAVE_LEN] PROGMEM = {
    0,  5, 15, 30, 60, 100, 150, 200,
  240,255,255,240,220,200,170,140,
  110, 80, 55, 35, 20, 10,  5,  0
};

const uint8_t wave_g[WAVE_LEN] PROGMEM = {
    0,  0,  2,  5, 10,  20,  40,  80,
  120,160,180,160,140,110, 80,  50,
   30, 15,  8,  3,  1,  0,  0,  0
};


int main(void) {
    WDT_DISABLE();
    PCINT0_ENABLE();
    PCINT0_ENABLE_PIN(PCINT4);

    tinyLED<3> led;
    led.setBrightness(200);

    PORTB_SET_INPUT(PB4);
    PORTB_SET_HIGH(PB4);

    sei();

    uint8_t step = 0;
    while (1) {
        for (int l = 0; l < 100; ++l) {
            int idx = (l + step) % WAVE_LEN;
            led.sendRGB(
                pgm_read_byte(&wave_g[idx]),
                pgm_read_byte(&wave_r[idx]),
                0
            );
        }
        if (step >= WAVE_LEN - 1) step = 0;
        else ++step;
        _delay_ms(100);
    }
}

