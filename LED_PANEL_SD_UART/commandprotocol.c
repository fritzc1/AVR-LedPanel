/*
 * commandprotocol.c
 *
 * Created: 12/7/2019 9:09:48 AM
 *  Author: ninhow
 * 
 * See commandprotocol.h for details
 *
 */ 


#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include "global.h"
#include "bufferchris.h"
#include "uartchris.h"
#include "commandprotocol.h"

// do these need to be available across compile units? ie. in main?
volatile unsigned char cmdBuffer[UART_RX_BUFFER_SIZE];
volatile u08 cmdReadyToProcess;

volatile u08 rxCompleteFlag; // indicate that a command has been fully rx'd
volatile u08 rxAddrNext; // indicate that the next byte rx will be an address
volatile u08 rxAddressed; // indicate whether this unit was addressed by master
volatile u08 rxAddrGlobal; // Do Not Transmit if global address was found
volatile u08 rxCommandProcessing; // command processing latch (started, not complete)
volatile u08 rxCommandOverloaded; // state when we are addressed while already busy
u08 myAddress; // in-memory storage of EEPROM address.
u08 customResponse; // if this is set, then don't send generic "ok" response

unsigned char sprintbuf[80]; // output message buffer

/************************************************************************
 * Chain Command Handler routine to intercept UART receives in ISR
 * 
 ************************************************************************/
void initCmdHandler(void) {
  rxCompleteFlag = FALSE; // initialize to no command received
  rxAddrNext = FALSE;
  rxAddressed = FALSE;
  rxAddrGlobal = FALSE;
  rxCommandOverloaded = FALSE;
  uartSetRxHandler(myUartRx);
}

/************************************************************************
 * UART Receive Handler, with extra Message Transmission Protocol implemented 
 *
 * Note that this function is called from an ISR!
 * Do not modify non-volatile variables.
 * 
 *
 * 
 ************************************************************************/
void myUartRx(unsigned char c) {
  
  if (!rxAddrNext) { // if non-address byte (typical)
    // first, scan the char received for special trigger values.
    switch (c)
    {
      case 0x24: // '$' command terminator byte (not included in cmd buffer!)
        if (rxAddressed) { // only take action if we were addressed
          // indicate that a cmd is fully received to initiate command processing
          rxCompleteFlag = TRUE; // allow mainline to process cmd now.
          rxAddressed = FALSE; // stop accumulating bytes into cmd buffer.
        }
        break;
      case 0x21: // '!' address byte next
        // indicate that the next byte received will be an address
        // this will have to be responded to before a message can be sent from the master
        rxAddrNext = TRUE;
        break;
      default:
        break;
    }
    if (rxAddressed) {
      // put received char in buffer, check if there's space
      if( !bufferAddToEnd(&uartRxBuffer, c) ) { // for now, use the built-in UART RX buffer to collect the cmd.
        // no space in buffer, count overflow
        uartRxOverflow++;
      }
    } // end rxAddressed
  } // end processing for non-address byte
  else { // if this byte IS an address byte
    rxAddrNext = FALSE; // only one addr byte per cmd, so next one won't be.
    if (isMyAddress(c)) {
      if (rxCommandProcessing) { // if we're already doing something, clear out
        // this should only happen if this device times out and the master can send another cmd
        // save the error state to print once the master finishes xmit
        rxCommandOverloaded = TRUE; // not used right now, but could be used to indicate overflow condition
        bufferClear(&uartRxBuffer,UART_RX_BUFFER_SIZE);
        rxAddressed = TRUE; // this unit is now active and will record bytes 
      }
      else { // not processing any cmd
        rxAddressed = TRUE; // this unit is now active and will record bytes 
      }
    }
    if (isGlobalAddress(c)) {
      rxAddrGlobal = TRUE; // mute any response on global cmds
      rxAddressed = TRUE; // this unit is now active and will record bytes 
    }
  }
}

/************************************************************************
 * initCommandProtocolAddr:
 * Get this device's address from EEPROM location.
 * 
 * If the EEPROM is not set, default to the command protocol header
 * address, and also program it into the EEPROM on 1st boot.
 ************************************************************************/
