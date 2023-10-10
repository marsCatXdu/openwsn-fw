/**
\brief This program shows the use of the "sctimer" bsp module.

Since the bsp modules for different platforms have the same declaration, you
can use this project with any platform.

Load this program onto your board, and start running. It will enable the sctimer. 
The sctimer is periodic, of period SCTIMER_PERIOD ticks. Each time it elapses:
    - the frame debugpin toggles
    - the error LED toggles

\author Tengfei Chang <tengfei.chang@eecs.berkeley.edu>, April 2017.
*/

#include "stdint.h"
#include "string.h"
#include "board.h"
#include "debugpins.h"
#include "leds.h"
#include "sctimer.h"

//=========================== defines =========================================

#define SCTIMER_PERIOD     32768 // @32kHz = 1s

//=========================== variables =======================================

typedef struct {
   uint16_t num_compare;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void cb_compare(void);

//=========================== main ============================================


uint16_t sleep_counter = 0;

/**
\brief The program starts executing here.
*/
int mote_main(void) {  
   
   // initialize board. 
   board_init();
   
   sctimer_set_callback(cb_compare);
   sctimer_setCompare(sctimer_readCounter()+SCTIMER_PERIOD/2);

   while(1) {
      // LED 1 (error LED)
     leds_error_toggle();     board_sleep();
     sleep_counter = sleep_counter + 1;
   
     // LED 2 (radio LED)
     if(sleep_counter%2==0) leds_radio_toggle();
     
     // LED 3 (sync LED)
     if(sleep_counter%3==0) leds_sync_toggle();

     // LED 4 (debug LED)
     if(sleep_counter%4==0) leds_debug_toggle();

   }

}

//=========================== callbacks =======================================

void cb_compare(void) {
   // increment counter
   app_vars.num_compare++;
   
   // schedule again
   sctimer_setCompare(sctimer_readCounter()+SCTIMER_PERIOD/2);
}
