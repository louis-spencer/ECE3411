#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include "uart.h"

#define F_CPU 16000000UL
#define S_CNT 20

#define DEBOUNCE_TIME 10
#define BTN5 (!(VPORTB.IN & PIN5_bm))

#define TCA0_PRESCALE 256
#define FREQ_CMP0(freq) ((F_CPU / (2 * freq * TCA0_PRESCALE)) - 1)

// --------- CPU clock ---------
void init_clock(void) {
	// initialize external 16MHz clock
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}
// --------- waveform generation ---------
void init_TCA0(void) {
	// initialize TCA0 as waveform generation
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc;
	TCA0.SINGLE.CMP0 = FREQ_CMP0(0);
	//TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP0EN_bm;
	
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV256_gc | TCA_SINGLE_ENABLE_bm);
	PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTD_gc;
	// rising and falling interrupts
	//PORTD.PIN0CTRL |= (PORT_ISC_BOTHEDGES_gc);
	PORTD.DIRSET = PIN0_bm;
}

void set_freq_TCA0(int freq) {
	// store the next frequency/CMP0 value in the buffer for smoother operation
	TCA0.SINGLE.CMP0BUF = FREQ_CMP0(freq);
}

// --------- timer counter ---------
void init_TCA1(void) {
	// initialize timer A0 with period of 1ms
	TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA1.SINGLE.PER = 249;
	TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	   
	TCA1.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

// --------- ADC ---------
void init_ADC(void) {
	// set input voltage at AIN2
	ADC0.MUXPOS = ADC_MUXPOS_AIN2_gc;
	// set ADC clock to 1MHz
	ADC0.CTRLC = ADC_PRESC_DIV16_gc;
	// Delay 16 ADC clock cycles
	ADC0.CTRLD = ADC_INITDLY_DLY16_gc;
	// use VDD as reference
	VREF.ADC0REF = VREF_REFSEL_VDD_gc;
	// enable ADC circuitry
	ADC0.CTRLA = ADC_ENABLE_bm;
	// set left adjust to 0 for 12 bit accuracy
	ADC0.CTRLA &= ~ADC_LEFTADJ_bm;
	// start first AD conversion
	ADC0.COMMAND = ADC_STCONV_bm;
	
	loop_until_bit_is_clear(ADC0.COMMAND, ADC_STCONV_bp);
}

void init_btn(void) {
	PORTB.PIN5CTRL |= PORT_ISC_RISING_gc;
	PORTB.DIRCLR = PIN5_bm;
}

volatile int tc = 0;
volatile int tc_btn5 = 0;
volatile int tc_pause = 0;
volatile char btn5_flag = 0;
volatile int meas_count = 0;
int tc_pause_cnt = 0;
float avg_temp_C = 0;
float avg_temp_F = 0;
float voltage = 0;


int main(void)
{
    init_clock();
	init_TCA1();
	init_TCA0();
	uart_init(3, 9600, NULL);
	
	init_ADC();
	printf("end of setup\n");
	sei();
    while (1) 
    {
		if (tc_pause_cnt >= 4 && btn5_flag == 1) {
			tc_pause_cnt = tc_pause = btn5_flag = 0;
		}
		
		if (tc >= 50 && btn5_flag == 0) {
			meas_count++;
			tc = 0;
			
			voltage = (float)ADC0.RES/4096.0 * 3.3;
			// not quite 10mV/K, calculate at known temp and voltage
			float t_celsius = (voltage * 100) - 273.15;
			float t_fahrenheit = t_celsius * 1.8 + 32.0;
			
			avg_temp_C += t_celsius;
			avg_temp_F += t_fahrenheit;
			
			ADC0.COMMAND = ADC_STCONV_bm;
			//char strbuf[6];
			//dtostrf(voltage, 4, 2, strbuf);
			//printf("%s Volts\n", strbuf);
		}
		
		if (meas_count >= S_CNT && btn5_flag == 0) {
			avg_temp_C /= meas_count;
			avg_temp_F /= meas_count;
			meas_count = 0;
			
			// print stuff
			char strbuf[6];
			dtostrf(voltage, 4, 2, strbuf);
			printf("%s Volts\t", strbuf);			
			dtostrf(avg_temp_C, 4, 2, strbuf);
			printf("T(C)=%s\t", strbuf);
			dtostrf(avg_temp_F, 4, 2, strbuf);
			printf("T(F)=%s\n", strbuf);
			
			// set the frequency
			int temp_freq = ((int)abs(avg_temp_F))%10;
			//if (temp_freq == 0) temp_freq = 60;
			
			if (temp_freq == 0) {
				PORTMUX.TCAROUTEA &= ~PORTMUX_TCA0_PORTD_gc;
				VPORTD.OUT |= PIN0_bm;
			} else {
				PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTD_gc;
				set_freq_TCA0(temp_freq);
			}			
			
			// clear avgs
			avg_temp_C = avg_temp_F = 0;
		}
		
		if (tc_pause >= 1000 && tc_pause_cnt < 4 && btn5_flag == 1) {
			tc_pause_cnt++;
			tc_pause = 0;
			
			char strbuf[6];
			dtostrf(voltage, 4, 2, strbuf);
			printf("%s Volts \t", strbuf);			
			dtostrf(avg_temp_C / meas_count, 4, 2, strbuf);
			printf("T(C)=%s\t", strbuf);
			dtostrf(avg_temp_F / meas_count, 4, 2, strbuf);
			printf("T(F)=%s\tPAUSED\n", strbuf);
		}
		
    }
}

ISR(TCA1_OVF_vect) {
	if (tc < INT_MAX && btn5_flag == 0) tc++;
	if (tc_pause < INT_MAX && btn5_flag == 1) tc_pause++;
	if (tc_btn5 < INT_MAX) tc_btn5++;
	if (tc_btn5 >= DEBOUNCE_TIME && BTN5) btn5_flag = 1;
	
	TCA1.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

ISR(PORTB_PORT_vect) {
	if (PORTB.INTFLAGS & PIN5_bm) {
		PORTB.INTFLAGS |= PIN5_bm;
		if (tc_btn5 >= DEBOUNCE_TIME) tc_btn5 = 0;
	}
}

