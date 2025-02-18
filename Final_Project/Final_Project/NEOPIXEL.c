#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include "NEOPIXEL.h"

/**
 * Color and Neopixel/WS2812B library
 * Used to communicate with the LED Matrix, send out data
 * Uses a hack of the SPI MOSI pin to transmit data
 * WS2812B protocol is T0H,T0L and T1H,T1L for each bit
 * Convert that to SPI byte with corresponding 1s and 0s
 * 
 * Inspired by the FastLED library for Arduino
 * Credit to Egil Rotevatn on GitLabs for inspiration and assistance
 * https://gitlab.com/printedcircuitbirds/libavr_neopixel_spi
 * 
 * 
 * TODO:
 * - further abstract Matrix params like NUM_LEDS and leds
 * - more customizability with PORTMUX rerouting
 */

// Global brightness variable
uint8_t brightness = 255;

CRGB COLORS[] = {
	{0x00,0x00,0x00},	// BLACK 
	{0xff,0xff,0xff},	// WHITE 
	{0xff,0x00,0x00},	// RED	
	{0x00,0xff,0x00},	// GREEN 
	{0x00,0x00,0xff},	// BLUE
	{0xff,0x00,0xff},	// MAGENTA
	{0xff,0xff,0x00},	// YELLOW
	{0x00,0xff,0xff}	// TEAL 
};

#define NUM_COLORS (sizeof(COLORS)/sizeof(CRGB))

CRGB* BLACK =	&(COLORS[0]);
CRGB* WHITE =	&(COLORS[1]);
CRGB* RED =		&(COLORS[2]);
CRGB* GREEN =	&(COLORS[3]);
CRGB* BLUE =	&(COLORS[4]);
CRGB* MAGENTA =	&(COLORS[5]);
CRGB* YELLOW =	&(COLORS[6]);
CRGB* TEAL =	&(COLORS[7]);

void LED_init(void) {
	LED_SPI.CTRLB = SPI_BUFEN_bm | SPI_SSD_bm | SPI_MODE_0_gc;
	LED_SPI.CTRLA = SPI_MASTER_bm | SPI_PRESC_DIV4_gc | SPI_ENABLE_bm;
	LED_SPI_PORT.OUTSET = LED_DATA_PIN;
	LED_SPI_PORT.LED_PINCTRL = PORT_INVEN_bm;
	LED_SPI_PORT.DIRSET = LED_DATA_PIN;
}

void LED_send_byte(uint8_t byte) {
	// Send NEOPIXEL byte through the SPI MOSI pin
	for (uint8_t i = 0; i < 8; i++)
	{
		// iterate through each bit of the byte
		// wait until data register empty
		while(!(LED_SPI.INTFLAGS & SPI_DREIF_bm)) {}
		// set SPI data to NEOPIXEL 1 or 0 depending on bit
		if (byte & 0x80) LED_SPI.DATA = NEOPIXEL_ONE;
		else LED_SPI.DATA = NEOPIXEL_ZERO;
		byte <<= 1;
	}

}

// unused
void LED_reset(void) {
	for (int i = 0; i < 7; i++) {
		(void)LED_send_byte(NEOPIXEL_RST);
	}
}

void LED_fill_color(CRGB* leds, CRGB color, uint16_t num) {
	for (uint8_t i = 0; i < num; i++) {
		leds[i].r = color.r;
		leds[i].g = color.g;
		leds[i].b = color.b;
	}
}

void LED_fill_color_rgb(CRGB* leds, uint8_t r, uint8_t g, uint8_t b, uint16_t num) {
	for (uint8_t i = 0; i < num; i++) {
		leds[i].r = r;
		leds[i].g = g;
		leds[i].b = b;
	}
}

// unused
void LED_set_color(CRGB* led, CRGB color) {
	*led = color;
}

// unused, originally would be used with HSV coloring scheme
void LED_set_color_rgb(CRGB* led, uint8_t r, uint8_t g, uint8_t b) {
	led->r = r;
	led->g = g;
	led->b = b;
}

void LED_clear(CRGB* leds, uint16_t num) {
	// 
	//LED_fill_color(leds, BLACK, num);
	LED_reset();
}

void LED_show(CRGB* leds, uint16_t num) {
	// Outputs leds buffer to the LED Matrix, allows display
	// Must be called to turn on LED Matrix
	// send: rst --> g --> r --> b --> rst
	
	uint8_t buf_r;
	uint8_t buf_g;
	uint8_t buf_b;
	
	// Have the function handle interrupts automatically
	#if (HANDLE_INTR)
		cli();
	#endif
	
	for (uint16_t i = 0; i < num; i++) {
		// compute brightness (check to see if this is not too time intensive)
		buf_r = ((uint16_t)(brightness * leds[i].r + 1) >> 8) % 256;
		buf_g = ((uint16_t)(brightness * leds[i].g + 1) >> 8) % 256;
		buf_b = ((uint16_t)(brightness * leds[i].b + 1) >> 8) % 256;
		// send data, LEDs ordered GRB
		LED_send_byte(buf_g);
		LED_send_byte(buf_r);
		LED_send_byte(buf_b);
	}
	
	#if (HANDLE_INTR)
		sei();
	#endif
	
}

void LED_set_brightness(uint8_t amnt) {
	// Setting function, sets the global LED Matrix brightness to amnt
	brightness = amnt;
}

