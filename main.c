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

#define CH_0 0xFF
#define CH_R 0
#define CH_G 1
#define CH_B 2

#define WAVE_LEN 24
#define MODE_COUNT 9

const uint8_t wave_hard[WAVE_LEN] PROGMEM = {
    0,  5, 15, 30, 60, 100, 150, 200,
  240,255,255,240,220,200,170,140,
  110, 80, 55, 35, 20, 10,  5,  0
};

const uint8_t wave_soft[WAVE_LEN] PROGMEM = {
    0,  0,  2,  5, 10,  20,  40,  80,
  120,160,180,160,140,110, 80,  50,
   30, 15,  8,  3,  1,  0,  0,  0
};

static inline void pgm_read_block(const void *s, void *dest, uint8_t len) {
    uint8_t *dp = (uint8_t *)dest;
    for (uint8_t i=0; i<len; i++) {
        dp[i] = pgm_read_byte(i + (const uint8_t *)s);
    }
}

typedef struct Mode {
    uint8_t soft;
    uint8_t hard;
} Mode;

const Mode modes[MODE_COUNT] PROGMEM = {
    { CH_G, CH_0 },
    { CH_R, CH_0 },
    { CH_B, CH_0 },
    { CH_G, CH_R },
    { CH_R, CH_B },
    { CH_B, CH_R },
    { CH_G, CH_B },
    { CH_R, CH_G },
    { CH_B, CH_G }
};

volatile uint8_t mode_num = 0;

ISR(PCINT0_vect) {
    PCINT0_DISABLE();

    if (++mode_num >= MODE_COUNT) mode_num = 0;

    WDT_PREPARE_CHANGE();
    WDT_ENABLE_INTERRUPT_250();
}

ISR(WDT_vect) {
    WDT_DISABLE();
    PCINT0_CLEANUP();
    PCINT0_ENABLE();
}

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
        Mode mode;
        pgm_read_block(&modes[mode_num], (void*)&mode, sizeof(Mode));

        for (int l = 0; l < 100; ++l) {
            int idx = (l + step) % WAVE_LEN;

            uint8_t color[3] = {0, 0, 0};
            if (mode.soft != CH_0) color[mode.soft] = pgm_read_byte(&wave_soft[idx]);
            if (mode.hard != CH_0) color[mode.hard] = pgm_read_byte(&wave_hard[idx]);

            led.sendRGB(color[CH_R], color[CH_G], color[CH_B]);
        }
        if (step >= WAVE_LEN - 1) step = 0;
        else ++step;
        _delay_ms(100);
    }
}

