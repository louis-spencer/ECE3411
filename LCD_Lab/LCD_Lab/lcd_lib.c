#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "lcd_lib.h"
#include <util/twi.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>

#define LCD_ADDR 0x3e
#define LED_ADDR 0x6b
#define CTRL_BYTE_CMD (0x00 | 0x80 | 0x40) // (1<<6) | (1<<7)
#define CTRL_BYTE_WRITE (0x00 | 0x40)

void TWI_Stop(void) {
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

void TWI_Address(uint8_t addr, uint8_t mode) {
	while (1) {
		// set addr and R/W bit, start transfer
		TWI0.MADDR = (addr << 1) | (mode);
		// wait for R/W interrupt flag
		uint8_t flag = (mode == TW_WRITE) ? TWI_WIF_bp : TWI_RIF_bp;
		loop_until_bit_is_set(TWI0.MSTATUS, flag);
		// if client didn't ACK, stop transaction
		if (TWI0.MSTATUS & TWI_RXACK_bm) TWI_Stop();
		// if arbitration or bus error, try again
		if (!(TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm))) break;
	}
}

int TWI_TXData(uint8_t data) {
	// start data transfer writing to MDATA
	TWI0.MDATA = data;
	// wait for Write Interrupt Flag
	loop_until_bit_is_set(TWI0.MSTATUS, TWI_WIF_bp);
	// return error if bus or arbitration error
	return ((TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) ? -1 : 0);
}

int TWI_2TXData(uint8_t addr, uint8_t data1, uint8_t data2) {
	TWI_Address(addr, TW_WRITE);
	TWI_TXData(data1);
	TWI_TXData(data2);
	TWI_Stop();
}

void TWI_Host_Init(void) {
	TWI0.MBAUD = 35;
	TWI0.MCTRLA |= TWI_ENABLE_bm;
	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;
}

void LCDsendCommand(uint8_t cmd) {
	TWI_2TXData(LCD_ADDR, 0x80, cmd);
}

void LCDsendData(uint8_t data) {
	TWI_2TXData(LCD_ADDR, 0x40, data);
}

void LCDinitialize(void) {
	TWI_Host_Init();
	// reset LED in 0x2f
	TWI_2TXData(LED_ADDR, 0x2f, 0x00);
	_delay_us(50);
	// enable LED in shutdown register 0x00
	TWI_2TXData(LED_ADDR, 0x00, 0b00100000);
	_delay_us(50);
	// max out PWM register for light in 0x04
	TWI_2TXData(LED_ADDR, 0x04, 0xff);
	_delay_us(50);
	// update the LED in 0x07
	TWI_2TXData(LED_ADDR, 0x07, 0x00);
	
	_delay_ms(15);
	
	LCDsendCommand(0x0c);
	_delay_us(10);
	LCDsendCommand(0x38);
	_delay_us(10);
	LCDsendCommand(0x01);
	_delay_ms(2);
	LCDsendCommand(0x04 | 0x02);
	
	//TWI_2TXData(LCD_ADDR, CTRL_BYTE_CMD, 0b00101100);
	//_delay_us(40);
	//TWI_2TXData(LCD_ADDR, CTRL_BYTE_CMD, 0b00001111);
	//_delay_us(40);
	//TWI_2TXData(LCD_ADDR, CTRL_BYTE_CMD, 0x01);
	//_delay_ms(2);
	//TWI_2TXData(LCD_ADDR, CTRL_BYTE_CMD, 0x02);
}

void LCDdataWrite(uint8_t data) {
	LCDsendData(data);
}

void LCDclr(void) {
	LCDsendCommand(0x01);
	_delay_ms(2);
}

void LCDhome(void) {
	LCDsendCommand(0x02);
	_delay_ms(2);
}

void LCDstring(char* str) {
	int i = 0;
	char p;
	while (str[i] != '\0') {
		LCDdataWrite(str[i]);
		_delay_us(43);
		i++;
	}
}

void LCDgotoXY(uint8_t x, uint8_t y) {
	if (y == 0) x |= 0x80;
	else x |= 0xc0;
	LCDsendCommand(x);
}

void LCDcopyStringFromFlash(const uint8_t* f_str, uint8_t x, uint8_t y) {
	LCDgotoXY(x, y);
	int i = 1;
	uint8_t f_byte = pgm_read_byte(&f_str[0]);
	while (f_byte != '\0') {
		LCDdataWrite(f_byte);
		f_byte = pgm_read_byte(&f_str[i]);
		i++;
	}
	LCDhome();
}

void LCDshiftRight(uint8_t n) {
	for (int i = 0; i < n; i++) {
		LCDsendCommand(0x10 | 0x08 | 0x04);
		_delay_us(40);
	}
}

void LCDshiftLeft(uint8_t n) {
	for (int i = 0; i < n; i++) {
		LCDsendCommand(0x10 | 0x08);
		_delay_us(40);
	}
}

void LCDcursorOn(void) {
	LCDsendCommand(0x08 | 0x04 | 0x02);
	_delay_us(40);
}

void LCDcursorOnBlink(void) {
	LCDsendCommand(0x08 | 0x04 | 0x02 | 0x01);
	_delay_us(40);
}

void LCDcursorOff(void) {
	LCDsendCommand(0x08 | 0x04);
	_delay_us(40);
}

void LCDblank(void) {
	LCDsendCommand(0x08);
	_delay_us(40);
}

void LCDvisible(void) {
	LCDsendCommand(0x08 | 0x04);
	_delay_us(40);
}

void LCDcursorLeft(uint8_t n) {
	for (int i = 0; i < n; i++) {
		LCDsendCommand(0x10);
		_delay_us(40);
	}
}

void LCDcursorRight(uint8_t n) {
	for (int i = 0; i < n; i++) {
		LCDsendCommand(0x14);
		_delay_us(40);
	}
}
