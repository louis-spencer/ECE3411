#ifndef I2C_H
#define I2C_H

#include <inttypes.h>
#include <stdbool.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif



//void TWI_Stop(void);
//void TWI_Address(uint8_t, uint8_t);
//int TWI_TXDATA(uint8_t);
//uint8_t TWI_RXDATA(void);
//void TWI_Host_Init(uint32_t, bool);
//void TWI_Host_Write(uint8_t, uint8_t);
//uint8_t TWI_Host_Read(uint8_t);

void TWI_Host_Init(uint32_t, bool);
void TWI_Stop(void);
void TWI_Address(uint8_t, uint8_t);
int TWI_Transmit_Data(uint8_t);
uint8_t TWI_Receive_Data(void);
void TWI_Host_Write(uint8_t, uint8_t);
void TWI_Host_Write_16bits(uint8_t, uint16_t);
void TWI_Host_Write_Bytes(uint8_t, uint8_t*, int);
uint8_t TWI_Host_Read(uint8_t);
void TWI_Host_Read_Bytes(uint8_t, uint8_t*, int);



#endif