#ifndef _H_WS2812
#define _H_WS2812

// define the LED panel dimensions
#define XBOUND 40
#define YBOUND 11 // NOTE this is 1/2 the screen height. The screen is split into two strings of LEDs, an upper half and lower half.

// define the number of pixels
#define NUM_WS2812    XBOUND*YBOUND
// define the number of RGB elements
#define NUM_LEDS      (NUM_WS2812*3)

// declaration of ASM function to output self-clocking LED data.
// NOTE: 3 outputs on PD3, 4 outputs on PD4. That's the only difference!
extern void output_grb3(u08 * ptr, u16 count);
extern void output_grb4(u08 * ptr, u16 count);

// Same as grb3; provided for library compatibility only
extern void output_grb(u08 * ptr, u16 count);

/****************************************************
 * Set the RGB components of an LED in p_buf, via
 * it's locaiton from the beginning of the string.
 */
void set_color(u08 * p_buf, u08 led, u08 r, u08 g, u08 b);

volatile u08 int_flag;

#endif //_H_WS2812