#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
//#include "lcd_lib.h"
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdbool.h>
#include <util/twi.h>

#define SAWTOOTH_MAX 4095

void init_clock() {
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.OSCHFCTRLA = CLKCTRL_FRQSEL_16M_gc;
}

void InitTimerTCA1()
{
	TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	TCA1.SINGLE.PER = 1999;
	//Initially at 10% duty cycle
	TCA1.SINGLE.CMP2 = 199; //Set for a frequency of
	
	TCA1.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;
	
	TCA1.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV8_gc | TCA_SINGLE_ENABLE_bm);
	
	TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	
	PORTMUX.TCAROUTEA |= PORTMUX_TCA1_PORTB_gc;
	
	
	PORTB.DIRSET = PIN2_bm;
	PORTB.OUTSET = PIN2_bm;
	
}


void InitTimerTCA0()
{
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
	//4095 for 12bit resolution
	TCA0.SINGLE.PER = 4095;
	
	TCA0.SINGLE.CMP2 = 0;
	
	TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;
	
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV4_gc | TCA_SINGLE_ENABLE_bm);
	
	//TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	
	PORTMUX.TCAROUTEA |= PORTMUX_TCA0_PORTC_gc;
	
	
	PORTC.DIRSET = PIN2_bm;
	PORTC.OUTSET = PIN2_bm;
	
}

//Port E0 potentiometer
void InitADCO(){
	ADC0.MUXPOS = ADC_MUXPOS_AIN8_gc;
	
	ADC0.CTRLC = ADC_PRESC_DIV8_gc;
	ADC0.CTRLD = ADC_INITDLY_DLY16_gc;
	VREF.ADC0REF = VREF_REFSEL_VDD_gc;
	ADC0.CTRLA = ADC_ENABLE_bm;
}



volatile int vCheck_time = 1000;
volatile float  CPM0_check = 0;
volatile float period_size = 1000.0;

int main(void)
{
	
	init_clock();

	InitADCO();
	InitTimerTCA1();
	InitTimerTCA0();

	sei();
	
	ADC0.COMMAND = ADC_STCONV_bm;
	while ((ADC0.COMMAND & (1<< ADC_STCONV_bp)));
	
	while (1)
	{
		
		unsigned int Ain = ADC0.RES;
		float Voltage = (float)Ain / 4096.0 * 3.3;
		float duty_cyle = (Voltage/3.3);
		
		//LED Brightness Control Output PortB2
		TCA1.SINGLE.CMP2 = (uint16_t)((duty_cyle * 1999));
		
		//Sawtooth Wave Output Port C2
		period_size = duty_cyle*1000.0;
		uint16_t new_compare = (CPM0_check * 4095) / period_size;
		TCA0.SINGLE.CMP2BUF = new_compare;
		
		
		
		if(vCheck_time == 0){
			ADC0.COMMAND = ADC_STCONV_bm;
			vCheck_time = 1000;
		}
	}
}

ISR(TCA1_OVF_vect){
	if (vCheck_time > 0) {
		vCheck_time--;
	}
	
	
	if (CPM0_check < period_size) {
		CPM0_check++;
		} else {
		CPM0_check = 0.0;
		
	}


	// Reset the timer overflow interrupt flag
	TCA1.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}