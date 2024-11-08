/**
\brief This program shows the use of the "radio" bsp module.

Since the bsp modules for different platforms have the same declaration, you
can use this project with any platform.

After loading this program, your board will switch on its radio on frequency
CHANNEL.

While receiving a packet (i.e. from the start of frame event to the end of
frame event), it will turn on its sync LED.

Every TIMER_PERIOD, it will also send a packet containing LENGTH_PACKET bytes
set to ID. While sending a packet (i.e. from the start of frame event to the
end of frame event), it will turn on its error LED.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2014.
*/

#include "board.h"
#include "radio.h"
#include "leds.h"
#include "sctimer.h"
#include "uart.h"

//=========================== defines =========================================

#define LENGTH_PACKET   125+LENGTH_CRC  ///< maximum length is 127 bytes
#define LEN_PKT_TO_SEND 20+LENGTH_CRC
#define CHANNEL         1             ///< 11=2.405GHz
#define TIMER_PERIOD    (0xffff>>6)    ///< 0xffff = 2s@32kHz
#define ID              0x99           ///< byte sent in the packets

uint8_t stringToSend[]  = "+002 Ptest.24.00.12.-010\n";

//=========================== variables =======================================

enum {
    APP_FLAG_START_FRAME = 0x01,
    APP_FLAG_END_FRAME   = 0x02,
    APP_FLAG_TIMER       = 0x04,
};

typedef enum {
    APP_STATE_TX         = 0x01,
    APP_STATE_RX         = 0x02,
} app_state_t;

typedef struct {
    uint8_t              num_startFrame;
    uint8_t              num_endFrame;
    uint8_t              num_timer;
    
    uint8_t              num_rx_startFrame;
    uint8_t              num_rx_endFrame;
} app_dbg_t;

app_dbg_t app_dbg;

