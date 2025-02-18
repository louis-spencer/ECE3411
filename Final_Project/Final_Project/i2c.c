#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void TWI_Host_Init(uint32_t scl, bool enable_pullups)
{
    //TWI0.MBAUD = F_CPU/(2*scl) - (5 + F_CPU * 10 / 2000000000);		// assume rise time of 10ns
    TWI0.MBAUD = 35;
	if (enable_pullups) {
		PORTA.DIRSET |= PIN2_bm | PIN3_bm;
        PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;	// enable internal pull-ups if
        PORTA.PIN3CTRL |= PORT_PULLUPEN_bm;	// there is no external pull-up
    }

    TWI0.MCTRLA |= TWI_ENABLE_bm;
    TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;
}

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

		// if the client didn't ack, or bus or arbitration error,
		// stop the transaction and try again
		if ((TWI0.MSTATUS & TWI_RXACK_bm) ||
		    (TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm))) {
			TWI_Stop();
			TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;
		} else {
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

uint8_t TWI_Receive_Data(void)
{
    // Wait for Read Interrupt Flag.
    loop_until_bit_is_set(TWI0.MSTATUS, TWI_RIF_bp);

    uint8_t data = TWI0.MDATA;

    // Respond with NACK
    TWI0.MCTRLB |= TWI_ACKACT_bm;

    return data;
}

void TWI_Host_Write(uint8_t Address, uint8_t Data)
{
    TWI_Address(Address, TW_WRITE);
    TWI_Transmit_Data(Data);
    TWI_Stop();
}

void TWI_Host_Write_16bits(uint8_t Address, uint16_t Data)
{
    TWI_Address(Address, TW_WRITE);
    TWI_Transmit_Data(Data >> 8);
    TWI_Transmit_Data(Data & 0xff);
    TWI_Stop();
}

void TWI_Host_Write_Bytes(uint8_t Address, uint8_t* Data, int numbytes)
{
    TWI_Address(Address, TW_WRITE);
    for (int i=0; i<numbytes; i++) {
        TWI_Transmit_Data(Data[i]);
    }
    TWI_Stop();
}

uint8_t TWI_Host_Read(uint8_t Address)
{
    TWI_Address(Address, TW_READ);
    uint8_t data = TWI_Receive_Data();
    TWI_Stop();
    return data;
}

void TWI_Host_Read_Bytes(uint8_t Address, uint8_t* data, int numbytes)
{
	TWI_Address(Address, TW_READ);

	for (int i=0; i<numbytes-1; i++) {
		// Wait for Read Interrupt Flag.
		loop_until_bit_is_set(TWI0.MSTATUS, TWI_RIF_bp);

		data[i] = TWI0.MDATA;
		// Respond with ACK
		TWI0.MCTRLB &= (~TWI_ACKACT_bm);
		TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;
	}

	data[numbytes-1] = TWI_Receive_Data();

	TWI_Stop();
}