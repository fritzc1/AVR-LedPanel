/*! \file uart.c \brief UART driver with buffer support. */
// *****************************************************************************
//
// See uartchris.h for details.
//
// File Name	: 'uartchris.c'
// Title		: UART driver with buffer support
// Author		: Pascal Stang - Copyright (C) 2000-2002
// Created		: 11/22/2000
// Revised		: 06/09/2003
// Version		: 1.3
// Target MCU	: ATMEL AVR Series
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
// *****************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>

#include "global.h"
#include "bufferchris.h"
#include "uartchris.h"

#include <util/delay.h> // depends on FCPU in global.h

// UART global variables
// flag variables
volatile u08   uartReadyTx;			///< uartReadyTx flag
volatile u08   uartBufferedTx;		///< uartBufferedTx flag

volatile u08   uartTxIntData;
volatile u08   uartRxIntData;
// receive and transmit buffers
cBuffer uartRxBuffer;				///< uart receive buffer
cBuffer uartTxBuffer;				///< uart transmit buffer
unsigned short uartRxOverflow;		///< receive overflow counter

#ifndef UART_BUFFERS_EXTERNAL_RAM
	// using internal ram,
	// automatically allocate space in ram for each buffer's data area
	static unsigned char uartRxData[UART_RX_BUFFER_SIZE];
	static unsigned char uartTxData[UART_TX_BUFFER_SIZE];
#endif

typedef void (*voidFuncPtru08)(unsigned char);
volatile static voidFuncPtru08 UartRxFunc;

// enable and initialize the uart
void uartInit(void) {
	// clear interrupts for the duration of set up (in case they were enabled before)
	CRITICAL_SECTION_START;
	// initialize the buffers
	uartInitBuffers();
	// initialize user receive handler
	UartRxFunc = 0;

	// enable RxD/TxD and interrupts
	// (added UDRIE0 for data reg empty interrupt for TX optional|BV(UDRIE))
	outb(UCSRB, BV(TXCIE)|BV(RXCIE)|BV(RXEN)|BV(TXEN));
	
	// set default baud rate
	uartSetBaudRate(UART_DEFAULT_BAUD_RATE);  
	// initialize states
	uartReadyTx = TRUE;
	uartBufferedTx = FALSE;
	// clear overflow count
	uartRxOverflow = 0;
  // if we are using RS485 standard, set outputs to Hi-Z
  #ifdef UART_USE_RS485
  uart485OutputDisable();
  uart485EnableDriverCntlPin();
  #endif
	// resume interrupt state upon entry
	CRITICAL_SECTION_END;
}

// create and initialize the uart transmit and receive buffers
void uartInitBuffers(void)
{
	#ifndef UART_BUFFERS_EXTERNAL_RAM
		// initialize the UART receive buffer
		bufferInit(&uartRxBuffer, uartRxData, UART_RX_BUFFER_SIZE);
		// initialize the UART transmit buffer
		bufferInit(&uartTxBuffer, uartTxData, UART_TX_BUFFER_SIZE);
	#else
		// initialize the UART receive buffer
		bufferInit(&uartRxBuffer, (u08*) UART_RX_BUFFER_ADDR, UART_RX_BUFFER_SIZE);
		// initialize the UART transmit buffer
		bufferInit(&uartTxBuffer, (u08*) UART_TX_BUFFER_ADDR, UART_TX_BUFFER_SIZE);
	#endif
}

// redirects received data to a user function
void uartSetRxHandler(void (*rx_func)(unsigned char c))
{
	// set the receive interrupt to run the supplied user function
	UartRxFunc = rx_func;
}

// set the uart baud rate
void uartSetBaudRate(u32 baudrate)
{
	// calculate division factor for requested baud rate, and set it
	u16 bauddiv = ((F_CPU+(baudrate*8L))/(baudrate*16L)-1);
	outb(UBRRL, bauddiv);
	#ifdef UBRRH
	outb(UBRRH, bauddiv>>8);
	#endif
}

// returns the receive buffer structure 
cBuffer* uartGetRxBuffer(void) {
	// return rx buffer pointer
	return &uartRxBuffer;
}

// returns the transmit buffer structure 
cBuffer* uartGetTxBuffer(void) {
	// return tx buffer pointer
	return &uartTxBuffer;
}

// transmits a byte over the uart
void uartSendByte(u08 txData) {
	// wait for the transmitter to be ready
	while(!uartReadyTx);
	// send byte
  #ifdef UART_USE_RS485
  uart485OutputEnable();
  #endif
	outb(UDR0, txData);
	// set ready state to FALSE
	uartReadyTx = FALSE;
}

// gets a single byte from the uart receive buffer (getchar-style)
int uartGetByte(void) {
	u08 c;
	if(uartReceiveByte(&c))
		return c;
	else
		return -1;
}

// gets a byte (if available) from the uart receive buffer
u08 uartReceiveByte(u08* rxData) {
	// make sure we have a receive buffer
	if(uartRxBuffer.size)
	{
		// make sure we have data
		if(uartRxBuffer.datalength)
		{
			// get byte from beginning of buffer
			*rxData = bufferGetFromFront(&uartRxBuffer);
			return TRUE;
		}
		else
		{
			// no data
			return FALSE;
		}
	}
	else
	{
		// no buffer
		return FALSE;
	}
}