typedef struct {
    volatile    uint8_t         uartDone;
    volatile    uint8_t         uartSendNow;
                uint8_t         uart_lastTxByteIndex;

                uint8_t         flags;
                app_state_t     state;
                uint8_t         packet[LENGTH_PACKET];
                uint8_t         packet_len;
                int8_t          rxpk_rssi;
                uint8_t         rxpk_lqi;
                bool            rxpk_crc;
                uint8_t         pktCounter;     // 发送的数据包的编号
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_startFrame(PORT_TIMER_WIDTH timestamp);
void     cb_endFrame(PORT_TIMER_WIDTH timestamp);
void     cb_timer(void);

void     cb_uart_tx_done(void);
uint8_t  cb_uart_rx(void);

void     initVars();
void     initEnvironment();       // init board, uart, timer
void     initRadio();

//=========================== main ============================================
int mote_main(void) {
    uint8_t freq_offset;
    uint8_t sign;
    uint8_t read;

    initVars();
    initEnvironment();
    initRadio();

    while (1) {
        while (app_vars.flags==0x00) {      // Sleep while no flags set
            board_sleep();
        }

        while (app_vars.flags) {            // handle and clear every flag
            if (app_vars.flags & APP_FLAG_START_FRAME) {      // APP_FLAG_START_FRAME: TX or RX
                switch (app_vars.state) {                     // start of frame
                    case APP_STATE_RX:          // start receiving a pkt
                        leds_error_on();        // light up!
                        break;
                    case APP_STATE_TX:          // start sending
                        leds_sync_on();
                        break;
                }
                app_vars.flags &= ~APP_FLAG_START_FRAME;      // clear THIS flag
            }

            if (app_vars.flags & APP_FLAG_END_FRAME) {        // APP_FLAG_END_FRAME (TX or RX). means end of frame
                switch (app_vars.state) {
                    case APP_STATE_RX:                        // receiving done
                        app_vars.packet_len = sizeof(app_vars.packet);
                        radio_getReceivedFrame(app_vars.packet, &app_vars.packet_len, sizeof(app_vars.packet),  // get packet from radio
                                               &app_vars.rxpk_rssi, &app_vars.rxpk_lqi, &app_vars.rxpk_crc);

                        freq_offset = radio_getFrequencyOffset();     // WHAT? WHY? 
                        sign = (freq_offset & 0x80) >> 7;
                        if (sign){
                            read = 0xff - (uint8_t)(freq_offset) + 1;
                        } else {
                            read = freq_offset;
                        }

                        uint8_t i = 0;
                        if (sign) {
                            stringToSend[i++] = '-';
                        } else {
                            stringToSend[i++] = '+';
                        }
                        stringToSend[i++] = '0'+read/100;
                        stringToSend[i++] = '0'+read/10;
                        stringToSend[i++] = '0'+read%10;
                        stringToSend[i++] = ' ';

                        stringToSend[i++] = app_vars.rxpk_rssi;
                        stringToSend[i++] = app_vars.rxpk_crc;
                        stringToSend[i++] = sizeof(app_vars.packet);

                        stringToSend[i++] = 'P';
                        memcpy(&stringToSend[i],&app_vars.packet[0],14);
                        i += 14;

                        sign = (app_vars.rxpk_rssi & 0x80) >> 7;
                        if (sign){
                            read = 0xff - (uint8_t)(app_vars.rxpk_rssi) + 1;
                        } else {
                            read = app_vars.rxpk_rssi;
                        }

                        if (sign) {
                            stringToSend[i++] = '-';
                        } else {
                            stringToSend[i++] = '+';
                        }
                        stringToSend[i++] = '0'+read/100;
                        stringToSend[i++] = '0'+read/10;  
                        stringToSend[i++] = '0'+read%10;

                        stringToSend[sizeof(stringToSend)-2] = '\r';
                        stringToSend[sizeof(stringToSend)-1] = '\n';

                        if (app_vars.uartDone == 1) {               // send string over UART
                            app_vars.uartDone              = 0;
                            app_vars.uart_lastTxByteIndex  = 0;
                            uart_writeByte(stringToSend[app_vars.uart_lastTxByteIndex]);
                        }
                        leds_error_off();
                        break;
                    case APP_STATE_TX:                       // pkt sent
                        radio_rxEnable();                    // switch to RX mode
                        radio_rxNow();
                        app_vars.state = APP_STATE_RX;
                        leds_sync_off();
                        break;
                }
                app_vars.flags &= ~APP_FLAG_END_FRAME;       // clear flag
            }

            if (app_vars.flags & APP_FLAG_TIMER) {           // APP_FLAG_TIMER. timer fired
                if (app_vars.state==APP_STATE_RX) {
                    radio_rfOff();                           // stop listening

                    app_vars.packet_len = sizeof(app_vars.packet);      // prepare packet
                    uint8_t i = 0;
                    app_vars.packet[i++] = 'J';
                    app_vars.packet[i++] = 'W';
                    app_vars.packet[i++] = 'L';
                    app_vars.packet[i++] = '_';
                    app_vars.packet[i++] = 't';
                    app_vars.packet[i++] = 'e';
                    app_vars.packet[i++] = 's';
                    app_vars.packet[i++] = 't';
                    app_vars.packet[i++] = CHANNEL;
                    app_vars.packet[i++] = app_vars.pktCounter;
                    app_vars.pktCounter++;
                    while (i<app_vars.packet_len) {
                        app_vars.packet[i++] = ID;
                    }

                    radio_loadPacket(app_vars.packet,LEN_PKT_TO_SEND);  // start transmitting packet
                    radio_txEnable();
                    radio_txNow();

                    app_vars.state = APP_STATE_TX;
                }

                app_vars.flags &= ~APP_FLAG_TIMER;                      // clear flag
            }
        }
    }
}

//=========================== callbacks =======================================

void cb_startFrame(PORT_TIMER_WIDTH timestamp) {
    app_vars.flags |= APP_FLAG_START_FRAME;             // set flag
    app_dbg.num_startFrame++;                           // update debug stats

    if (app_vars.state == APP_STATE_RX) {
        app_dbg.num_rx_startFrame++;
    }
}

void cb_endFrame(PORT_TIMER_WIDTH timestamp) {
    app_vars.flags |= APP_FLAG_END_FRAME;
    app_dbg.num_endFrame++;

    if (app_vars.state == APP_STATE_RX) {
        app_dbg.num_rx_endFrame++;
    }
}

void cb_timer(void) {
    app_vars.flags |= APP_FLAG_TIMER;
    app_dbg.num_timer++;

    sctimer_setCompare(sctimer_readCounter()+TIMER_PERIOD);
}

void cb_uart_tx_done(void) {
    app_vars.uart_lastTxByteIndex++;
    if (app_vars.uart_lastTxByteIndex<sizeof(stringToSend)) {
        uart_writeByte(stringToSend[app_vars.uart_lastTxByteIndex]);
    } else {
        app_vars.uartDone = 1;
    }
}

uint8_t cb_uart_rx(void) {
    uint8_t byte;
    leds_error_toggle();
    byte = uart_readByte();       // read received byte
    uart_writeByte(byte);         // echo that byte over serial
    return 0;
}

void initVars() {
    memset(&app_vars,0,sizeof(app_vars_t));           // clear local variables
    app_vars.uartDone = 1;

    app_vars.packet_len = sizeof(app_vars.packet);    // prepare packet. 127 Bytes
    for (uint8_t i=0;i<app_vars.packet_len;i++) {
        app_vars.packet[i] = ID;
    }
}

void initEnvironment() {
    board_init();
    uart_setCallbacks(cb_uart_tx_done,cb_uart_rx);    // setup UART
    uart_enableInterrupts();

    sctimer_set_callback(cb_timer);                   // start bsp timer
    sctimer_setCompare(sctimer_readCounter()+TIMER_PERIOD);
    sctimer_enable();
}

void initRadio() {
    radio_setStartFrameCb(cb_startFrame);             // add callback functions radio
    radio_setEndFrameCb(cb_endFrame);

    radio_rfOn();                                     // prepare radio
    radio_setFrequency(CHANNEL, FREQ_RX);             // freq type only effects on scum port

    radio_rxEnable();                                 // switch in RX by default
    
    app_vars.state = APP_STATE_RX;
    app_vars.flags |= APP_FLAG_TIMER;                 // start by a transmit
}
