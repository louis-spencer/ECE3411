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
	// 4Hz for 2 seconds
	for (int i = 0; i < 8; i++) {
		VPORTD.OUT |= (1 << n);
		_delay_ms(125);
		VPORTD.OUT &= ~(1 << n);
		_delay_ms(125);
	}
}

int main(void)
{
    // initialize virtual PORTD
	VPORTD.DIR = 0xff;
	VPORTD.OUT = 0x00;
	
    // PORTD PIN_N
	char pd_n = 0;
	
	while (1) {
		blink_led_cycle(pd_n);
		pd_n += 1;
		if (pd_n >= 8) pd_n = 0;
	}
	return (0);
}

