#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "lcd_lib.h"
#include "uart.h"

// whoever is reading this pls help me get a gf thx 

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

void printf_reg8(uint8_t reg) {
	printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(reg));
} 

#define F_CPU 16000000UL
#define DIGIPOT_MAX (0x80)
#define DIGIPOT_MIN (0x00)

//---------------------Clock---------------------
void init_clock(void) {
	// initialize external 16MHz clock
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	// wait for clock to startup
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

//---------------------Serial Peripheral Interface (SPI)---------------------
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
	SPI_PORT.OUTCLR |= SPI_SS;
	SPI0.DATA = pck1;
	loop_until_bit_is_set(SPI0.INTFLAGS, SPI_IF_bp);
	SPI0.DATA = pck2;
	loop_until_bit_is_set(SPI0.INTFLAGS, SPI_IF_bp);
	// set CS pin high
	SPI_PORT.OUTSET |= SPI_SS;
	
	return SPI0.DATA;
}

//---------------------Analog Comparator (AC0)---------------------
void init_AC0(void) {
	AC0.CTRLA = AC_ENABLE_bm;
	// compare pins PE0 (0, AINP1) and PD7 (0, AINN2)
	AC0.MUXCTRL = (1 << AC_MUXPOS_gp) | (2 << AC_MUXNEG_gp);
}

uint16_t read_AC0(void) {
	// returns 0 if V_digital > V_analog
	// returns 1 if V_digital < V_analog
	return !(AC0.STATUS & AC_CMPSTATE_bm);
}

//---------------------Analog-Digital Converter (ADC0)---------------------
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

float read_voltage_ADC0(uint8_t ain) {
	// AIN8 is PE0, AIN7 is PD7
	// AIN8 is analog pot, AIN7 is digital pot
	if (ain == ADC_MUXPOS_AIN8_gc || ain == ADC_MUXPOS_AIN7_gc) ADC0.MUXPOS = ain;
	float voltage = (float)(ADC0.RES)/4096 * 3.30;
	ADC0.COMMAND = ADC_STCONV_bm;
	return voltage;
}

float read_voltage_DigitalPot(void) {
	return read_voltage_ADC0(ADC_MUXPOS_AIN8_gc);
}

float read_voltage_AnalogPot(void) {
	return read_voltage_ADC0(ADC_MUXPOS_AIN7_gc);
}

//---------------------Timer A0 (TCA0)---------------------
void init_TCA0(void) {
	// initialize timer A0 with period of 1ms
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA0.SINGLE.PER = 249;
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

//---------------------Binary Search Function---------------------
void binary_search(void) {
	uint16_t upper_bound = DIGIPOT_MAX;
	uint16_t lower_bound = DIGIPOT_MIN;
	uint16_t mid = (DIGIPOT_MAX-DIGIPOT_MIN) / 2;
	
	for (int i = 0; i < 9; i++) {
		(void)SPI_MCP4131_RTX(0x00, 0x00, mid);
			
		if (read_AC0()) {
			// if digital < analog
			upper_bound = mid + 0x01;
				
		} else {
			// if digital > analog
			lower_bound = mid > 0x01 ? mid - 0x01 : 0x00;
		}
			
		mid = lower_bound + ((upper_bound - lower_bound) / 2);
	}
}

volatile uint16_t millis = 0;

float V_delta = 0.005;
float V_analog = 0.00;
float V_digital = 0.00;

int main(void)
{
    /* Replace with your application code */
	// ^ shut up
	
	// initialize everything
	init_clock();
	init_TCA0();
	init_ADC0();
	init_AC0();
	SPI_Master_init();
	
	LCDinitialize();
	uart_init(3, 9600, NULL);
	
	sei();
	// enable interrupts and check init worked
	printf("init done\n");
    while (1) 
    {
		if (millis >= 500) {
			millis = 0;
			LCDclr();


			// get the voltage from the analog pot
			V_analog = read_voltage_AnalogPot();
			V_delta = V_analog;
			char strbuf[6];
			dtostrf(V_analog, 2, 4, strbuf);
			printf("VA:%s\t|\t", strbuf);	
			
			LCDstring("A_Pot: ");
			LCDstring(strbuf);
			LCDstring("V");
						
			
			// need to figure out how to read wiper
			printf("MID: ");
			uint16_t spi_mid = SPI_MCP4131_RTX(0x00, 0b11, 0xff);
			printf_reg8(spi_mid >> 8);
			printf_reg8((uint8_t)(spi_mid & 0x00ff));
			printf("\t|\t");
			
			
			// get the voltage from the digital pot
			V_digital = read_voltage_DigitalPot();
			dtostrf(V_digital, 2, 4, strbuf);
			printf("VD:%s", strbuf);
			printf("\t|\t");
			
			LCDgotoXY(0, 1);
			LCDstring("D_Pot: ");
			LCDstring(strbuf);
			LCDstring("V");
			
			
			// compare the voltage readings
			V_delta -= V_digital;
			V_delta = fabs(V_delta);
			
			dtostrf(V_delta, 2, 4, strbuf);
			printf("dV:%s", strbuf);
			printf("\n");
			
			// if the voltages are too different
			// perform binary search
			if (V_delta > 0.02) {
				binary_search();
			}
		}
    }
}

ISR(TCA0_OVF_vect) {
	if (millis < UINT16_MAX) millis++;
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