// flush all data out of the receive buffer
void uartFlushReceiveBuffer(void)
{
	// flush all data from receive buffer
	//bufferFlush(&uartRxBuffer);
	// same effect as above
	uartRxBuffer.datalength = 0;
}

// return true if uart receive buffer is empty
u08 uartReceiveBufferIsEmpty(void) {
	if(uartRxBuffer.datalength == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// add byte to end of uart Tx buffer
u08 uartAddToTxBuffer(u08 data) {
	// add data byte to the end of the tx buffer
	return bufferAddToEnd(&uartTxBuffer, data);
}

// start transmission of the current uart Tx buffer contents
void uartSendTxBuffer(void) {
	
	// wait for the transmitter to be ready
	while(!uartReadyTx);
	
	
	
	// turn on buffered transmit
	uartBufferedTx = TRUE;
	// send the first byte to get things going by interrupts
  #ifdef UART_USE_RS485
  uart485OutputEnable();
  #endif
	uartSendByte(bufferGetFromFront(&uartTxBuffer));
}

// transmit nBytes from buffer out the uart
u08 uartSendBuffer(char *buffer, u16 nBytes) {
	register u08 first;
	register u16 i;
	
	// wait for the transmitter to be ready
	while(!uartReadyTx);
	
	
	
	// check if there's space (and that we have any bytes to send at all)
	if((uartTxBuffer.datalength + nBytes < uartTxBuffer.size) && nBytes)
	{
		// grab first character
		first = *buffer++;
		// copy THE REST OF user buffer to uart transmit buffer
		for(i = 0; i < nBytes-1; i++)
		{
			// put data bytes at end of buffer
			bufferAddToEnd(&uartTxBuffer, *buffer++);
		}

		// send the first byte to get things going by interrupts
		uartBufferedTx = TRUE;
    #ifdef UART_USE_RS485
    uart485OutputEnable();
    _delay_us(1);
    #endif
    
		uartSendByte(first);
		// return success
		return TRUE;
	}
	else
	{
		// return failure
		return FALSE;
	}
}

#ifdef UART_USE_RS485
inline void uart485OutputEnable(void) {
  UARTRS485PORT |= BV(RS485PIN);
}
inline void uart485OutputDisable(void) {
  UARTRS485PORT &= ~BV(RS485PIN);
}

inline void uart485EnableDriverCntlPin(void) {
  UARTRS485DDR |= BV(RS485PIN);
}
#endif
// UART Data Register Empty Interrupt Handler
UART_INTERRUPT_HANDLER(SIG_UART_DATA) {
  // nop
}

// UART Transmit Complete Interrupt Handler
UART_INTERRUPT_HANDLER(SIG_UART_TRANS) {
	PORTD |= (1 << PIND5); // DEBUG TURN ON LED INDICATOR
	//UDR0 = uartBufferedTx;
	// check if buffered tx is enabled
	if(uartBufferedTx)
	{
		
		// check if there's data left in the buffer
		if(uartTxBuffer.datalength)
		{
			// send byte from top of buffer
      /* The Following section is expanded from bufferGetFromFront() */
			//uartTxIntData = bufferGetFromFront(&uartTxBuffer);
      uartTxIntData = 0;
			if(uartTxBuffer.datalength)
			{
				// get the first character from buffer
				uartTxIntData = uartTxBuffer.dataptr[uartTxBuffer.dataindex];
				// move index down and decrement length
				uartTxBuffer.dataindex++;
				if(uartTxBuffer.dataindex >= uartTxBuffer.size)
				{
					uartTxBuffer.dataindex -= uartTxBuffer.size;
				}
				uartTxBuffer.datalength--;
			}
			outb(UDR0, uartTxIntData);
		}
		else
		{
			// no data left
			uartBufferedTx = FALSE;
			// return to ready state
			uartReadyTx = TRUE;
      #ifdef UART_USE_RS485
      uart485OutputDisable();
      #endif
		}
	}
	else
	{
		// we're using single-byte tx mode
		// indicate transmit complete, back to ready
		uartReadyTx = TRUE;
    #ifdef UART_USE_RS485
    uart485OutputDisable();
    #endif
	}
}

// UART Receive Complete Interrupt Handler
UART_INTERRUPT_HANDLER(SIG_UART_RECV)
{
	u08 c;
	PORTD |= (1 << PIND5); // DEBUG TURN ON LED INDICATOR
	// get received char
	c = inb(UDR);
  
	// if there's a user function to handle this receive event
	if(UartRxFunc)
	{
		// call it and pass the received data
		UartRxFunc(c);
	}
	else
	{
		// otherwise do default processing
		// put received char in buffer
		// check if there's space
		if( !bufferAddToEnd(&uartRxBuffer, c) )
		{
			// no space in buffer
			// count overflow
			uartRxOverflow++;
		}
	}
}
