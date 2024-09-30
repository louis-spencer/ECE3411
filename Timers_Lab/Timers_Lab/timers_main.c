#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "uart.h"

#define F_CPU 16000000UL
#define BTN2 (!(VPORTB.IN & PIN2_bm)) // macro for detecting button presses
#define BTN5 (!(VPORTB.IN & PIN5_bm))
#define DEBOUNCE_TIME 10

volatile int tc1 = 0;			// timer counter 1
volatile int tc2 = 0;			// timer counter 2
volatile int debounce_tc = DEBOUNCE_TIME;	// timer counter debounce

volatile char btn2pushed = 0;
volatile char btn5pushed = 0;

void init_clock(void) {
	// initialize external 16MHz clock
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

void init_TCA0(void) {
	// initialize timer A0 with period of 1ms
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA0.SINGLE.PER = 249;
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

typedef enum {RELEASED, MAYBE_PUSHED, PUSHED, MAYBE_RELEASED} btn_state_t;

// button state machines
void buttonSM2(void) {
	static btn_state_t state2 = RELEASED;
	switch (state2) {
		case RELEASED:
			if (BTN2) state2 = MAYBE_PUSHED;
			break;
		case MAYBE_PUSHED:
			if (BTN2) {
				state2 = PUSHED;
				btn2pushed = 1;
			} else {
				state2 = RELEASED;
			}
			break;
		case MAYBE_RELEASED:
			if (BTN2) {
				state2 = PUSHED;
			} else {
				state2 = RELEASED;
			}
			break;
		case PUSHED:
			if (!BTN2) state2 = MAYBE_RELEASED; 
			break; 
	}
}

void buttonSM5(void) {
	static btn_state_t state5 = RELEASED;
	switch (state5) {
		case RELEASED:
			if (BTN5) state5 = MAYBE_PUSHED;
			break;
		case MAYBE_PUSHED:
			if (BTN5) {
				state5 = PUSHED;
				btn5pushed = 1;
			} else {
				state5 = RELEASED;
			}
			break;
		case MAYBE_RELEASED:
			if (BTN5) {
				state5 = PUSHED;
			} else {
				state5 = RELEASED;
			}
			break;
		case PUSHED:
			if (!BTN5) state5 = MAYBE_RELEASED; 
			break; 
	}
}

int main(void) {
	init_clock();
	init_TCA0();
	uart_init(3, 9600, NULL);
	
	VPORTD.DIR = 0xff;
	VPORTD.OUT = 0xff;
	
	// when using on-board button on PORTB
	// make sure to &= PIN0 and PIN1
	// those pins are used for UART on AVR
	PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
	
	// set output of everything except
	// PIN0 and PIN1 to out
	// alternatively, call PORTB.DIR before uart_init
	PORTB.DIR &= (0x00 | PIN0_bm | PIN1_bm);
	
	int freq = 5;
	
	printf("\nsetup complete\n");
	
	sei();
	while (1) {
		if (btn2pushed == 1) {
			btn2pushed = 0;
			freq++;
		}
		
		if (btn5pushed == 1) {
			btn5pushed = 0;
			freq--;
			if (freq < 1) freq = 1;
		}
		
		if (tc1 >= 500 / freq) {
			// turn on/off LED at 1*period/2
			// turn off/on LED at 2*period/2
			VPORTD.OUT = ~VPORTD.OUT;
			tc1 = 0;
		}
		
		if (tc2 >= 5000) {
			printf("frequency: %d\n", freq);
			tc2 = 0;
		}
	}
}

ISR(TCA0_OVF_vect) {
	if (tc1 < INT_MAX) tc1++;
	if (tc2 < INT_MAX) tc2++;
	
	if (debounce_tc < DEBOUNCE_TIME) {
		debounce_tc++;
	} else {
		// every DEBOUNCE_TIME run button state machine
		buttonSM2();
		buttonSM5();
		debounce_tc = 0;
	}
	
	// make sure to clear flags
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}