void initCommandProtocolAddr(u08 inAddr) {
  myAddress = getCommandProtocolAddr();
  // if value in memory is zero, something must be written
  if (myAddress == 0) {
    if (inAddr != 0) {
      myAddress = inAddr;
    } else {
      myAddress = CMD_UART_THIS_DEVICE_ADDRESS;
    }    
    setCommandProtocolAddr(myAddress);
  }
}

/************************************************************************
 * getCommandProtocolAddr:
 * Get this device's address from EEPROM location, and return it.
 ************************************************************************/
u08 getCommandProtocolAddr(void) {
  return (u08) eeprom_read_byte((uint8_t*)CMD_EEPROM_ADDR_THIS_DEVICE_ADDR);
  
}

/************************************************************************
 * setCommandProtocolAddr:
 * Set a new address to EEPROM location, and to running copy.
 * 
 * Reject address 0.
 ************************************************************************/
u08 setCommandProtocolAddr(u08 newAddr) {
  if (newAddr == 0) {
    return 1;
  }
  myAddress = newAddr;
  eeprom_write_byte((uint8_t*)CMD_EEPROM_ADDR_THIS_DEVICE_ADDR, newAddr);
  return 0;
}

/************************************************************************
 * isMyAddress:
 * Check address received against my address set in commandprotocol headr
 * 
 * Routine is inline to avoid extra function calls in ISR
 ************************************************************************/
inline u08 isMyAddress(u08 inAddr) {
  if (inAddr == myAddress) {
    // disable MPCM mode, start looking for data bytes
    //uartSendByte('a');
    return TRUE;
  }
  return FALSE;
}

/************************************************************************
 * isGlobalAddress:
 * Check if the address was the global address (all units on bus)
 * 
 * Routine is inline to avoid extra function calls in ISR
 ************************************************************************/
inline u08 isGlobalAddress(u08 inAddr) {
  if (inAddr == CMD_UART_GLOBAL_CMD_ADDR) {
    // disable MPCM mode, start looking for data bytes
    return TRUE;
  }
  return FALSE;
}

/************************************************************************
 * sendMsg:
 * 
 * Routine is inline to avoid extra function calls in ISR
 ************************************************************************/
void sendMsg(void) {
  if (rxAddrGlobal) {
    memset(sprintbuf, 0, sizeof(sprintbuf));
    return; // don't send messages for global cmd
  }
  uartSendBuffer(sprintbuf,strlen(sprintbuf));
  memset(sprintbuf, 0, sizeof(sprintbuf));
}

/************************************************************************
 * Set up state to process a command.
 *
 * args? [data] [crc code] ?
 ************************************************************************/
void beginCmdProcessing(void) {
  rxCommandProcessing = TRUE; // cmd interpretation in progress
  rxCompleteFlag = FALSE;
}

/************************************************************************
 * Set up state after done processing a command.
 *
 * args? [data] [crc code] ?
 ************************************************************************/
void endCmdProcessing(void) {
  bufferClear(&uartRxBuffer,UART_RX_BUFFER_SIZE); // clear to get another cmd, so send response.
  if (!customResponse) { // always send a "k" if we're not sending something else.
    sprintf_P(sprintbuf,PSTR("k$"));
    sendMsg();
  }
  customResponse = FALSE; // reset state
  rxAddrGlobal = FALSE; // reset address state. this was saved to mute responses on global cmds.
  rxCommandProcessing = FALSE; // command interpretation and response done
}

/*********************************************************************
 * getNextNonNumericChar:
 *
 * Modify the pointer passed in to point to next non-numeric char.
 * We need the argument passed by address so we can modify the
 * caller's storage.
 *********************************************************************/
void pointToNextNonNumericChar(unsigned char **ppRxDataStr) {
 /* Count the digits. Numbers are 0x30 < n < 0x39 */
  while((**ppRxDataStr >= 0x30) & (**ppRxDataStr <= 0x39)) {
    (*ppRxDataStr)++;
  }
}