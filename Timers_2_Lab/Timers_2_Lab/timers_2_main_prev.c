#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "uart.h"

#define F_CPU 16000000UL
#define TCA0_PRESCALE 64
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

void init_TCA1(void) {
	// initialize timer A1 (TCA1) with period of 1ms
	TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA1.SINGLE.PER = 249;
	TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA1.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

void init_TCA0(void) {
	// timer counter A0 (TCA0) cannot be mapped onto PORTD (LED output)
	// initialize TCA0 as waveform generation
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc;
	TCA0.SINGLE.CMP0 = 124999;
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP0EN_bm;
	// enable TCA0 CMP0 interrupt
	//TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;
	
	// set prescaler to 64 and enable timer
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
	PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTD_gc;
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

volatile int tc = 0;
volatile int tc_btn2 = 0;
volatile int tc_btn5 = 0;
volatile char btn2_flag = 0;
volatile char btn5_flag = 0;

int main(void)
{
	init_clock();
	init_TCA0();
	init_TCA1();
	
	int freq = 1;
	
	VPORTD.DIR = 0xff;
	PORTB.DIR = 0x00;
	PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
	
	
	init_btn();
	uart_init(3, 9600, NULL);
	
	sei();
    while (1) 
    {
		if (tc >= 5000) {
			tc = 0;
			printf("Frequency: %d\n", freq);
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
	if (tc < INT_MAX) tc++;
	
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

