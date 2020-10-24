/* FUNCTION (2): Pulse every fourth LED */
#define F_CPU   16000000

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include "WS2812.h"

int incmult(int);

void Function2 () 
{
	int mult = 0;
	u8 state = S_R;
	u8 val = 0;
	u8 first_time = 1;
	u8 buf[NUM_LEDS];
	while(int_flag == 0)
	{
		output_grb(buf, sizeof(buf));
		
		switch (state)
		{
			case S_R:
			if ((val=val+1) <= MAX)
			{
				if (!first_time)
				{
					/*set_color(buf, mult+5, val, MAX-val, MAX-val);*/
				}
				set_color(buf, mult, val, val, val);
			}
			else
			{
				first_time = 0;
				/*state = S_G;*/
				state = S_O;
				val = 0;
			}
			break;
			
			case S_O:
			if ((val=val+1) <= MAX)
			{
				set_color(buf, mult+1, 0, 0, 0);
			}
			else
			{
				state = S_R;
				val = 0; mult=incmult(mult+1);
			}
			break;
			
			case S_G:
			if (++val <= MAX)
			{
				set_color(buf, mult+0, MAX-val, val, 0);
				set_color(buf, mult+1, 0, val, 0);
			}
			else
			{
				state = S_B;
				val = 0;
			}
			break;
			
			case S_B:
			if (++val <= MAX)
			{
				set_color(buf, mult+1, 0, MAX-val, val);
				set_color(buf, mult+2, 0, 0, val);
			}
			else
			{
				state = S_Y;
				val = 0;
			}
			break;
			
			case S_Y:
			if (++val <= MAX)
			{
				set_color(buf, mult+2, val, 0, MAX-val);
				set_color(buf, mult+3, val, val, 0);
			}
			else
			{
				state = S_V;
				val = 0;
			}
			break;
			
			case S_V:
			if (++val <= MAX)
			{
				set_color(buf, mult+3, MAX-val, MAX-val, val);
				set_color(buf, mult+4, val, 0, val);
			}
			else
			{
				state = S_T;
				val = 0;
			}
			break;
			
			case S_T:
			if (++val <= MAX)
			{
				set_color(buf, mult+4, MAX-val, val, MAX-val);
				set_color(buf, mult+5, 0, val, val);
			}
			else
			{
				state = S_R;
				val = 0;
			}
			break;
			
			default:
			state = S_R;
			break;
		}
		
		_delay_ms(1);
	}
}

int incmult(int mult)
{
	if (mult<NUM_LEDS)
	{
		mult=mult+2;
	}
	else
	{
		mult=0;
	}
	return mult;
}