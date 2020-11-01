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

#include "bufferchris.h"
#include "uartchris.h"

#define CMDPROT_MY_ADDRESS	0x31 // ASCII "1"
        // this (dec 20) was the addr of the last device programmed
        //Addrs so far:
        // 10 - 17: Recessed lights in soffits
        // 18 - 19: Step lights
        // 20 ?
#define CMDPROT_GLOBAL_ADDRESS		0x00

// location in EEPROM from which to load/store address byte
#define CMDPROT_EEPROMADDR_MY_ADDRESS		0x00
// location in EEPROM to store pattern when address has been initialized
#define CMDPROT_EEPROMADDR_ADDR_INIT		0x01
// pattern to store when address is initialized on first power-on
#define CMDPROT_ADDRESS_INITIALIZED     0xA5

void initCommandProtocolLibrary(void);
u08 getCommandProtocolAddr(void);
u08 setCommandProtocolAddr(u08);

u08 isCommandReady(void);
void beginCmdProcessing(void);
void endCmdProcessing(void);
// used by command processors, advances the pointer over ASCII numbers. pass the [address] of your char ptr
void pointToNextNonNumericChar(unsigned char **);
// this gets called by endCmdProc() but you can also call it to send (and clear) a message in cmdprotprintbuf
void sendMsg(void);
// call this to force sendMsg to send a response even if the received address was global (0)
void forceGlobalCmdResponse(void);


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


// the following vars are used to interact with uart receive ISR
extern cBuffer uartRxBuffer;	// defined in uartchris.c
extern unsigned short uartRxOverflow; // defined in uartchris.c
extern char cmdprotprintbuf[80]; // output message buffer

#endif

