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

#include <util/delay.h> // FOR DEBUGGING ONLY!!!

// do these need to be available across compile units? ie. in main?
volatile char cmdBuffer[UART_RX_BUFFER_SIZE];
volatile u08 rxCompleteFlag; // indicate that a command has been fully rx'd
volatile u08 rxAddrNext; // indicate that the next byte rx will be an address
volatile u08 rxAddressed; // indicate whether this unit was addressed by master
volatile u08 rxAddrGlobal; // Do Not Transmit if global address was found
volatile u08 rxCommandProcessing; // command processing latch (started, not complete)
volatile u08 rxCommandOverloaded; // state when we are addressed while already busy
u08 myAddress; // in-memory storage of EEPROM address.
u08 customResponse; // if this is set, then don't send generic "ok" response
u08 flg_forceGlobalCmdResponse;
char sprintbuf[80]; // output message buffer


// function prototypes, internal to library
void initCommandProtocolAddr(void);
u08 isAddressInitialized(void);
void setAddressInitialized(void);
void myUartRx(unsigned char);
u08 isMyAddress(u08);
u08 isGlobalAddress(u08);


// Externalized Routines 

/************************************************************************
 * Chain Command Handler routine to intercept UART receives in ISR
 * 
 ************************************************************************/
void initCommandProtocolLibrary(void) {
  // set state:
  // no message received
  rxCompleteFlag = FALSE;
  // next byte is not address to check
  rxAddrNext = FALSE;
  // this device not addressed
  rxAddressed = FALSE;
  // not global command
  rxAddrGlobal = FALSE;
  // no overloaded command condition
  rxCommandOverloaded = FALSE;
  // do not force response on global command
  flg_forceGlobalCmdResponse = FALSE;
  // Chain Command Handler routine to intercept UART receives in ISR
  uartSetRxHandler(myUartRx);
  // Initialize dynamic variable with saved address or header value
  initCommandProtocolAddr();
}

/************************************************************************
 * getCommandProtocolAddr:
 * Get this device's address from EEPROM location, and return it.
 ************************************************************************/
u08 getCommandProtocolAddr(void) {
  return (u08) eeprom_read_byte((uint8_t*)CMDPROT_EEPROMADDR_MY_ADDRESS);
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
  eeprom_write_byte((uint8_t*)CMDPROT_EEPROMADDR_MY_ADDRESS, newAddr);
  return 0;
}

u08 isCommandReady(void) {
  if (rxCompleteFlag) {
    return TRUE;
  }
  return FALSE;
}


// Internal routines 

/************************************************************************
 * initCommandProtocolAddr:
 * Load into memory this device's address from EEPROM location.
 * 
 * On 1st boot: If the EEPROM is not set, default to the command protocol 
 * header address: CMDPROT_MY_ADDRESS
 ************************************************************************/
void initCommandProtocolAddr() {
  myAddress = getCommandProtocolAddr();
  // if value in memory is zero, something must be written
  // if EEPROM was never initialized, write the value to EEPROM
  if ( (myAddress == 0) || (!isAddressInitialized()) ) {
    myAddress = CMDPROT_MY_ADDRESS;
    setCommandProtocolAddr(myAddress);
    setAddressInitialized();
  }
}



/************************************************************************
 * isAddressInitialized:
 * Return true if EEPROM indicates address was set.
 ************************************************************************/
u08 isAddressInitialized(void) {
  u08 temp;
  temp = eeprom_read_byte((u08*)CMDPROT_EEPROMADDR_ADDR_INIT);
  if (temp == CMDPROT_ADDRESS_INITIALIZED) {
    return TRUE;
  }
  return FALSE;
}

/************************************************************************
 * setAddressInitialized:
 * Return true if EEPROM indicates address was set.
 ************************************************************************/
void setAddressInitialized(void) {
  static u08 multiWriteBlock = FALSE;
  if (!multiWriteBlock) {
     eeprom_write_byte((u08*)CMDPROT_EEPROMADDR_ADDR_INIT, CMDPROT_ADDRESS_INITIALIZED);
     multiWriteBlock = TRUE;
  }
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
  if (inAddr == CMDPROT_GLOBAL_ADDRESS) {
    // disable MPCM mode, start looking for data bytes
    return TRUE;
  }
  return FALSE;
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
    switch (c) {
      case 0x24: // '$' command terminator byte (not included in cmd buffer!)
        if (rxAddressed) { // only take action if we were addressed
          // indicate that a cmd is fully received to initiate command processing
          rxCompleteFlag = TRUE; // allow mainline to process cmd now.
          rxAddressed = FALSE; // stop accumulating bytes into cmd buffer.
          PORTD &= ~(1 << PIND5); // DEBUG TURN OFF LED INDICATOR
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
      PORTD |= (1 << PIND5); // DEBUG TURN ON BLUE LED INDICATOR
      if (rxCommandProcessing) { // if we're already doing something, clear out buffer and record an error condition
        rxCommandOverloaded = TRUE; // not used right now, but could be used to indicate overflow condition
        flg_forceGlobalCmdResponse = FALSE;
        rxAddrGlobal = TRUE; // these two statements will prevent any output from endCmdProc.
        endCmdProcessing(); // reset all state from previous cmd.
      }
      rxAddressed = TRUE; // this unit is now active and will record bytes
    }
    if (isGlobalAddress(c)) {
      PORTD |= (1 << PIND5); // DEBUG TURN ON BLUE LED INDICATOR
      rxAddrGlobal = TRUE; // mute any response on global cmds
      rxAddressed = TRUE; // this unit is now active and will record bytes 
    }
  }
}

/************************************************************************
 * sendMsg:
 * 
 * Send a response to a cmd if not global, unless forced.
 ************************************************************************/
void sendMsg(void) {
  // don't send messages for global cmd, unless forced
  if (!rxAddrGlobal || flg_forceGlobalCmdResponse) {
    flg_forceGlobalCmdResponse = FALSE;
    uartSendBuffer(sprintbuf,strlen(sprintbuf));
  }
  memset(sprintbuf, 0, sizeof(sprintbuf)); 
}

/************************************************************************
 * forceGlobalCmdResponse:
 * 
 * DANGER: forces sendMsg to xmit a response to global cmd.
 ************************************************************************/
void forceGlobalCmdResponse(void) {
  flg_forceGlobalCmdResponse = TRUE;
}

/************************************************************************
 * sendCustomResponse:
 * 
 * Set library state to send a custom response string
 ************************************************************************/
void sendCustomResponse(void) {
  customResponse = TRUE;
}


/************************************************************************
 * Set up state to process a command.
 ************************************************************************/
void beginCmdProcessing(void) {
  rxCommandProcessing = TRUE; // cmd interpretation in progress
  rxCompleteFlag = FALSE; // reset until next cmd comes in
}

/************************************************************************
 * Handle standard command end processing.
 ************************************************************************/
void endCmdProcessing(void) {
  
   // always send a "k" if we're not sending something else.
  if (!customResponse) {
    sprintf_P(sprintbuf,PSTR("k$"));
  } else {
    customResponse = FALSE;
  }
  sendMsg();
  
  bufferClear(&uartRxBuffer,UART_RX_BUFFER_SIZE);
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