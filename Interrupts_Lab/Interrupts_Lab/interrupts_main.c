#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "uart.h"

#define F_CPU 16000000UL
#include <util/delay.h>

void init_clock() {
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.OSCHFCTRLA = CLKCTRL_FRQSEL_16M_gc;
}

void delay(double time_ms) {
	// delay function to accept arbitrary times
	for (int i = 0; i < time_ms; i++) {
		_delay_ms(1);
	}
}

void blink_cycle(char pin, unsigned char freq) {
	double period_ms = 1000 / freq;
	PORTD.OUT |= (1 << pin);
	delay(period_ms/2);
	PORTD.OUT &= ~(1 << pin);
	delay(period_ms/2);
	
}

// cli() clears interrupts, sei() enables interrupts
// use kinda like pipes or threads or something

volatile char input = 0;
volatile char select_f = 0;

int main(void)
{
	init_clock();
	
	uart_init(3, 9600, NULL);
	// enable interrupt
	USART3.CTRLA |= USART_RXCIE_bm;
	
	VPORTD.DIR = 0xff;
	VPORTD.OUT = 0x00;
	
	char pin = 3;
	unsigned char freq = 5;
	unsigned char mode = 0;
	
	sei();
	printf("Do you want to change frequency or position? (F/P)\n");
	
	while (1) 
    {
		blink_cycle(pin, freq);
		
		// kind of like thread management
		if (input > 0) {
			// clear interrupts
			cli();
			printf("%c\n", input);
			// detect either F or P
			if (input == 0x46 || input == 0x66) {
				printf("Frequency: \n");
				mode = 1;
			} else if (input == 0x50 || input == 0x70) {
				printf("Position: \n");
				mode = 2;
			} else {
				// do operation based on extra input
				if (mode == 1) {
					freq = input - 0x30;
					mode = 0;
				} else if (mode == 2) {
					pin = input - 0x30;
					mode = 0;
				}
				printf("Do you want to change frequency or position? (F/P)\n");
			}
			// clear value for input
			if (input > 0) input = 0;
			sei();
		}
    }
}

// vector for usart3 RX interrupt
ISR(USART3_RXC_vect) {
	// set input to usart3 data
	// make sure to sample from DATAL
	// unless 9 bit serial used
	input = USART3.RXDATAL;
}

