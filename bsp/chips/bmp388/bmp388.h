#ifndef BMP388_H
#define BMP388_H

#include "stdint.h"

#define BMP388_ADDR            0x76

#define BMP388_REG_ADDR_CHIPID     0x00
#define BMP388_REG_ADDR_ERR        0x02
#define BMP388_REG_ADDR_STATUS     0x03

#define BMP388_REG_ADDR_PRESSURE_DATA  0x04
#define BMP388_REG_ADDR_TEMP_DATA      0x07
#define BMP388_REG_ADDR_SENSOR_TIME    0x0c

#define BMP388_REG_ADDR_PWR_CTRL   0x1b
#define BMP388_REG_ADDR_OSR        0x1c
#define BMP388_REG_ADDR_ODR        0x1d
#define BMP388_REG_ADDR_CONFIG     0x1f
#define BMP388_REG_ADDR_CMD        0x7e

uint8_t bmp388Init(void);
uint8_t bmp388ReadChipId(void);
void bmp388ConfigPowerMode(uint8_t powerMode);
int32_t bmp388ReadPressure(void);
float bmp388ReadTemp(void);

#endif
