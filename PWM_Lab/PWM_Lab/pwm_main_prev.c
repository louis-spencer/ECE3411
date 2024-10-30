#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "lcd_lib.h"

#define F_CPU 16000000UL
//#define PWM_FREQ		1000
#define PWM_SCALE		1
#define PWM_PER			4095

void init_clock(void) {
	// initialize external 16MHz clock
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

void set_pwm_dc(int dc) {
	// dc ranges from 0-100
	// waveform output (WO2) is assigned to PORTB2
	TCA0.SINGLE.CMP2 = dc == 0? 0 : (int)((float)dc*PWM_PER/100.0 - 1);
}

void init_TCA0(void) {
	// PWM generation
	PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTB_gc;
	PORTB.DIRSET = PIN2_bm;
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;
	TCA0.SINGLE.PER = PWM_PER;
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm);
}

void init_TCA1(void) {
	// initialize timer A0 with period of 1ms
	TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA1.SINGLE.PER = 249;
	TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA1.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

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

volatile int millis = 0;
int dc;

int main(void)
{
	init_clock();
	init_TCA1();
	init_TCA0();
	init_ADC0();
	LCDinitialize();
	
	set_pwm_dc(100);
	
	sei();
    while (1) 
    {
		if (millis >= 50) {
			LCDclr();
			millis = 0;
			dc = (float)(ADC0.RES)/4096 * 100;
			ADC0.COMMAND = ADC_STCONV_bm;
			set_pwm_dc(dc);
			char strbuf[5];
			//snprintf(strbuf, "%d", dc);
			itoa(dc, strbuf, 10);
			LCDstring(strbuf);
		}
    }
}

ISR(TCA1_OVF_vect) {
	if (millis < INT_MAX) millis++;
	TCA1.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}