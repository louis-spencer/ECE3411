#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include "uart.h"

#define F_CPU 16000000UL
void init_clock() {
	// make sure to configure the clock with init function
	// setup appropriate registers when F_CPU 16MHz
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.OSCHFCTRLA = CLKCTRL_FRQSEL_16M_gc;
}

#include <util/delay.h>
// AVR128DB48 at COM5


void delay(double time_ms) {
	// delay function to accept arbitrary times
	for (int i = 0; i < time_ms; i++) {
		_delay_ms(1);
	}
}

void blink_cycle(char pin, char freq) {
	double period_ms = 1000 / freq;
	for (int i = 0; i < 5000 / period_ms; i++) {
		PORTD.OUT |= (1 << pin);
		delay(period_ms/2);
		PORTD.OUT &= ~(1 << pin);
		delay(period_ms/2);
	}
}


int main(void)
{
	init_clock();
	uart_init(3, 9600, NULL);
	
	PORTD.DIR = 0xff;
	PORTD.OUT = 0x00;
	
	char input[20];
	int value;
	
	char curr_freq = 2;
	char pos = 2;
	
    while (1) 
    {
		blink_cycle(pos, curr_freq);
		_delay_ms(100);
		printf("Do you want to change frequency or position? (F/P)\n");
		while( scanf("%s", input) < 0 );
		
		if (input[0] == 'f') {
			// change frequency
			printf("Frequency: ");
			while( scanf("%d", &value) < 0 );
			curr_freq = (char)value;
			// printf('\n');
		} else if (input[0] == 'p') {
			// change position
			printf("Position: ");
			while( scanf("%d", &value) < 0 );
			pos = (char)value;
			// printf('\n');
		}
    }
}

