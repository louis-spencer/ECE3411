#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <util/twi.h>
#include "LIS331.h"
#include "i2c.h"

/**
 * Library for communicating with the h3lis331dl Sparkfun triple-axis accelerometer
 * Built off of the provided i2c code
 * Allows reading and writing multiple registers of the accelerometer
 * Heavily inspired by existing Sparkfun library wfor working with accelerometer
 */


int LIS331_write(uint8_t reg_addr, uint8_t* data, uint8_t len) {
  // Write data to accelerometer register
  // Write I2C address first, W mode
  TWI_Address(ACC_ADDR, TW_WRITE);
  // Write register address to write to
  TWI_Transmit_Data(reg_addr);
  // Transmit data
  for (int i = 0; i < len; i++) {
    TWI_Transmit_Data(data[i]);
  }
  TWI_Stop();
  return 0;
}

int LIS331_read(uint8_t reg_addr, uint8_t* data, uint8_t len) {
  // Read data from accelerometer register
  // Open W mode first, write register to read from
  TWI_Host_Write(ACC_ADDR, reg_addr);
  // Open R mode to read from register
  TWI_Address(ACC_ADDR, TW_READ);
  for (int i=0; i<len-1; i++) {
		// Wait for Read Interrupt Flag.
		loop_until_bit_is_set(TWI0.MSTATUS, TWI_RIF_bp);

		data[i] = TWI0.MDATA;

		// Respond with ACK
		TWI0.MCTRLB &= (~TWI_ACKACT_bm);
		TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;
	}

	data[len-1] = TWI_Receive_Data();

	TWI_Stop();
  return 0;
}


int LIS331_write_reg(uint8_t reg_addr, uint8_t data) {
  // more abstracted, single register
  return LIS331_write(reg_addr, &data, 1);
}

int LIS331_read_reg(uint8_t reg_addr, uint8_t* data) {
  // more abstracted, single register
  return LIS331_read(reg_addr, data, 1);
}

uint8_t LIS331_ret_reg(uint8_t reg_addr) {
  // literally just read_reg but with a return statement 
  // (as opposed to err code)
  uint8_t data;
  (void)LIS331_read_reg(reg_addr, &data);
  return data;
}

int LIS331_init(void) {
  // initialize accelerometer with:
  // x,y,z axes enabled
  // no hi pass filter
	TWI_Host_Init(0, true);
	return LIS331_write_reg(CTRL_REG1, 0b00100111);
}

int LIS331_readX(int16_t* x) {
  // reads X axis acceleration
  uint8_t x_l, x_h;
  LIS331_read_reg(OUT_X_L, &x_l);
  LIS331_read_reg(OUT_X_H, &x_h);

  #if (INVERTED)
    *x = -1*REGS_TO_DATA(x_l, x_h);
  #else
    *x =    REGS_TO_DATA(x_l, x_h);
  #endif       
  
  return 0;
}

int LIS331_readY(int16_t* y) {
  // reads Y axis acceleration
  uint8_t y_l, y_h;
  LIS331_read_reg(OUT_Y_L, &y_l);
  LIS331_read_reg(OUT_Y_H, &y_h);


  #if (INVERTED)
    *y = -1*REGS_TO_DATA(y_l, y_h);
  #else
    *y =    REGS_TO_DATA(y_l, y_h);
  #endif

  return 0;
}

int LIS331_readZ(int16_t* z) {
  // reads Z axis acceleration
  uint8_t z_l, z_h;
  LIS331_read_reg(OUT_Z_L, &z_l);
  LIS331_read_reg(OUT_Z_H, &z_h);
  
  #if (INVERTED)
    *z = -1*REGS_TO_DATA(z_l, z_h);
  #else
    *z =    REGS_TO_DATA(z_l, z_h);
  #endif

  return 0;
}

int LIS331_readAxes(int16_t* x, int16_t* y, int16_t* z) {
  // reads X,Y,Z axis accelerations
  uint8_t data[6];
  uint8_t addr = OUT_X_L;

  for (int i = 0; i < 6; i++) {
    LIS331_read_reg(addr, &data[i]);
    addr++;
  }

  #if (INVERTED)
    *x = -1*REGS_TO_DATA(data[0], data[1]);
    *y = -1*REGS_TO_DATA(data[2], data[3]);
    *z = -1*REGS_TO_DATA(data[4], data[5]);
  #else
    *x =    REGS_TO_DATA(data[0], data[1]);
    *y =    REGS_TO_DATA(data[2], data[3]);
    *z =    REGS_TO_DATA(data[4], data[5]);
  #endif

  return 0;
}

// unused
float LIS331_convertToG(uint8_t full_scale, int16_t reading) {
  // converts reading to G (9.8 m/s^2)
  return ((float)full_scale * (float)reading / 2047.0);
}