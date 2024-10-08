#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "uart.h"

#define F_CPU 16000000UL
#define TCA0_PRESCALE 256
#define FREQ_CMP0(freq) ((F_CPU / (2 * freq * TCA0_PRESCALE)) - 1)

#define DEBOUNCE_TIME 10
#define BTN5 (!(VPORTB.IN & PIN5_bm))
#define BTN2 (!(VPORTB.IN & PIN2_bm))

void init_clock(void) {
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

// counter timer
void init_TCA1(void) {
	// initialize timer A1 (TCA1) with period of 1us
	// time it takes for timer to reach match value:
	// T = N * (1 + PER) / f_clk
	// But, timer ticks at: f_tca = f_clk / N
	// Or, timer ticks occur every: t_tca = N / f_clk
	TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA1.SINGLE.PER = 999;
	TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	// counting every 1us with DIV16 instead of DIV64
	TCA1.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm);
}

// waveform timer
void init_TCA0(void) {
	// timer counter A0 (TCA0) cannot be mapped onto PORTD (LED output)
	// initialize TCA0 as waveform generation
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc;
	TCA0.SINGLE.CMP0 = FREQ_CMP0(5);
	//TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP0EN_bm;
	// enable TCA0 CMP0 interrupt
	
	// set prescaler to 64 and enable timer
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV256_gc | TCA_SINGLE_ENABLE_bm);
	PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTD_gc;
	// rising and falling interrupts
	PORTD.PIN0CTRL |= (PORT_ISC_BOTHEDGES_gc);
	PORTD.DIRSET = PIN0_bm;
}

void init_btn(void) {
	PORTB.PIN5CTRL |= PORT_ISC_RISING_gc;
	PORTB.PIN2CTRL |= PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
	PORTB.DIRCLR = PIN5_bm | PIN2_bm;
}

void set_freq_TCA0(int freq) {
	// store the next frequency/CMP0 value in the buffer for smoother operation
	TCA0.SINGLE.CMP0BUF = FREQ_CMP0(freq);
}
//
//volatile int tc = 0;
//volatile int freq = 1;
//
//volatile unsigned long t_us = 0;	// in us
//volatile unsigned long t_prev = 0;
//volatile unsigned long t_avg = 0;
//volatile unsigned int count = 0;
//volatile char mode = 1;

volatile long tc = 0;
volatile long total = 0;
volatile long t_prev = 0;
volatile int count = 0;
volatile char mode = 0;

volatile int freq = 5;
volatile int tc_btn2 = 0;
volatile int tc_btn5 = 0;
volatile char btn2_flag = 0;
volatile char btn5_flag = 0;

int main(void)
{
	init_clock();
	init_TCA0();
	init_TCA1();
	
	
	VPORTD.DIR = 0xff;
	PORTB.DIR = 0x00;
	PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
	
	
	init_btn();
	uart_init(3, 9600, NULL);
	
	sei();
    while (1) 
    {
		if (tc >= 1000000) {
			//cli();
			printf("Avg: %ld us\t| Freq: %d Hz\t| Count: %d\n", (long)(tc/(2*count)), freq, count);
			//printf("hello world");
			tc = 0;
			t_prev = 0;
			mode = 0;
			count = 0;
			//sei();
		}
		
		if (btn5_flag) {
			if (!BTN5) {
				btn5_flag = 0;
				freq < 10 ? freq++ : 0;
				set_freq_TCA0(freq);	
			}
		}
		
		if (btn2_flag) {
			if (!BTN2) {
				btn2_flag = 0;
				freq > 1 ? freq-- : 0;
				set_freq_TCA0(freq);
			}
		}
    }
}

ISR(TCA1_OVF_vect) {
	if (tc < UINT32_MAX) tc+=1000;
	
	if (tc_btn2 < INT_MAX) tc_btn2++;
	if (tc_btn5 < INT_MAX) tc_btn5++;
	if (tc_btn2 >= DEBOUNCE_TIME && BTN2) btn2_flag = 1; 
	if (tc_btn5 >= DEBOUNCE_TIME && BTN5) btn5_flag = 1;
	
	TCA1.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

ISR(PORTB_PORT_vect) {
	if (PORTB.INTFLAGS & PIN5_bm) {
		PORTB.INTFLAGS |= PIN5_bm;
		if (tc_btn5 >= DEBOUNCE_TIME) tc_btn5 = 0;
	}
	if (PORTB.INTFLAGS & PIN2_bm) {
		PORTB.INTFLAGS |= PIN2_bm;
		if (tc_btn2 >= DEBOUNCE_TIME) tc_btn2 = 0;
	}
}

ISR(PORTD_PORT_vect) {
	if (PORTD.INTFLAGS & PIN0_bm) {
		PORTD.INTFLAGS |= PIN0_bm;
		
		if (mode == 0) {
			t_prev = tc;
			mode = 1;
		} else if (mode == 1) {
			total += (tc - t_prev);
			count++;
			mode = 0;
		}
	}
}

