#include "bmp388.h"
#include "i2c.h"

#include <math.h>

uint8_t bmp388ReadChipId(void)
{
    uint8_t chipId;
    i2c_read_bytes(BMP388_REG_ADDR_CHIPID, &chipId, 1);
    return chipId;
}

uint8_t bmp388Init(void)
{
    uint8_t chipId = bmp388ReadChipId();
    if (chipId!=0x50) {
        return 1;
    }
    bmp388ConfigPowerMode(0b00110011);    // enable pressure and temperature sensors, normal mode
}

void bmp388ConfigPowerMode(uint8_t powerMode)
{
    i2c_write_bytes(BMP388_REG_ADDR_PWR_CTRL, &powerMode, 1);
}

int32_t bmp388ReadPressure(void)
{
    uint8_t buf[3];
    i2c_read_bytes(BMP388_REG_ADDR_PRESSURE_DATA, buf, 3);
    return (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
}

float bmp388ReadTemp(void)
{
    uint8_t buf[3];
    i2c_read_bytes(BMP388_REG_ADDR_TEMP_DATA, buf, 3);
    
    uint16_t T1;
    i2c_read_bytes(0x31, (uint8_t*)&T1, 2);
    uint16_t T2;
    i2c_read_bytes(0x33, (uint8_t*)&T2, 2);
    int8_t T3;
    i2c_read_bytes(0x35, &T3, 1);

    uint32_t uncomp_temp = 0;
    uncomp_temp = ((buf[0]) | (buf[1] << 8) | (buf[2] << 16));

    double partial_data1;
    partial_data1 = uncomp_temp - ((double)T1 / (double)pow(2, -8));
    double partial_data2;
    partial_data2 = partial_data1 * ((double)T2 / (double)pow(2, 30));
    double res;
    res = partial_data2 + (partial_data1 * partial_data1) * (((double)T3)/(double)pow(2,48));
    return (float)res;
}

