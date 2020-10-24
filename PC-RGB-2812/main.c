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
#include <avr/pgmspace.h>
#include "WS2812.h"

void Function1();
extern void Function2();
u16 translatebuffer(u16);


ISR(INT0_vect)
{
	
}

u8 buf[NUM_LEDS];
u16 bufindex;

int main(void)
{
	u16 halfbound;
	u8 tempR;
	u8 tempG;
	u8 tempB;
	u8 Rval;
	u8 Gval;
	u8 Bval;
	u16 i;
	u16 y;
	u8 Rsw;
	u8 Gsw;
	u8 Bsw; // count up 1 or down 0
	u8 Rdiv = 4;
	u8 Gdiv = 8;
	u8 Bdiv = 8;
	DDRC |= (1 << DDC2); // set PD3 to OUTPUT

	//Start 100% blue
	Rval=0xFF;
	Gval=0x00;
	Bval=0x00;
	//Start all colors idle
	Rsw=1;
	Gsw=1;
	Bsw=1;
	while (1) {
		
		/* Build buffer from color data */
	
		for (i=0;i<NUM_LEDS;i++)
			buf[i]=0;
		
		// determine values based on current values and flags
		
		// ramp red up if: blue is full green is 0
		if (Bval==0xFF && Gval == 0x00) {Rsw = 2;};
		// ramp red dn if: blue 0 green full
		if (Bval==0x00 && Gval == 0xFF) {Rsw = 0;};
		// ramp green up if: red full blue 0
		if (Rval==0xFF && Bval == 0x00) {Gsw = 2;};
		// ramp green dn if: red 0 blue full
		if (Rval==0x00 && Bval == 0xFF) {Gsw = 0;};
		// ramp blue up if: green full red 0
		if (Gval==0xFF && Rval == 0x00) {Bsw = 2;};
		// ramp blue dn if: red full green 0
		if (Rval==0xFF && Gval == 0x00) {Bsw = 0;};
		
			
		// increment decrement or leave the same:
		// switch key: 0 dec, 1 leave same, 2 inc
		if (Rsw == 2 && Rval != 0xFF)
			{Rval = Rval+1;};
		if (Rsw == 0 && Rval != 0x00)
			{Rval = Rval-1;};
		if (Gsw == 2 && Gval != 0xFF)
			{Gval = Gval+1;};
		if (Gsw == 0 && Gval != 0x00)
			{Gval = Gval-1;};
		if (Bsw == 2 && Bval != 0xFF)
			{Bval = Bval+1;};
		if (Bsw == 0 && Bval != 0x00)
			{Bval = Bval-1;};
			
		bufindex = 0;
		for (y=0;y<NUM_WS2812;y++)
		{
			tempR = Rval/Rdiv;
			tempG = Gval/Gdiv;
			tempB = Bval/Bdiv;
			
			buf[bufindex] = tempG;
			bufindex++;
			buf[bufindex] = tempR;
			bufindex++;
			buf[bufindex] = tempB;
			bufindex++;
			
		}
		output_grb_c2(buf, sizeof(buf));
	
		_delay_ms(10);
	
	}
}