/*
 * LCD.c
 *
 * Written by John Chandy Sept. 2024
 */

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "lcd_lib.h"

void init_clock()
{
    //	set internal oscillator to 16MHz
    //	CPU_CCP = CCP_IOREG_gc;
    //	CLKCTRL.OSCHFCTRLA = CLKCTRL_FRQSEL_16M_gc;

    //	use external 16MHz crystal
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
    while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

const uint8_t testing[] PROGMEM = "Testing\0";
const uint8_t clear[] PROGMEM = "Clear  \0"; 
const uint8_t cursor_on[] PROGMEM = "Cursor On\0";
const uint8_t cursor_off[] PROGMEM = "Cursor Off\0";
const uint8_t cursor_on_blink[] PROGMEM = "Cursor Blink\0";
const uint8_t home[] PROGMEM = "Cursor Home\0";
const uint8_t cursor_right[] PROGMEM = "Cursor Right 1\0";
const uint8_t cursor_left[] PROGMEM = "Cursor Left 4\0";
const uint8_t gotostring[] PROGMEM = "Goto 1,6/String\0";
const uint8_t shiftleft[] PROGMEM =  "Shift Left 5   \0";
const uint8_t shiftright[] PROGMEM = "Shift Right 3\0";
const uint8_t blank[] PROGMEM = "Blank/Visible\0";

int main(void) { 
    // Initializations:
    init_clock();
    LCDinitialize(); 
	
    while(1) {
		//char str[] = "hello world\0";
		//LCDstring(str);
		//_delay_ms(500);
		//LCDclr();
		//_delay_ms(500);
		//LCDgotoXY(3, 1);
		//LCDstring(str);
		//_delay_ms(500);
		//LCDhome();
		//LCDclr();
		//_delay_ms(500);
		//LCDcopyStringFromFlash(testing, 0, 0);
		//_delay_ms(500);
		//LCDshiftRight(5);
		//_delay_ms(500);
		//LCDshiftLeft(5);
		//_delay_ms(500);
		//LCDclr();
		//_delay_ms(500);
		
        LCDcopyStringFromFlash(testing, 0, 0);
        _delay_ms(2000); // Display message for 2 seconds 

        LCDcopyStringFromFlash(clear, 0, 0); 
        _delay_ms(2000); // Display message for 2 seconds 
        LCDclr(); 
        _delay_ms(2000); // wait for 2 seconds 

        LCDcopyStringFromFlash(cursor_on, 0, 0);
        _delay_ms(2000); // Display message for 2 seconds
        LCDcursorOn();
        _delay_ms(2000); // wait for 2 seconds
		
        LCDcopyStringFromFlash(cursor_off, 0, 0);
        _delay_ms(2000); // Display message for 2 seconds
        LCDcursorOff();
        _delay_ms(2000); // wait for 2 seconds
		
        LCDclr();
        LCDcopyStringFromFlash(cursor_on_blink, 0, 0);
        _delay_ms(2000); // Display message for 2 seconds
        LCDcursorOnBlink();
        _delay_ms(2000); // wait for 2 seconds

        LCDclr();
        LCDcopyStringFromFlash(home, 0, 0); 
        _delay_ms(2000); // Display message for 2 seconds 
        LCDhome(); 
        _delay_ms(2000); // wait for 2 seconds 
 
        LCDclr();
        LCDcursorOnBlink();
        LCDcopyStringFromFlash(cursor_right, 0, 0);
        _delay_ms(2000); // Display message for 2 seconds
        LCDcursorRight(1);
        _delay_ms(2000); // wait for 2 seconds

        LCDclr();
        LCDcopyStringFromFlash(cursor_left, 0, 0);
        _delay_ms(2000); // Display message for 2 seconds
        LCDcursorLeft(4);
        _delay_ms(2000); // wait for 2 seconds

        LCDcopyStringFromFlash(gotostring, 0, 0); 
        LCDcursorOff();
        _delay_ms(2000); // Display message for 2 seconds 
        LCDgotoXY(6, 1); 
        LCDstring("string"); 
        _delay_ms(2000); // wait for 2 seconds 
 
        LCDcopyStringFromFlash(shiftleft, 0, 0); 
        _delay_ms(2000); // Display message for 2 seconds 
        LCDgotoXY(0, 0);
        LCDstring("            ");
        LCDgotoXY(0, 0);
        LCDshiftLeft(5); 
        _delay_ms(2000); // wait for 2 seconds 
 
        LCDcopyStringFromFlash(shiftright, 5, 0); 
        _delay_ms(2000); // Display message for 2 seconds 
        LCDgotoXY(5, 0);
        LCDstring("             ");
        LCDshiftRight(3); 
        _delay_ms(2000); // wait for 2 seconds 
 
        LCDclr();
        LCDcopyStringFromFlash(blank, 0, 0); 
        _delay_ms(2000); // Display message for 2 seconds 
        LCDblank(); 
        _delay_ms(2000); // wait for 2 seconds 

        LCDvisible(); 
        _delay_ms(2000); // wait for 2 seconds 
        LCDclr();
    }
}
