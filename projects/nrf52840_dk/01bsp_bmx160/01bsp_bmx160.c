/**
\brief This program shows the use of the "bmx160" bsp module.

Since the bsp modules for different platforms have the same declaration, you
can use this project with any platform.

This project configure bmx160 and read the gyroscope values in 3 axises.
Then it sends out the gyro data through uart at interval of SAMPLE_PERIOD.

\author Tengfei Chang <tengfei.chang@gmail.com>, Nov 2021.
*/

#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "board.h"
#include "leds.h"
#include "sctimer.h"
#include "i2c.h"
#include "uart.h"
#include "bmx160.h"
#include "bmp388.h"

//=========================== defines =========================================

#define SAMPLE_PERIOD   (32768>>5)  // @32kHz = 1s
#define BUFFER_SIZE     0x10        // 16 bytes, 0-5 for bmx160, 6-14 for bmp388, 15-16 \\r\\n 

//=========================== variables =======================================

typedef struct {
   uint16_t num_compare;
   bool sampling_now;
   uint8_t axes[6];
   float axis_x;
   float axis_y;
   float axis_z;

   uint8_t who_am_i;
   int16_t temp;
   float   temp_f;

   // uart specific
              uint8_t uart_lastTxByteIndex;
   volatile   uint8_t uartDone;
   volatile   uint8_t uartSendNow;
   volatile   uint8_t uartToSend[BUFFER_SIZE];
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void cb_compare(void);
void cb_uartTxDone(void);
uint8_t cb_uartRxCb(void);

int16_t tmp;
uint8_t i;

void initBmx160()
{
    // alway set address first
    i2c_set_addr(BMX160_ADDR);

    // should be 0xd8 for bmx160
    app_vars.who_am_i = bmx160_who_am_i();    
  
    // configure bmx160

    // 0x8: 100Hz
    // 0xb: 800Hz
    // 0xc: 1600Hz
    bmx160_acc_config(0x0c); 
    // 0x8: 100Hz
    // 0xc: 1600Hz
    // 0xd: 3200Hz
    bmx160_gyr_config(0x0d);
    // 0x8: 100Hz
    // 0xb: 800Hz
    bmx160_mag_config(0x0b);

    // 0x3: +/-2g 
    // 0x5: +/-4g
    // 0x8: +/-8g 
    // 0xc: +/-16g
    bmx160_acc_range(0x8);
    // 0x0: +/-2000°/s = 16.4LSB/°/s
    // 0x1: +/-1000°/s = 32.8LSB/°/s
    // 0x2: +/-500°/s  = 131.2LSB/°/s
    // 0x3: +/-250°/s  = 262.4LSB/°/s
    bmx160_gyr_range(0x1);
}

void initBmp388()
{
    i2c_set_addr(BMP388_ADDR);
    bmp388Init();
}

void fillBmx160Data()
{
    i2c_set_addr(BMX160_ADDR);

    bmx160_read_9dof_data();

    tmp = bmx160_read_gyr_x();
    app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);
    
    tmp = bmx160_read_gyr_y();
    app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);

    tmp = bmx160_read_gyr_z();
    app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);
}

void fillBmp388Data()
{
    i2c_set_addr(BMP388_ADDR);
    float temp = bmp388ReadTemp();
    uint32_t tempUint32Presentation;
    memcpy(&tempUint32Presentation, &temp, 4);
    app_vars.uartToSend[i++] = (uint8_t)((tempUint32Presentation>>24) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((tempUint32Presentation>>16) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((tempUint32Presentation>>8) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((tempUint32Presentation>>0) & 0x00ff);

    uint32_t pressure = bmp388ReadPressure();
    app_vars.uartToSend[i++] = (uint8_t)((pressure>>24) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((pressure>>16) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((pressure>>8) & 0x00ff);
    app_vars.uartToSend[i++] = (uint8_t)((pressure>>0) & 0x00ff);
}

//=========================== main ============================================

/**
\brief The program starts executing here.
*/
int mote_main(void) {

    // initialize board. 
    board_init();

    // setup UART
    uart_setCallbacks(cb_uartTxDone,cb_uartRxCb);
    uart_enableInterrupts();
   
    sctimer_set_callback(cb_compare);
    sctimer_setCompare(sctimer_readCounter()+SAMPLE_PERIOD);

    initBmx160();
    initBmp388();

    while (1) {

        // wait for timer to elapse
        while (app_vars.uartSendNow==0);
        app_vars.uartSendNow = 0;

        i=0;
        fillBmx160Data();
        fillBmp388Data();

        app_vars.uartToSend[i++] = '\r';
        app_vars.uartToSend[i++] = '\n';

        // send string over UART
        app_vars.uartDone              = 0;
        app_vars.uart_lastTxByteIndex  = 0;
        uart_writeByte(app_vars.uartToSend[app_vars.uart_lastTxByteIndex]);
        while(app_vars.uartDone==0);
    }
}

//=========================== callbacks =======================================

void cb_compare(void) {
   
    // have main "task" send over UART
    app_vars.uartSendNow = 1;

    // schedule again
    sctimer_setCompare(sctimer_readCounter()+SAMPLE_PERIOD);
}

void cb_uartTxDone(void) {

    app_vars.uart_lastTxByteIndex++;
    if (app_vars.uart_lastTxByteIndex<sizeof(app_vars.uartToSend)) {
        uart_writeByte(app_vars.uartToSend[app_vars.uart_lastTxByteIndex]);
    } else {
        app_vars.uartDone = 1;
    }
}

uint8_t cb_uartRxCb(void) {
   
   uint8_t byte;
   
   // toggle LED
   leds_error_toggle();
   
   // read received byte
   byte = uart_readByte();
   
   // echo that byte over serial
   uart_writeByte(byte);
   
   return 0;
}