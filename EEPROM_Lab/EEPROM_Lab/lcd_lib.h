// LCD interface for AIP31068 LCD controllers
// By John Chandy, 2024

#include <inttypes.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

void LCDinitialize(void);		//Initializes LCD
void LCDdataWrite(uint8_t);
void LCDclr(void);				//Clears LCD
void LCDhome(void);				//LCD cursor home
void LCDstring(char*);			//Outputs string to LCD
void LCDgotoXY(uint8_t, uint8_t);	//Cursor to X Y position
void LCDcopyStringFromFlash(const uint8_t*, uint8_t, uint8_t); //copies flash string to LCD at x,y
void LCDshiftRight(uint8_t);	//shift by n characters Right
void LCDshiftLeft(uint8_t);	    //shift by n characters Left
void LCDcursorOn(void);			//cursor ON
void LCDcursorOnBlink(void);	//Blinking cursor ON
void LCDcursorOff(void);		//Both underline and blinking cursor OFF
void LCDblank(void);			//LCD blank but not cleared
void LCDvisible(void);			//LCD visible
void LCDcursorLeft(uint8_t);	//shift cursor left by n
void LCDcursorRight(uint8_t);	//shift cursor right by n
uint8_t TWI_RXData(void);
void TWI_Host_Write(uint8_t, uint8_t);
uint8_t TWI_Host_Read(uint8_t);