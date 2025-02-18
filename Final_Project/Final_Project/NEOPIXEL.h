/* Thank you to Egil Rotevatn:
 * For F_CPU = 20 or 10 MHz => 625 kbit/s = ~26k RGB/s or ~20k RGBW/s
 *   zero bit = 25% duty SPI byte = 8bit/(5MHz) * 25% = 400ns high, 1200ns low
 *   one bit  = 50% duty SPI byte = 8bit/(5MHz) * 50% = 800ns high, 800ns low
 * For F_CPU = 16 or 8 MHz => 500 kbit/s = ~21k RGB/s or ~16k RGBW/s
 *   zero bit = 25% duty SPI byte = 8bit/(4MHz) * 25% = 500ns high, 1500ns low (very close to the limit!)
 *   one bit  = 50% duty SPI byte = 8bit/(4MHz) * 50% = 1000ns high, 1000ns low
 */


#ifndef __NEOPIXEL_H__
#define __NEOPIXEL_H__

// based on the neopixel or ws2812 protocol
// use hack to convert mosi pin of spi to correct tx

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HANDLE_INTR		0
#define LED_SPI			SPI0
#define LED_SPI_PORT	PORTA
#define LED_DATA_PIN	PIN4_bm
#define LED_PINCTRL		PIN4CTRL

#define NEOPIXEL_ZERO	((uint8_t) ~0x80)
#define NEOPIXEL_ONE	((uint8_t) ~0xf0)	// originally 0xc0, decreasing it has better results
#define NEOPIXEL_RST	((uint8_t) ~0x00)	

extern uint8_t brightness;

typedef struct __attribute__ ((__packed__)) CRGB {
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};
		uint8_t raw[3];
	};
} CRGB;

extern CRGB COLORS[];

extern CRGB* BLACK;
extern CRGB* WHITE;
extern CRGB* RED;
extern CRGB* GREEN;
extern CRGB* BLUE;
extern CRGB* MAGENTA;
extern CRGB* YELLOW;
extern CRGB* TEAL;

void LED_init(void);
void LED_send_byte(uint8_t byte);
void LED_reset(void);
void LED_fill_color(CRGB* leds, CRGB color, uint16_t num);
void LED_fill_color_rgb(CRGB* leds, uint8_t r, uint8_t g, uint8_t b, uint16_t num);
void LED_set_color(CRGB* leds, CRGB color);
void LED_set_color_rgb(CRGB* leds, uint8_t r, uint8_t g, uint8_t b);
void LED_clear(CRGB* leds, uint16_t num);
void LED_show(CRGB* leds, uint16_t num);
void LED_set_brightness(uint8_t);

#endif