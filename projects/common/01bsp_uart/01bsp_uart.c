/**
\brief This is a program which shows how to use the bsp modules for the board
       and UART.

\note: Since the bsp modules for different platforms have the same declaration,
       you can use this project with any platform.

Load this program on your board. Open a serial terminal client (e.g. PuTTY or
TeraTerm):
- You will read "Hello World!" printed over and over on your terminal client.
- when you enter a character on the client, the board echoes it back (i.e. you
  see the character on the terminal client) and the "ERROR" led blinks.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, February 2012
*/

#include "stdint.h"
#include "stdio.h"
#include "string.h"
// bsp modules required
#include "board.h"
#include "uart.h"
#include "sctimer.h"
#include "leds.h"

//=========================== variables =======================================

typedef struct {
   uint8_t uart_lastTxByteIndex;

   volatile uint8_t strReceived[32];               // string received from uart
   volatile uint8_t receivedStrLen;                // string lenght

   uint8_t doEcho;                                 // flag when ok to start echoing
   uint8_t echoDone;                               // flag when echo is done

} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void cb_uartTxDone(void);
uint8_t cb_uartRxCb(void);

//=========================== main ============================================

/**
\brief The program starts executing here.
*/
int mote_main(void) {
   
   // clear local variable
   memset(&app_vars,0,sizeof(app_vars_t));

   for(uint8_t i=0; i<32; i++) app_vars.strReceived[i] = 0;
   app_vars.receivedStrLen = 0;

   app_vars.doEcho = 0;
   app_vars.echoDone = 0;
   
   // initialize the board
   board_init();
   
   // setup UART
   uart_setCallbacks(cb_uartTxDone, cb_uartRxCb);
   uart_enableInterrupts();
   
   while(1) {
      while(app_vars.doEcho==0);    // loop and wait when don't need to echo
      app_vars.doEcho = 0;
      app_vars.echoDone = 0;
      // echo bytes over serial
      app_vars.uart_lastTxByteIndex = 0;
      uart_writeByte(app_vars.strReceived[0]);

      while(app_vars.echoDone==0);  // loop and wait when echo is not done yet
   }
}

//=========================== callbacks =======================================

void cb_uartTxDone(void) {
   app_vars.uart_lastTxByteIndex++;
   if(app_vars.uart_lastTxByteIndex<app_vars.receivedStrLen) {
      uart_writeByte(app_vars.strReceived[app_vars.uart_lastTxByteIndex]);
   } else {
      if(app_vars.receivedStrLen!=0) { 
         uart_writeByte('\n');
         for(uint8_t i=0; i<32; i++) app_vars.strReceived[i] = 0;
         app_vars.receivedStrLen = 0;
         app_vars.echoDone = 1;
      }
   }
}

uint8_t cb_uartRxCb(void) {
   // read received bytes
   while(1) {
      uint8_t byte = uart_readByte();
      if(byte==0) break;
      app_vars.strReceived[app_vars.receivedStrLen] = byte;
      app_vars.receivedStrLen++;
      if(byte=='\r') {
         app_vars.doEcho = 1;
         break;
      }
   }
   return 0;
}