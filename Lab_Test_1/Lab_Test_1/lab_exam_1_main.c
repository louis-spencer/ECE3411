#include <avr/io.h>
#define F_CPU 4000000UL
#include <util/delay.h>

void delay(double time_ms) {
	for (int i = 0; i < time_ms; i++) {
		_delay_ms(1);
	}
}

void blink_LED(char num_bm, char dc) {
	double period_ms = 1000 / 4;
	for (int i = 0; i < 4; i++) {
		PORTD.OUT |= num_bm;
		delay(period_ms * dc / 100);
		PORTD.OUT &= ~num_bm;
		delay(period_ms * (100 - dc) / 100);
	}
}

int main(void)
{
	// enable output on PORTD, output low
    PORTD.DIR = 0xff;
	PORTD.OUT = 0x00;
	
	// enable input on PORTB, use on-board button
	PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
	PORTB.DIR = 0x00;
	
	// duty cycle in percentage
	int dc = 50;
	char dir = 1;
	
	// number bitmask
	char num_bm = (1 << 2);
	
	char seq = 0x00;
	
	unsigned char btn_handled2 = 0;
	unsigned char btn_handled5 = 0;
	
	// BTN1 is on-board and BTN2 is external 
	
    while (1) 
    {	
		
		// BTN2 external, 1
		if ( !(PORTB.IN & PIN5_bm) ) {
			if (!btn_handled5) {
				dir = -1*dir;
				
				seq = seq << 1;
				
				btn_handled5 = 1;
			}
		} else {
			btn_handled5 = 0;
		}
		
		// BTN1 on-board, 0
		if ( !(PORTB.IN & PIN2_bm) ) {
			if (!btn_handled2) {
				if (dc == 25) {
					dc = 50;
				} else if (dc == 50) {
					dc = 25;
				}
				
				// shift first then reset
				seq = seq << 1;
				seq |= 1;
				
				btn_handled2 = 1;
			}
		} else {
			btn_handled2 = 0;
		}
		
		if ((seq & 0x0f) == 0b00001101) {
			seq = 0;
			num_bm = 0;
			dir = 1;
		}
		
		blink_LED(num_bm, dc);
		
		num_bm += dir;
		if (num_bm > 0xff) num_bm = 0;
    }
}

