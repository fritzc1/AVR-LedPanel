/*********************************************************************
 *
 * Message Protocol Layer
 *
 * Author: Chris Fritz
 *
 * Purpose: This is to be used on top of the UART layer (RS485) to 
 *          implement a transmission control protocol.
 
 
 Message format ideas: 
 
 ->If i want better responses, such as responses to queries, I need to standardize
 a way to fill tx buffer, and set a response flag if i want a response other than 
 the standard "ok"
 
 ->I could include a byte count near the beginning of my message, That way no terminating char is needed.
 What other way could I indicate "address" byte? 0x21 is ok, vvv binary data send in nibbles 0x3n vvv
 
 ->How to send binary data idea;
  - send nibble at a time, ie 0x3n (n=data)
    - 0x3x makes printable chars
  - reconstruct every 2 nibbles back into byte at slave
  - no data byte can be read by any slave as an ADDR_NEXT byte (0x21)
  - no data byte can be mistaken as the END_CMD byte (0x24)
 Use case: Might need to use binary data for a passthrough device to relay command
  data to a projector serial input.
 *********************************************************************/
#ifndef COMMANDPROTOCOL_H
#define COMMANDPROTOCOL_H

#include "global.h"

#define CMD_UART_THIS_DEVICE_ADDRESS	0x01
        // this (dec 20) was the addr of the last device programmed
        //Addrs so far:
        // 10 - 17: Recessed lights in soffits
        // 18 - 19: Step lights
        // 20 ?
#define CMD_UART_GLOBAL_CMD_ADDR		0x00

// the location in EEPROM from which to load/store address byte
#define CMD_EEPROM_ADDR_THIS_DEVICE_ADDR		0x00

void initCommandProtocolAddr(u08);
u08 setCommandProtocolAddr(u08);
u08 getCommandProtocolAddr(void);
void initCmdHandler(void);
void myUartRx(unsigned char);
u08 isMyAddress(u08);
u08 isGlobalAddress(u08);
void sendMsg(void);
void beginCmdProcessing(void);
void endCmdProcessing(void);
// used by command processors, advances the pointer over ASCII numbers. pass the address of your char *
void pointToNextNonNumericChar(unsigned char **);

/*********************************************************************
 * Use these flags to control your application's behavior. They will tell
 * when it is safe to begin processing a command. You probably will want
 * to copy the command buffer into a "local" buffer, and control command
 * overrun in the application so another command isn't sent while the
 * current one is still being processed. Unlikely but possible.
 *********************************************************************/
extern volatile u08 rxCompleteFlag; // indicate that a command has been fully rx'd
extern volatile u08 rxAddrNext; // indicate that the next byte rx will be an address
extern volatile u08 rxAddressed; // indicate whether this unit was addressed by master
extern volatile u08 rxAddrGlobal; // Do Not Transmit if global address was found
extern volatile u08 rxCommandProcessing; // command processing latch (started, not complete)
extern u08 customResponse; // if this is set, then don't send generic "ok" response


// the following vars are used to interact with uart receive ISR
extern cBuffer uartRxBuffer;	// defined in uartchris.c
extern unsigned short uartRxOverflow; // defined in uartchris.c
extern char sprintbuf[80]; // output message buffer

#endif

