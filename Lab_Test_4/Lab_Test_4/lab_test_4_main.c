#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "lcd_lib.h"
//#include "uart.h"

#define F_CPU 16000000UL
#define PWM_SCALE		1
#define PWM_PER			4095
#define TEMPR_ADDR		0b1001000

void init_clock(void) {
	// initialize external 16MHz clock
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

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

typedef enum state {ZERO, RISING, TOP, FALLING} state_t;

// timer count
void init_TCA1(void) {
	// timer with period of 1ms
	
	//-----initialize 1ms timer-----
	TCA1.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	
	//-----set the timer frequency-----
	// 1ms = (PER+1) * 1/(F_CLK / DIV) = 2000 * 1/(16e6 / 8)
	TCA1.SINGLE.CTRLA |= TCA_SINGLE_CLKSEL_DIV8_gc;
	TCA1.SINGLE.PER = 1999;
	
	//-----enable interrupt control-----
	TCA1.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	
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

void start_ADC0(void) {
	ADC0.COMMAND = ADC_STCONV_bm;
	//loop_until_bit_is_clear(ADC0.COMMAND, ADC_STCONV_bp);
}

uint8_t read_i2c_temp(void) {
	TWI_Host_Write(TEMPR_ADDR, 0);
	uint8_t data = TWI_Host_Read(TEMPR_ADDR);
	return data;
}

#define DEBOUNCE_TIME 50
#define BTN5 (!(VPORTB.IN & PIN5_bm))

void init_btn(void) {
	PORTB.PIN5CTRL |= PORT_ISC_RISING_gc;
	PORTB.DIRCLR = PIN5_bm;
}

volatile uint16_t print_counter = 0;
volatile uint16_t millis = 0;
volatile uint16_t tc_btn5 = 0;
volatile char btn5_flag = 0;

char tempr_stdby = 0;
uint8_t tempr_data = 0;
int tempr_thresh = 24;
state_t waveform_state = ZERO;


int main(void)
{
    init_clock();
	init_TCA1();
	init_TCA0();
	init_ADC0();
	
	LCDinitialize();
	PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
	PORTA.PIN3CTRL |= PORT_PULLUPEN_bm;
	
	//uart_init(3, 9600, NULL);
	
	sei();
    while (1) 
    {
		if (print_counter >= 200) {
			start_ADC0();
		
			LCDclr();
			print_counter = 0;
			tempr_data = read_i2c_temp();
			
			char strbuf[2];
			dtostrf(tempr_data, 2, 0, strbuf);
			
			//printf("%d %d %d %d\t", tempr_data, millis, tempr_thresh, (int)tempr_stdby);
			//if (waveform_state == ZERO) printf("zero\n");
			//else if (waveform_state == RISING) printf("rising\n");
			//else if (waveform_state == TOP) printf("top\n");
			//else if (waveform_state == FALLING) printf("falling\n");
			
			LCDstring("TC74: ");
			LCDstring(strbuf);
			LCDstring("C");
			if (tempr_stdby > 0) LCDstring(" *");
			
			memset(strbuf, NULL, 2);
			dtostrf(tempr_thresh, 2, 0, strbuf);
			LCDgotoXY(0, 1);
			LCDstring("Threshold: ");
			LCDstring(strbuf);
			LCDstring("C");
			
					
		}
		
		if (btn5_flag == 1) {
			if (!BTN5) {
				btn5_flag = 0;
				tempr_stdby = ~tempr_stdby;
			
				if (tempr_stdby > 0) {
					TWI_2TXData(TEMPR_ADDR, 0x01, 0x80);
				} else {
					TWI_2TXData(TEMPR_ADDR, 0x01, 0x00);
				}
			}
		}
		
		float adc_reading = (float)(ADC0.RES)/4096.0;
		tempr_thresh = (int)((30.0-22.0)*adc_reading + 22.0);
			
		if (waveform_state == ZERO) {
			waveform_state = ZERO;
			TCA0.SINGLE.CMP2BUF = 0;
			if (tempr_data >= tempr_thresh) {
				waveform_state = RISING;
			}
			
		} else if (waveform_state == RISING) {
			if (millis < 2000) {
				TCA0.SINGLE.CMP2BUF = (uint16_t)((1.0*millis/2000.0)*2482.0);
			} else {
				millis = 0;
				waveform_state = TOP;
			}
			
		} else if (waveform_state == TOP) {
			waveform_state = TOP;
			TCA0.SINGLE.CMP2BUF = (2.0/3.3) * 4095;
			if (tempr_data < tempr_thresh) waveform_state = FALLING;
			
			
		} else if (waveform_state == FALLING) {
			//waveform_state = FALLING;
			if (millis < 2000) {
				TCA0.SINGLE.CMP2BUF = (uint16_t)((1.0*(2000.0-millis)/2000.0)*2482.0);
			} else {
				millis = 0;
				waveform_state = ZERO;
			}
			
		}
		
    }
}

ISR(TCA1_OVF_vect) {
	if (print_counter <= UINT16_MAX) print_counter++;
	if (waveform_state != ZERO && waveform_state != TOP &&millis <= UINT16_MAX) millis++;
	
	if (tc_btn5 < UINT16_MAX) tc_btn5++;
	if (tc_btn5 >= DEBOUNCE_TIME && BTN5) btn5_flag = 1;
	
	TCA1.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

ISR(PORTB_PORT_vect) {
	if (PORTB.INTFLAGS & PIN5_bm) {
		PORTB.INTFLAGS |= PIN5_bm;
		if (tc_btn5 >= DEBOUNCE_TIME) tc_btn5 = 0;
	}
}
