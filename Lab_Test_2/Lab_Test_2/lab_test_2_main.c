#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "uart.h"

#define F_CPU 16000000UL 
#include <util/delay.h>

void delay(double time_ms) {
	for (int i = 0; i < time_ms; i++) {
		_delay_ms(1);
	}
}

void blink_LED(char num_bm, char freq) {
	// blink LED one on/off cycle
	double period_ms = 1000 / freq;
	PORTD.OUT |= num_bm;
	delay(period_ms / 2);
	PORTD.OUT &= ~num_bm;
	delay(period_ms / 2);
}

void init_clock(void) {
	// initialize external 16MHz clock
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

void clear_buf(char buf[], char *buf_sz, int sz) {
	// clear buffer by setting to NULL
	// set buf size tracker to 0
	for (int i = 0; i < sz; i++) {
		buf[i] = NULL;
	}
	*buf_sz = 0;
}

volatile char input = 0;

int main(void)
{
	// initialize clock
	init_clock();
	
	// setup as output
    VPORTD.DIR = 0xff;
	VPORTD.OUT = 0x00;
	
	// enable on-board button
	PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
	PORTB.DIR = PIN3_bm;
	
	uart_init(3, 9600, NULL);
	// enable interrupt
	USART3.CTRLA |= USART_RXCIE_bm;
	
	char freq = 4;
	char led_bm = 0x01;
	char buf[3] = {NULL, NULL, NULL};
	char pt = 0;
	
	char btn_handled2 = 0;
	// 0 if freq mode, 1 if led mode
	char mode = 0;
	
	sei();
	printf("\nFrequency: ");
    while (1) 
    {
		blink_LED(led_bm, freq);
		
		// on-board button handling
		if ( !(PORTB.IN & PIN2_bm) ) {
			if (!btn_handled2) {
				mode = !mode;
				if (mode == 0x00) {
					// frequency mode, toggle on
					printf("\nFrequency: ");
					PORTB.OUT &= ~PIN3_bm;
				} else {
					// led mode, toggle LED off
					printf("\n#LEDS: ");
					PORTB.OUT |= PIN3_bm;
				}
				
				btn_handled2 = 1;
			}
		} else {
			btn_handled2 = 0;
		}
		
		if (input > 0) {
			cli();
			if (input >= 0x30 && input <= 0x39 && pt < 2) {
				// if weird buffer overflow thing, clear it
				if (pt >= 3) clear_buf(buf, &pt, 3);
				// buffer as queue, increment pointer
				buf[pt] = input;
				pt++;
				printf("%c", input);
			}
			if (input == '\r' && pt > 0 && mode == 0) {
				// in frequency mode
				// if not single digit 0 or double digit 0 set freq to input
				// clear buffer
				if (pt == 1 && buf[0] != 0x30) freq = buf[0]-0x30;
				if (pt == 2 && ((buf[0]-0x30) | (buf[1]-0x30))) freq = (buf[0]-0x30)*10 + (buf[1]-0x30);
				clear_buf(buf, &pt, 3);
				printf("\nFrequency: ");
			}
			if (input == '\r' && pt > 0 && mode == 1) {
				// if input is not single digit or 9
				// bit manipulation to set bits
				// clear buffer
				if (buf[0] < 0x39 && pt == 1) {
					led_bm = 0xff;
					led_bm >>= 8 - (buf[0]-0x30);
				}
				clear_buf(buf, &pt, 3);
				printf("\n#LEDS: ");
			}
			
			if (input > 0) input = 0;
			sei();	
		}
    }
}

ISR(USART3_RXC_vect) {
	input = USART3.RXDATAL;
}

