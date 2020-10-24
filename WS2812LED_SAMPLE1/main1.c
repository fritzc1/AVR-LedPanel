/*
 * CApp1.c
 *
 * Created: 7/26/2017 10:04:06 PM
 * Author: Chris Fritz
 * Description: This program is a playground for learning C concepts.
 * Agenda/Notes:-Get build working
 *					For custom makefiles from the internet, use CYGWIN make.exe and shellutils, but for projects created in Atmel Studio 7, using the builtin configuration use C:\Program Files (x86)\Atmel\Studio\7.0\shellUtils\make.exe (DEFAULT BUILDER shellutils)
 *				-Experiment with avr/io.h
				-add subroutines that use pass by reference to set IO pins on off
				-Experiment with Delay.h functionality
				-Experiment with stdint.h definitions
 */ 

#include <avr/io.h>
#include <stdint.h>
#include <string.h>
//#define F_CPU 16000000UL // 16,000,000Hz defined for delay
#define F_CPU   16000000 //2812 assembly routine
#include <util/delay.h>

typedef uint8_t   u8;
typedef uint16_t  u16;

#define NUM_WS2812		10 //Number of LED elements
#define NUM_RGBS		(NUM_WS2812*3) //Calculate number of RGB elements (bytes) total

enum {S_R, S_G, S_B, S_Y, S_V, S_T}; //

#define MAX				155 //

// declaration of our ASM function
extern void output_grb(uint8_t * ptr, uint16_t count);

void pinon(volatile uint8_t *portno,int pinno);
void pinoff(volatile uint8_t *portno,int pinno);

/* fill the buffer with "GRB" values. First parm is ptr to buffer. 
   Second parm is the led we are setting. (BEGIN ADDRESSING AT 1)
   Parms 3-5 are the byte brightness values for "G" "R" "B". */
void set_color(uint8_t * p_buf, uint8_t lednum, uint8_t rval, uint8_t gval, uint8_t bval)
{
	uint16_t index = (3*lednum); // calculate the first of three bytes to populate
	p_buf[index++] = gval;
	p_buf[index++] = rval;
	p_buf[index] = bval;
}

void pwm(int pDelay1, int pDelay2, int pR, int pG, int pB)
{
	set_color(buf, 0, 255, 255, 255);
	output_grb(buf, sizeof(buf));
	_delay_ms(pDelay1);
	
	set_color(buf, 0, 255, 255, 255);
	output_grb(buf, sizeof(buf));
	_delay_ms(pDelay2);
}

int dontrun(void)
{
			
	/*DDRD &= ~(1 << DDBx);*/ // set to INPUT	
	
	/*uint8_t inttest = 0;
	DDRD |= (1 << DDB2); // set to OUTPUT
	pinoff(&PORTD,PORTD2); // Init pin*/	
	
	DDRD |= (1 << DDB3); // set to OUTPUT
	pinon(&PORTD,PORTD3); // Init pin	
	uint8_t buf[NUM_RGBS];
	//int count = 0;
	
	memset(buf, 0, sizeof(buf));
	
	
	uint8_t val = 1;
	
	
	/*while(1)
	{
		pwm(10,10,255,255,255);
		
		
	} */

	while (1) 
	{
		switch (inttest)
		{
			case 100: case 200: case 255:
				_delay_ms(1000);
				for (int i = 0; i < (inttest/10); i++)
				{
					pinon(&PORTD, PORTD2);
					_delay_ms(300);
					pinoff(&PORTD, PORTD2);
					_delay_ms(300);
				}
				_delay_ms(1000);
			break;
		}
		for (int i = 0; i < inttest; i++)
		{
			_delay_ms(1);
		}
		PORTD &= ~(1 << PORTD2); // Pin D2 goes low
		inttest += 1;
		for (int i = 0; i < inttest; i++)
		{
			_delay_ms(1);
		}
		PORTD |= (1 << PORTD2); // Pin D2 goes high
		inttest += 1;
    }
	
	
} // end main

void pinon(volatile uint8_t *portno,int pinno)
{
	*portno |= (1 << pinno);
}

void pinoff(volatile uint8_t *portno,int pinno)
{
	*portno &= ~(1 << pinno);
}