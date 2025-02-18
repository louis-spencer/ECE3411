#ifndef __LED_MATRIX__
#define __LED_MATRIX__

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define ROWS 16
#define COLS 16
#define ROTATE (-90)
#define FLIPPED 1

#define XY(x,y) (y*COLS+x)

uint16_t Matrix_XY(uint16_t x, uint16_t y);
void Matrix_line(CRGB* leds, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, CRGB color);
void Matrix_triangle(CRGB* leds, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, CRGB color);
static void circle_helper(CRGB* leds, uint16_t x0, uint16_t y0, int x, int y, CRGB color);
void Matrix_circle(CRGB* leds, uint16_t x0, uint16_t y0, int16_t r, CRGB color);

#endif