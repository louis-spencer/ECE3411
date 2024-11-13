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


#define F_CPU 16000000UL

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


volatile char pckn = 1;
volatile uint8_t pot_pck1 = 0;
volatile uint8_t pot_pck2 = 0;


void SPI_Master_init(void) {
	// set MOSI, SCK, SS as outputs
	SPI_PORT.DIR = SPI_MOSI | SPI_SCK | SPI_SS;
	// Enable SPI, Host, set clock rate to 2 * f_clk / 128 = f_clk / 64 = 250kHz
	SPI0.CTRLA = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESC_DIV64_gc;
	// check datasheet, 1 - disable the client select line when operating as SPI host
	SPI0.CTRLB |= SPI_SSD_bm;

	SPI0.INTCTRL |= SPI_IE_bm;
	// SPI0.INTCTRL |= SPI_IE_bm | SPI_TXCIE_bm;
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
	SPI_Master_Transceiver(pck1);
	SPI_Master_Transceiver(pck2);
	// set CS pin high
	SPI_PORT.OUTSET |= SPI_SS;
	
	return SPI0.DATA;
}

void set_voltage_MCP4131(uint8_t voltage) {
	uint16_t voltage_cmd = (uint16_t)(128.0 * voltage / 3.3);
	//(void)SPI_MCP4131_RTX(0x00, 0x00, voltage_cmd);
	pckn = 1;
	uint16_t tx_data = 0x0000 | voltage_cmd;
	pot_pck2 = tx_data & 0x00ff;
	SPI_PORT.OUTCLR |= SPI_SS;
	SPI0.DATA = tx_data >> 8;
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
	return read_voltage_ADC0(ADC_MUXPOS_AIN7_gc);
}

float read_voltage_AnalogPot(void) {
	return read_voltage_ADC0(ADC_MUXPOS_AIN8_gc);
}

//---------------------Timer A0 (TCA0)---------------------
void init_TCA0(void) {
	// initialize timer A0 with period of 1ms
	TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
	TCA0.SINGLE.PER = 249;
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

//---------------------Button---------------------
#define DEBOUNCE_TIME 10
#define BTN2 (!(VPORTB.IN & PIN2_bm))

void init_btn(void) {
	PORTB.PIN2CTRL |= PORT_ISC_RISING_gc | PORT_PULLUPEN_bm;
	PORTB.DIRCLR = PIN2_bm;
}

//---------------------EEPROM---------------------
#define EEPROM_VOLT_ADDR (uint8_t*)0

volatile char input;
volatile uint16_t millis = 0;

volatile int tc_btn2 = 0;
volatile char btn2_flag = 0;

float V_digital = 0.00;
volatile uint8_t voltage_cmd = 0;

int main(void)
{
	init_clock();
	init_TCA0();
	init_ADC0();
	init_btn();
	
	//init_AC0();
	SPI_Master_init();
	
	LCDinitialize();
	uart_init(3, 9600, NULL);
	USART3.CTRLA |= USART_RXCIE_bm;
	SPI0.INTCTRL |= SPI_IE_bm;
	
	sei();
	printf("init done\n");
	
	if (eeprom_read_byte(EEPROM_VOLT_ADDR) != 0xff) {
		voltage_cmd = eeprom_read_byte(EEPROM_VOLT_ADDR);
		printf("in eeprom: %d\n", voltage_cmd);
	}
	
	while (1) 
    {	
		if (millis >= 100) {
			millis = 0;
			LCDclr();
			
			if (input > 0) {
				cli();
				if (input >= 0x30 && input <= 0x33) {
					voltage_cmd = (uint8_t)(input-0x30);
					printf("%d\n", voltage_cmd);
					
				}
				if (input > 0) input = 0;
				sei();
			}
			
			set_voltage_MCP4131(voltage_cmd);
			
			V_digital = read_voltage_DigitalPot();
			char strbuf[6];
			dtostrf(V_digital, 2, 4, strbuf);
			
			LCDstring("pot: ");
			LCDstring(strbuf);
			LCDstring("V");
			
		}	
		
		if (btn2_flag) {
			if (!BTN2) {
				btn2_flag = 0;
				eeprom_write_byte(EEPROM_VOLT_ADDR, voltage_cmd);
				printf("voltage saved to eeprom\n");
			}
		}
			
    }
}

ISR(TCA0_OVF_vect) {
	if (millis < UINT16_MAX) millis++;
	if (tc_btn2 < INT_MAX) tc_btn2++;
	if (tc_btn2 >= DEBOUNCE_TIME && BTN2) btn2_flag = 1; 
	
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

ISR(PORTB_PORT_vect) {
	if (PORTB.INTFLAGS & PIN2_bm) {
		PORTB.INTFLAGS |= PIN2_bm;
		if (tc_btn2 >= DEBOUNCE_TIME) tc_btn2 = 0;
	}
}

ISR(USART3_RXC_vect) {
	input = USART3.RXDATAL;
}

ISR(SPI0_INT_vect) {
	SPI0.INTFLAGS |= SPI_IF_bm;
	
	if (pckn == 1) {
		pckn = 2;
		//SPI0.DATA = data >> 8;
		SPI0.DATA = pot_pck2;
	} else if (pckn == 2) {
		//SPI0.DATA = data & 0x00ff;

		SPI_PORT.OUTSET |= SPI_SS;
		pckn = 1;
	}
}
