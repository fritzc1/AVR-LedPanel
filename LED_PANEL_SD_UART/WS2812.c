
#include <stdint.h>
#include "global.h"
#include "WS2812.h"


void set_color(u08 * p_buf, u08 led, u08 r, u08 g, u08 b)
{
	u16 index = 3*led;
	p_buf[index++] = g;
	p_buf[index++] = r;
	p_buf[index] = b;
}