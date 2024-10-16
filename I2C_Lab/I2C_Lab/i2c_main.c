#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <util/twi.h> 

#include "uart.h"

#define F_CPU 16000000UL
#define TEMPR_ADDR 0b1001000

// ---------------- clock and timers setup ----------------
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
	TCA0.SINGLE.PER = 999;
	TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
	
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm);
}

// ---------------- i2c setup ----------------
void TWI_Stop(void)
{
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}
void TWI_Address(uint8_t Address, uint8_t mode)
{
	while (1) {
		// set addr & R/W bit, starts transfer
		TWI0.MADDR = (Address << 1) | (mode);
		// Wait for Read/Write Interrupt Flag.
		uint8_t flag = (mode == TW_WRITE) ? TWI_WIF_bp : TWI_RIF_bp;
		loop_until_bit_is_set(TWI0.MSTATUS, flag);
		// if the client didn’t ack, stop the transaction
		if (TWI0.MSTATUS & TWI_RXACK_bm) {
			TWI_Stop();
		}
		// if no bus or arbitration error, all good. otherwise try it again
		if (!(TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm))) {
			break;
		}
	}
}
int TWI_Transmit_Data(uint8_t data)
{
	// start data transfer by writing to MDATA
	TWI0.MDATA = data;
	// Wait for Write Interrupt Flag.
	loop_until_bit_is_set(TWI0.MSTATUS, TWI_WIF_bp);
	// if bus or arbitration error, return error
	return ((TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) ? -1 : 0);
}
uint8_t TWI_Receive_Data()
{
	// Wait for Read Interrupt Flag.
	loop_until_bit_is_set(TWI0.MSTATUS, TWI_RIF_bp);
	uint8_t data = TWI0.MDATA;
	// Respond with NACK
	TWI0.MCTRLB |= TWI_ACKACT_bm;
	return data;
}
void TWI_Host_Init(void) {
	//TWI0.MBAUD = 95; // baud adjusted for 80kHz clock
	TWI0.MBAUD = 35;
	TWI0.MCTRLA |= TWI_ENABLE_bm;			// enable i2c
	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;	// force bus state idle
}
void TWI_Host_Write(uint8_t Address, uint8_t Data)
{
	TWI_Address(Address, TW_WRITE);
	TWI_Transmit_Data(Data);
	TWI_Stop();
}
uint8_t TWI_Host_Read(uint8_t Address)
{
	TWI_Address(Address, TW_READ);
	uint8_t data = TWI_Receive_Data();
	TWI_Stop();
	return data;
}

volatile int tc = 0;

int main(void)
{
    init_clock();
	init_TCA0();
	uart_init(3, 9600, NULL);
	TWI_Host_Init();
	PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
	PORTA.PIN3CTRL |= PORT_PULLUPEN_bm;
	
	printf("setup done\n");
	sei();
    while (1) 
    {
		if (tc >= 500) {
			tc = 0;
			TWI_Host_Write(TEMPR_ADDR, 0);
			int data = TWI_Host_Read(TEMPR_ADDR);
			printf("temperature: %d\n", data);
			//printf("wtf");
		}
    }
}

ISR(TCA0_OVF_vect) {
	if (tc < INT_MAX) tc++;
	TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}

