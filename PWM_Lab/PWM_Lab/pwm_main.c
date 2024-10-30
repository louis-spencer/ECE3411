#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
//#include "lcd_lib.h"
#include "uart.h"

#define F_CPU 16000000UL
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

//void init_TCA0(void) {
	// PWM generation
	//PORTMUX.TCAROUTEA = PORTMUX_TCA0_PORTB_gc;
	//PORTB.DIRSET = PIN2_bm;
	//TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	//TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;
	//TCA0.SINGLE.PER = PWM_PER;
	//TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm);
	
//}

//void init_TCA1(void) {
	// initialize timer A0 with period of 1ms
	//TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	// for normal mode, PER is how many TIMER_CLK ticks before OVF/RESET
	//TCA1.SINGLE.PER = 249;
	//TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	// for normal mode, DIV divides the system clock F_CLK --> F_CLK / DIV = TIMER_CLK
	//TCA1.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
	
//}

// sawtooth waveform
void init_TCA0(void) {
	// use to generate sawtooth wave
	
	//-----initialize PWM timer-----
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	
	//-----set the timer frequency-----
	// settings for 12 bit resolution
	TCA0.SINGLE.CTRLA |= TCA_SINGLE_CLKSEL_DIV4_gc;
	TCA0.SINGLE.PER = 4095;
	TCA0.SINGLE.CMP2 = 0;
	
	//-----mux rerouting-----
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;
	PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTC_gc;
	PORTC.DIRSET |= PIN2_bm;
	
	//-----start timer-----
	TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
}

// led brightness
void init_TCA1(void) {
	// use for the led
	
	//-----initialize PWM timer-----
	TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	
	//-----set the timer frequency and dc-----
	// 1ms = (PER+1) * 1/(F_CLK / DIV) = 2000 * 1/(16e6 / 8)
	TCA1.SINGLE.CTRLA |= TCA_SINGLE_CLKSEL_DIV8_gc;
	TCA1.SINGLE.PER = 1999;
	// 10% duty cycle
	TCA1.SINGLE.CMP2 = 199;
	
	//-----enable interrupt control-----
	TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	
	//-----mux rerouting-----
	TCA1.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;
	PORTMUX.TCAROUTEA |= PORTMUX_TCA1_PORTB_gc;
	PORTB.DIRSET |= PIN2_bm;
	
	//-----start timer-----
	TCA1.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
}

void set_dc_TCA1(float f_pc) {
	TCA1.SINGLE.CMP2 = (uint16_t)(f_pc * 1999.0);
	//TCA1.SINGLE.CMP2 = (uint8_t)((percent/100)*(PWM_PER+1))/100)-1;
}

void init_ADC0(void) {
	ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;
	ADC0.CTRLC = ADC_PRESC_DIV8_gc;
	ADC0.CTRLD = ADC_INITDLY_DLY16_gc;
	VREF.ADC0REF = VREF_REFSEL_VDD_gc;
	ADC0.CTRLA = ADC_ENABLE_bm;
	loop_until_bit_is_clear(ADC0.COMMAND, ADC_STCONV_bp);
}

void read_ADC0(void) {
	ADC0.COMMAND = ADC_STCONV_bm;
	//loop_until_bit_is_clear(ADC0.COMMAND, ADC_STCONV_bp);
}

volatile uint16_t millis = 0;
float dc = 0;
int dc_cmp = 0;
int sawtooth_period = 1000;
float pre_cmp2buf = 0;

int main(void) {
	
	init_clock();
	init_TCA0();
	init_TCA1();
	init_ADC0();
	
	uart_init(3, 9600, NULL);
	printf("done init\n");
	
	sei();
	while (1) {
		if (millis >= 1000) {
			millis = 0;
			read_ADC0();
			char strbuf[6];
			dtostrf((float)(100.0*dc_cmp/sawtooth_period), 4, 2, strbuf);
			printf("%d\t%d\t%s\n", sawtooth_period, dc_cmp, strbuf);
		}
		
		dc = (float)(ADC0.RES)/4096;
		
		// this controls the brightness of the led
		set_dc_TCA1(dc);
		
		sawtooth_period = dc * 1000;
		pre_cmp2buf = (1.0*dc_cmp/sawtooth_period) * (PWM_PER+1);
		TCA0.SINGLE.CMP2BUF = (uint16_t)pre_cmp2buf;
		//TCA0.SINGLE.CMP2BUF = (4096/2) - 1;
	}
	
	return 0;
}

ISR(TCA1_OVF_vect) {
	if (millis < UINT16_MAX) millis++;
	dc_cmp = (dc_cmp + 1) % sawtooth_period;
	
	TCA1.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}