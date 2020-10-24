#ifndef _H_WS2812
#define _H_WS2812

#include <stdint.h>

typedef uint8_t   u8;
typedef uint16_t  u16;

#define NUM_WS2812    10
#define NUM_LEDS      (NUM_WS2812*3)
#define MAX   50
enum {S_R, S_O, S_G, S_B, S_Y, S_V, S_T};


// declaration of our ASM function
extern void output_grb(u8 * ptr, u16 count);

void set_color(u8 * p_buf, u8 led, u8 r, u8 g, u8 b);

volatile u8 int_flag;


#endif //_H_WS2812