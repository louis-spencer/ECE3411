#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "lcd_lib.h"

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

#define SPI_PORT	PORTA
#define SPI_MOSI	PIN4_bm
#define SPI_MISO	PIN5_bm
#define SPI_SCK		PIN6_bm
#define SPI_SS		PIN7_bm

void SPI_Master_init(void) {
	// set MOSI, SCK, SS as outputs
	SPI_PORT.DIR = SPI_MOSI | SPI_SCK | SPI_SS;
	// Enable SPI, Host, set clock rate to f_clk / 128 = 125kHz
	SPI0.CTRLA = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESC_DIV128_gc;
	// check datasheet, 1 - disable the client select line when operating as SPI host
	SPI0.CTRLB |= SPI_SSD_bm;
}

uint8_t SPI_Master_Transceiver(uint8_t cData) {
	SPI0.DATA = cData; // start tx - SS pulled down by hardware
	loop_until_bit_is_set(SPI0.INTFLAGS, SPI_IF_bp); // wait for tx complete
	return SPI0.DATA; // return rx data
}

uint8_t SPI_MCP4131_RTX(uint8_t addr, uint8_t cmd, uint16_t data) {
	// combine fields into 16bit frame
	uint16_t tx_data = (addr << 12) | (cmd << 10) | data;
	// divide frame into two cmd bytes
	uint8_t pck1 = (tx_data >> 8);
	uint8_t pck2 = (tx_data & 0x00ff);
	
	// pull CS pin low
	SPI_PORT.OUT &= ~SPI_SS;
	SPI0.DATA = pck1;
	loop_until_bit_is_set(SPI0.INTFLAGS, SPI_IF_bp);
	SPI0.DATA = pck2;
	loop_until_bit_is_set(SPI0.INTFLAGS, SPI_IF_bp);
	// set CS pin high
	SPI_PORT.OUT |= SPI_SS;
	
	return SPI0.DATA;
}

void init_AC0(void) {
	AC0.CTRLA = AC_ENABLE_bm;
	// compare pins PE0 (0, AINP1) and PD7 (0, AINN2)
	AC0.MUXCTRL = (1 << AC_MUXPOS_gp) | (2 << AC_MUXNEG_gp);
}

uint16_t read_AC0(void) {
	return !(AC0.STATUS & AC_CMPSTATE_bm);
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

float read_voltage_ADC0(uint16_t ain) {
	if (ain == ADC_MUXPOS_AIN8_gc || ain == ADC_MUXPOS_AIN7_gc) ADC0.MUXPOS = ain;
	float voltage = (float)(ADC0.RES)/4096 * 3.30;
	ADC0.COMMAND = ADC_STCONV_bm;
	return voltage;
}

void init_TCA0(void) {
	// initialize timer A0 with period of 1ms
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA0.SINGLE.PER = 249;
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

volatile uint16_t millis = 0;

int main(void)
{
    /* Replace with your application code */
	
	init_clock();
	init_TCA0();
	init_ADC0();
	init_AC0();
	LCDinitialize();
	
	sei();
    while (1) 
    {
		if (millis >= 100) {
			millis = 0;
			LCDclr();
			LCDstring("hello world");
		}
    }
}

ISR(TCA0_OVF_vect) {
	if (millis < UINT16_MAX) millis++;
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

