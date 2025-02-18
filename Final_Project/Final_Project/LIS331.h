#ifndef __LIS331_H__
#define __LIS331_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define INVERTED			0       // either a 1 or a 0
#define ACC_ADDR			0x19	// according to sparkfun

#define WHO_AM_I			0x0f  // registers provided by the datasheet
#define CTRL_REG1			0x20
#define CTRL_REG2			0x21
#define CTRL_REG3			0x22
#define CTRL_REG4			0x23
#define CTRL_REG5			0x24
#define HP_FILTER_RST		0x25
#define REF_REG				0x26
#define STATUS_REG			0x27
#define OUT_X_L				0x28
#define OUT_X_H				0x29
#define OUT_Y_L				0x2a
#define OUT_Y_H				0x2b
#define OUT_Z_L				0x2c
#define OUT_Z_H				0x2d

// properly converts low and high values to useable data
#define REGS_TO_DATA(low, hi) ((low | hi << 8) >> 4)

int     LIS331_write(uint8_t reg_addr, uint8_t* data, uint8_t len);
int     LIS331_read(uint8_t reg_addr, uint8_t* data, uint8_t len);
int     LIS331_write_reg(uint8_t reg_addr, uint8_t data);
int     LIS331_read_reg(uint8_t reg_addr, uint8_t* data);
uint8_t LIS331_ret_reg(uint8_t reg_addr);
int     LIS331_init(void);
int     LIS331_readX(int16_t* x);
int     LIS331_readY(int16_t* y);
int     LIS331_readZ(int16_t* z);
int     LIS331_readAxes(int16_t* x, int16_t* y, int16_t* z);
float   LIS331_convertToG(uint8_t full_scale, int16_t reading);

#endif