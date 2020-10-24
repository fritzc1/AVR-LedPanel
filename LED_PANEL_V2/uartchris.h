/*! \file uart.h \brief UART driver with buffer support. */
//*****************************************************************************
//
// This UART library has been debugged and modified to work properly, using
// the Pascal Stang lib as a base. Note that to use this library, you also
// need bufferchris.h and .c, as well as global.h to define clock speed.
// The primary addition is for the uart buffered send routines to wait until 
// ready for TX flag is true, before enabling buffered mode. This prevents 
// the ISR from the last send to erroneously send the next string before the 
// first char.
//
// Note 10/25/2019 - this copy is updated to add support for ATTINY13A in 
// addition to the AtMega328
//
//
// File Name	: 'uartchris.h'
// Title		: UART driver with buffer support
// Author		: Pascal Stang - Copyright (C) 2000-2002
// Created		: 11/22/2000
// Revised		: 02/01/2004
// Version		: 1.3
// Target MCU	: ATMEL AVR Series
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
///	\ingroup driver_avr
/// \defgroup uart UART Driver/Function Library (uart.c)
/// \code #include "uart.h" \endcode
/// \par Overview
///		This library provides both buffered and unbuffered transmit and receive
///		functions for the AVR processor UART.  Buffered access means that the
///		UART can transmit and receive data in the "background", while your code
///		continues executing.  Also included are functions to initialize the
///		UART, set the baud rate, flush the buffers, and check buffer status.
///
/// \note	For full text output functionality, you may wish to use the rprintf
///		functions along with this driver.
///
/// \par About UART operations
///		Most Atmel AVR-series processors contain one or more hardware UARTs
///		(aka, serial ports).  UART serial ports can communicate with other 
///		serial ports of the same type, like those used on PCs.  In general,
///		UARTs are used to communicate with devices that are RS-232 compatible
///		(RS-232 is a certain kind of serial port).
///	\par
///		By far, the most common use for serial communications on AVR processors
///		is for sending information and data to a PC running a terminal program.
///		Here is an exmaple:
///	\code
/// uartInit();					// initialize UART (serial port)
/// uartSetBaudRate(9600);		// set UART speed to 9600 baud
/// rprintfInit(uartSendByte);  // configure rprintf to use UART for output
/// rprintf("Hello World\r\n");	// send "hello world" message via serial port
/// \endcode
///
/// \warning The CPU frequency (F_CPU) must be set correctly in \c global.h
///		for the UART library to calculate correct baud rates.  Furthermore,
///		certain CPU frequencies will not produce exact baud rates due to
///		integer frequency division round-off.  See your AVR processor's
///		 datasheet for full details.
//
//*****************************************************************************
//@{

#ifndef UART_H
#define UART_H

#include "global.h"
#include "bufferchris.h"

//! Default uart baud rate.
/// This is the default speed after a uartInit() command,
/// and can be changed by using uartSetBaudRate().
#define UART_DEFAULT_BAUD_RATE	19200

// buffer memory allocation defines
// buffer sizes
#ifndef UART_TX_BUFFER_SIZE
//! Number of bytes for uart transmit buffer.
/// Do not change this value in uart.h, but rather override
/// it with the desired value defined in your project's global.h
#define UART_TX_BUFFER_SIZE		0x0040
#endif
#ifndef UART_RX_BUFFER_SIZE
//! Number of bytes for uart receive buffer.
/// Do not change this value in uart.h, but rather override
/// it with the desired value defined in your project's global.h
#define UART_RX_BUFFER_SIZE		0x0040
#endif

// define this key if you wish to use
// external RAM for the	UART buffers
//#define UART_BUFFER_EXTERNAL_RAM
#ifdef UART_BUFFER_EXTERNAL_RAM
	// absolute address of uart buffers
	#define UART_TX_BUFFER_ADDR	0x1000
	#define UART_RX_BUFFER_ADDR	0x1100
#endif

// define this key if you wish to use RS485 standard, 
// multiple slave with output disabling
#define UART_USE_RS485

#ifdef UART_USE_RS485
  #define UARTRS485PORT PORTD
  #define UARTRS485DDR DDRD
  #define RS485PIN PIND2
#endif

