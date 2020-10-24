//
// AVR_2812
// 6 WS2812B LEDs
// 16MHz internal osc
//

#define F_CPU   16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h> // memset()
#include "WS2812.h"

extern void Function2();


ISR(INT0_vect) {
	int_flag = -1;
}

int main(void) {
	u8 buf[NUM_LEDS];
	
	DDRD &= ~(1 << DDD2);     // Clear the PD2 pin
	// PD2 (PCINT0 pin) is now an input
	PORTD |= (1 << PORTD2);    // turn On the Pull-up
	// PD2 is now an input with pull-up enabled
	EICRA |= (1 << ISC00);    // set INT0 to trigger on ANY logic change
	EIMSK |= (1 << INT0);     // Turns on INT0
	sei();                    // turn on interrupts
	
	DDRD |= (1 << DDD3); // set to OUTPUT
	/*pinon(&PORTD,PORTD3); */ // Init pin
	
	/* make sure LEDs init as OFF */
	memset(buf, 0, sizeof(buf));
	output_grb(buf, sizeof(buf));
	
	
	int function_select = 1;
	
	while (1) {
	int_flag = 0;

	//output_grb(buf, sizeof(buf));
	switch (function_select) {
		case 1:
			// Illustrate an internal function
			while (int_flag == 0) {
				set_color(buf, 1, 100, 0, 0);
				output_grb(buf, sizeof(buf));
			}
			function_select=2;
		break;
		case 2:
			// call a function in a different compile unit
			Function2();
			function_select=3;
		break;
		case 3:
			// no function, just rotating back to case beginning
			while (int_flag == 0) {
				;
			}
			function_select=1;
		break;
		
	}
	
	}
}

