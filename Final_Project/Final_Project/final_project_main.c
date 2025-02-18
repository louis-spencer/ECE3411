#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint-gcc.h>
#include <limits.h>
#include <math.h>
#include <util/delay.h>
#include "i2c.h"
#include "uart.h"
#include "LIS331.h"	
#include "NEOPIXEL.h"
#include "LED_Matrix.h"

#define CONSTRAIN(n, min, max) (fmax(min, fmin(max, n)))

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
((byte) & 0x80 ? '1' : '0'), \
((byte) & 0x40 ? '1' : '0'), \
((byte) & 0x20 ? '1' : '0'), \
((byte) & 0x10 ? '1' : '0'), \
((byte) & 0x08 ? '1' : '0'), \
((byte) & 0x04 ? '1' : '0'), \
((byte) & 0x02 ? '1' : '0'), \
((byte) & 0x01 ? '1' : '0')

void printf_reg8(uint8_t reg) {
	printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(reg));
}


//---------------------Clock---------------------
void init_clock(void) {
	// initialize external 16MHz clock
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = 0x00;
	
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

//---------------------Analog-Digital Converter (ADC0)---------------------
void init_ADC0(void) {
	ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;
	ADC0.CTRLC = ADC_PRESC_DIV16_gc;
	ADC0.CTRLD = ADC_INITDLY_DLY16_gc;
	VREF.ADC0REF = VREF_REFSEL_VDD_gc;
	ADC0.CTRLA = ADC_ENABLE_bm;
	ADC0.CTRLA &= ~ADC_LEFTADJ_bm;
	ADC0.COMMAND = ADC_STCONV_bm;
	loop_until_bit_is_clear(ADC0.COMMAND, ADC_STCONV_bp);
}

void start_ADC0(void) {
	ADC0.COMMAND = ADC_STCONV_bm;
}

void read_ADC0(float* reading) {
	*reading = (float)(ADC0.RES)/4096.0;
	start_ADC0();
}

//---------------------Timer A0 (TCA0)---------------------
void init_TCA0(void) {
	// initialize timer A0 with period of 1ms
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA0.SINGLE.PER = 249;
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

//---------------------Button---------------------
#define DEBOUNCE_TIME 10
#define BTN2 (!(VPORTB.IN & PIN2_bm))

void init_btn(void) {
	PORTB.PIN2CTRL |= PORT_ISC_RISING_gc | PORT_PULLUPEN_bm;
	PORTB.DIRCLR = PIN2_bm;
}

// absolute min time step is 40
// printing walls of text take a lot of time
// noticeable speed up when no printing

#define TIME_STEP 40
#define NUM_LEDS 255

enum Animation {RIPPLE_ANIM, DEBUG_ANIM, BB_POT_ANIM};
enum Animation mode = BB_POT_ANIM;

CRGB leds[NUM_LEDS+1];

volatile uint16_t millis = 0;
volatile int tc_btn2 = 0;
volatile char btn2_flag = 0;

int16_t acc_x = 0;
int16_t acc_y = 0;
int16_t acc_z = 0;

int16_t acc_x_prev = 0;
int16_t acc_y_prev = 0;
int16_t acc_z_prev = 0;

uint16_t radius = 0;

float adc_data;

int main(void)
{
	init_clock();
	init_TCA0();
	init_ADC0();
	uart_init(3, 9600, NULL);
	LIS331_init();
	LED_init();
	init_btn();
	
	printf("end of init functions\n");
	LED_set_brightness(10);
	
	sei();
    while (1) {	
			if (millis > TIME_STEP) {
				millis = 0;
				
				acc_y_prev = acc_y;
				acc_x_prev = acc_x;
				acc_z_prev = acc_z;
				
				// read acceleration data
				(void)LIS331_readAxes(&acc_x, &acc_y, &acc_z);

				// read ADC0 and calculate radius
				read_ADC0(&adc_data);
				radius = CONSTRAIN(25*adc_data, 1, 25*adc_data);
				
				// apply biasing and attenuation of signal
				acc_y = (acc_y - 7)*0.7;
				acc_x = (acc_x - 17)*0.7;
				acc_z *= 0.7;
				
				// some exponential smoothing for less noise
				acc_y = acc_y * 0.2 + acc_y_prev * 0.8;
				acc_x = acc_x * 0.2 + acc_x_prev * 0.8;
				acc_z = acc_z * 0.2 + acc_z_prev * 0.8;


				// print for debugging purposes
				/*char buf[6];
				dtostrf(acc_x, 4, 2, buf);
				printf("RAWX:%s\t|\t", buf);
				dtostrf(abs(acc_z-acc_z_prev), 4, 2, buf);
				printf("RAWZ:%s\t|\t", buf);
				
				dtostrf(adc_data, 4, 2, buf);
				printf("ADC:%s\t|\t", buf);

				printf("\n");*/
				
				// switch different modes of animation
				switch (mode) {
					case RIPPLE_ANIM:
						anim_ripple();
						break;
					case DEBUG_ANIM:
						anim_debug();
						break;
					case BB_POT_ANIM:
						anim_brickbreaker();
						break;
				}
			}

			// standard hardware intr button
			if (btn2_flag) {
				if (!BTN2) {
					btn2_flag = 0;
					if (mode == BB_POT_ANIM) mode = RIPPLE_ANIM;
					else mode++;
				}
			}
  	}
}

ISR(TCA0_OVF_vect) {
	// timer isr, provides timekeeping
	if (millis < UINT16_MAX) millis++;
	if (tc_btn2 < INT_MAX) tc_btn2++;
	if (tc_btn2 >= DEBOUNCE_TIME && BTN2) btn2_flag = 1;
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

ISR(PORTB_PORT_vect) {
	// hardware intr isr for button
	if (PORTB.INTFLAGS & PIN2_bm) {
		PORTB.INTFLAGS |= PIN2_bm;
		if (tc_btn2 >= DEBOUNCE_TIME) tc_btn2 = 0;
	}
}

void anim_ripple(void) {
	// ripple pattern animation
	// when board is shaken, ripples appear
	// simple (confusing) state machine code

	static uint8_t g = 0;
	static uint8_t anim = 0;
	
	if (abs(acc_z_prev - acc_z) > 5) {
		// if change in z acc > threshold, animation occurs
		anim = 1;
		printf("trigd\n");
	}
	
	// clear the Matrix
	LED_fill_color(leds, *BLACK, NUM_LEDS);
	if (anim > 0) {
		g++;
		// Bresenham's circle algorithm
		Matrix_circle(leds, 7, 7, g%radius, *TEAL);
		Matrix_circle(leds, 7, 7, (g-5)%radius, *TEAL);
	}
	if (g > 20) {
		// stop animation
		anim = 0;
		g = 0;
	}
	LED_show(leds, NUM_LEDS);
}

void anim_debug(void) {
	// debugging animation
	// vector visualization of gravity dir, mag
	// circle radius from ADC0

	float theta = atan2(acc_y, acc_x);	// atan2 to get the correct angle
	float mag = (sqrt(acc_x*acc_x + acc_y*acc_y)/10); // normalized magnitude
	float c = round(7+7*mag*cos(theta));
	float s = round(7+7*mag*sin(theta));
	c = CONSTRAIN(c, 0, 15);
	s = CONSTRAIN(s, 0, 15);
	
	LED_fill_color(leds, *BLACK, NUM_LEDS);
	Matrix_circle(leds, 7, 7, radius, *TEAL);
	// x, y axis vectors
	Matrix_line(leds, 7, 7, 12, 7, *RED);
	Matrix_line(leds, 7, 7, 7, 12, *BLUE);
	// gravity vector
	Matrix_line(leds, 7, 7, c, s, *GREEN);
	
	LED_show(leds, NUM_LEDS);
}

void anim_brickbreaker() {
	// supposed to be a playable brickbreaker game
	// currently does not work
	// paddle moves according to the pot on ADC0
	// TODO: add functionality 

	uint8_t r = 2; // half width of paddle
	uint8_t paddle_y = 1;
	uint8_t paddle_x = 8 + 6*(-2*adc_data + 1); // paddle x pos calculated from ADC0
	
	LED_fill_color(leds, *BLACK, NUM_LEDS);
	Matrix_line(leds, paddle_x-r, 1, paddle_x+r, 1, *TEAL);
	
	static uint8_t ball_x = 4;
	static uint8_t ball_y = 2;
	static int8_t	ball_vel_x = 1;
	static int8_t	ball_vel_y = 1;
	
	// wall collisions
	if (ball_x >= COLS) ball_vel_x = -1;
	if (ball_x <= 0)	ball_vel_x = 1;
	
	if (ball_y >= ROWS) ball_vel_y = -1;
	
	//uint8_t collision = 0;
	//for (uint8_t x = paddle_x-r; x >= paddle_x-r; x++) {
		//if (ball_y+1 == paddle_y && ball_y == paddle_x) {
			//collision &= 1;
		//}
	//}
	
	
	if (ball_y <= 0)	ball_vel_y = 1;
	
	ball_x += ball_vel_x;
	ball_y += ball_vel_y;
	
	Matrix_set_pixel(leds, ball_x, ball_y, *YELLOW);
	
	LED_show(leds, NUM_LEDS);
}
