#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "uart.h"

#define F_CPU 16000000UL

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

volatile int tc = 0;

int main(void)
{
	init_clock();
	init_TCA0();
	uart_init(3, 9600, NULL);
	
	// set input voltage at AIN8
	ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;
	// set ADC clock to 1MHz
	ADC0.CTRLC = ADC_PRESC_DIV16_gc;
	// Delay 16 ADC clock cycles
	ADC0.CTRLD = ADC_INITDLY_DLY16_gc;
	// use VDD as reference
	VREF.ADC0REF = VREF_REFSEL_VDD_gc;
	// enable ADC circuitry
	ADC0.CTRLA = ADC_ENABLE_bm;
	ADC0.CTRLA &= ~ADC_LEFTADJ_bm;
	// start first AD conversion
	ADC0.COMMAND = ADC_STCONV_bm;
	loop_until_bit_is_clear(ADC0.COMMAND, ADC_STCONV_bp);
	
    sei();
	while (1) {
		if (tc >= 1000) {
			tc = 0;
			
			float voltage = (float)(ADC0.RES)/4096 * 3.3;
			ADC0.COMMAND = ADC_STCONV_bm;
			
			char strbuf[6];
			dtostrf(voltage, 4, 2, strbuf);
			printf("Input: %sV\n", strbuf);
		}
		// printf("%d\t", tc);
	}
}

ISR(TCA0_OVF_vect) {
	if (tc < INT_MAX) tc++;
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}
