/**
\brief This is a program which shows how to use the bsp modules for the board
       and leds.

\note: Since the bsp modules for different platforms have the same declaration,
       you can use this project with any platform.

Load this program on your boards. The LEDs should start blinking furiously.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2014.
*/

#include "stdint.h"
#include "stdio.h"
// bsp modules required
#include "board.h"
#include "leds.h"

#include "sctimer.h"
#define SCTIMER_PERIOD     1638 // @32kHz = 1s. So 1638 should be near 50ms

typedef struct {
   uint16_t num_compare;
} app_vars_t;

app_vars_t app_vars;
void cb_compare(void);


void delay_50ms(void);

/**
\brief The program starts executing here.
*/
int mote_main(void) {uint8_t i;
   
   board_init();
   
   sctimer_set_callback(cb_compare);
   sctimer_setCompare(sctimer_readCounter()+SCTIMER_PERIOD);

   // LED 1 (error LED)
   leds_error_on();          delay_50ms();
   leds_error_off();
   
   // LED 2 (radio LED)
   leds_radio_on();          delay_50ms();
   leds_radio_off();
      
   // LED 4 (debug LED)
   leds_debug_on();          delay_50ms();
   leds_debug_off();
   
   // LED 3 (sync LED)
   leds_sync_on();           delay_50ms();
   leds_sync_off();

   // reset the board, so the program starts running again
   board_reset();
   
   return 0;
}

void delay_50ms(void) {
  board_sleep();
}

void cb_compare(void) {
   
   // increment counter
   app_vars.num_compare++;
   
   // schedule again
   sctimer_setCompare(sctimer_readCounter()+SCTIMER_PERIOD);
}
