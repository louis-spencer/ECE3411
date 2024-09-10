#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>

#define BM_(x) ((1 << x))


void delay(double time_ms) {
	// delay function to accept arbitrary times
	for (int i = 0; i < time_ms; i++) {
		_delay_ms(1);
	}
}

void blink_cycle(char pin, double period_ms) {
	// on/off cycle for led with specific period (1/f)
	PORTD.OUT |= (1 << pin);
	delay(period_ms/2);
	PORTD.OUT &= ~(1 << pin);
	delay(period_ms/2);
}

int main(void)
{
    // configure PORTB to use on board button
	// set direction to input
	PORTB.PIN2CTRL |= PORT_PULLUPEN_bm;
    PORTB.DIR = 0x00;
	
	// set PORTD to output and low
	PORTD.DIR = 0xff;
	PORTD.OUT = 0x00;
	
	// current frequency (Hz) and period (ms)
	int curr_freq = 3;
	double curr_per_ms = 1000 / curr_freq;
	
	// setup variables to handle button presses
	unsigned char btn_handled2 = 0;
	unsigned char btn_handled5 = 0;
	unsigned char btn_handled25 = 0;
	
	// useful variables for direction and position
	char dir = 1;
	char pin = 3;
	
	
	while (1) 
    {
		// blink the leds at current period
		blink_cycle(pin, curr_per_ms);
		
		// one shot pin2 button BTN1
		// BTN1 increase frequency
		if ( !(PORTB.IN & PIN2_bm) ) {
			if (!btn_handled2) {
				// add functionality
				// update period once instead of every loop
				curr_freq++;
				curr_per_ms = 1000 / curr_freq;
				btn_handled2 = 1;
			}
		} else {
			btn_handled2 = 0;
		}
		
		// one shot pin5 button BTN2
		// BTN2 decrease frequency
		if ( !(PORTB.IN & PIN5_bm) ) {
			if (!btn_handled5) {
				// add functionality
				// update period once instead of every loop
				curr_freq--;
				if (curr_freq < 1) curr_freq = 1;
				curr_per_ms = 1000 / curr_freq;
				btn_handled5 = 1;
			}
		} else {
			btn_handled5 = 0;
		}
		
		// one shot pin2 and pin5 BTN1+BTN2
		// BTN1 and BTN2 change position
		// improve this by using timer and interrupts
		if ( !(PORTB.IN & (PIN5_bm | PIN2_bm)) ) {
			if (!btn_handled25) {
				// if edge case, switch direction
				if (pin >= 7 || pin <= 0) dir = -dir;
				pin += dir;
				btn_handled25 = 1;
			}
		} else {
			btn_handled25 = 0;	
		}
    }
}

