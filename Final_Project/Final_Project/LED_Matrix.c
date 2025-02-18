#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include "NEOPIXEL.h"
#include "LED_Matrix.h"

/**
 * Graphics library for LED Matrix, allows simple primitives like circles and lines
 * Circles and lines are built off of Bresenham's algorithms
 * Credit to Wikipedia and GeeksForGeeks for docs and resources
 */

uint16_t Matrix_XY(uint16_t x, uint16_t y) {
	// correctly converts Cartesian x,y coords to Matrix indices
	// accounts for Matrix serpentine pattern
	// changing ROTATE param changes how the LED matrix is displayed
	// having this #if #endif sections is probably not the best

	uint16_t idx;
	#if (ROTATE == 0)
		if (y & 0x01) idx = y*COLS + x;
		else idx = (y*COLS + COLS - 1 - x);
	#elif (ROTATE == -90)
		if (x & 0x01) idx = x*ROWS + y;
		else idx = (x*ROWS + COLS - 1 - y);
	#elif (ROTATE == 90) 
		if (x & 0x01) idx = (x*ROWS + COLS - 1 - y);
		else idx = x*ROWS + y;
	#else 
		if (y & 0x01) idx = (y*COLS + COLS - 1 - x);
		else idx = y*COLS + x;
	#endif
	
	return idx;
}

void Matrix_set_pixel(CRGB* leds, uint16_t x, uint16_t y, CRGB color) {
	// dedicated function for setting Matrix pixel
	// skips x, y if they are out of bounds
	if (x < COLS && y < ROWS ) {
		uint16_t idx = Matrix_XY(x, y);
		leds[idx].r = color.r;
		leds[idx].b = color.b;
		leds[idx].g = color.g;
	}
}

void Matrix_line(CRGB* leds, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, CRGB color) {
	// Bresenham's line algorithm from GeeksForGeeks/Wikipedia
	// possibly upgrade with anti-aliasing?
	
	int dx = abs(x1-x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1-y0);
	int sy = y0 < y1 ? 1 : -1;
	int error = dx + dy;
	
	while (1) {
		if (x0 < 0 || y0 < 0 || x0 > COLS || y0 > ROWS) continue;
		
		Matrix_set_pixel(leds, x0, y0, color);
				
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * error;
		if (e2 >= dy) {
			error += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			error += dx;
			y0 += sy;
		}
	}
}

void circle_helper(CRGB* leds, uint16_t x0, uint16_t y0, int x, int y, CRGB color) {
	// helper function for creating circles
	// sets all the symmetric pixels to color

	Matrix_set_pixel(leds, x0+x, y0+y, color);
	Matrix_set_pixel(leds, x0+x, y0-y, color);
	Matrix_set_pixel(leds, x0-x, y0+y, color);
	Matrix_set_pixel(leds, x0-x, y0-y, color);
	Matrix_set_pixel(leds, x0+y, y0+x, color);
	Matrix_set_pixel(leds, x0+y, y0-x, color);
	Matrix_set_pixel(leds, x0-y, y0+x, color);
	Matrix_set_pixel(leds, x0-y, y0-x, color);
	
}

void Matrix_circle(CRGB* leds, uint16_t x0, uint16_t y0, int16_t r, CRGB color) {
	// Bresenham's midpoint circle algorithm from GeeksForGeeks/Wikipedia
	// creates a circle with radius r centered at x0, y0

	int x = 0;
	int y = r;
	int d = 3 - (r << 1);
	if (r < 0) return;
	circle_helper(leds, x0, y0, x, y, color);
	while (y >= x) {
		if (d > 0) {
			y--;
			d = d + 4*(x-y) + 10;
		} else {
			d = d + 4*x + 6;
		}
		x++;
		circle_helper(leds, x0, y0, x, y, color);
	}
}

void Matrix_triangle(CRGB* leds, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, CRGB color) {
	// three line segments a triangle makes
	// creates a triangle using 3 points

	Matrix_line(leds, x0, y0, x1, y1, color);
	Matrix_line(leds, x1, y1, x2, y2, color);
	Matrix_line(leds, x2, y2, x0, y0, color);
}