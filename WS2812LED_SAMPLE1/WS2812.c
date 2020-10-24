#include "WS2812.h"

void set_color(u8 * p_buf, u8 led, u8 r, u8 g, u8 b)
{
	u16 index = 3*led;
	p_buf[index++] = g;
	p_buf[index++] = r;
	p_buf[index] = b;
}