//! Type of interrupt handler to use for uart interrupts.
/// Value may be SIGNAL or INTERRUPT.
/// \warning Do not change unless you know what you're doing.
#ifndef UART_INTERRUPT_HANDLER
//#define UART_INTERRUPT_HANDLER	SIGNAL
#define UART_INTERRUPT_HANDLER	ISR
#endif


// compatibility with old Mega processors
#if defined(UBRR) && !defined(UBRRL)
	#define	UBRRL				UBRR
#endif

#if	defined(__AVR_ATmega328P__) | defined(__AVR_ATmega328PB__)
//#include <avr/iom328p.h> //<-io.h, uncomment to goto actual implementation
//#include <avr/iom328pb.h> //<-io.h, uncomment to goto actual implementation
	#define UDR					UDR0
	#define UCSRA				UCSR0A
	#define UCSRB				UCSR0B
	#define UCSRC				UCSR0C
	#define RXCIE				RXCIE0
	#define TXCIE				TXCIE0
	#define UDRIE				UDRIE0
	#define RXC					RXC0
	#define TXC					TXC0
	#define RXEN				RXEN0
	#define TXEN				TXEN0
	#define UBRRL				UBRR0L
	#define UBRRH				UBRR0H
#endif
#if	defined(__AVR_ATmega328P__)
	#define SIG_UART_TRANS	USART_TX_vect
	#define SIG_UART_RECV		USART_RX_vect
	#define SIG_UART_DATA		USART_UDRE_vect
#endif
#if	defined(__AVR_ATmega328PB__) // vectors are different for the PB
  #define SIG_UART_TRANS	USART0_TX_vect
  #define SIG_UART_RECV		USART0_RX_vect
  #define SIG_UART_DATA		USART0_UDRE_vect
#endif

// functions

//! Initializes uart.
/// \note	After running this init function, the processor
/// I/O pins that used for uart communications (RXD, TXD)
/// are no long available for general purpose I/O.
void uartInit(void);

//! Initializes transmit and receive buffers.
/// Automatically called from uartInit()
void uartInitBuffers(void);

//! Redirects received data to a user function.
///
void uartSetRxHandler(void (*rx_func)(unsigned char c));

//! Sets the uart baud rate.
/// Argument should be in bits-per-second, like \c uartSetBaudRate(9600);
void uartSetBaudRate(u32 baudrate);

//! Returns pointer to the receive buffer structure.
///
cBuffer* uartGetRxBuffer(void);

//! Returns pointer to the transmit buffer structure.
///
cBuffer* uartGetTxBuffer(void);

//! Sends a single byte over the uart.
/// \note This function waits for the uart to be ready,
/// therefore, consecutive calls to uartSendByte() will
/// go only as fast as the data can be sent over the
/// serial port.
void uartSendByte(u08 data);

//! Gets a single byte from the uart receive buffer.
/// Returns the byte, or -1 if no byte is available (getchar-style).
int uartGetByte(void);

//! Gets a single byte from the uart receive buffer.
/// Function returns TRUE if data was available, FALSE if not.
/// Actual data is returned in variable pointed to by "data".
/// Example usage:
/// \code
/// char myReceivedByte;
/// uartReceiveByte( &myReceivedByte );
/// \endcode
u08 uartReceiveByte(u08* data);

//! Returns TRUE/FALSE if receive buffer is empty/not-empty.
///
u08 uartReceiveBufferIsEmpty(void);

//! Flushes (deletes) all data from receive buffer.
///
void uartFlushReceiveBuffer(void);

//! Add byte to end of uart Tx buffer.
///	Returns TRUE if successful, FALSE if failed (no room left in buffer).
u08 uartAddToTxBuffer(u08 data);

//! Begins transmission of the transmit buffer under interrupt control.
///
void uartSendTxBuffer(void);

//! Sends a block of data via the uart using interrupt control.
/// \param buffer	pointer to data to be sent
///	\param nBytes	length of data (number of bytes to sent)
u08  uartSendBuffer(char *buffer, u16 nBytes);

#ifdef UART_USE_RS485
void uart485OutputEnable(void);
void uart485OutputDisable(void);
void uart485EnableDriverCntlPin(void);
#endif

#endif
//@}


