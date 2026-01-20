#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
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

#define PINB_GET(pin) PINB & (1 << pin)

#define WDT_DISABLE() WDTCR = 0x0
#define WDT_PREPARE_CHANGE() WDTCR = (1 << WDCE)
#define WDT_ENABLE_INTERRUPT_16() WDTCR =  (1 << WDTIE)
#define WDT_ENABLE_INTERRUPT_32() WDTCR =  (1 << WDTIE) | (1 << WDP0)
#define WDT_ENABLE_INTERRUPT_64() WDTCR =  (1 << WDTIE) | (1 << WDP1)
#define WDT_ENABLE_INTERRUPT_125() WDTCR = (1 << WDTIE) | (1 << WDP0) | (1 << WDP1)
#define WDT_ENABLE_INTERRUPT_250() WDTCR = (1 << WDTIE) | (1 << WDP2)

#define UNDEFINED 0xFF

#define CH_0 0xFF
#define CH_R 0
#define CH_G 1
#define CH_B 2

#define WAVE_LEN 24
#define MODE_COUNT 9

#define EE_SIZE 64

uint8_t EEMEM ee_values[EE_SIZE] = {
    UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
    UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
    UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
    UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
    UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
    UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
    UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED,
    UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED, UNDEFINED
};

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
volatile uint8_t cell_idx = 0;
volatile uint8_t remembered_mode = 0;
volatile uint8_t button_tick_counter = 0;

/*
 * > 800ms
 */
#define LONG_PRESS 50

/*
 * > 50ms
 */
#define SINGLE_PRESS 3

ISR(PCINT0_vect) {
    if (PINB_GET(PB4)) {
        if (button_tick_counter > LONG_PRESS) {
            uint8_t previous_cell_idx = cell_idx;
            if (++cell_idx >= EE_SIZE) cell_idx = 0;
            eeprom_update_byte(&ee_values[previous_cell_idx], UNDEFINED);
            eeprom_update_byte(&ee_values[cell_idx], mode_num);
            remembered_mode = MODE_COUNT;
        } else if (button_tick_counter > SINGLE_PRESS) {
            if (++mode_num >= MODE_COUNT) mode_num = 0;
        }
    }
    /*
     * button is pressed or unpressed - reset counter
     */
    button_tick_counter = 0;
}

ISR(WDT_vect) {
    ++button_tick_counter;
}

int main(void) {
    _delay_ms(100);

    PORTB_SET_INPUT(PB4);
    PORTB_SET_HIGH(PB4);

    {
        for (int i = 0; i < EE_SIZE; ++i) {
            uint8_t value = eeprom_read_byte(&ee_values[i]);
            if (value != UNDEFINED) {
                cell_idx = i;
                mode_num = value;
            }
        }
        if (mode_num >= MODE_COUNT) mode_num = 0;
    }

    button_tick_counter = 0;

    tinyLED<3> led;
    led.setBrightness(200);

    WDT_PREPARE_CHANGE();
    WDT_ENABLE_INTERRUPT_16();
    PCINT0_ENABLE();
    PCINT0_ENABLE_PIN(PCINT4);
    sei();

    uint8_t step = 0;
    while (1) {
        Mode mode;
        uint8_t current_mode = mode_num;
        if (remembered_mode > 0) current_mode = --remembered_mode;
        pgm_read_block(&modes[current_mode], (void*)&mode, sizeof(Mode));

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

