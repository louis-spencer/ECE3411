#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>

void blink_led_cycle(char n) {
	// 2Hz for 2 seconds
	for (int i = 0; i < 4; i++) {
		VPORTD.OUT |= (1 << n);
		_delay_ms(250);
		VPORTD.OUT &= ~(1 << n);
		_delay_ms(250);
	}
	// 
	for (int i = 0; i < 8; i++) {
		VPORTD.OUT |= (1 << n);
		_delay_ms(125);
		VPORTD.OUT &= ~(1 << n);
		_delay_ms(125);
	}
}

int main(void)
{
	VPORTD.DIR = 0xff;
	VPORTD.OUT = 0x00;
	
	char pd_n = 0;
	
	//PORTB.DIRSET = PIN3_bm;
	
	while (1) {
		
		blink_led_cycle(pd_n);
		pd_n += 1;
		if (pd_n >= 8) pd_n = 0;
		
		/*PORTB.OUTSET = PIN3_bm;
		VPORTD.OUT = 0xff;
		_delay_ms(1000);
		PORTB.OUTCLR = PIN3_bm;
		VPORTD.OUT = 0x01;
		_delay_ms(1000);*/
		
		/*if (VPORTD.OUT == 0x00) {
			count = 0;
			VPORTD.OUT = ~count;
			_delay_ms(10);
		} else {
			count += 1;
			VPORTD.OUT = ~count;
			_delay_ms(10);
		}*/
	}
	return (0);
}

